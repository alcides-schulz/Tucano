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
//  Static exchange evaluation functions.
//-------------------------------------------------------------------------------------------------

static const int SEE_VALUE[NUM_PIECES] = {100, 300, 300, 500, 900, 10000};

int     get_lowest_attacker(BOARD *board, int target_square, U64 *occup, int color);
int     get_piece_value_see(int target_square, int piece_type);
int     is_see_promotion(int target_rank, int piece_type);
int     is_target_attacked(BOARD *board, int target_square, U64 occup, int color);

//-------------------------------------------------------------------------------------------------
//  Piece value for SEE.
//-------------------------------------------------------------------------------------------------
int piece_value_see(int piece)
{
    assert(piece >= PAWN && piece <= KING);
    return SEE_VALUE[piece];
}

int see_move(BOARD *board, MOVE move)
{
    int     gain_loss[32] = { 0 };
    int     capture = unpack_piece(move);
    U64     occupied = occupied_bb(board);

    bb_clear_bit(&occupied, unpack_from(move));

    switch (unpack_type(move)) {
    case MT_CAPPC:
        gain_loss[0] = piece_value_see(unpack_capture(move));
        break;
    case MT_PROMO:
        capture = unpack_prom_piece(move);
        gain_loss[0] = piece_value_see(capture) - piece_value_see(PAWN);
        break;
    case MT_CPPRM:
        capture = unpack_prom_piece(move);
        gain_loss[0] = piece_value_see(capture) - piece_value_see(PAWN) + piece_value_see(unpack_capture(move));
        break;
    case MT_EPCAP:
        gain_loss[0] = piece_value_see(PAWN);
        bb_clear_bit(&occupied, unpack_ep_pawn_square(move));
        break;
    }

    int turn = side_on_move(board);
    int attacker_side = flip_color(turn);
    int target_square = unpack_to(move);
    int target_rank = get_rank(target_square);

    int index = 0;

    while (TRUE) {
        int attacker_type = get_lowest_attacker(board, target_square, &occupied, attacker_side);
        
        if (attacker_type == NO_PIECE) break;

        if (attacker_type == KING) {
            if (is_target_attacked(board, target_square, occupied, flip_color(attacker_side))) {
                break;
            }
        }
        
        index++;
        gain_loss[index] = piece_value_see(capture) - gain_loss[index - 1];

        if (is_see_promotion(target_rank, attacker_type)) {
            gain_loss[index] += piece_value_see(QUEEN) - piece_value_see(PAWN);
            capture = QUEEN;
        }
        else {
            capture = attacker_type;
        }

        attacker_side = flip_color(attacker_side);
    }

    if (index == 0) return gain_loss[0];

    do {
        if (-gain_loss[index] < gain_loss[index - 1]) gain_loss[index - 1] = -gain_loss[index];
    } while (--index);

    return gain_loss[0];
}

int is_see_promotion(int target_rank, int piece_type)
{
    assert(target_rank >= 0 && target_rank <= 7);
    assert(piece_type >= PAWN && piece_type <= KING);

    if (piece_type == PAWN && (target_rank == RANK1 || target_rank == RANK8))
        return TRUE;
    else
        return FALSE;
}

int get_piece_value_see(int target_rank, int piece_type)
{
    assert(target_rank >= 0 &&  target_rank <= 7);
    assert(piece_type >= PAWN && piece_type <= KING);

    if (piece_type == PAWN && (target_rank == RANK1 || target_rank == RANK8))
        return piece_value_see(QUEEN);
    return piece_value_see(piece_type);
}

int is_target_attacked(BOARD *board, int target_square, U64 occupied, int color)
{
    if (pawn_attack_bb(color, target_square) & occupied & pawn_bb(board, color)) return TRUE;
    if (knight_moves_bb(target_square) & occupied & knight_bb(board, color)) return TRUE;
    if (bb_bishop_attacks(target_square, occupied) & queen_bishop_bb(board, color) & occupied) return TRUE;
    if (bb_rook_attacks(target_square, occupied) & queen_rook_bb(board, color) & occupied) return TRUE;
    if (king_moves_bb(target_square) & king_bb(board, color) & occupied) return TRUE;
    return FALSE;
}

int get_lowest_attacker(BOARD *board, int target_square, U64 *occupied, int color)
{
    U64     attacks;

    attacks = pawn_attack_bb(color, target_square) & (*occupied) & pawn_bb(board, color);
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return PAWN;
    }
    attacks = knight_moves_bb(target_square) & (*occupied) & knight_bb(board, color);
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return KNIGHT;
    }
    U64 diagonal_attacks = bb_bishop_attacks(target_square, (*occupied));
    attacks = diagonal_attacks & bishop_bb(board, color) & (*occupied);
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return BISHOP;
    }
    U64 rankfile_attacks = bb_rook_attacks(target_square, (*occupied));
    attacks = rankfile_attacks & rook_bb(board, color) & (*occupied);
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return ROOK;
    }
    attacks = (rankfile_attacks | diagonal_attacks) & (queen_bb(board, color) & (*occupied));
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return QUEEN;

    }
    attacks = king_moves_bb(target_square) & king_bb(board, color) & (*occupied);
    if (attacks) {
        bb_clear_bit(occupied, bb_first_index(attacks));
        return KING;
    }
    return NO_PIECE;
}

//END
