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
#include <math.h>

//-------------------------------------------------------------------------------------------------
//  Search utilities
//-------------------------------------------------------------------------------------------------

static const int PIECE_VALUE[NUM_PIECES] = 
    {VALUE_PAWN, 
     VALUE_KNIGHT, 
     VALUE_BISHOP, 
     VALUE_ROOK, 
     VALUE_QUEEN, 
     VALUE_KING};

static const U64 BB_RANK7[COLORS] = {BB_RANK_7, BB_RANK_2};

//-------------------------------------------------------------------------------------------------
//  Prepare search control data: time, depth, flags
//-------------------------------------------------------------------------------------------------
void prepare_search(GAME *game, SETTINGS *settings)
{
    game->search.post_flag = settings->post_flag;
    game->search.use_book = settings->use_book;
    game->search.max_depth = settings->max_depth;

    //  Specific time per move
    if (settings->single_move_time > 0) {
        UINT single_move_time = settings->single_move_time;
        UINT time_buffer = (UINT)(single_move_time * 0.10);
        if (time_buffer > 1000) time_buffer = 1000;
        if (time_buffer < 200) time_buffer = single_move_time / 2;
        single_move_time -= time_buffer;
        game->search.normal_move_time = single_move_time;
        game->search.extended_move_time = single_move_time;
        return;
    }

    //  Calculate moves/time period
    int played_moves = get_played_moves_count(&game->board, side_on_move(&game->board));
    int moves_to_go = 0;
    if (settings->moves_per_level > 0) { // In XBoard we can receive move_per_level.
        // calculate moves to go in this level
        moves_to_go = settings->moves_per_level - (played_moves % settings->moves_per_level);
        int half_moves = (int)(settings->moves_per_level / 2);
        if (moves_to_go > half_moves) moves_to_go = half_moves; // use more time in early moves
        if (moves_to_go < 1) moves_to_go = 1;
    }
    else {
        moves_to_go = settings->moves_to_go; // In UCI mode we can receive moves_to_go.
        if (moves_to_go == 0) { // estimate moves to go
            moves_to_go = 40 - played_moves / 2;
            if (moves_to_go < 1) moves_to_go = 1;
        }
        else {
            if (moves_to_go > 20) moves_to_go = 20; // allocate more time for initial moves.
        }
    }

    // allocate time for this move
    game->search.normal_move_time = settings->total_move_time / moves_to_go;

    //  Calculate extended move time and allocate time buffer to avoid timeout
    game->search.extended_move_time = game->search.normal_move_time * 4;
    int time_buffer = (int)(settings->total_move_time * 0.10);
    if (time_buffer > 1000) time_buffer = 1000;
    if (time_buffer < 200) time_buffer = settings->total_move_time / 2;
    if (game->search.extended_move_time > settings->total_move_time - time_buffer) {
        game->search.extended_move_time = settings->total_move_time - time_buffer;
    }
    if (game->search.normal_move_time > game->search.extended_move_time) {
        game->search.normal_move_time = game->search.extended_move_time;
    }
}

//-------------------------------------------------------------------------------------------------
//  Return piece value
//-------------------------------------------------------------------------------------------------
int piece_value(int piece)
{
    assert(piece >= PAWN && piece <= KING);
    return PIECE_VALUE[piece];
}

//-------------------------------------------------------------------------------------------------
//  Update principal variation
//-------------------------------------------------------------------------------------------------
void update_pv(PV_LINE *pv_line, int ply, MOVE move) 
{
    pv_line->pv_line[ply][ply] = move;
    for (int i = ply + 1; i < pv_line->pv_size[ply + 1]; i++) {
        pv_line->pv_line[ply][i] = pv_line->pv_line[ply + 1][i];
    }
    pv_line->pv_size[ply] = pv_line->pv_size[ply + 1];
}

//-------------------------------------------------------------------------------------------------
//  Calculate and return the depth for null move search
//-------------------------------------------------------------------------------------------------
int null_depth(int depth) 
{
    return depth - 4 - ((depth - 3) / 4);
}

//-------------------------------------------------------------------------------------------------
//  Check if time for search has ended.
//-------------------------------------------------------------------------------------------------
void check_time(GAME *search_data)
{
    if (!search_data->is_main_thread) { // check time for main thread only
        return;
    }
    if (search_data->search.nodes & TIME_CHECK) {
        return;
    }
    UINT current_time = util_get_time();
    if (current_time >= search_data->search.extended_finish_time) {
        search_data->search.abort = TRUE;
    }
}

