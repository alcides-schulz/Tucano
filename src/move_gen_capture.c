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
//    Generate capture and promotion moves.
//-------------------------------------------------------------------------------------------------

void    gen_white_pawn_captures(BOARD *board, MOVE_LIST *ml);
void    gen_black_pawn_captures(BOARD *board, MOVE_LIST *ml);

//-------------------------------------------------------------------------------------------------
//  Generate capture moves.
//-------------------------------------------------------------------------------------------------
void gen_caps(BOARD *board, MOVE_LIST *ml)
{
    BBIX    piece, attacks;
    int     from, to;
    int     turn = side_on_move(board);
    int     opp = flip_color(turn);

    assert(ml != NULL);

    //  Pawns
    if (turn == WHITE)
        gen_white_pawn_captures(board, ml);
    else
        gen_black_pawn_captures(board, ml);

    //  Knight
    piece.u64 = knight_bb(board, turn);
    while (piece.u64) {
        from = bb_first(piece);
        attacks.u64 = knight_moves_bb(from) & all_pieces_bb(board, opp);
        while (attacks.u64) {
            to = (turn == WHITE ? bb_first(attacks) : bb_last(attacks));
            add_move(ml, pack_capture(KNIGHT, piece_on_square(board, opp, to), from, to));
            bb_clear_bit(&attacks.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  Rook and queen
    piece.u64 = queen_rook_bb(board, turn);
    while (piece.u64) {
        from = bb_first(piece);

        attacks.u64 = bb_rook_attacks(from, occupied_bb(board)) & all_pieces_bb(board, opp);
        while (attacks.u64) {
            to = (turn == WHITE ? bb_first(attacks) : bb_last(attacks));
            add_move(ml, pack_capture(piece_on_square(board, turn, from), piece_on_square(board, opp, to), from, to));
            bb_clear_bit(&attacks.u64, to);
        }

        bb_clear_bit(&piece.u64, from);
    }

    //  Bishop and Queen
    piece.u64 = queen_bishop_bb(board, turn);
    while (piece.u64) {
        from = bb_first(piece);
        attacks.u64 = bb_bishop_attacks(from, occupied_bb(board)) & all_pieces_bb(board, opp);
        while (attacks.u64) {
            to = (turn == WHITE ? bb_first(attacks) : bb_last(attacks));
            add_move(ml, pack_capture(piece_on_square(board, turn, from), piece_on_square(board, opp, to), from, to));
            bb_clear_bit(&attacks.u64, to);
        }
        bb_clear_bit(&piece.u64, from);
    }

    //  King
    from = king_square(board, turn);
    attacks.u64 = king_moves_bb(from) & all_pieces_bb(board, opp);
    while (attacks.u64) {
        to = (turn == WHITE ? bb_first(attacks) : bb_last(attacks));
        add_move(ml, pack_capture(KING, piece_on_square(board, opp, to), from, to));
        bb_clear_bit(&attacks.u64, to);
    }
}

//-------------------------------------------------------------------------------------------------
//  White pawn captures and promotions
//-------------------------------------------------------------------------------------------------
void gen_white_pawn_captures(BOARD *board, MOVE_LIST *ml)
{
    BBIX    moves, attacks, ep_capture;
    int     from, to;
    int     opp = flip_color(side_on_move(board));

    // promotions
    moves.u64 = ((pawn_bb(board, WHITE) & BB_RANK_7) << 8) & empty_bb(board);
    while (moves.u64) {
        to = bb_first(moves);
        add_all_promotions(ml, to + 8, to);
        bb_clear_bit(&moves.u64, to);
    }

    //  attacks to northwest
    attacks.u64 = (pawn_bb(board, WHITE) & BB_NO_AFILE) << 9;
    ep_capture.u64 = attacks.u64 & ep_square_bb(board);
    attacks.u64 &= all_pieces_bb(board, opp);
    while (attacks.u64) {
        to = bb_first(attacks);
        from = to + 9;
        if (to < 8)
            add_all_capture_promotions(ml, from, to, piece_on_square(board, BLACK, to));
        else
            add_move(ml, pack_capture(PAWN, piece_on_square(board, BLACK, to), from, to));
        bb_clear_bit(&attacks.u64, to);
    }
    if (ep_capture.u64) {
        to = bb_last(ep_capture);
        add_move(ml, pack_en_passant_capture(to + 9, to, to + 8));
    }

    // attacks northeast
    attacks.u64 = (pawn_bb(board, WHITE) & BB_NO_HFILE) << 7;
    ep_capture.u64 = attacks.u64 & ep_square_bb(board);
    attacks.u64 &= all_pieces_bb(board, opp);
    while (attacks.u64) {
        to = bb_first(attacks);
        from = to + 7;
        if (to < 8)
            add_all_capture_promotions(ml, from, to, piece_on_square(board, BLACK, to));
        else
            add_move(ml, pack_capture(PAWN, piece_on_square(board, BLACK, to), from, to));
        bb_clear_bit(&attacks.u64, to);
    }
    if (ep_capture.u64) {
        to = bb_last(ep_capture);
        add_move(ml, pack_en_passant_capture(to + 7, to, to + 8));
    }
}

//-------------------------------------------------------------------------------------------------
//  Black pawn captures and promotions
//-------------------------------------------------------------------------------------------------
void gen_black_pawn_captures(BOARD *board, MOVE_LIST *ml) 
{
    BBIX    moves, attacks, ep_capture;
    int     from, to;
    int     opp = flip_color(side_on_move(board));

    //  promotions
       moves.u64 = ((pawn_bb(board, BLACK) & BB_RANK_2) >> 8) & empty_bb(board);
    while (moves.u64) {
        to = bb_last(moves);
        add_all_promotions(ml, to - 8, to);
        bb_clear_bit(&moves.u64, to);
    }
    // attacks southeast
    attacks.u64 = (pawn_bb(board, BLACK) & BB_NO_HFILE) >> 9;
    ep_capture.u64 = attacks.u64 & ep_square_bb(board);
    attacks.u64 &= all_pieces_bb(board, opp);
    while (attacks.u64) {
        to = bb_last(attacks);
        from = to - 9;
        if (to > 55)
            add_all_capture_promotions(ml, from, to, piece_on_square(board, WHITE, to));
        else
            add_move(ml, pack_capture(PAWN, piece_on_square(board, WHITE, to), from, to));
        bb_clear_bit(&attacks.u64, to);
    }
    if (ep_capture.u64) {
        to = bb_last(ep_capture);
        add_move(ml, pack_en_passant_capture(to - 9, to, to - 8));
    }
    // attacks southwest
    attacks.u64 = (pawn_bb(board, BLACK) & BB_NO_AFILE) >> 7;
    ep_capture.u64 = attacks.u64 & ep_square_bb(board);
    attacks.u64 &= all_pieces_bb(board, opp);
    while (attacks.u64) {
        to = bb_last(attacks);
        from = to - 7;
        if (to > 55)
            add_all_capture_promotions(ml, from, to, piece_on_square(board, WHITE, to));
        else
            add_move(ml, pack_capture(PAWN, piece_on_square(board, WHITE, to), from, to));
        bb_clear_bit(&attacks.u64, to);
    }
    if (ep_capture.u64) {
        to = bb_last(ep_capture);
        add_move(ml, pack_en_passant_capture(to - 7, to, to - 8));
    }
}

//end
