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
//  Passed pawns evaluation
//-------------------------------------------------------------------------------------------------
int SCORE_ZERO  = MAKE_SCORE(0, 0);

int *B_PASSED[RANKS]    = {&SCORE_ZERO,
                           &SCORE_ZERO,
                           &B_PASSED_RANK3,
                           &B_PASSED_RANK4,
                           &B_PASSED_RANK5,
                           &B_PASSED_RANK6,
                           &B_PASSED_RANK7,
                           &SCORE_ZERO};

int *B_UNBLOCKED[RANKS] = {&SCORE_ZERO,
                           &SCORE_ZERO,
                           &B_UNBLOCKED_RANK3,
                           &B_UNBLOCKED_RANK4,
                           &B_UNBLOCKED_RANK5,
                           &B_UNBLOCKED_RANK6,
                           &B_UNBLOCKED_RANK7,
                           &SCORE_ZERO};

//-------------------------------------------------------------------------------------------------
//  Evaluate passed pawns.
//-------------------------------------------------------------------------------------------------
void eval_passed(BOARD *board, EVALUATION *eval_values)
{
    BBIX    pawns;
    int     king_dist[2];

    for (int myc = WHITE; myc <= BLACK; myc++) {
        int opp = flip_color(myc);

        // additional bonus/penalties for passed pawns considering king/pieces.
        pawns.u64 = eval_values->bb_passers[myc];
        while (pawns.u64) {
            int pcsq = bb_first(pawns);
            int rank = get_rank(pcsq);
            int relative_rank = get_relative_rank(myc, rank);
            int fwsq = get_front_square(myc, pcsq);
    
            assert(piece_on_square(board, myc, pcsq) == PAWN);
            assert(pawn_is_passed(board, pcsq, myc));
            
            eval_values->passed[myc] += *B_PASSED[relative_rank];

            // bonus for unblocked passed pawns
            if (!(square_bb(fwsq) & occupied_bb(board)))
                eval_values->passed[myc] += *B_UNBLOCKED[relative_rank];

            // bonus/penalty according king distance to square in front
            king_dist[myc] = square_distance(king_square(board, myc), fwsq);
            king_dist[opp] = square_distance(king_square(board, opp), fwsq);
            eval_values->passed[myc] -= (king_dist[myc] * P_KING_FAR_MYC);
            eval_values->passed[myc] += (king_dist[opp] * B_KING_FAR_OPP);

            bb_clear_bit(&pawns.u64, pcsq);
        }

    }
}

// END
