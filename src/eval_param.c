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
    SCORE_PAWN = MAKE_SCORE(168, 230);
    SCORE_KNIGHT = MAKE_SCORE(649, 728);
    SCORE_BISHOP = MAKE_SCORE(659, 725);
    SCORE_ROOK = MAKE_SCORE(1029, 1378);
    SCORE_QUEEN = MAKE_SCORE(2274, 2187);
    B_BISHOP_PAIR = MAKE_SCORE(81, 129);
    B_TEMPO = 23;
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-29, 48);
    P_PAWN_SHIELD = MAKE_SCORE(10, 26);
    P_PAWN_STORM = MAKE_SCORE(-3, 2);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(6, 15);
    B_CONNECTED = MAKE_SCORE(38, -7);
    P_DOUBLED = MAKE_SCORE(23, -16);
    P_ISOLATED = MAKE_SCORE(9, 11);
    P_ISOLATED_OPEN = MAKE_SCORE(19, 35);
    P_WEAK = MAKE_SCORE(16, 24);
    B_PAWN_SPACE = MAKE_SCORE(4, 10);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-26, -15);
    B_PASSED_RANK4 = MAKE_SCORE(-26, 49);
    B_PASSED_RANK5 = MAKE_SCORE(59, 114);
    B_PASSED_RANK6 = MAKE_SCORE(224, 224);
    B_PASSED_RANK7 = MAKE_SCORE(448, 348);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(23, -4);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(21, 15);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(15, 65);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(-14, 144);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(2, 267);
    P_KING_FAR_MYC = MAKE_SCORE(-7, 24);
    B_KING_FAR_OPP = MAKE_SCORE(-17, 54);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(50, -5);
    B_ROOK_FULL_OPEN = MAKE_SCORE(97, 4);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(16, 13);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(8, -2);
    B_ROOK_MOBILITY = MAKE_SCORE(6, 11);
    B_BISHOP_MOBILITY = MAKE_SCORE(13, 12);
    B_KNIGHT_MOBILITY = MAKE_SCORE(28, 7);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 8;
    KING_ATTACK_BISHOP = 6;
    KING_ATTACK_ROOK = 5;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 6;
    KING_ATTACK_EGPCT = 147;
    B_KING_ATTACK = 57;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(116, 27);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(72, 130);
    P_PAWN_ATK_ROOK = MAKE_SCORE(105, 43);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(102, -20);
    B_THREAT_PAWN = MAKE_SCORE(-18, 78);
    B_THREAT_KNIGHT = MAKE_SCORE(0, 110);
    B_THREAT_BISHOP = MAKE_SCORE(13, 118);
    B_THREAT_ROOK = MAKE_SCORE(-25, 72);
    B_THREAT_QUEEN = MAKE_SCORE(55, 87);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(35, 15);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(17, 36);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(93, 3);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(29, 24);
    // PST
    PST_P_FILE_OP = -6;
    PST_P_RANK_EG = -26;
    PST_P_CENTER = 24;
    PST_N_BORDER = MAKE_SCORE(7, 16);
    PST_N_CENTER = MAKE_SCORE(22, 24);
    PST_B_BORDER = MAKE_SCORE(113, 16);
    PST_B_DIAGONAL = MAKE_SCORE(158, 26);
    PST_B_CENTER = MAKE_SCORE(191, 57);
    PST_B_BASIC = MAKE_SCORE(140, 58);
    PST_R_CENTER = MAKE_SCORE(15, -22);
    PST_Q_RANK0_OP = 202;
    PST_Q_RANKS_OP = 203;
    PST_Q_BORDER0_EG = 47;
    PST_Q_BORDER1_EG = 83;
    PST_Q_BORDER2_EG = 113;
    PST_Q_BORDER3_EG = 117;
    PST_K_RANK_OP = 33;
    PST_K_RANK_EG = 11;
    PST_K_FILE0_OP = -32;
    PST_K_FILE1_OP = 150;
    PST_K_FILE2_OP = 56;
    PST_K_FILE3_OP = 69;
    PST_K_FILE0_EG = -8;
    PST_K_FILE1_EG = 49;
    PST_K_FILE2_EG = 53;
    PST_K_FILE3_EG = 48;
}

// END
