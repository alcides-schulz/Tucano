/*-------------------------------------------------------------------------------
  tucano is a chess playing engine developed by Alcides Schulz.
  Copyright (C) 2011-present - Alcides Schulz

  tucano is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  tucano is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You can find the GNU General Public License at http://www.gnu.org/licenses/
-------------------------------------------------------------------------------*/

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//  Search management process.
//-------------------------------------------------------------------------------------------------

U64     cnt = 0;
U64     hit = 0;

int     search_asp(GAME *game_data, int incheck, int depth, int prev_score);
void    *iterative_deepening(void *pv_game);

GAME    *thread_data = NULL;
int     additional_threads = 0;

//-------------------------------------------------------------------------------------------------
//  Create threads data.
//-------------------------------------------------------------------------------------------------
void threads_init(int threads_count)
{
    if (thread_data != NULL) free(thread_data);
    if (threads_count <= 0) threads_count = 1;
    additional_threads = threads_count - 1;
    if (additional_threads == 0) return;
    thread_data = (GAME *)malloc(sizeof(GAME) * additional_threads);
    if (thread_data == NULL) {
        fprintf(stderr, "Error allocating memory for %d additional threads. Running with no paralel search.\n", additional_threads);
        additional_threads = 0;
        return;
    }
    memset(thread_data, 0, (size_t)(sizeof(GAME) * additional_threads));
}

void *ponder_search(void *game)
{
    SETTINGS    ponder_settings;

    ponder_settings.single_move_time = MAX_TIME;
    ponder_settings.total_move_time = MAX_TIME;
    ponder_settings.moves_per_level = 0;
    ponder_settings.max_depth = MAX_DEPTH;
    ponder_settings.post_flag = POST_XBOARD;
    ponder_settings.use_book = FALSE;

    search_run((GAME *)game, &ponder_settings);

    return NULL;
}

//-------------------------------------------------------------------------------------------------
//  Search preparation and threads coordination
//-------------------------------------------------------------------------------------------------
void search_run(GAME *game, SETTINGS *settings)
{
    //  Prepare search control
    prepare_search(game, settings);

    game->search.start_time = util_get_time();
    game->search.normal_finish_time = game->search.start_time + game->search.normal_move_time;
    game->search.extended_finish_time = game->search.start_time + game->search.extended_move_time;
    game->search.score_drop = FALSE;
    game->search.best_move = MOVE_NONE;
    game->search.best_score = 0;
    game->search.ponder_move = MOVE_NONE;
    game->search.abort = FALSE;
    game->search.nodes = 0;
    game->search.tbhits = 0;

    game->is_main_thread = TRUE;

    //  Try to find a move from book.
    if (game->search.use_book) {
        MOVE bookmove = book_next_move(game);
        if (bookmove != MOVE_NONE) {
            game->search.best_move = bookmove;
            game->search.end_time = util_get_time();
            game->search.elapsed_time = 1;
            return;
        }
    }

    // Prepare search data
    set_ply(&game->board, 0);
    memset(&game->pv_line, 0, sizeof(PV_LINE));
    memset(&game->move_order, 0, sizeof(MOVE_ORDER));
    tt_age();

    //  Multi Thread: copy data to additional threads and start them.
    for (int i = 0; i < additional_threads; i++) {
        memcpy(&thread_data[i].board, &game->board, sizeof(BOARD));
		memcpy(&thread_data[i].search, &game->search, sizeof(SEARCH));
        memcpy(&thread_data[i].move_order, &game->move_order, sizeof(MOVE_ORDER));
        thread_data[i].is_main_thread = FALSE;
        thread_data[i].search.post_flag = POST_NONE;
        thread_data[i].thread_number = i;
        THREAD_CREATE(thread_data[i].thread_handle, iterative_deepening, &thread_data[i]);
    }

    //  Run main search
    iterative_deepening(game);

	//  Notify additional threads to finish
    for (int i = 0; i < additional_threads; i++) {
        thread_data[i].search.abort = TRUE;
        THREAD_WAIT(thread_data[i].thread_handle);
    }

    game->search.end_time = util_get_time();
    game->search.elapsed_time = game->search.end_time - game->search.start_time;
}

