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
//  Generate quiet moves.
//-------------------------------------------------------------------------------------------------

void    gen_white_pawn_moves(BOARD *board, MOVE_LIST *ml, U64 empty_squares);
void    gen_black_pawn_moves(BOARD *board, MOVE_LIST *ml, U64 empty_squares);

//-------------------------------------------------------------------------------------------------
//  Generate moves.
//-------------------------------------------------------------------------------------------------
void gen_moves(BOARD *board, MOVE_LIST *ml)
{
    int     from, to;
    BBIX    piece;
    BBIX    moves;
    U64     empty_squares = empty_bb(board);
    int     turn = side_on_move(board);

    //  Pawn
    if (turn == WHITE)
        gen_white_pawn_moves(board, ml, empty_squares);
    else
        gen_black_pawn_moves(board, ml, empty_squares);

    //  Knight
    piece.u64 = knight_bb(board, turn);
    while (piece.u64) {
        from = bb_last(piece);
        moves.u64 = knight_moves_bb(from) & empty_squares;
        while (moves.u64) {
            to = (turn == WHITE ? bb_first(moves) : bb_last(moves));
            add_move(ml, pack_quiet(KNIGHT, from, to));
            bb_clear_bit(&moves.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Rook and Queen
    piece.u64 = queen_rook_bb(board, turn);
    while (piece.u64) {
        from = bb_first(piece);

        moves.u64 = bb_rook_attacks(from, occupied_bb(board)) & empty_squares;
        while (moves.u64) {
            to = (turn == WHITE ? bb_first(moves) : bb_last(moves));
            add_move(ml, pack_quiet(piece_on_square(board, turn, from), from, to));
            bb_clear_bit(&moves.u64, to);
        }

        bb_clear_bit(&piece.u64, from);
    }

    //  Bishop and Queen
    piece.u64 = queen_bishop_bb(board, turn);
    while (piece.u64) {
        from = bb_first(piece);

        moves.u64 = bb_bishop_attacks(from, occupied_bb(board)) & empty_squares;
        while (moves.u64) {
            to = (turn == WHITE ? bb_first(moves) : bb_last(moves));
            add_move(ml, pack_quiet(piece_on_square(board, turn, from), from, to));
            bb_clear_bit(&moves.u64, to);
        }

        bb_clear_bit(&piece.u64, from);
    }

    //  King
    from = king_square(board, turn);
    moves.u64 = king_moves_bb(from) & empty_squares;
    while (moves.u64) {
        to = bb_first(moves);
        add_move(ml, pack_quiet(KING, from, to));
        bb_clear_bit(&moves.u64, to);
    }

    //  Castle
    if (turn == WHITE) {
        if (can_generate_castle_ks(board, WHITE)) add_move(ml, pack_castle(from, G1, MT_CSWKS));
        if (can_generate_castle_qs(board, WHITE)) add_move(ml, pack_castle(from, C1, MT_CSWQS));
    }
    else {
        if (can_generate_castle_ks(board, BLACK)) add_move(ml, pack_castle(from, G8, MT_CSBKS));
        if (can_generate_castle_qs(board, BLACK)) add_move(ml, pack_castle(from, C8, MT_CSBQS));
    }
}

//-------------------------------------------------------------------------------------------------
//  White pawns
//-------------------------------------------------------------------------------------------------
void gen_white_pawn_moves(BOARD *board, MOVE_LIST *ml, U64 empty_squares)
{
    BBIX    moves1sq;
    BBIX    moves2sq;
    int     to;

    moves1sq.u64 = ((pawn_bb(board, WHITE) & ~BB_RANK_7) << 8) & empty_squares;
    moves2sq.u64 = (((moves1sq.u64 & BB_RANK_3) << 8) & empty_squares);
    //  1 square move
    while (moves1sq.u64) {
        to = bb_first(moves1sq);
        add_move(ml, pack_quiet(PAWN, to + 8, to));
        bb_clear_bit(&moves1sq.u64, to);
    }
    // 2 square move
    while (moves2sq.u64) {
        to = bb_first(moves2sq);
        add_move(ml, pack_pawn_2square(to + 16, to, to + 8));
        bb_clear_bit(&moves2sq.u64, to);
    }
}

//-------------------------------------------------------------------------------------------------
//  Black pawn
//-------------------------------------------------------------------------------------------------
void gen_black_pawn_moves(BOARD *board, MOVE_LIST *ml, U64 empty_squares) 
{
    BBIX    moves1sq;
    BBIX    moves2sq;
    int     to;

    moves1sq.u64 = ((pawn_bb(board, BLACK) & ~BB_RANK_2) >> 8) & empty_squares;
    moves2sq.u64 = (((moves1sq.u64 & BB_RANK_6) >> 8) & empty_squares);

    // 1 square move
    while (moves1sq.u64) {
        to = bb_last(moves1sq);
        add_move(ml, pack_quiet(PAWN, to - 8, to));
        bb_clear_bit(&moves1sq.u64, to);
    }
    // 2 square move
    while (moves2sq.u64) {
        to = bb_last(moves2sq);
        add_move(ml, pack_pawn_2square(to - 16, to, to - 8));
        bb_clear_bit(&moves2sq.u64, to);
    }
}

//end
