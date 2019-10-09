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

//------------------------------------------------------------------------------------
//  King evaluation.
//------------------------------------------------------------------------------------

int     pawn_shelter_penalty(BOARD *board, int square, int color);
int     pawn_storm_penalty(BOARD *board, int square, int color);

//------------------------------------------------------------------------------------
//  Eval king.
//------------------------------------------------------------------------------------
void eval_kings(BOARD *board, EVALUATION *eval_values)
{
    int     myc;
    int     pcsq;
    int     rank;
    int     file;
    U64     pawns;

    assert(eval_values->flag_king_safety[WHITE] == FALSE || eval_values->flag_king_safety[WHITE] == TRUE);
    assert(eval_values->flag_king_safety[BLACK] == FALSE || eval_values->flag_king_safety[BLACK] == TRUE);
    
    pawns = pawn_bb(board, WHITE) | pawn_bb(board, BLACK);

    for (myc = WHITE; myc <= BLACK; myc++) {

        assert(king_count(board, myc) == 1);

        pcsq = king_square(board, myc);
        rank = get_rank(pcsq);
        file = get_file(pcsq);
        
        assert(pcsq >= 0 && pcsq < 64);
        assert(file >= 0 && file <= 7);
        assert(rank >= 0 && rank <= 7);

        // king pst
        eval_values->king[myc] += eval_pst_king(myc, pcsq);

        // bonus for staying close to pawns on one side of the board in end games.
        if ((pawns | BB_FILES_QS) == BB_FILES_QS)
            eval_values->king[myc] += B_PAWN_PROXIMITY * (4 - file);
        if ((pawns | BB_FILES_KS) == BB_FILES_KS)
            eval_values->king[myc] += B_PAWN_PROXIMITY * (file - 3);

        // pawn structure in front of the king and enemy pawns approaching (pawn storm).
        if (eval_values->flag_king_safety[myc] && rank != (myc == WHITE ? RANK8 : RANK1)) {
            // pawn shelter
            eval_values->king[myc] -= pawn_shelter_penalty(board, pcsq + (myc == WHITE ? -8 : 8), myc);
            if (file != FILEA)
                eval_values->king[myc] -= pawn_shelter_penalty(board, pcsq + (myc == WHITE ? -8 : 8) - 1, myc);
            if (file != FILEH)
                eval_values->king[myc] -= pawn_shelter_penalty(board, pcsq + (myc == WHITE ? -8 : 8) + 1, myc);
            // pawn storm
            eval_values->king[myc] -= pawn_storm_penalty(board, pcsq, myc);
            if (file != FILEA)
                eval_values->king[myc] -= pawn_storm_penalty(board, pcsq - 1, myc);
            if (file != FILEH)
                eval_values->king[myc] -= pawn_storm_penalty(board, pcsq + 1, myc);
        }
    }
}

//------------------------------------------------------------------------------------
//  Eval pawn protection
//------------------------------------------------------------------------------------
int pawn_shelter_penalty(BOARD *board, int pcsq, int color)
{
    int     penalty = 0;

    assert(valid_square(pcsq));
    assert(valid_color(color));

    if (piece_on_square(board, color, pcsq) != PAWN) {
        penalty += P_PAWN_SHIELD * 2;
        if (get_rank(pcsq) != (color == WHITE ? RANK8 : RANK1)) {
            if (piece_on_square(board, color, pcsq + (color == WHITE ? -8 : +8)) != PAWN)  {
                penalty += P_PAWN_SHIELD;
            }
        }
    }

    return penalty;
}

//------------------------------------------------------------------------------------
//  Eval opponent pawn storm.
//------------------------------------------------------------------------------------
int pawn_storm_penalty(BOARD *board, int pcsq, int color)
{
    BBIX    opp_pawn;
    int        penalty = 0;

    assert(valid_square(pcsq));
    assert(valid_color(color));

    if (color == WHITE) {
        opp_pawn.u64 = north_moves_bb(pcsq) & pawn_bb(board, BLACK);
        if (opp_pawn.u64) {
            switch (get_rank(bb_last(opp_pawn))) {
            case RANK5: penalty += P_PAWN_STORM * 1; break;
            case RANK4: penalty += P_PAWN_STORM * 2; break;
            case RANK3: penalty += P_PAWN_STORM * 3; break;
            }
        }
    }
    else {
        opp_pawn.u64 = south_moves_bb(pcsq) & pawn_bb(board, WHITE);
        if (opp_pawn.u64) {
            switch (get_rank(bb_first(opp_pawn))) {
            case RANK4: penalty += P_PAWN_STORM * 1; break;
            case RANK5: penalty += P_PAWN_STORM * 2; break;
            case RANK6: penalty += P_PAWN_STORM * 3; break;
            }
        }

    }

    return penalty;
}

// END
