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
    SCORE_PAWN = MAKE_SCORE(79, 100);
    SCORE_KNIGHT = MAKE_SCORE(300, 317);
    SCORE_BISHOP = MAKE_SCORE(319, 327);
    SCORE_ROOK = MAKE_SCORE(453, 608);
    SCORE_QUEEN = MAKE_SCORE(1081, 1010);
    B_BISHOP_PAIR = MAKE_SCORE(39, 54);
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-2, 19);
    P_PAWN_SHIELD = MAKE_SCORE(7, 12);
    P_PAWN_STORM = MAKE_SCORE(4, -9);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(2, 6);
    B_CONNECTED = MAKE_SCORE(13, 1);
    P_DOUBLED = MAKE_SCORE(13, 1);
    P_ISOLATED = MAKE_SCORE(7, 10);
    P_ISOLATED_OPEN = MAKE_SCORE(13, 20);
    P_BACKWARD = MAKE_SCORE(9, 10);
    P_BACKWARD_OPEN = MAKE_SCORE(59, -8);
    B_PAWN_SPACE = MAKE_SCORE(1, 2);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-17, -6);
    B_PASSED_RANK4 = MAKE_SCORE(-10, 23);
    B_PASSED_RANK5 = MAKE_SCORE(31, 43);
    B_PASSED_RANK6 = MAKE_SCORE(120, 70);
    B_PASSED_RANK7 = MAKE_SCORE(180, 139);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(3, 3);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(8, 7);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(-3, 31);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(-16, 71);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(5, 118);
    P_KING_FAR_MYC = MAKE_SCORE(-1, 10);
    B_KING_FAR_OPP = MAKE_SCORE(-3, 20);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(15, -8);
    B_ROOK_FULL_OPEN = MAKE_SCORE(34, -2);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(4, 3);
    B_ROOK_MOBILITY = MAKE_SCORE(4, 6);
    B_BISHOP_MOBILITY = MAKE_SCORE(5, 7);
    B_KNIGHT_MOBILITY = MAKE_SCORE(10, 6);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 2;
    KING_ATTACK_BISHOP = 1;
    KING_ATTACK_ROOK = 3;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 29;
    KING_ATTACK_EGPCT = 33;
    B_KING_ATTACK = 27;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(44, 19);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(29, 55);
    P_PAWN_ATK_ROOK = MAKE_SCORE(44, 18);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(35, -5);
    B_THREAT_PAWN = MAKE_SCORE(-7, 33);
    B_THREAT_KNIGHT = MAKE_SCORE(6, 43);
    B_THREAT_BISHOP = MAKE_SCORE(8, 45);
    B_THREAT_ROOK = MAKE_SCORE(-6, 35);
    B_THREAT_QUEEN = MAKE_SCORE(29, 29);
    // CHECK_THREAT
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(8, 0);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(16, 4);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(16, 4);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(18, -9);
    // PST
    PST_P_FILE_OP = -1;
    PST_P_RANK_EG = -4;
    PST_P_CENTER = 10;
    PST_N_BORDER = MAKE_SCORE(3, 6);
    PST_N_CENTER = MAKE_SCORE(10, 9);
    PST_B_BORDER = MAKE_SCORE(4, -4);
    PST_B_DIAGONAL = MAKE_SCORE(22, 0);
    PST_B_CENTER = MAKE_SCORE(35, 12);
    PST_B_BASIC = MAKE_SCORE(12, 11);
    PST_R_CENTER = MAKE_SCORE(5, -3);
    PST_Q_RANK0_OP = 12;
    PST_Q_RANKS_OP = 13;
    PST_Q_BORDER0_EG = 9;
    PST_Q_BORDER1_EG = 11;
    PST_Q_BORDER2_EG = 18;
    PST_Q_BORDER3_EG = 18;
    PST_K_RANK_OP = 7;
    PST_K_RANK_EG = 7;
    PST_K_FILE0_OP = -5;
    PST_K_FILE1_OP = 51;
    PST_K_FILE2_OP = 27;
    PST_K_FILE3_OP = 37;
    PST_K_FILE0_EG = 0;
    PST_K_FILE1_EG = 18;
    PST_K_FILE2_EG = 18;
    PST_K_FILE3_EG = 19;
}

// END