U64 get_additional_threads_nodes(void)
{
    U64     total = 0;

    for (int i = 0; i < additional_threads; i++) {
        total += thread_data[i].search.nodes;
    }

    return total;
}

U64 get_additional_threads_tbhits(void)
{
    U64     total = 0;

    for (int i = 0; i < additional_threads; i++) {
        total += thread_data[i].search.tbhits;
    }

    return total;
}

//-------------------------------------------------------------------------------------------------
//  Main search loop (iterative deepening)
//-------------------------------------------------------------------------------------------------
void *iterative_deepening(void *pv_game)
{
    GAME *game = (GAME *)pv_game;

    if (game->search.post_flag == POST_DEFAULT) {
        printf("Ply      Nodes  Score Time Principal Variation\n");
    }

    int incheck = is_incheck(&game->board, side_on_move(&game->board));
    set_ply(&game->board, 0);

    // Count moves at root node (for analyse info)
    game->search.root_move_count = 0;
    MOVE_LIST   root;
    MOVE        move;

    select_init(&root, game, incheck, MOVE_NONE, FALSE);
    while ((move = next_move(&root)) != MOVE_NONE) {
        if (is_pseudo_legal(&game->board, root.pins, move))
            game->search.root_move_count++;
    }

    //  Start the iterative deepening
    int prev_score = 0;
    int var = 0;
    for (int depth = 1; depth <= game->search.max_depth; depth++) {

        // Search on next depth if more than 1/2 threads are already searching it.
        // Based on demolito chess engine.
        if (!game->is_main_thread && depth > 1 && depth < game->search.max_depth) {
            int count = 0;
            for (int i = 0; i < additional_threads; i++) {
                if (thread_data[i].search.cur_depth >= depth) count++;
            }
            if (count > additional_threads / 2) depth++;
        }

        game->search.cur_depth = depth;

        int score = search_asp(game, incheck, depth, prev_score);
        if (game->search.abort) break;

        game->search.best_score = score;

        // Verify if score dropped from last iteration.
        if (depth > 4) {
            if (score + 20 < prev_score)
                var -= 3;
            else
                var += 1;
            if (score + 40 < prev_score)
                var = -3;
        }
        if (var < 0)
            game->search.score_drop = TRUE;
        else
            game->search.score_drop = FALSE;

        //  Don't start another iteration if most of time was used.
        UINT used_time = util_get_time() - game->search.start_time;

        // normal termination after completed iteration.
        if (!game->search.score_drop && depth > 1) {
            if (used_time >= game->search.normal_move_time) {
                break;
            }
        }
        if (depth > 1) {
            if (used_time >= game->search.extended_move_time * 0.6) {
                break;
            }
        }

        prev_score = score;
    }

    // collect best and ponder moves
    if (game->is_main_thread) {
        game->search.best_move = game->pv_line.pv_line[0][0];
        game->search.ponder_move = game->pv_line.pv_line[0][1];
    }

    return NULL;
}

//-------------------------------------------------------------------------------------------------
//  Aspiration window search.
//-------------------------------------------------------------------------------------------------
int search_asp(GAME *game, int incheck, int depth, int prev_score)
{
    if (depth > 4 && !is_mate_score(prev_score)) {

        for (int window = 25; window <= 400; window *= 4) {

            int alpha = prev_score - window;
            int beta = prev_score + window;

            int score = search_pv(game, incheck, alpha, beta, depth);
            if (game->search.abort) return 0;

            if (score > alpha && score < beta) return score;

            if (is_mate_score(score)) break; // do a full window search

            prev_score = score;
        }

    }

    return search_pv(game, incheck, -MAX_SCORE, MAX_SCORE, depth);
}

//END
