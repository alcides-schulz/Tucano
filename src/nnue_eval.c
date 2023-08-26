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

//-------------------------------------------------------------------------------------------------
//  Tucano piece to nnue piece.
//-------------------------------------------------------------------------------------------------
int8_t nnue_piece(int color, int piece)
{
    return nnue_pieces[color][piece];
}

//-------------------------------------------------------------------------------------------------
//  Tucano square to nnue square.
//-------------------------------------------------------------------------------------------------
int8_t nnue_square(int square)
{
    return nnue_squares[square];
}

//-------------------------------------------------------------------------------------------------
//  Indicate if accumulators can be updated in this tree. Have to find a computed accumulator, and
//  should not have king moves, that forces the accumulator refresh.
//-------------------------------------------------------------------------------------------------
int nnue_can_update(BOARD *board)
{
    int history_ply = get_history_ply(board) - 1;
    while (history_ply > 0) {
        if (nnue_has_king_move(&board->nnue_data[history_ply].changes)) {
            return FALSE;
        }
        if (board->nnue_data[history_ply].accumulator.computed == TRUE) {
            return TRUE;
        }
        history_ply--;
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Update each accumulator in this tree, starting from a computed accumulator.
//-------------------------------------------------------------------------------------------------
void nnue_update_tree(BOARD *board, int history_ply, NNUE_POSITION *position)
{
    if (history_ply > 0) {
        if (board->nnue_data[history_ply - 1].accumulator.computed == FALSE) {
            nnue_update_tree(board, history_ply - 1, position);
        }
    }
    if (history_ply == 0) {
        position->current_nnue_data = &board->nnue_data[0];
        position->previous_nnue_data = NULL;
        nnue_refresh_accumulator(position);
        return;
    }
    position->current_nnue_data = &board->nnue_data[history_ply];
    position->previous_nnue_data = &board->nnue_data[history_ply - 1];
    nnue_update_accumulator(position);
}

//-------------------------------------------------------------------------------------------------
//  Calculate current position score.
//  If accumulators are not updated/computed then will use nnue data from move history to update.
//-------------------------------------------------------------------------------------------------
int evaluate(GAME *game)
{
    int score = 0;

    EVAL_TABLE *eval_slot = game->eval_table + (board_key(&game->board) % EVAL_TABLE_SIZE);
    if (eval_slot->key == board_key(&game->board)) {
        return eval_slot->score;
    }

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

    if (nnue_can_update(&game->board)) {
        nnue_update_tree(&game->board, history_ply, &position);
        position.current_nnue_data = &game->board.nnue_data[history_ply];
        position.previous_nnue_data = NULL;
    }
    else {
        position.current_nnue_data = &game->board.nnue_data[history_ply];
        position.previous_nnue_data = NULL;
        nnue_refresh_accumulator(&position);
    }

    score = nnue_calculate(&position);

    eval_slot->key = board_key(&game->board);
    eval_slot->score = score;

    return score;
}

//END
