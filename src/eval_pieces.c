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
static const U64 MY_BB_RANK_678[COLORS] = {BB_RANK_6 | BB_RANK_7 | BB_RANK_8, BB_RANK_3 | BB_RANK_2 | BB_RANK_1};
static const U64 BB_OUTPOST[COLORS] = {((U64)0x007E7E7E00000000), ((U64)0x000000007E7E7E00)};
static const U64 BB_BLOCK_PAWN[COLORS] = {((U64)0x0000000000180000), ((U64)0x0000180000000000)};
static const int SQUARE_BEHIND[COLORS] = {+8, -8};
static const int BISHOP_TRAP_A7[COLORS] = {A7, A2};
static const int BISHOP_TRAP_A7_PAWN[COLORS][2] = {{B6, C7}, {B3, C2}};
static const int BISHOP_TRAP_H7[COLORS] = {H7, H2};
static const int BISHOP_TRAP_H7_PAWN[COLORS][2] = {{G6, F7}, {G3, F2}};

static const int ROOK_TRAP_A1_RSQ[COLORS] = {A1, A8};
static const int ROOK_TRAP_B1_RSQ[COLORS] = {B1, B8};
static const U64 ROOK_TRAP_A1_KSQ[COLORS] = {((U64)0x0000000000000060), ((U64)0x6000000000000000)}; // B1-C1, B8-C8
static const int ROOK_TRAP_H1_RSQ[COLORS] = {H1, H8};
static const int ROOK_TRAP_G1_RSQ[COLORS] = {G1, G8};
static const U64 ROOK_TRAP_H1_KSQ[COLORS] = {((U64)0x0000000000000006), ((U64)0x0600000000000000)}; // G1-F1, G8-F8

void    eval_pieces_prepare(BOARD *board, EVALUATION *eval_values);
void    eval_knights(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_bishops(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_rooks(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_queens(BOARD *board, EVALUATION *eval_values, int myc, int opp);
void    eval_pieces_finalize(BOARD *board, EVALUATION *eval_values, int myc, int opp);



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

    eval_pieces_finalize(board, eval_values, WHITE, BLACK);
    eval_pieces_finalize(board, eval_values, BLACK, WHITE);
}

//-------------------------------------------------------------------------------------------------
//  Initialize data used by pieces evaluation
//-------------------------------------------------------------------------------------------------
void eval_pieces_prepare(BOARD *board, EVALUATION *eval_values)
{
    eval_values->king_attack_count[WHITE] = eval_values->king_attack_count[BLACK] = 0;
    eval_values->king_attack_value[WHITE] = eval_values->king_attack_value[BLACK] = 0;

    eval_values->pawn_attacks[WHITE] = (pawn_bb(board, WHITE) & BB_NO_AFILE) << 9 |
        (pawn_bb(board, WHITE) & BB_NO_HFILE) << 7;
    eval_values->pawn_attacks[BLACK] = (pawn_bb(board, BLACK) & BB_NO_HFILE) >> 9 |
        (pawn_bb(board, BLACK) & BB_NO_AFILE) >> 7;

    eval_values->undefended[WHITE] = all_pieces_bb(board, WHITE) & ~eval_values->pawn_attacks[WHITE];
    eval_values->undefended[BLACK] = all_pieces_bb(board, BLACK) & ~eval_values->pawn_attacks[BLACK];

    eval_values->mobility_target[WHITE] = ~all_pieces_bb(board, WHITE) | qrnb_bb(board, WHITE);
    eval_values->mobility_target[BLACK] = ~all_pieces_bb(board, BLACK) | qrnb_bb(board, BLACK);
}

//-------------------------------------------------------------------------------------------------
//  Calculate final eval terms for pieces
//-------------------------------------------------------------------------------------------------
void eval_pieces_finalize(BOARD *board, EVALUATION *eval_values, int myc, int opp)
{
    int     king_attack;

    // pawns attacking king zone
    if (eval_values->pawn_attacks[myc] & king_moves_bb(king_square(board, opp)) & ~pawn_bb(board, opp))
        eval_values->king[opp] -= P_PAWN_ATK_KING;

    // calculate king attack
    if (eval_values->flag_king_safety[opp] && eval_values->king_attack_count[myc] > 1)  {
        king_attack = (int)(eval_values->king_attack_value[myc] * B_KING_ATTACK *
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

        //  Blocking central pawns
        if (square_bb(pcsq) & BB_BLOCK_PAWN[myc] && piece_on_square(board, myc, pcsq + SQUARE_BEHIND[myc]) == PAWN)
            eval_values->pieces[myc] -= P_MINOR_BLOCK_PAWN;

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

        // king attack
        if (moves & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_BISHOP;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_BISHOP;
        }

        // Blocking central pawns
        if (square_bb(pcsq) & BB_BLOCK_PAWN[myc] && piece_on_square(board, myc, pcsq + SQUARE_BEHIND[myc]) == PAWN)
            eval_values->pieces[myc] -= P_MINOR_BLOCK_PAWN;

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

        // king attack
        if (moves & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_ROOK;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_ROOK;
        }

        // rook on 7th rank
        if (get_rank(pcsq) == MY_RANK_7[myc]) {
            if ((MY_BB_RANK_7[myc] & pawn_bb(board, opp)) || (MY_BB_RANK_8[myc] & king_bb(board, opp))) {
                eval_values->pieces[myc] += B_ROOK_RANK_7;
            }
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

        // king attack
        if ((moves1 | moves2) & king_moves_bb(king_square(board, opp))) {
            eval_values->king_attack_value[myc] += KING_ATTACK_QUEEN;
            eval_values->king_attack_count[myc]++;
        }

        // penalty when attacked by pawn
        if (eval_values->pawn_attacks[opp] & square_bb(pcsq)) {
            eval_values->pieces[myc] -= P_PAWN_ATK_QUEEN;
        }

        // queen on 7th rank
        if (get_rank(pcsq) == MY_RANK_7[myc]) {
            if ((MY_BB_RANK_7[myc] & pawn_bb(board, opp)) || (MY_BB_RANK_8[myc] & king_bb(board, opp))) {
                eval_values->pieces[myc] += B_QUEEN_RANK_7;
                if (MY_BB_RANK_7[myc] & rook_bb(board, myc))
                    eval_values->pieces[myc] += B_DOUBLE_RANK_7;
            }
        }

        bb_clear_bit(&piece.u64, pcsq);
    }
}

// END
