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
    SCORE_PAWN = MAKE_SCORE(174, 226);
    SCORE_KNIGHT = MAKE_SCORE(646, 725);
    SCORE_BISHOP = MAKE_SCORE(654, 740);
    SCORE_ROOK = MAKE_SCORE(1051, 1373);
    SCORE_QUEEN = MAKE_SCORE(2255, 2224);
    B_BISHOP_PAIR = MAKE_SCORE(92, 116);
    B_TEMPO = 19;
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-34, 48);
    P_PAWN_SHIELD = MAKE_SCORE(14, 27);
    P_PAWN_STORM = MAKE_SCORE(-2, 0);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(10, 14);
    B_CONNECTED = MAKE_SCORE(42, -5);
    P_DOUBLED = MAKE_SCORE(22, -20);
    P_ISOLATED = MAKE_SCORE(15, 16);
    P_ISOLATED_OPEN = MAKE_SCORE(9, 51);
    P_WEAK = MAKE_SCORE(10, 18);
    B_PAWN_SPACE = MAKE_SCORE(3, 9);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-13, -18);
    B_PASSED_RANK4 = MAKE_SCORE(-24, 53);
    B_PASSED_RANK5 = MAKE_SCORE(67, 111);
    B_PASSED_RANK6 = MAKE_SCORE(238, 179);
    B_PASSED_RANK7 = MAKE_SCORE(461, 306);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(18, 6);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(19, 25);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(21, 66);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(14, 162);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(-6, 316);
    P_KING_FAR_MYC = MAKE_SCORE(-6, 25);
    B_KING_FAR_OPP = MAKE_SCORE(-19, 56);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(48, -11);
    B_ROOK_FULL_OPEN = MAKE_SCORE(105, -7);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(7, 9);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(11, -4);
    B_ROOK_MOBILITY = MAKE_SCORE(5, 14);
    B_BISHOP_MOBILITY = MAKE_SCORE(17, 11);
    B_KNIGHT_MOBILITY = MAKE_SCORE(32, 6);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 10;
    KING_ATTACK_BISHOP = 7;
    KING_ATTACK_ROOK = 5;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 6;
    KING_ATTACK_EGPCT = 124;
    B_KING_ATTACK = 56;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(136, 45);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(94, 122);
    P_PAWN_ATK_ROOK = MAKE_SCORE(101, 59);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(99, -13);
    B_THREAT_PAWN = MAKE_SCORE(-21, 78);
    B_THREAT_KNIGHT = MAKE_SCORE(10, 114);
    B_THREAT_BISHOP = MAKE_SCORE(17, 115);
    B_THREAT_ROOK = MAKE_SCORE(-31, 76);
    B_THREAT_QUEEN = MAKE_SCORE(69, 96);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(34, 15);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(25, 32);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(84, 18);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(35, 15);
    // PST
    PST_P_FILE_OP = 3;
    PST_P_RANK_EG = -22;
    PST_P_CENTER = 21;
    PST_N_BORDER = MAKE_SCORE(6, 14);
    PST_N_CENTER = MAKE_SCORE(24, 27);
    PST_B_BORDER = MAKE_SCORE(96, 13);
    PST_B_DIAGONAL = MAKE_SCORE(146, 44);
    PST_B_CENTER = MAKE_SCORE(162, 57);
    PST_B_BASIC = MAKE_SCORE(118, 62);
    PST_R_CENTER = MAKE_SCORE(15, -15);
    PST_Q_RANK0_OP = 193;
    PST_Q_RANKS_OP = 181;
    PST_Q_BORDER0_EG = 63;
    PST_Q_BORDER1_EG = 96;
    PST_Q_BORDER2_EG = 120;
    PST_Q_BORDER3_EG = 110;
    PST_K_RANK_OP = 20;
    PST_K_RANK_EG = 7;
    PST_K_FILE0_OP = -55;
    PST_K_FILE1_OP = 132;
    PST_K_FILE2_OP = 33;
    PST_K_FILE3_OP = 54;
    PST_K_FILE0_EG = -14;
    PST_K_FILE1_EG = 53;
    PST_K_FILE2_EG = 53;
    PST_K_FILE3_EG = 51;
}

// END
