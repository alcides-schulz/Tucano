/*-------------------------------------------------------------------------------
  tucano is a XBoard chess playing engine developed by Alcides Schulz.
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
void    iterative_deepening(GAME *game_data);

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

void ponder_search(GAME *game)
{
    SETTINGS    ponder_settings;

    ponder_settings.single_move_time = MAX_TIME;
    ponder_settings.total_move_time = MAX_TIME;
    ponder_settings.moves_per_level = 0;
    ponder_settings.max_depth = MAX_DEPTH;
    ponder_settings.post_flag = POST_XBOARD;
    ponder_settings.use_book = FALSE;

    search_run(game, &ponder_settings);
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
            game->search.elapsed_time = 0.0001;
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
    game->search.elapsed_time = (double)(game->search.end_time - game->search.start_time) / 1000.0;
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
void iterative_deepening(GAME *game)
{
    test_cnt = test_hit = 0;

    if (game->search.post_flag == POST_DEFAULT)
        printf("Ply      Nodes  Score Time Principal Variation\n");

    int incheck = is_incheck(&game->board, side_on_move(&game->board));

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

        if (!game->is_main_thread && depth > 1 && game->thread_number % 2 == 0)
            if (depth == game->thread_number && depth < game->search.max_depth)
                depth++;

        game->search.cur_depth = depth;

        int score = search_asp(game, incheck, depth, prev_score);
        if (game->search.abort)
            break;

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

        //  Used when testing mate positions.
        if (game->search.mate_search != 0 && score == game->search.mate_search)
            break;

        //  Don't start another iteration if most of time was used.
        UINT used_time = util_get_time() - game->search.start_time;

        assert(used_time <= game->search.extended_move_time + 1000);

        if (!game->search.score_drop && depth > 1) {
            if ((double)used_time >= ((double)game->search.normal_move_time * 0.6))
                break;
        }
        if (depth > 1) {
            if ((double)used_time >= ((double)game->search.extended_move_time * 0.6))
                break;
        }

        prev_score = score;
    }

    // collect best and ponder moves
    if (game->is_main_thread) {
        game->search.best_move = game->pv_line.pv_line[0][0];
        game->search.ponder_move = game->pv_line.pv_line[0][1];
    }

    // print counters, used for testing.
    if (game->is_main_thread && test_cnt) {
        cnt += test_cnt;
        hit += test_hit;
        printf("-------->  TEST  CNT=%" PRIu64 " HIT=%" PRIu64 " PCT=%3.4f\n", test_cnt, test_hit, (double)(test_cnt == 0 ? 0 : test_hit * 100.0 / test_cnt));
        printf("-------->  TOTAL CNT=%" PRIu64 " HIT=%" PRIu64 " PCT=%3.4f\n", cnt, hit, (double)(cnt == 0 ? 0 : hit * 100.0 / cnt));
    }
}

//-------------------------------------------------------------------------------------------------
//  Aspiration window search.
//-------------------------------------------------------------------------------------------------
int search_asp(GAME *game, int incheck, int depth, int prev_score)
{
    int    alpha;
    int    beta;
    int    window;
    int    score;

    if (depth > 6) {
        for (window = 50; window <= 800; window *= 4) {
            alpha = prev_score - window;
            beta  = prev_score + window;
            score = search_pv(game, incheck, alpha, beta, depth);
            if (game->search.abort)
                return 0;
            if (score > alpha && score < beta)
                return score;
            if (is_mate_score(score))
                break;
        }
    }

    return search_pv(game, incheck, -MAX_SCORE, MAX_SCORE, depth);
}

//END
