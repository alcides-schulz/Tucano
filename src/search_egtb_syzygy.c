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

#ifdef _MSC_VER
#pragma warning (disable : 4206)
#endif

#ifdef EGTB_SYZYGY

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//  End Game Table Base - Syzygy
//-------------------------------------------------------------------------------------------------

unsigned TB_PROBE_DEPTH = 10;
extern unsigned TB_LARGEST;

U64 convert_to_fathom(U64 bitboard);

U32 egtb_probe_wdl(BOARD *board, int depth, int ply)
{
    int can_castle = FALSE;

    if (board->state[WHITE].can_castle_ks || board->state[WHITE].can_castle_qs) can_castle = TRUE;
    if (board->state[BLACK].can_castle_ks || board->state[BLACK].can_castle_qs) can_castle = TRUE;

    if (board->fifty_move_rule != 0 || can_castle || board->ep_square != 0 || ply == 0) {
        return TB_RESULT_FAILED;
    }

    int piece_count = bb_count_u64(all_pieces_bb(board, WHITE) | all_pieces_bb(board, BLACK));

    if (piece_count > (int)TB_LARGEST || (piece_count == (int)TB_LARGEST && depth < (int)TB_PROBE_DEPTH)) {
        return TB_RESULT_FAILED;
    }

    U64 white = convert_to_fathom(all_pieces_bb(board, WHITE));
    U64 black = convert_to_fathom(all_pieces_bb(board, BLACK));
    U64 kings = convert_to_fathom(king_bb(board, WHITE) | king_bb(board, BLACK));
    U64 queens = convert_to_fathom(queen_bb(board, WHITE) | queen_bb(board, BLACK));
    U64 rooks = convert_to_fathom(rook_bb(board, WHITE) | rook_bb(board, BLACK));
    U64 bishops = convert_to_fathom(bishop_bb(board, WHITE) | bishop_bb(board, BLACK));
    U64 knights = convert_to_fathom(knight_bb(board, WHITE) | knight_bb(board, BLACK));
    U64 pawns = convert_to_fathom(pawn_bb(board, WHITE) | pawn_bb(board, BLACK));

    return tb_probe_wdl(white, black, kings, queens, rooks, bishops, knights, pawns, 0, 0, 0, side_on_move(board) == WHITE ? 1 : 0);
}

U64 convert_to_fathom(U64 bitboard)
{
    U64 fathom_bb = 0;
    while (bitboard) {
        int square = bb_first_index(bitboard);
        int rank = 7 - get_rank(square);
        int file = get_file(square);
        int fathom_square = rank * 8 + file;
        fathom_bb |= (U64)1 << fathom_square;
        bb_clear_bit(&bitboard, square);
    }
    return fathom_bb;
}

#endif
