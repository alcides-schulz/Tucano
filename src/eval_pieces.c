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
//  Queen, rook, bishop and knight evaluation
//-------------------------------------------------------------------------------------------------

int THREAT_ZERO = MAKE_SCORE(0, 0);
int *B_THREAT[NUM_PIECES] = {&B_THREAT_PAWN, &B_THREAT_KNIGHT, &B_THREAT_BISHOP, &B_THREAT_ROOK, &B_THREAT_QUEEN, &THREAT_ZERO};

static const int MY_RANK_6[COLORS] = {RANK6, RANK3};
static const int MY_RANK_7[COLORS] = {RANK7, RANK2};
static const int MY_RANK_8[COLORS] = {RANK8, RANK1};
static const U64 MY_BB_RANK_7[COLORS] = {BB_RANK_7, BB_RANK_2};
static const U64 MY_BB_RANK_8[COLORS] = {BB_RANK_8, BB_RANK_1};

void    eval_pieces_prepare(BOARD *board, EVALUATION *eval_values);
void    eval_knights(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_bishops(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_rooks(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_queens(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_pieces_finalize(EVALUATION *eval_values, int myc, int opp);

//-------------------------------------------------------------------------------------------------
//  Evaluate pieces.
//-------------------------------------------------------------------------------------------------
void eval_pieces(BOARD *board, EVALUATION *eval_values)
{
    eval_pieces_prepare(board, eval_values);

    eval_knights(board, eval_values, WHITE, BLACK);
    eval_knights(board, eval_values, BLACK, WHITE);

    eval_bishops(board, eval_values, WHITE, BLACK);
    eval_bishops(board, eval_values, BLACK, WHITE);

    eval_rooks(board, eval_values, WHITE, BLACK);
    eval_rooks(board, eval_values, BLACK, WHITE);

    eval_queens(board, eval_values, WHITE, BLACK);
    eval_queens(board, eval_values, BLACK, WHITE);

    eval_pieces_finalize(eval_values, WHITE, BLACK);
    eval_pieces_finalize(eval_values, BLACK, WHITE);
}

//-------------------------------------------------------------------------------------------------
//  Initialize data used by pieces evaluation
//-------------------------------------------------------------------------------------------------
void eval_pieces_prepare(BOARD *board, EVALUATION *eval_values)
{
    eval_values->king_attack_count[WHITE] = eval_values->king_attack_count[BLACK] = 0;
    eval_values->king_attack_value[WHITE] = eval_values->king_attack_value[BLACK] = 0;

    eval_values->pawn_attacks[WHITE] = 0;
    eval_values->pawn_attacks[WHITE] |= (pawn_bb(board, WHITE) & BB_NO_AFILE) << 9;
    eval_values->pawn_attacks[WHITE] |= (pawn_bb(board, WHITE) & BB_NO_HFILE) << 7;
    eval_values->pawn_attacks[BLACK] = 0;
    eval_values->pawn_attacks[BLACK] |= (pawn_bb(board, BLACK) & BB_NO_HFILE) >> 9;
    eval_values->pawn_attacks[BLACK] |= (pawn_bb(board, BLACK) & BB_NO_AFILE) >> 7;

    eval_values->undefended[WHITE] = all_pieces_bb(board, WHITE) & ~eval_values->pawn_attacks[WHITE];
    eval_values->undefended[BLACK] = all_pieces_bb(board, BLACK) & ~eval_values->pawn_attacks[BLACK];

    eval_values->mobility_target[WHITE] = ~all_pieces_bb(board, WHITE) | qrnb_bb(board, WHITE);
    eval_values->mobility_target[BLACK] = ~all_pieces_bb(board, BLACK) | qrnb_bb(board, BLACK);
}

//-------------------------------------------------------------------------------------------------
//  Calculate final eval terms for pieces
//-------------------------------------------------------------------------------------------------
void eval_pieces_finalize(EVALUATION *eval_values, int myc, int opp)
{
    // calculate king attack
    if (eval_values->flag_king_safety[opp] && eval_values->king_attack_count[myc] > 1)  {
        int king_attack = (int)(eval_values->king_attack_value[myc] * B_KING_ATTACK *
                                KING_ATTACK_MULTI * eval_values->king_attack_count[myc] / 100);
        eval_values->king[opp] -= MAKE_SCORE(king_attack, king_attack >> 3);
    }
}

//-------------------------------------------------------------------------------------------------
//  Knights
//-------------------------------------------------------------------------------------------------
void eval_knights(BOARD *board, EVALUATION *eval_values, int myc, int opp)
{
    BBIX    piece;
    int     pcsq;
    BBIX    mobility;
    BBIX    attacks;
    int     attacked;
    U64     threat;
    int     relative_pcsq;

    piece.u64 = knight_bb(board, myc);
    while (piece.u64) {
        pcsq = bb_first(piece);
        relative_pcsq = get_relative_square(myc, pcsq);

        assert(piece_on_square(board, myc, pcsq) == KNIGHT);

        // pst
        eval_values->pieces[myc] += eval_pst_knight(pcsq);
        
        // mobility
        mobility.u64 = knight_moves_bb(pcsq) & eval_values->mobility_target[myc] & ~eval_values->pawn_attacks[opp];
        eval_values->mobility[myc] += B_KNIGHT_MOBILITY * bb_count(mobility);

        //  threats
        attacks.u64 = mobility.u64 & eval_values->undefended[opp];
        while (attacks.u64) {
            attacked = bb_first(attacks);
			assert(piece_on_square(board, opp, attacked) >= PAWN && piece_on_square(board, opp, attacked) <= KING);
            eval_values->pieces[myc] += *B_THREAT[piece_on_square(board, opp, attacked)];
            bb_clear_bit(&attacks.u64, attacked);
        }

        // checks on next move
        U64 opp_checks = (empty_bb(board) | all_pieces_bb(board, opp)) & knight_moves_bb(king_square(board, opp)) & ~eval_values->pawn_attacks[opp];
        BBIX cc;
        cc.u64 = knight_moves_bb(pcsq) & opp_checks;
        if (cc.u64) eval_values->pieces[myc] += B_CHECK_THREAT_KNIGHT * bb_count(cc);
    
        // king attack
        threat = knight_moves_bb(pcsq) & king_moves_bb(king_square(board, opp));
        if (threat) {
            eval_values->king_attack_value[myc] += KING_ATTACK_KNIGHT;
            eval_values->king_attack_count[myc]++;
        }

        //  penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_KNIGHT;
        }

        bb_clear_bit(&piece.u64, pcsq);
    }
}

//-------------------------------------------------------------------------------------------------
//  Bishops
//-------------------------------------------------------------------------------------------------
void eval_bishops(BOARD *board, EVALUATION *eval_values, int myc, int opp)
{
    BBIX    piece;
    int     pcsq;
    BBIX    mobility;
    BBIX    attacks;
    int     attacked;
    U64     moves;
    int     relative_pcsq;

    piece.u64 = bishop_bb(board, myc);
    while (piece.u64) {
        pcsq = bb_first(piece);
        relative_pcsq = get_relative_square(myc, pcsq);

        assert(piece_on_square(board, myc, pcsq) == BISHOP);

        // pst
        eval_values->pieces[myc] += eval_pst_bishop(pcsq);

        // mobility
        moves = bb_bishop_attacks(pcsq, occupied_bb(board));
        mobility.u64 = moves & eval_values->mobility_target[myc];
        eval_values->mobility[myc] += B_BISHOP_MOBILITY * bb_count(mobility);

        //  threats
        attacks.u64 = mobility.u64 & eval_values->undefended[opp];
        while (attacks.u64) {
            attacked = bb_first(attacks);
			assert(piece_on_square(board, opp, attacked) >= PAWN && piece_on_square(board, opp, attacked) <= KING);
            eval_values->pieces[myc] += *B_THREAT[piece_on_square(board, opp, attacked)];
            bb_clear_bit(&attacks.u64, attacked);
        }

        // checks on next move
        U64 opp_checks;
        opp_checks = bb_bishop_attacks(king_square(board, opp), occupied_bb(board)) & (empty_bb(board) | all_pieces_bb(board, opp)) & ~eval_values->pawn_attacks[opp];
        BBIX cc;
        cc.u64 = moves & opp_checks;
        if (cc.u64) eval_values->pieces[myc] += B_CHECK_THREAT_BISHOP * bb_count(cc);
    
        // king attack
        if (moves & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_BISHOP;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_BISHOP;
        }

        bb_clear_bit(&piece.u64, pcsq);
    }
}

//-------------------------------------------------------------------------------------------------
//  Rooks
//-------------------------------------------------------------------------------------------------
void eval_rooks(BOARD *board, EVALUATION *eval_values, int myc, int opp)
{
    BBIX    piece;
    int     pcsq;
    BBIX    mobility;
    BBIX    attacks;
    int     attacked;
    U64     moves;
    int     relative_pcsq;
    U64     rook_file_bb;

    piece.u64 = rook_bb(board, myc);
    while (piece.u64) {
        pcsq = bb_first(piece);
        relative_pcsq = get_relative_square(myc, pcsq);

        assert(piece_on_square(board, myc, pcsq) == ROOK);

        // pst
        eval_values->pieces[myc] += eval_pst_rook(pcsq);

        // mobility
        moves = bb_rook_attacks(pcsq, occupied_bb(board));
        mobility.u64 = moves & eval_values->mobility_target[myc];
        eval_values->mobility[myc] += B_ROOK_MOBILITY * bb_count(mobility);

        //  threats
        attacks.u64 = mobility.u64 & eval_values->undefended[opp];
        while (attacks.u64) {
            attacked = bb_first(attacks);
			assert(piece_on_square(board, opp, attacked) >= PAWN && piece_on_square(board, opp, attacked) <= KING);
            eval_values->pieces[myc] += *B_THREAT[piece_on_square(board, opp, attacked)];
            bb_clear_bit(&attacks.u64, attacked);
        }

        // checks on next move
        U64 opp_checks;
        opp_checks = bb_rook_attacks(king_square(board, opp), occupied_bb(board)) & (empty_bb(board) | all_pieces_bb(board, opp)) & ~eval_values->pawn_attacks[opp];
        BBIX cc;
        cc.u64 = moves & opp_checks;
        if (cc.u64) eval_values->pieces[myc] += B_CHECK_THREAT_ROOK * bb_count(cc);

        // king attack
        if (moves & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_ROOK;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_ROOK;
        }

        // rook in open file.
        rook_file_bb = north_moves_bb(pcsq) | south_moves_bb(pcsq);
        if (!(rook_file_bb & pawn_bb(board, myc))) {
            if (!(rook_file_bb & pawn_bb(board, opp)))
                eval_values->pieces[myc] += B_ROOK_FULL_OPEN;
            else
                eval_values->pieces[myc] += B_ROOK_SEMI_OPEN;
        }

        bb_clear_bit(&piece.u64, pcsq);
    }
}

//-------------------------------------------------------------------------------------------------
//  Queens
//-------------------------------------------------------------------------------------------------
void eval_queens(BOARD *board, EVALUATION *eval_values, int myc, int opp)
{
    BBIX    piece;
    int     pcsq;
    BBIX    mobility;
    BBIX    attacks;
    int     attacked;
    U64     moves1;
    U64     moves2;
    int     relative_pcsq;

    piece.u64 = queen_bb(board, myc);
    while (piece.u64) {

        pcsq = bb_first(piece);
        relative_pcsq = get_relative_square(myc, pcsq);

        assert(piece_on_square(board, myc, pcsq) == QUEEN);

        // pst
        eval_values->pieces[myc] += eval_pst_queen(myc, pcsq);

        // mobility
        moves1 = bb_rook_attacks(pcsq, occupied_bb(board));
        mobility.u64 = moves1 & eval_values->mobility_target[myc];
        moves2 = bb_bishop_attacks(pcsq, occupied_bb(board));
        mobility.u64 |= moves2 & eval_values->mobility_target[myc];
        eval_values->mobility[myc] += B_QUEEN_MOBILITY * bb_count(mobility);

        //  threats
        attacks.u64 = mobility.u64 & eval_values->undefended[opp];
        while (attacks.u64) {
            attacked = bb_first(attacks);
			assert(piece_on_square(board, opp, attacked) >= PAWN && piece_on_square(board, opp, attacked) <= KING);
			eval_values->pieces[myc] += *B_THREAT[piece_on_square(board, opp, attacked)];
            bb_clear_bit(&attacks.u64, attacked);
        }

        // checks on next move
        U64 opp_checks;
        opp_checks = bb_rook_attacks(king_square(board, opp), occupied_bb(board));
        opp_checks |= bb_bishop_attacks(king_square(board, opp), occupied_bb(board));
        opp_checks &= (empty_bb(board) | all_pieces_bb(board, opp)) & ~eval_values->pawn_attacks[opp];
        BBIX cc;
        cc.u64 = (moves1 | moves2) & opp_checks;
        if (cc.u64) eval_values->pieces[myc] += B_CHECK_THREAT_QUEEN * bb_count(cc);

        // king attack
        if ((moves1 | moves2) & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_QUEEN;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_QUEEN;
        }

        bb_clear_bit(&piece.u64, pcsq);
    }
}

// END
