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

int nnue_squares[64] = {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8
};
int nnue_pieces[2][6] = {
    {6, 5, 4, 3, 2, 1},
    {12, 11, 10, 9, 8, 7}
};

//-------------------------------------------------------------------------------------------------
//  Calculate the score for current position.
//-------------------------------------------------------------------------------------------------
int nnue_eval_full(GAME *game)
{
    int score = 0;

    // NNUE evaluation
    /*
    * Piece codes are
    *     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
    *     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
    * Squares are
    *     A1=0, B1=1 ... H8=63
    * Input format:
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

    pieces[0] = 1;
    squares[0] = nnue_squares[king_square(&game->board, WHITE)];
    pieces[1] = 7;
    squares[1] = nnue_squares[king_square(&game->board, BLACK)];

    int next_index = 2;
    for (int color = WHITE; color <= BLACK; color++) {
        for (int piece = QUEEN; piece >= PAWN; piece--) {
            U64 pieces_bb = game->board.state[color].piece[piece];
            while (pieces_bb) {
                int square = bb_first_index(pieces_bb);
                pieces[next_index] = nnue_pieces[color][piece];
                squares[next_index] = nnue_squares[square];
                next_index++;
                bb_clear_bit(&pieces_bb, square);
            }
        }
    }
    pieces[next_index] = 0;
    squares[next_index] = 0;

    score = nnue_evaluate(player, pieces, squares);

    return score;
}

//END
