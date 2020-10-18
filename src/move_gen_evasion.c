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
//  When in check generate evasion moves.
//-------------------------------------------------------------------------------------------------
void gen_check_evasions(BOARD *board, MOVE_LIST *ml)
{
    int     king_pcsq = king_square(board, side_on_move(board));
    int     ep_pos[COLORS] = { -8, +8 };
    int     myc = side_on_move(board);
    int     opp = flip_color(myc);

    assert(is_incheck(board, side_on_move(board)));

    // step 1: count how many pieces are attacking the king, and get its square.
    int attack_count = 0;
    int attack_square = 0;

    U64 knight_checks = knight_moves_bb(king_pcsq) & knight_bb(board, opp);
    if (knight_checks) {
        attack_square = bb_last_index(knight_checks);
        attack_count++;
    }
    U64 queen_rook_checks = bb_rook_attacks(king_pcsq, occupied_bb(board)) & queen_rook_bb(board, opp);
    while (queen_rook_checks) {
        attack_square = bb_last_index(queen_rook_checks);
        attack_count++;
        bb_clear_bit(&queen_rook_checks, attack_square);
    }
    U64 queen_bishop_checks = bb_bishop_attacks(king_pcsq, occupied_bb(board)) & queen_bishop_bb(board, opp);
    while (queen_bishop_checks) {
        attack_square = bb_last_index(queen_bishop_checks);
        attack_count++;
        bb_clear_bit(&queen_bishop_checks, attack_square);
    }
    U64 pawn_checks = pawn_attack_bb(opp, king_pcsq) & pawn_bb(board, opp);
    while (pawn_checks) {
        attack_square = bb_last_index(pawn_checks);
        attack_count++;
        bb_clear_bit(&pawn_checks, attack_square);
    }

    U64 king_captures = king_moves_bb(king_pcsq) & all_pieces_bb(board, opp);
    while (king_captures) {
        int to = bb_first_index(king_captures);
        add_move(ml, pack_capture(KING, piece_on_square(board, opp, to), king_pcsq, to));
        bb_clear_bit(&king_captures, to);
    }
    U64 king_moves = king_moves_bb(king_pcsq) & empty_bb(board);
    while (king_moves) {
        int to = bb_first_index(king_moves);
        add_move(ml, pack_quiet(KING, king_pcsq, to));
        bb_clear_bit(&king_moves, to);
    }

    // When there is more than one attacker, only king moves are possible.
    if (attack_count > 1) return;

    // Step 3: generate moves that capture the attacker
    int attacker_piece = piece_on_square(board, opp, attack_square);

    U64 knight_capture = knight_moves_bb(attack_square) & knight_bb(board, myc);
    while (knight_capture) {
        int from = bb_last_index(knight_capture);
        add_move(ml, pack_capture(KNIGHT, attacker_piece, from, attack_square));
        bb_clear_bit(&knight_capture, from);
    }
    U64 queen_rook_capture = bb_rook_attacks(attack_square, occupied_bb(board)) & queen_rook_bb(board, myc);
    while (queen_rook_capture) {
        int from = bb_last_index(queen_rook_capture);
        add_move(ml, pack_capture(piece_on_square(board, myc, from), attacker_piece, from, attack_square));
        bb_clear_bit(&queen_rook_capture, from);
    }
    U64 queen_bishop_capture = bb_bishop_attacks(attack_square, occupied_bb(board)) & queen_bishop_bb(board, myc);
    while (queen_bishop_capture) {
        int from = bb_last_index(queen_bishop_capture);
        add_move(ml, pack_capture(piece_on_square(board, myc, from), attacker_piece, from, attack_square));
        bb_clear_bit(&queen_bishop_capture, from);
    }

    U64 pawn_capture = pawn_attack_bb(myc, attack_square) & pawn_bb(board, myc);
    while (pawn_capture) {
        int from = bb_last_index(pawn_capture);
        if (attack_square < 8 || attack_square > 55)
            add_all_capture_promotions(ml, from, attack_square, attacker_piece);
        else
            add_move(ml, pack_capture(PAWN, attacker_piece, from, attack_square));
        bb_clear_bit(&pawn_capture, from);
    }
    if (attacker_piece == PAWN && ep_square_bb(board)) {
        int to = ep_square(board);
        if (attack_square + ep_pos[myc] == to) {
            if (get_file(attack_square) > FILEA && piece_on_square(board, myc, attack_square - 1) == PAWN) {
                add_move(ml, pack_en_passant_capture(attack_square - 1, to, attack_square));
            }
            if (get_file(attack_square) < FILEH && piece_on_square(board, myc, attack_square + 1) == PAWN) {
                add_move(ml, pack_en_passant_capture(attack_square + 1, to, attack_square));
            }
        }
    }

    if (attacker_piece <= KNIGHT) return; // pawns and knigths cannot be blocked.

    //  Step 4: generate moves that place a piece between king and attacker.
    U64 attack_path = from_to_path_bb(king_pcsq, attack_square);
    U64 knights = knight_bb(board, myc);
    while (knights) {
        int from = bb_first_index(knights);
        U64 knight_moves = knight_moves_bb(from) & attack_path;
        while (knight_moves) {
            int to = bb_first_index(knight_moves);
            add_move(ml, pack_quiet(KNIGHT, from, to));
            bb_clear_bit(&knight_moves, to);
        }
        bb_clear_bit(&knights, from);
    }
    U64 queen_bishop = queen_bishop_bb(board, myc);
    while (queen_bishop) {
        int from = bb_first_index(queen_bishop);
        U64 queen_bishop_moves = bb_bishop_attacks(from, occupied_bb(board)) & attack_path;
        while (queen_bishop_moves) {
            int to = bb_first_index(queen_bishop_moves);
            add_move(ml, pack_quiet(piece_on_square(board, myc, from), from, to));
            bb_clear_bit(&queen_bishop_moves, to);
        }
        bb_clear_bit(&queen_bishop, from);
    }
    U64 queen_rook = queen_rook_bb(board, myc);
    while (queen_rook) {
        int from = bb_first_index(queen_rook);
        U64 queen_rook_moves = bb_rook_attacks(from, occupied_bb(board)) & attack_path;
        while (queen_rook_moves) {
            int to = bb_first_index(queen_rook_moves);
            add_move(ml, pack_quiet(piece_on_square(board, myc, from), from, to));
            bb_clear_bit(&queen_rook_moves, to);
        }
        bb_clear_bit(&queen_rook, from);
    }
    if (myc == WHITE) {
        U64 pawns = pawn_bb(board, WHITE);
        U64 moves = (pawns << 8) & empty_bb(board);
        U64 moves2sq = (((moves & BB_RANK_3) << 8) & empty_bb(board));
        U64 pawn_block = moves & attack_path;
        while (pawn_block) {
            int to = bb_first_index(pawn_block);
            if (get_rank(to) == RANK8)
                add_all_promotions(ml, to + 8, to);
            else
                add_move(ml, pack_quiet(PAWN, to + 8, to));
            bb_clear_bit(&pawn_block, to);
        }
        pawn_block = moves2sq & attack_path;
        while (pawn_block) {
            int to = bb_first_index(pawn_block);
            add_move(ml, pack_pawn_2square(to + 16, to, to + 8));
            bb_clear_bit(&pawn_block, to);
        }
    }
    else {
        U64 pawns = pawn_bb(board, BLACK);
        U64 moves = (pawns >> 8) & empty_bb(board);
        U64 moves2sq = (((moves & BB_RANK_6) >> 8) & empty_bb(board));
        U64 pawn_block = moves & attack_path;
        while (pawn_block) {
            int to = bb_first_index(pawn_block);
            if (get_rank(to) == RANK1)
                add_all_promotions(ml, to - 8, to);
            else
                add_move(ml, pack_quiet(PAWN, to - 8, to));
            bb_clear_bit(&pawn_block, to);
        }
        pawn_block = moves2sq & attack_path;
        while (pawn_block) {
            int to = bb_first_index(pawn_block);
            add_move(ml, pack_pawn_2square(to - 16, to, to - 8));
            bb_clear_bit(&pawn_block, to);
        }
    }
}

//end
