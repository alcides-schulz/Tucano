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
    //// MATERIAL
    //SCORE_PAWN = MAKE_SCORE(85, 98);
    //SCORE_KNIGHT = MAKE_SCORE(299, 303);
    //SCORE_BISHOP = MAKE_SCORE(319, 308);
    //SCORE_ROOK = MAKE_SCORE(449, 571);
    //SCORE_QUEEN = MAKE_SCORE(1081, 965);
    //B_BISHOP_PAIR = MAKE_SCORE(33, 49);
    //// KING
    //B_PAWN_PROXIMITY = MAKE_SCORE(-7, 16);
    //P_PAWN_SHIELD = MAKE_SCORE(6, 12);
    //P_PAWN_STORM = MAKE_SCORE(5, -8);
    //// PAWN
    //B_CANDIDATE = MAKE_SCORE(2, 5);
    //B_CONNECTED = MAKE_SCORE(13, -1);
    //P_DOUBLED = MAKE_SCORE(28, 1);
    //P_ISOLATED = MAKE_SCORE(0, 13);
    //P_ISOLATED_OPEN = MAKE_SCORE(9, 20);
    //P_BACKWARD = MAKE_SCORE(13, 6);
    //P_BACKWARD_OPEN = MAKE_SCORE(44, -15);
    //// PASSED
    //B_PASSED_RANK3 = MAKE_SCORE(-19, 5);
    //B_PASSED_RANK4 = MAKE_SCORE(-13, 26);
    //B_PASSED_RANK5 = MAKE_SCORE(32, 40);
    //B_PASSED_RANK6 = MAKE_SCORE(114, 69);
    //B_PASSED_RANK7 = MAKE_SCORE(168, 96);
    //B_UNBLOCKED_RANK3 = MAKE_SCORE(-1, 2);
    //B_UNBLOCKED_RANK4 = MAKE_SCORE(10, 7);
    //B_UNBLOCKED_RANK5 = MAKE_SCORE(-2, 31);
    //B_UNBLOCKED_RANK6 = MAKE_SCORE(-8, 59);
    //B_UNBLOCKED_RANK7 = MAKE_SCORE(16, 119);
    //P_KING_FAR_MYC = MAKE_SCORE(0, 10);
    //B_KING_FAR_OPP = MAKE_SCORE(-4, 18);
    //// PIECES
    //B_ROOK_SEMI_OPEN = MAKE_SCORE(14, -2);
    //B_ROOK_FULL_OPEN = MAKE_SCORE(33, 0);
    //B_ROOK_RANK_8 = MAKE_SCORE(5, 8);
    //B_ROOK_RANK_7 = MAKE_SCORE(19, 8);
    //P_ROOK_TRAP = MAKE_SCORE(69, 9);
    //B_DOUBLE_RANK_7 = MAKE_SCORE(49, 40);
    //P_MINOR_BLOCK_PAWN = MAKE_SCORE(19, -6);
    //B_QUEEN_RANK_7 = MAKE_SCORE(-29, 33);
    //P_BISHOP_TRAP1 = MAKE_SCORE(117, 26);
    //P_BISHOP_TRAP2 = MAKE_SCORE(9, 110);
    //// MOBILITY
    //B_QUEEN_MOBILITY = MAKE_SCORE(4, 1);
    //B_ROOK_MOBILITY = MAKE_SCORE(4, 5);
    //B_BISHOP_MOBILITY = MAKE_SCORE(5, 7);
    //B_KNIGHT_MOBILITY = MAKE_SCORE(10, 5);
    //// KING_ATTACK
    //KING_ATTACK_KNIGHT = 2;
    //KING_ATTACK_BISHOP = 1;
    //KING_ATTACK_ROOK = 2;
    //KING_ATTACK_QUEEN = 1;
    //KING_ATTACK_MULTI = 31;
    //B_KING_ATTACK = 29;
    //// THREAT
    //P_PAWN_ATK_KING = MAKE_SCORE(42, -38);
    //P_PAWN_ATK_KNIGHT = MAKE_SCORE(55, 27);
    //P_PAWN_ATK_BISHOP = MAKE_SCORE(32, 60);
    //P_PAWN_ATK_ROOK = MAKE_SCORE(44, 29);
    //P_PAWN_ATK_QUEEN = MAKE_SCORE(48, 4);
    //B_THREAT_PAWN = MAKE_SCORE(-9, 31);
    //B_THREAT_KNIGHT = MAKE_SCORE(8, 43);
    //B_THREAT_BISHOP = MAKE_SCORE(9, 43);
    //B_THREAT_ROOK = MAKE_SCORE(-1, 32);
    //B_THREAT_QUEEN = MAKE_SCORE(30, 46);
    //// PST
    //PST_P_FILE_OP = 0;
    //PST_P_RANK_EG = 7;
    //PST_P_CENTER = 14;
    //PST_N_BORDER = MAKE_SCORE(4, 7);
    //PST_N_CENTER = MAKE_SCORE(8, 9);
    //PST_B_BORDER = MAKE_SCORE(0, -11);
    //PST_B_DIAGONAL = MAKE_SCORE(16, -5);
    //PST_B_CENTER = MAKE_SCORE(26, 5);
    //PST_B_BASIC = MAKE_SCORE(2, 6);
    //PST_R_CENTER = MAKE_SCORE(3, -3);
    //PST_Q_RANK0_OP = 9;
    //PST_Q_RANKS_OP = 6;
    //PST_Q_BORDER0_EG = -4;
    //PST_Q_BORDER1_EG = 5;
    //PST_Q_BORDER2_EG = 14;
    //PST_Q_BORDER3_EG = 18;
    //PST_K_RANK_OP = 0;
    //PST_K_RANK_EG = 2;
    //PST_K_FILE0_OP = -20;
    //PST_K_FILE1_OP = 34;
    //PST_K_FILE2_OP = 24;
    //PST_K_FILE3_OP = 31;
    //PST_K_FILE0_EG = -2;
    //PST_K_FILE1_EG = 19;
    //PST_K_FILE2_EG = 18;
    //PST_K_FILE3_EG = 21;

    // MATERIAL
    SCORE_PAWN           = MAKE_SCORE(81, 101);
    SCORE_KNIGHT         = MAKE_SCORE(303, 309);
    SCORE_BISHOP         = MAKE_SCORE(320, 322);
    SCORE_ROOK           = MAKE_SCORE(438, 585);
    SCORE_QUEEN          = MAKE_SCORE(1080, 985);
    B_BISHOP_PAIR        = MAKE_SCORE(40, 45);
    // KING
    B_PAWN_PROXIMITY     = MAKE_SCORE(-4, 17);
    P_PAWN_SHIELD        = MAKE_SCORE(7, 9);
    P_PAWN_STORM         = MAKE_SCORE(6, -4);
    // PAWN
    B_CANDIDATE          = MAKE_SCORE(2, 7);
    B_CONNECTED          = MAKE_SCORE(8, 3);
    P_DOUBLED            = MAKE_SCORE(12, 7);
    P_ISOLATED           = MAKE_SCORE(7, 8);
    P_ISOLATED_OPEN      = MAKE_SCORE(14, 16);
    P_BACKWARD           = MAKE_SCORE(22, -11);
    P_BACKWARD_OPEN      = MAKE_SCORE(43, -13);
    // PASSED
    B_PASSED_RANK3       = MAKE_SCORE(-6, 0);
    B_PASSED_RANK4       = MAKE_SCORE(-7, 23);
    B_PASSED_RANK5       = MAKE_SCORE(38, 40);
    B_PASSED_RANK6       = MAKE_SCORE(117, 61);
    B_PASSED_RANK7       = MAKE_SCORE(153, 111);
    B_UNBLOCKED_RANK3    = MAKE_SCORE(0, 0);
    B_UNBLOCKED_RANK4    = MAKE_SCORE(19, 6);
    B_UNBLOCKED_RANK5    = MAKE_SCORE(4, 28);
    B_UNBLOCKED_RANK6    = MAKE_SCORE(-13, 69);
    B_UNBLOCKED_RANK7    = MAKE_SCORE(13, 116);
    P_KING_FAR_MYC       = MAKE_SCORE(1, 9);
    B_KING_FAR_OPP       = MAKE_SCORE(-4, 19);
    // PIECES
    B_ROOK_SEMI_OPEN     = MAKE_SCORE(10, 3);
    B_ROOK_FULL_OPEN     = MAKE_SCORE(25, 8);
    B_ROOK_RANK_8        = MAKE_SCORE(5, 8);
    B_ROOK_RANK_7        = MAKE_SCORE(23, 14);
    B_DOUBLE_RANK_7      = MAKE_SCORE(39, 43);
    P_MINOR_BLOCK_PAWN   = MAKE_SCORE(24, -15);
    B_QUEEN_RANK_7       = MAKE_SCORE(-28, 31);
    // MOBILITY
    B_QUEEN_MOBILITY     = MAKE_SCORE(3, 3);
    B_ROOK_MOBILITY      = MAKE_SCORE(4, 5);
    B_BISHOP_MOBILITY    = MAKE_SCORE(5, 6);
    B_KNIGHT_MOBILITY    = MAKE_SCORE(9, 6);
    // KING_ATTACK
    KING_ATTACK_KNIGHT   = 2;
    KING_ATTACK_BISHOP   = 1;
    KING_ATTACK_ROOK     = 2;
    KING_ATTACK_QUEEN    = 2;
    KING_ATTACK_MULTI    = 29;
    B_KING_ATTACK        = 27;
    // THREAT
    P_PAWN_ATK_KING      = MAKE_SCORE(30, -26);
    P_PAWN_ATK_KNIGHT    = MAKE_SCORE(45, 26);
    P_PAWN_ATK_BISHOP    = MAKE_SCORE(37, 50);
    P_PAWN_ATK_ROOK      = MAKE_SCORE(38, 31);
    P_PAWN_ATK_QUEEN     = MAKE_SCORE(39, 16);
    B_THREAT_PAWN        = MAKE_SCORE(-4, 26);
    B_THREAT_KNIGHT      = MAKE_SCORE(5, 41);
    B_THREAT_BISHOP      = MAKE_SCORE(12, 38);
    B_THREAT_ROOK        = MAKE_SCORE(-3, 35);
    B_THREAT_QUEEN       = MAKE_SCORE(28, 40);
    // PST
    PST_P_FILE_OP        = 1;
    PST_P_RANK_EG        = 7;
    PST_P_CENTER         = 7;
    PST_N_BORDER         = MAKE_SCORE(7, 5);
    PST_N_CENTER         = MAKE_SCORE(8, 8);
    PST_B_BORDER         = MAKE_SCORE(-4, -4);
    PST_B_DIAGONAL       = MAKE_SCORE(15, 3);
    PST_B_CENTER         = MAKE_SCORE(22, 13);
    PST_B_BASIC          = MAKE_SCORE(5, 9);
    PST_R_CENTER         = MAKE_SCORE(4, -2);
    PST_Q_RANK0_OP       = 5;
    PST_Q_RANKS_OP       = 3;
    PST_Q_BORDER0_EG     = -2;
    PST_Q_BORDER1_EG     = 10;
    PST_Q_BORDER2_EG     = 17;
    PST_Q_BORDER3_EG     = 19;
    PST_K_RANK_OP        = 1;
    PST_K_RANK_EG        = 3;
    PST_K_FILE0_OP       = 0;
    PST_K_FILE1_OP       = 37;
    PST_K_FILE2_OP       = 26;
    PST_K_FILE3_OP       = 16;
    PST_K_FILE0_EG       = -6;
    PST_K_FILE1_EG       = 16;
    PST_K_FILE2_EG       = 18;
    PST_K_FILE3_EG       = 25;
}

// END
