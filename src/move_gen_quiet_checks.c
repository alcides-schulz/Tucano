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
//  Generate quiet check moves.
//-------------------------------------------------------------------------------------------------

void gen_discovered(BOARD *board, MOVE_LIST *ml, int myc, int from, U64 path);

//-------------------------------------------------------------------------------------------------
//  Generate moves.
//-------------------------------------------------------------------------------------------------
void gen_quiet_checks(BOARD *board, MOVE_LIST *ml)
{
    int     from;
    int     to;
    BBIX    piece;
    BBIX    moves;
    BBIX    checks;
    BBIX    queen_checks;
    BBIX    block;
    int     myc = side_on_move(board);
    int     opp = flip_color(myc);
    U64     discovered = 0;
    U64     pawns;
    int     opp_king = king_square(board, opp);
    U64     empty_squares = empty_bb(board);

    // discovered queen/rook attacks.
    piece.u64 = rankfile_moves_bb(opp_king) & queen_rook_bb(board, myc);
    while (piece.u64) {
        from = bb_first(piece);
        block.u64 = from_to_path_bb(from, opp_king) & occupied_bb(board);
        if (bb_count(block) == 1 && (block.u64 & all_pieces_bb(board, myc))) {
            gen_discovered(board,ml, myc, bb_first(block), from_to_path_bb(from, opp_king));
            discovered |= block.u64;
        }
        bb_clear_bit(&piece.u64, from);
    }

    // discovered queen/bishop attacks.
    piece.u64 = diagonal_moves_bb(opp_king) & queen_bishop_bb(board, myc);
    while (piece.u64) {
        from = bb_first(piece);
        block.u64 = from_to_path_bb(from, opp_king) & occupied_bb(board);
        if (bb_count(block) == 1 && (block.u64 & all_pieces_bb(board, myc))) {
            gen_discovered(board, ml, myc, bb_first(block), from_to_path_bb(from, opp_king));
            discovered |= block.u64;
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Knight checks
    piece.u64 = knight_bb(board, myc) & ~discovered;
    while (piece.u64) {
        from = bb_last(piece);
        moves.u64 = knight_moves_bb(from) & knight_moves_bb(opp_king) & empty_squares;
        while (moves.u64) {
            to = bb_first(moves);
            add_move(ml, pack_quiet(KNIGHT, from, to));
            bb_clear_bit(&moves.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Rook 
    piece.u64 = rook_bb(board, myc) & ~discovered;
    checks.u64 = bb_rook_attacks(opp_king, occupied_bb(board)) & empty_squares;
    queen_checks.u64 = checks.u64;
    while (piece.u64) {
        from = bb_first(piece);
        moves.u64 = bb_rook_attacks(from, occupied_bb(board)) & checks.u64;
        while (moves.u64) {
            to = bb_first(moves);
            add_move(ml, pack_quiet(ROOK, from, to));
            bb_clear_bit(&moves.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Bishop
    piece.u64 = bishop_bb(board, myc) & ~discovered;
    checks.u64 = bb_bishop_attacks(opp_king, occupied_bb(board)) & empty_squares;
    queen_checks.u64 |= checks.u64;
    while (piece.u64) {
        from = bb_first(piece);
        moves.u64 = bb_bishop_attacks(from, occupied_bb(board)) & checks.u64;
        while (moves.u64) {
            to = bb_first(moves);
            add_move(ml, pack_quiet(BISHOP, from, to));
            bb_clear_bit(&moves.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Queen
    piece.u64 = queen_bb(board, myc) & ~discovered;
    while (piece.u64) {
        from = bb_first(piece);
        moves.u64 = bb_bishop_attacks(from, occupied_bb(board)) & queen_checks.u64;
        moves.u64 |= bb_rook_attacks(from, occupied_bb(board)) & queen_checks.u64;
        while (moves.u64) {
            to = bb_first(moves);
            add_move(ml, pack_quiet(QUEEN, from, to));
            bb_clear_bit(&moves.u64, to);
        }

        bb_clear_bit(&piece.u64, from);
    }

    // Pawns
    pawns = pawn_bb(board, myc) & ~discovered;
    if (myc == WHITE) {
        moves.u64 = pawn_attack_bb(WHITE, king_square(board, BLACK)) & empty_squares;
        piece.u64 = (moves.u64 >> 8) & pawns;
        while (piece.u64) {
            from = bb_first(piece);
            add_move(ml, pack_quiet(PAWN, from, from - 8));
            bb_clear_bit(&piece.u64, from);
        }
        if (get_rank(king_square(board, BLACK)) == RANK5) {
            piece.u64 = (moves.u64 >> 8) & empty_squares;
            piece.u64 = (piece.u64 >> 8) & pawns;
            while (piece.u64) {
                from = bb_first(piece);
                add_move(ml, pack_pawn_2square(from, from - 16, from - 8));
                bb_clear_bit(&piece.u64, from);
            }
        }
    }
    else {
        moves.u64 = pawn_attack_bb(BLACK, king_square(board, WHITE)) & empty_squares;
        piece.u64 = (moves.u64 << 8) & pawns;
        while (piece.u64) {
            from = bb_first(piece);
            add_move(ml, pack_quiet(PAWN, from, from + 8));
            bb_clear_bit(&piece.u64, from);
        }
        if (get_rank(king_square(board, WHITE)) == RANK4) {
            piece.u64 = (moves.u64 << 8) & empty_squares;
            piece.u64 = (piece.u64 << 8) & pawns;
            while (piece.u64) {
                from = bb_first(piece);
                add_move(ml, pack_pawn_2square(from, from + 16, from + 8));
                bb_clear_bit(&piece.u64, from);
            }
        }
    }

    // castle
    if (myc == WHITE) {
        if (can_generate_castle_ks(board, myc) && (rankfile_moves_bb(F1) & king_bb(board, opp)) && !(from_to_path_bb(F1, opp_king) & occupied_bb(board)))
            add_move(ml, pack_castle(E1, G1, MT_CSWKS));
        if (can_generate_castle_qs(board, myc) && (rankfile_moves_bb(D1) & king_bb(board, opp)) && !(from_to_path_bb(D1, opp_king) & occupied_bb(board)))
            add_move(ml, pack_castle(E1, C1, MT_CSWQS));
    }
    else {
        if (can_generate_castle_ks(board, BLACK) && (rankfile_moves_bb(F8) & king_bb(board, opp)) && !(from_to_path_bb(F8, opp_king) & occupied_bb(board)))
            add_move(ml, pack_castle(E8, G8, MT_CSBKS));
        if (can_generate_castle_qs(board, BLACK) && (rankfile_moves_bb(D8) & king_bb(board, opp)) && !(from_to_path_bb(D8, opp_king) & occupied_bb(board)))
            add_move(ml, pack_castle(E8, C8, MT_CSBQS));
    }
}

void gen_discovered(BOARD *board, MOVE_LIST *ml, int myc, int from, U64 path)
{
    int     piece = piece_on_square(board, myc, from);
    BBIX    moves;
    BBIX    pawn2;
    int     to;
    U64     empty_squares = empty_bb(board);

    moves.u64 = 0;
    switch (piece) {
    case PAWN:
        if (myc == WHITE) {
            if (get_rank(from) != RANK7) {
                moves.u64 = (square_bb(from) << 8) & empty_squares;
                pawn2.u64 = ((moves.u64 & BB_RANK_3) << 8) & empty_squares;
                if (pawn2.u64 & ~path)
                    add_move(ml, pack_pawn_2square(from, bb_first(pawn2), from - 8));
            }
        }
        else {
            if (get_rank(from) != RANK2) {
                moves.u64 = (square_bb(from) >> 8) & empty_squares;
                pawn2.u64 = ((moves.u64 & BB_RANK_6) >> 8) & empty_squares;
                if (pawn2.u64 & ~path)
                    add_move(ml, pack_pawn_2square(from, bb_first(pawn2), from + 8));
            }
        }
        break;
    case KNIGHT:
        moves.u64 = knight_moves_bb(from) & empty_squares;
        break;
    case BISHOP:
        moves.u64 = bb_bishop_attacks(from, occupied_bb(board)) & empty_squares;
        break;
    case ROOK:
        moves.u64 = bb_rook_attacks(from, occupied_bb(board)) & empty_squares;
        break;
    case QUEEN:
        moves.u64 = bb_bishop_attacks(from, occupied_bb(board)) & empty_squares;
        moves.u64 |= bb_rook_attacks(from, occupied_bb(board)) & empty_squares;
        break;
    case KING:
        moves.u64 = king_moves_bb(from) & empty_squares & ~king_moves_bb(king_square(board, flip_color(myc)));
        break;
    }

    moves.u64 &= ~path;
    while (moves.u64) {
        to = bb_first(moves);
        add_move(ml, pack_quiet(piece, from, to));
        bb_clear_bit(&moves.u64, to);
    }
}

//end
