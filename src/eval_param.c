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
//  Define eval parameter values
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
//  Assign values for evaluation. Created manually or using automated tuning.
//------------------------------------------------------------------------------------
void eval_param_init(void)
{
    // MATERIAL
    SCORE_PAWN = MAKE_SCORE(84, 105);
    SCORE_KNIGHT = MAKE_SCORE(319, 352);
    SCORE_BISHOP = MAKE_SCORE(319, 365);
    SCORE_ROOK = MAKE_SCORE(518, 655);
    SCORE_QUEEN = MAKE_SCORE(1081, 1110);
    B_BISHOP_PAIR = MAKE_SCORE(39, 43);
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-11, 21);
    P_PAWN_SHIELD = MAKE_SCORE(7, 9);
    P_PAWN_STORM = MAKE_SCORE(-1, 6);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(0, 10);
    B_CONNECTED = MAKE_SCORE(14, 2);
    P_DOUBLED = MAKE_SCORE(15, -7);
    P_ISOLATED = MAKE_SCORE(1, 10);
    P_ISOLATED_OPEN = MAKE_SCORE(9, 20);
    P_BACKWARD = MAKE_SCORE(-22, 23);
    P_BACKWARD_OPEN = MAKE_SCORE(52, 12);
    B_PAWN_SPACE = MAKE_SCORE(1, 3);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-7, -11);
    B_PASSED_RANK4 = MAKE_SCORE(-9, 23);
    B_PASSED_RANK5 = MAKE_SCORE(37, 47);
    B_PASSED_RANK6 = MAKE_SCORE(124, 81);
    B_PASSED_RANK7 = MAKE_SCORE(220, 144);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(10, -3);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(18, 7);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(13, 34);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(2, 73);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(-14, 142);
    P_KING_FAR_MYC = MAKE_SCORE(-1, 10);
    B_KING_FAR_OPP = MAKE_SCORE(-8, 24);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(16, -2);
    B_ROOK_FULL_OPEN = MAKE_SCORE(39, 2);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(3, 4);
    B_ROOK_MOBILITY = MAKE_SCORE(2, 7);
    B_BISHOP_MOBILITY = MAKE_SCORE(6, 8);
    B_KNIGHT_MOBILITY = MAKE_SCORE(11, 8);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 2;
    KING_ATTACK_BISHOP = 1;
    KING_ATTACK_ROOK = 3;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 26;
    KING_ATTACK_EGPCT = 33;
    B_KING_ATTACK = 27;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(48, 19);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(24, 60);
    P_PAWN_ATK_ROOK = MAKE_SCORE(56, 19);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(42, 0);
    B_THREAT_PAWN = MAKE_SCORE(-7, 32);
    B_THREAT_KNIGHT = MAKE_SCORE(2, 50);
    B_THREAT_BISHOP = MAKE_SCORE(9, 46);
    B_THREAT_ROOK = MAKE_SCORE(-9, 36);
    B_THREAT_QUEEN = MAKE_SCORE(29, 33);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(16, -7);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(11, 12);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(28, 9);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(14, 5);
    // PST
    PST_P_FILE_OP = 1;
    PST_P_RANK_EG = -7;
    PST_P_CENTER = 6;
    PST_N_BORDER = MAKE_SCORE(2, 6);
    PST_N_CENTER = MAKE_SCORE(8, 14);
    PST_B_BORDER = MAKE_SCORE(25, 3);
    PST_B_DIAGONAL = MAKE_SCORE(42, 10);
    PST_B_CENTER = MAKE_SCORE(56, 21);
    PST_B_BASIC = MAKE_SCORE(31, 19);
    PST_R_CENTER = MAKE_SCORE(2, -3);
    PST_Q_RANK0_OP = 75;
    PST_Q_RANKS_OP = 75;
    PST_Q_BORDER0_EG = 36;
    PST_Q_BORDER1_EG = 46;
    PST_Q_BORDER2_EG = 46;
    PST_Q_BORDER3_EG = 48;
    PST_K_RANK_OP = 5;
    PST_K_RANK_EG = 8;
    PST_K_FILE0_OP = 0;
    PST_K_FILE1_OP = 47;
    PST_K_FILE2_OP = 23;
    PST_K_FILE3_OP = 36;
    PST_K_FILE0_EG = -6;
    PST_K_FILE1_EG = 17;
    PST_K_FILE2_EG = 22;
    PST_K_FILE3_EG = 27;
}

// END
