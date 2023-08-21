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

int8_t nnue_squares[64] = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};
int8_t nnue_pieces[2][6] = {
    {6, 5, 4, 3, 2, 1},
    {12, 11, 10, 9, 8, 7}
};

int8_t nnue_piece(int color, int piece)
{
    return nnue_pieces[color][piece];
}

int8_t nnue_square(int square)
{
    return nnue_squares[square];
}

//int nnue_can_update(BOARD *board)
//{
//    int ply = get_ply(board) - 1;
//    if (ply == -1) {
//        return FALSE; // case for new games, setposition and evaluate call.
//    }
//    while (ply >= 0) {
//        if (!board->nnue_data[ply].accumulator.computed) {
//            return FALSE;
//        }
//        if (NNUE_IS_KING(board->nnue_data[ply].dirty_piece.piece[0])) {
//            return FALSE;
//        }
//        ply--;
//    }
//    return TRUE;
//}
//
//void nnue_append_changed_indexes(NNUE_DIRTY *dirty_piece)
//{
//
//}
//
//void nnue_update_accumulator(BOARD *board, int ply)
//{
//    if (ply > 1) {
//        nnue_update_accumulator(board, ply - 1);
//    }
//    memcpy(&board->nnue_data[ply].accumulator, &board->nnue_data[ply - 1].accumulator, sizeof(NNUE_ACCUM));
//
//    NNUE_INDEXES removed_indices[2], added_indices[2];
//    removed_indices[0].size = removed_indices[1].size = 0;
//    added_indices[0].size = added_indices[1].size = 0;
//
//
//
//}
//
//void nnue_refresh_accumulator()
//{
//
//}

void nnue_debug(char *desc, int history_ply, NNUE_POSITION *position);

//-------------------------------------------------------------------------------------------------
//  Calculate the score for current position.
//-------------------------------------------------------------------------------------------------
int evaluate(GAME *game)
{
    int score = 0;

    EVAL_TABLE *eval_slot = game->eval_table + (board_key(&game->board) % EVAL_TABLE_SIZE);
    if (eval_slot->key == board_key(&game->board)) {
        return eval_slot->score;
    }

    /*  NNUE setup
    *   Piece codes are
    *     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
    *     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
    *   Squares are
    *     A1=0, B1=1 ... H8=63
    *   Input format:
    *     piece[0] is white king, square[0] is its location
    *     piece[1] is black king, square[1] is its location
    *     ..
    *     piece[x], square[x] can be in any order
    *     ..
    *     piece[n+1] is set to 0 to represent end of array
    */
    int player = side_on_move(&game->board);
    int pieces[33];
    int squares[33];

    pieces[0] = nnue_piece(WHITE, KING);
    squares[0] = nnue_square(king_square(&game->board, WHITE));
    pieces[1] = nnue_piece(BLACK, KING);
    squares[1] = nnue_square(king_square(&game->board, BLACK));

    int next_index = 2;
    for (int color = WHITE; color <= BLACK; color++) {
        for (int piece = QUEEN; piece >= PAWN; piece--) {
            U64 pieces_bb = game->board.state[color].piece[piece];
            while (pieces_bb) {
                int square = bb_first_index(pieces_bb);
                pieces[next_index] = nnue_piece(color, piece);
                squares[next_index] = nnue_square(square);
                next_index++;
                bb_clear_bit(&pieces_bb, square);
            }
        }
    }
    pieces[next_index] = 0;
    squares[next_index] = 0;

    int history_ply = get_history_ply(&game->board);
    
    NNUE_POSITION position;
    position.player = player;
    position.pieces = pieces;
    position.squares = squares;
    position.nnue_data[0] = &game->board.nnue_data[history_ply];
    position.nnue_data[1] = history_ply > 0 ? &game->board.nnue_data[history_ply - 1] : NULL;
    position.nnue_data[2] = history_ply > 1 ? &game->board.nnue_data[history_ply - 2] : NULL;

    //position.nnue_data[1] = NULL;
    //position.nnue_data[2] = NULL;

    //nnue_debug("before", history_ply, &position);
    score = nnue_calculate(&position);
    //nnue_debug("after", history_ply, &position);

    eval_slot->key = board_key(&game->board);
    eval_slot->score = score;

    return score;
}

void nnue_debug(char *desc, int history_ply, NNUE_POSITION *position)
{
    printf("%s: history ply: %d player:%d \n", desc, history_ply, position->player);
    for (int i = 0; i < 3; i++) {
        if (position->nnue_data[i] != NULL) {
            printf("nnue_data: %d computed: %d changes: %d\n", i, position->nnue_data[i]->accumulator.computed, position->nnue_data[i]->dirty_piece.count);
            NNUE_DIRTY *dp = &position->nnue_data[i]->dirty_piece;
            for (int j = 0; j < dp->count; j++) {
                printf("    dirty_piece: %d type: %d from: %d to: %d\n", j, dp->piece[j], dp->from[j], dp->to[j]);
            }
        }
    }
    getchar();
}

//END
