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
//  Game components (board, search, tables)
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Init game data
//-------------------------------------------------------------------------------------------------
void new_game(GAME *game, char *fen)
{
    set_fen(&game->board, fen);
    memset(&game->search, 0, sizeof(SEARCH));
    memset(&game->move_order, 0, sizeof(MOVE_ORDER));
    memset(&game->pv_line, 0, sizeof(PV_LINE));
    memset(&game->eval_table, 0, sizeof(game->eval_table));
    memset(&game->pawn_table, 0, sizeof(game->pawn_table));
    if (!EVAL_TUNING) tt_clear();
    game->is_main_thread = TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Returns game result for current position.
//-------------------------------------------------------------------------------------------------
int get_game_result(GAME *game)
{
    int         can_move = FALSE;
    MOVE        move;
    MOVE_LIST   ml;

    set_ply(&game->board, 0);

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), 0, 0);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        make_move(&game->board, move);
        if (is_illegal(&game->board, move)) {
            undo_move(&game->board);
            continue;
        }
        can_move = TRUE;
        undo_move(&game->board);
    }

    if (!can_move) {
        if (is_incheck(&game->board, side_on_move(&game->board))) {
            if (side_on_move(&game->board) == WHITE)
                return GR_BLACK_WIN;
            else
                return GR_WHITE_WIN;
        }
        else
            return GR_DRAW;
    }
    else {
        if (reached_fifty_move_rule(&game->board))
            return GR_DRAW;
    }

    return GR_NOT_FINISH;
}

//-------------------------------------------------------------------------------------------------
//  Verify if game ended and print the results.
//-------------------------------------------------------------------------------------------------
void print_game_result(GAME *game)
{
    int         can_move = FALSE;
    MOVE        move;
    MOVE_LIST   ml;

    set_ply(&game->board, 0);

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), 0, 0);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        make_move(&game->board, move);
        if (is_illegal(&game->board, move)) {
            undo_move(&game->board);
            continue;
        }
        can_move = TRUE;
        undo_move(&game->board);
    }

    if (can_move == FALSE) {
        if (is_incheck(&game->board, side_on_move(&game->board))) {
            if (side_on_move(&game->board) == WHITE)
                printf("0-1 {Black mates}\n");
            else
                printf("1-0 {White mates}\n");
        }
        else
            printf("1/2-1/2 {Stalemate}\n");
    }
    else {
        if (reached_fifty_move_rule(&game->board))
            printf("1/2-1/2 {Draw by fifty move rule}\n");
        //else
        //    if (insufficient_material())
        //        printf("1/2-1/2 {Insufficient Material}\n");
        //    else
        //        if (is_threefold_repetition())
        //            printf("1/2-1/2 {Draw by repetition}\n");
    }
}

//END
