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
        int single_move_time = settings->single_move_time;
        if (single_move_time > 20) single_move_time -= 10; // allow some time buffer
        game->search.normal_move_time = single_move_time;
        game->search.extended_move_time = single_move_time;
        return;
    }

    //  Calculate move time
    game->search.total_move_time = settings->total_move_time;
    UINT max_time = (int)(settings->total_move_time * 0.9);
    int played_moves = get_played_moves_count(&game->board, side_on_move(&game->board));
    int moves_to_go = 40; // initial guess
    if (settings->moves_level > 0) // subtract moves played
        moves_to_go = settings->moves_level - (played_moves % settings->moves_level);
    if (moves_to_go <= 0 || moves_to_go > 28) {
        if (played_moves <= 40)
            moves_to_go = 20;
        else
            if (played_moves <= 60)
                moves_to_go = 25;
            else
                moves_to_go = 28;
    }

    game->search.normal_move_time = (UINT)(max_time / moves_to_go);

    //  Calculate extended move time
    game->search.extended_move_time = (UINT)(game->search.normal_move_time * 4);
    if (game->search.extended_move_time > max_time)
        game->search.extended_move_time = max_time;
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
    int     i;

    pv_line->pv_line[ply][ply] = move;
    for (i = ply + 1; i < pv_line->pv_size[ply + 1]; i++)
        pv_line->pv_line[ply][i] = pv_line->pv_line[ply + 1][i];
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
    UINT    current_time;
    char    move_string[100];
    double  etime;
    char    *p;

    if (!search_data->is_main_thread) // check time for main thread only
        return;

    if (search_data->search.nodes & TIME_CHECK)
        return;

    current_time = util_get_time();
    if (search_data->search.cur_depth > 1 && !search_data->search.score_drop)
        if (current_time >= search_data->search.normal_finish_time)
            search_data->search.abort = TRUE;
    if (current_time >= search_data->search.extended_finish_time)
        search_data->search.abort = TRUE;

    if (is_pondering && util_input_available())
        search_data->search.abort = TRUE;

    if (is_analysis && util_input_available()) {
        if (fgets(analysis_command, MAX_READ, stdin) == NULL) 
            if (feof(stdin))
                return;
        if ((p = strchr(analysis_command, '\n')) != NULL)
            *p = '\0';

        if (!strcmp(analysis_command, ".")) {
            etime = (double)(current_time - search_data->search.start_time) / 10.0;
            printf("stat01: %0.f %llu %d %d %d\n",
                etime,
                search_data->search.nodes,
                search_data->search.cur_depth,
                (search_data->search.root_move_count - search_data->search.root_move_search),
                search_data->search.root_move_count);
            fflush(stdout);
        }
        else
        if (!strcmp(analysis_command, "bk")) {
            // not supported
        }
        else
        if (!strcmp(analysis_command, "hint")) {
            if (search_data->pv_line.pv_line[0][0]) {
                util_get_move_string(search_data->pv_line.pv_line[0][0], move_string);
                printf("Hint: %s\n", move_string);
                fflush(stdout);
            }
        }
        else
            search_data->search.abort = TRUE;
    }
}

//-------------------------------------------------------------------------------------------------
//  Test if move made is a free pawn. No enemy pawns in front of it.
//-------------------------------------------------------------------------------------------------
int is_free_pawn(BOARD *board, int turn, MOVE move)
{
    if (unpack_piece(move) != PAWN)
        return FALSE;
    if (unpack_type(move) == MT_PROMO || unpack_type(move) == MT_CPPRM)
        return TRUE;
    if (!(passed_mask_bb(turn, unpack_to(move)) & pawn_bb(board, flip_color(turn))))
        return TRUE;
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
    if (score > MAX_EVAL && score != MAX_SCORE)
        return TRUE;
    if (score < -MAX_EVAL && score != -MAX_SCORE)
        return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Display search information on screen.
//-------------------------------------------------------------------------------------------------
void post_info(GAME *game, int score, int depth)
{
    int     pvi;
    char    move_string[20];
    char    info_line[200];
    int     move_number;
    double  time;
    char    *space;

    if (game->search.post_flag == POST_NONE)
        return;
    if (get_history_ply(&game->board) % 2)
        move_number = (get_history_ply(&game->board) + 1) / 2;
    else
        move_number = (get_history_ply(&game->board) + 2) / 2;

    if (game->search.post_flag == POST_DEFAULT)  {
        time = ((float)(util_get_time() - game->search.start_time) / 1000.0);
        if (time < 10.0)
            space = " ";
        else
            space = "";
        sprintf(info_line, "%3d  %9lld %6.2f %s%2.1f %d.%s", depth, game->search.nodes, 
            ((float)(side_on_move(&game->board) == BLACK ? -score : score) / 100.0),
            space, time,
            move_number, 
            (side_on_move(&game->board) == WHITE ? "" : ".."));
    }

    if (game->search.post_flag == POST_XBOARD)  {
        sprintf(info_line, "%d %d %d %lld %d.%s", depth, (side_on_move(&game->board) == BLACK ? -score : score),
            (util_get_time() - game->search.start_time) / 10, game->search.nodes, 
            move_number, 
            (side_on_move(&game->board) == WHITE ? "" : ".."));
    }

    if (game->search.post_flag != POST_NONE) {
        printf("%s", info_line);

        for (pvi = 0; pvi < game->pv_line.pv_size[0]; pvi++) {
            if (game->pv_line.pv_line[0][pvi]) {
                util_get_move_string(game->pv_line.pv_line[0][pvi], move_string);
                printf(" %s", move_string);
            }
        }
        printf("\n");
        fflush(stdout);
    }
}

//END
