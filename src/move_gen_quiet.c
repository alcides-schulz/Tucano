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
    U64     piece;
    U64     moves;
    U64     empty_squares = empty_bb(board);
    int     turn = side_on_move(board);

    //  Pawn
    if (turn == WHITE) {
        gen_white_pawn_moves(board, ml, empty_squares);
    }
    else {
        gen_black_pawn_moves(board, ml, empty_squares);
    }

    //  Knight
    piece = knight_bb(board, turn);
    while (piece) {
        from = bb_pop_last_index(&piece);
        moves = knight_moves_bb(from) & empty_squares;
        while (moves) {
            to = (turn == WHITE ? bb_pop_first_index(&moves) : bb_pop_last_index(&moves));
            add_move(ml, pack_quiet(KNIGHT, from, to));
        }
    }

    //  Rook and Queen
    piece = queen_rook_bb(board, turn);
    while (piece) {
        from = bb_pop_first_index(&piece);
        moves = bb_rook_attacks(from, occupied_bb(board)) & empty_squares;
        while (moves) {
            to = (turn == WHITE ? bb_pop_first_index(&moves) : bb_pop_last_index(&moves));
            add_move(ml, pack_quiet(piece_on_square(board, turn, from), from, to));
        }
    }

    //  Bishop and Queen
    piece = queen_bishop_bb(board, turn);
    while (piece) {
        from = bb_pop_first_index(&piece);
        moves = bb_bishop_attacks(from, occupied_bb(board)) & empty_squares;
        while (moves) {
            to = (turn == WHITE ? bb_pop_first_index(&moves) : bb_pop_last_index(&moves));
            add_move(ml, pack_quiet(piece_on_square(board, turn, from), from, to));
        }
    }

    //  King
    from = king_square(board, turn);
    moves = king_moves_bb(from) & empty_squares;
    while (moves) {
        to = bb_pop_first_index(&moves);
        add_move(ml, pack_quiet(KING, from, to));
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
    U64 moves1sq = ((pawn_bb(board, WHITE) & ~BB_RANK_7) << 8) & empty_squares;
    U64 moves2sq = (((moves1sq & BB_RANK_3) << 8) & empty_squares);
    //  1 square move
    while (moves1sq) {
        int to = bb_pop_first_index(&moves1sq);
        add_move(ml, pack_quiet(PAWN, to + 8, to));
    }
    // 2 square move
    while (moves2sq) {
        int to = bb_pop_first_index(&moves2sq);
        add_move(ml, pack_pawn_2square(to + 16, to, to + 8));
    }
}

//-------------------------------------------------------------------------------------------------
//  Black pawn
//-------------------------------------------------------------------------------------------------
void gen_black_pawn_moves(BOARD *board, MOVE_LIST *ml, U64 empty_squares) 
{
    U64 moves1sq = ((pawn_bb(board, BLACK) & ~BB_RANK_2) >> 8) & empty_squares;
    U64 moves2sq = (((moves1sq & BB_RANK_6) >> 8) & empty_squares);
    // 1 square move
    while (moves1sq) {
        int to = bb_pop_last_index(&moves1sq);
        add_move(ml, pack_quiet(PAWN, to - 8, to));
    }
    // 2 square move
    while (moves2sq) {
        int to = bb_pop_last_index(&moves2sq);
        add_move(ml, pack_pawn_2square(to - 16, to, to - 8));
    }
}

//end