//-------------------------------------------------------------------------------------------------
//  Test if move made is a free pawn. No enemy pawns in front of it.
//-------------------------------------------------------------------------------------------------
int is_free_pawn(BOARD *board, int turn, MOVE move)
{
    if (unpack_piece(move) != PAWN) return FALSE;
    if (unpack_type(move) == MT_PROMO || unpack_type(move) == MT_CPPRM) return TRUE;
    if (!(passed_mask_bb(turn, unpack_to(move)) & pawn_bb(board, flip_color(turn)))) return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Test if move is a pawn advance to 7th or 8th rank
//-------------------------------------------------------------------------------------------------
int is_pawn_to_rank78(int turn, MOVE move)
{
    if (unpack_piece(move) == PAWN && ((turn == WHITE && unpack_to(move) <= H7) || (turn == BLACK && unpack_to(move) >= A2)))
        return TRUE;
    else
        return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Test if color has pawn on its rank 7
//-------------------------------------------------------------------------------------------------
int has_pawn_on_rank7(BOARD *board, int color)
{
    return pawn_bb(board, color) & BB_RANK7[color] ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Indicate if is mate score (MATE - ply)
//-------------------------------------------------------------------------------------------------
int is_mate_score(int score)
{
    if (score >= MATE_VALUE - MAX_PLY && score <= MATE_VALUE) return TRUE;
    if (score >= -MATE_VALUE  && score <= -MATE_VALUE + MAX_PLY) return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Indicate if is an evaluation score
//-------------------------------------------------------------------------------------------------
int is_eval_score(int score)
{
    return (score >= -MAX_EVAL && score <= MAX_EVAL ? TRUE : FALSE);
}

//-------------------------------------------------------------------------------------------------
//  Display search information on screen.
//-------------------------------------------------------------------------------------------------
void post_info(GAME *game, int score, int depth)
{
    if (game->search.post_flag == POST_NONE) return;

    // node and egtb hits sum
    U64 total_node_count = game->search.nodes + get_additional_threads_nodes();
#ifdef EGTB_SYZYGY
    U64 total_tbhits = game->search.tbhits + get_additional_threads_tbhits();
#endif

    // Evaluation score baseline is 200 centipawns, so we have to adjust it for display.
    if (is_eval_score(score)) score /= 2;

    // tucano formatted output 
    if (game->search.post_flag == POST_DEFAULT) {
        double score_display = score / 100.0;
        if (side_on_move(&game->board) == BLACK) score_display = -score_display;
        double time = ((float)(util_get_time() - game->search.start_time) / 1000.0);
        char *space = time < 10.0 ? " " : "";
        printf("%3d  %9" PRIu64 " %6.2f %s%2.1f", depth, total_node_count, score_display, space, time);
#ifdef EGTB_SYZYGY
        printf(" (EGTB: %" PRIu64 " hits)", total_tbhits);
#endif
    }

    // xboard output
    if (game->search.post_flag == POST_XBOARD) {
        int xboard_score = (side_on_move(&game->board) == BLACK ? -score : score);
        int xboard_time = (util_get_time() - game->search.start_time) / 10;
        printf("%d %d %d %" PRIu64 "", depth, xboard_score, xboard_time, total_node_count);
    }

    // uci output
    if (game->search.post_flag == POST_UCI) {
        int elapsed_milliseconds = util_get_time() - game->search.start_time;
        if (elapsed_milliseconds == 0) elapsed_milliseconds = 1;
        U64 nodes_per_second = 1000 * total_node_count / elapsed_milliseconds;
        int uci_score = score;
        char *score_type = "cp"; //centipawns
        if (is_mate_score(uci_score)) {
            score_type = "mate";
            if (score > 0) { // mate in
                uci_score = (MATE_VALUE - score + 1) / 2;
            }
            else { // mated in
                uci_score = -(score + MATE_VALUE) / 2;
            }
        }
        printf("info ");
        printf("depth %d ", depth);
        printf("score %s %d ", score_type, uci_score);
        printf("time %d ", elapsed_milliseconds);
        printf("nodes %" PRIu64 " ", total_node_count);
        printf("nps %" PRIu64 " ", nodes_per_second);
#ifdef EGTB_SYZYGY
        printf("tbhits %" PRIu64 " ", total_tbhits);
#endif
        printf("pv");
    }

    // print pv (works for all options above)
    char move_string[20];
    for (int pvi = 0; pvi < game->pv_line.pv_size[0]; pvi++) {
        util_get_move_string(game->pv_line.pv_line[0][pvi], move_string);
        printf(" %s", move_string);
    }

    printf("\n");
    fflush(stdout);
}

//END
