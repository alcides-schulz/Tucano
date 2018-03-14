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
//  Generate check evasion moves
//-------------------------------------------------------------------------------------------------
void    gm_evasion_king_moves(BOARD *board, MOVE_LIST *ml, int king_square);

//-------------------------------------------------------------------------------------------------
//  When in check generate evasion moves.
//-------------------------------------------------------------------------------------------------
void gen_check_evasions(BOARD *board, MOVE_LIST *ml)
{
    BBIX    temp;
    int     idx;
    int     king_pcsq = king_square(board, side_on_move(board));
    int     attack_count = 0;
    int     attack_square = 0;
    BBIX    attack_path;
    int     block_square;
    U64     pawns, moves, moves2sq;
    int     ep_pos[COLORS] = {-8, +8};
    int     myc = side_on_move(board);
    int     opp = flip_color(myc);

    assert(is_incheck(board, side_on_move(board)));

    // step 1: count how many pieces are attacking the king.

    // knight attacks (we can't have 2 knights attacking the king)
    temp.u64 = knight_moves_bb(king_pcsq) & knight_bb(board, opp);
    if (temp.u64) {
        idx = bb_last(temp);
        attack_square = idx;
        attack_count++;
    }
    // queen or rook attack.
    temp.u64 = bb_rook_attacks(king_pcsq, occupied_bb(board)) & queen_rook_bb(board, opp);
    while (temp.u64)  {
        idx = bb_last(temp);
        attack_square = idx;
        attack_count++;
        bb_clear_bit(&temp.u64, idx);
    }
    //  queen or bishop attacks
    temp.u64 = bb_bishop_attacks(king_pcsq, occupied_bb(board)) & queen_bishop_bb(board, opp);
    while (temp.u64) {
        idx = bb_last(temp);
        attack_square = idx;
        attack_count++;
        bb_clear_bit(&temp.u64, idx);
    }
    //  pawn attacks
    temp.u64 = pawn_attack_bb(opp, king_pcsq) & pawn_bb(board, opp);
    while (temp.u64) {
        idx = bb_last(temp);
        attack_square = idx;
        attack_count++;
        bb_clear_bit(&temp.u64, idx);
    }

    // Step 2: generate king escape moves
    gm_evasion_king_moves(board, ml, king_pcsq);

    // When there is more than one attacker, only king moves are possible.
    if (attack_count > 1)
        return;

    // Step 3: generate moves that capture the attacker

    int attacker_piece = piece_on_square(board, opp, attack_square);

    //  capture with knight
    temp.u64 = knight_moves_bb(attack_square) & knight_bb(board, myc);
    while (temp.u64) {
        idx = bb_last(temp);
        add_move(ml, pack_capture(KNIGHT, attacker_piece, idx, attack_square));
        bb_clear_bit(&temp.u64, idx);
    }
    //  capture with queen or rook
    temp.u64 = bb_rook_attacks(attack_square, occupied_bb(board)) & queen_rook_bb(board, myc);
    while (temp.u64) {
        idx = bb_last(temp);
        add_move(ml, pack_capture(piece_on_square(board, myc, idx), attacker_piece, idx, attack_square));
        bb_clear_bit(&temp.u64, idx);
    }
    //  capture with queen or bishop
    temp.u64 = bb_bishop_attacks(attack_square, occupied_bb(board)) & queen_bishop_bb(board, myc);
    while (temp.u64) {
        idx = bb_last(temp);
        add_move(ml, pack_capture(piece_on_square(board, myc, idx), attacker_piece, idx, attack_square));
        bb_clear_bit(&temp.u64, idx);
    }
    //  capture with pawn including capture/promotion
    temp.u64 = pawn_attack_bb(myc, attack_square) & pawn_bb(board, myc);
    while (temp.u64)  {
        idx = bb_last(temp);
        if (attack_square < 8 || attack_square > 55)
            add_all_capture_promotions(ml, idx, attack_square, attacker_piece);
        else
            add_move(ml, pack_capture(PAWN, attacker_piece, idx, attack_square));
        bb_clear_bit(&temp.u64, idx);
    }
    //  if attacker is pawn verify if can be ep-captured.
    if (attacker_piece == PAWN && ep_square_bb(board)) {
        idx = ep_square(board);
        if (attack_square + ep_pos[myc] == idx) {
            if (get_file(attack_square) > FILEA && piece_on_square(board, myc, attack_square - 1) == PAWN)
                add_move(ml, pack_en_passant_capture(attack_square - 1, idx, attack_square));
            if (get_file(attack_square) < FILEH && piece_on_square(board, myc, attack_square + 1) == PAWN)
                add_move(ml, pack_en_passant_capture(attack_square + 1, idx, attack_square));
        }
    }

    //  Step 4: generate moves that place a piece between king and attacker.
    attack_path.u64 = from_to_path_bb(king_pcsq, attack_square);
    while (attack_path.u64) {
        block_square = bb_first(attack_path);

        //  block with knight
        temp.u64 = knight_moves_bb(block_square) & knight_bb(board, myc);
        while (temp.u64) {
            idx = bb_last(temp);
            add_move(ml, pack_quiet(KNIGHT, idx, block_square));
            bb_clear_bit(&temp.u64, idx);
        }
        //  block with queen or rook
        temp.u64 = bb_rook_attacks(block_square, occupied_bb(board)) & queen_rook_bb(board, myc);
        while (temp.u64) {
            idx = bb_last(temp);
            add_move(ml, pack_quiet(piece_on_square(board, myc, idx), idx, block_square));
            bb_clear_bit(&temp.u64, idx);
        }
        //  block with queen or bishop
        temp.u64 = bb_bishop_attacks(block_square, occupied_bb(board)) & queen_bishop_bb(board, myc);
        while (temp.u64)  {
            idx = bb_last(temp);
            add_move(ml, pack_quiet(piece_on_square(board, myc, idx), idx, block_square));
            bb_clear_bit(&temp.u64, idx);
        }
        //  block with pawn moves, including promotions
        if (side_on_move(board) == WHITE) {
            pawns = pawn_bb(board, WHITE);
            moves = (pawns << 8) & empty_bb(board);
            moves2sq = (((moves & BB_RANK_3) << 8) & empty_bb(board));
            if (moves & square_bb(block_square)) {
                if (block_square < 8)
                    add_all_promotions(ml, block_square + 8, block_square);
                else
                    add_move(ml, pack_quiet(PAWN, block_square + 8, block_square));
            }
            if (moves2sq & square_bb(block_square))
                add_move(ml, pack_pawn_2square(block_square + 16, block_square, block_square + 8));
        }
        else {
            pawns = pawn_bb(board, BLACK);
            moves = (pawns >> 8) & empty_bb(board);
            moves2sq = (((moves & BB_RANK_6) >> 8) & empty_bb(board));
            if (moves & square_bb(block_square)) {
                if (block_square > 55)
                    add_all_promotions(ml, block_square - 8, block_square);
                else
                    add_move(ml, pack_quiet(PAWN, block_square - 8, block_square));
            }
            if (moves2sq & square_bb(block_square))
                add_move(ml, pack_pawn_2square(block_square - 16, block_square, block_square - 8));
        }

        bb_clear_bit(&attack_path.u64, block_square);
    }
}

//-------------------------------------------------------------------------------------------------
//  Generate king evasion moves.
//-------------------------------------------------------------------------------------------------
void gm_evasion_king_moves(BOARD *board, MOVE_LIST *ml, int king_square)
{
    BBIX    temp;
    int     to;
    int     opp = flip_color(side_on_move(board));

    temp.u64 = king_moves_bb(king_square) & all_pieces_bb(board, flip_color(side_on_move(board)));
    while (temp.u64) {
        to = bb_first(temp);
        add_move(ml, pack_capture(KING, piece_on_square(board, opp, to), king_square, to));
        bb_clear_bit(&temp.u64, to);
    }
    temp.u64 = king_moves_bb(king_square) & empty_bb(board);
    while (temp.u64) {
        to = bb_first(temp);
        add_move(ml, pack_quiet(KING, king_square, to));
        bb_clear_bit(&temp.u64, to);
    }
}

//end
