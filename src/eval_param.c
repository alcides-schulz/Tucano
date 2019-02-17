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
    // Evaluation tuning information: iteration=46 7087 minutes param_size: 136

    // MATERIAL
    SCORE_PAWN = MAKE_SCORE(187, 209);
    SCORE_KNIGHT = MAKE_SCORE(638, 698);
    SCORE_BISHOP = MAKE_SCORE(653, 716);
    SCORE_ROOK = MAKE_SCORE(1023, 1297);
    SCORE_QUEEN = MAKE_SCORE(2176, 2199);
    B_BISHOP_PAIR = MAKE_SCORE(94, 104);
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-6, 44);
    P_PAWN_SHIELD = MAKE_SCORE(9, 20);
    P_PAWN_STORM = MAKE_SCORE(-1, -11);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(14, 19);
    B_CONNECTED = MAKE_SCORE(36, -5);
    P_DOUBLED = MAKE_SCORE(9, -37);
    P_ISOLATED = MAKE_SCORE(21, 10);
    P_ISOLATED_OPEN = MAKE_SCORE(1, 46);
    P_WEAK = MAKE_SCORE(9, 20);
    B_PAWN_SPACE = MAKE_SCORE(2, 8);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-41, -20);
    B_PASSED_RANK4 = MAKE_SCORE(-35, 50);
    B_PASSED_RANK5 = MAKE_SCORE(75, 87);
    B_PASSED_RANK6 = MAKE_SCORE(257, 150);
    B_PASSED_RANK7 = MAKE_SCORE(440, 315);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(12, 12);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(22, 19);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(28, 57);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(18, 147);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(-12, 273);
    P_KING_FAR_MYC = MAKE_SCORE(-12, 24);
    B_KING_FAR_OPP = MAKE_SCORE(-18, 52);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(45, 8);
    B_ROOK_FULL_OPEN = MAKE_SCORE(97, -1);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(6, 11);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(9, 2);
    B_ROOK_MOBILITY = MAKE_SCORE(7, 10);
    B_BISHOP_MOBILITY = MAKE_SCORE(16, 10);
    B_KNIGHT_MOBILITY = MAKE_SCORE(26, 6);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 9;
    KING_ATTACK_BISHOP = 5;
    KING_ATTACK_ROOK = 6;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 6;
    KING_ATTACK_EGPCT = 104;
    B_KING_ATTACK = 60;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(109, 48);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(54, 111);
    P_PAWN_ATK_ROOK = MAKE_SCORE(81, 21);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(89, -1);
    B_THREAT_PAWN = MAKE_SCORE(-10, 62);
    B_THREAT_KNIGHT = MAKE_SCORE(8, 111);
    B_THREAT_BISHOP = MAKE_SCORE(10, 75);
    B_THREAT_ROOK = MAKE_SCORE(-11, 62);
    B_THREAT_QUEEN = MAKE_SCORE(70, 72);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(28, 10);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(28, 28);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(71, 17);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(29, 20);
    // PST
    PST_P_FILE_OP = -2;
    PST_P_RANK_EG = -19;
    PST_P_CENTER = 27;
    PST_N_BORDER = MAKE_SCORE(7, 16);
    PST_N_CENTER = MAKE_SCORE(24, 22);
    PST_B_BORDER = MAKE_SCORE(55, -11);
    PST_B_DIAGONAL = MAKE_SCORE(81, 24);
    PST_B_CENTER = MAKE_SCORE(124, 45);
    PST_B_BASIC = MAKE_SCORE(72, 38);
    PST_R_CENTER = MAKE_SCORE(16, -16);
    PST_Q_RANK0_OP = 158;
    PST_Q_RANKS_OP = 144;
    PST_Q_BORDER0_EG = 60;
    PST_Q_BORDER1_EG = 88;
    PST_Q_BORDER2_EG = 103;
    PST_Q_BORDER3_EG = 114;
    PST_K_RANK_OP = 22;
    PST_K_RANK_EG = 6;
    PST_K_FILE0_OP = -30;
    PST_K_FILE1_OP = 110;
    PST_K_FILE2_OP = 28;
    PST_K_FILE3_OP = 65;
    PST_K_FILE0_EG = -24;
    PST_K_FILE1_EG = 42;
    PST_K_FILE2_EG = 48;
    PST_K_FILE3_EG = 59;


    // Equal old

    //// MATERIAL
    //SCORE_PAWN = MAKE_SCORE(187, 209);
    //SCORE_KNIGHT = MAKE_SCORE(638, 699);
    //SCORE_BISHOP = MAKE_SCORE(653, 717);
    //SCORE_ROOK = MAKE_SCORE(1025, 1299);
    //SCORE_QUEEN = MAKE_SCORE(2175, 2201);
    //B_BISHOP_PAIR = MAKE_SCORE(94, 103);
    //// KING
    //B_PAWN_PROXIMITY = MAKE_SCORE(-6, 44);
    //P_PAWN_SHIELD = MAKE_SCORE(9, 20);
    //P_PAWN_STORM = MAKE_SCORE(-1, -11);
    //// PAWN
    //B_CANDIDATE = MAKE_SCORE(14, 19);
    //B_CONNECTED = MAKE_SCORE(36, -5);
    //P_DOUBLED = MAKE_SCORE(9, -37);
    //P_ISOLATED = MAKE_SCORE(21, 10);
    //P_ISOLATED_OPEN = MAKE_SCORE(1, 46);
    //P_WEAK = MAKE_SCORE(9, 20);
    //B_PAWN_SPACE = MAKE_SCORE(2, 8);
    //// PASSED
    //B_PASSED_RANK3 = MAKE_SCORE(-34, -19);
    //B_PASSED_RANK4 = MAKE_SCORE(-35, 50);
    //B_PASSED_RANK5 = MAKE_SCORE(75, 87);
    //B_PASSED_RANK6 = MAKE_SCORE(258, 150);
    //B_PASSED_RANK7 = MAKE_SCORE(448, 310);
    //B_UNBLOCKED_RANK3 = MAKE_SCORE(8, 11);
    //B_UNBLOCKED_RANK4 = MAKE_SCORE(22, 19);
    //B_UNBLOCKED_RANK5 = MAKE_SCORE(28, 57);
    //B_UNBLOCKED_RANK6 = MAKE_SCORE(16, 147);
    //B_UNBLOCKED_RANK7 = MAKE_SCORE(-21, 280);
    //P_KING_FAR_MYC = MAKE_SCORE(-12, 24);
    //B_KING_FAR_OPP = MAKE_SCORE(-18, 52);
    //// PIECES
    //B_ROOK_SEMI_OPEN = MAKE_SCORE(45, 8);
    //B_ROOK_FULL_OPEN = MAKE_SCORE(97, -1);
    //P_PAWN_BISHOP_SQ = MAKE_SCORE(6, 11);
    //// MOBILITY
    //B_QUEEN_MOBILITY = MAKE_SCORE(9, 2);
    //B_ROOK_MOBILITY = MAKE_SCORE(7, 10);
    //B_BISHOP_MOBILITY = MAKE_SCORE(16, 10);
    //B_KNIGHT_MOBILITY = MAKE_SCORE(26, 6);
    //// KING_ATTACK
    //KING_ATTACK_KNIGHT = 9;
    //KING_ATTACK_BISHOP = 5;
    //KING_ATTACK_ROOK = 6;
    //KING_ATTACK_QUEEN = 1;
    //KING_ATTACK_MULTI = 6;
    //KING_ATTACK_EGPCT = 104;
    //B_KING_ATTACK = 60;
    //// THREAT
    //P_PAWN_ATK_KNIGHT = MAKE_SCORE(109, 48);
    //P_PAWN_ATK_BISHOP = MAKE_SCORE(54, 113);
    //P_PAWN_ATK_ROOK = MAKE_SCORE(93, 21);
    //P_PAWN_ATK_QUEEN = MAKE_SCORE(89, -5);
    //B_THREAT_PAWN = MAKE_SCORE(-10, 62);
    //B_THREAT_KNIGHT = MAKE_SCORE(8, 110);
    //B_THREAT_BISHOP = MAKE_SCORE(10, 75);
    //B_THREAT_ROOK = MAKE_SCORE(-11, 63);
    //B_THREAT_QUEEN = MAKE_SCORE(70, 73);
    //B_CHECK_THREAT_KNIGHT = MAKE_SCORE(28, 10);
    //B_CHECK_THREAT_BISHOP = MAKE_SCORE(28, 28);
    //B_CHECK_THREAT_ROOK = MAKE_SCORE(71, 17);
    //B_CHECK_THREAT_QUEEN = MAKE_SCORE(29, 20);
    //// PST
    //PST_P_FILE_OP = -2;
    //PST_P_RANK_EG = -19;
    //PST_P_CENTER = 27;
    //PST_N_BORDER = MAKE_SCORE(7, 16);
    //PST_N_CENTER = MAKE_SCORE(24, 22);
    //PST_B_BORDER = MAKE_SCORE(55, -11);
    //PST_B_DIAGONAL = MAKE_SCORE(81, 25);
    //PST_B_CENTER = MAKE_SCORE(124, 45);
    //PST_B_BASIC = MAKE_SCORE(72, 38);
    //PST_R_CENTER = MAKE_SCORE(16, -16);
    //PST_Q_RANK0_OP = 158;
    //PST_Q_RANKS_OP = 144;
    //PST_Q_BORDER0_EG = 61;
    //PST_Q_BORDER1_EG = 88;
    //PST_Q_BORDER2_EG = 102;
    //PST_Q_BORDER3_EG = 111;
    //PST_K_RANK_OP = 22;
    //PST_K_RANK_EG = 6;
    //PST_K_FILE0_OP = -26;
    //PST_K_FILE1_OP = 110;
    //PST_K_FILE2_OP = 28;
    //PST_K_FILE3_OP = 65;
    //PST_K_FILE0_EG = -24;
    //PST_K_FILE1_EG = 42;
    //PST_K_FILE2_EG = 48;
    //PST_K_FILE3_EG = 59;



    
    //// MATERIAL
    //SCORE_PAWN = MAKE_SCORE(168, 210);
    //SCORE_KNIGHT = MAKE_SCORE(638, 704);
    //SCORE_BISHOP = MAKE_SCORE(638, 730);
    //SCORE_ROOK = MAKE_SCORE(1036, 1310);
    //SCORE_QUEEN = MAKE_SCORE(2162, 2220);
    //B_BISHOP_PAIR = MAKE_SCORE(78, 86);
    //// KING
    //B_PAWN_PROXIMITY = MAKE_SCORE(-22, 42);
    //P_PAWN_SHIELD = MAKE_SCORE(14, 18);
    //P_PAWN_STORM = MAKE_SCORE(-2, 12);
    //// PAWN
    //B_CANDIDATE = MAKE_SCORE(0, 20);
    //B_CONNECTED = MAKE_SCORE(28, 4);
    //P_DOUBLED = MAKE_SCORE(30, -14);
    //P_ISOLATED = MAKE_SCORE(2, 20);
    //P_ISOLATED_OPEN = MAKE_SCORE(18, 40);
    //P_WEAK = MAKE_SCORE(6, 0);
    //B_PAWN_SPACE = MAKE_SCORE(2, 6);
    //// PASSED
    //B_PASSED_RANK3 = MAKE_SCORE(-14, -22);
    //B_PASSED_RANK4 = MAKE_SCORE(-18, 46);
    //B_PASSED_RANK5 = MAKE_SCORE(74, 94);
    //B_PASSED_RANK6 = MAKE_SCORE(248, 162);
    //B_PASSED_RANK7 = MAKE_SCORE(440, 288);
    //B_UNBLOCKED_RANK3 = MAKE_SCORE(20, -6);
    //B_UNBLOCKED_RANK4 = MAKE_SCORE(36, 14);
    //B_UNBLOCKED_RANK5 = MAKE_SCORE(26, 68);
    //B_UNBLOCKED_RANK6 = MAKE_SCORE(4, 146);
    //B_UNBLOCKED_RANK7 = MAKE_SCORE(-28, 284);
    //P_KING_FAR_MYC = MAKE_SCORE(-2, 20);
    //B_KING_FAR_OPP = MAKE_SCORE(-16, 48);
    //// PIECES
    //B_ROOK_SEMI_OPEN = MAKE_SCORE(32, -4);
    //B_ROOK_FULL_OPEN = MAKE_SCORE(78, 4);
    //P_PAWN_BISHOP_SQ = MAKE_SCORE(2, 8);
    //// MOBILITY
    //B_QUEEN_MOBILITY = MAKE_SCORE(6, 8);
    //B_ROOK_MOBILITY = MAKE_SCORE(4, 14);
    //B_BISHOP_MOBILITY = MAKE_SCORE(12, 16);
    //B_KNIGHT_MOBILITY = MAKE_SCORE(22, 16);
    //// KING_ATTACK
    //KING_ATTACK_KNIGHT = 14;
    //KING_ATTACK_BISHOP = 10;
    //KING_ATTACK_ROOK = 12;
    //KING_ATTACK_QUEEN = 2;
    //KING_ATTACK_MULTI = 10;
    //KING_ATTACK_EGPCT = 88;
    //B_KING_ATTACK = 66;
    //// THREAT
    //P_PAWN_ATK_KNIGHT = MAKE_SCORE(96, 38);
    //P_PAWN_ATK_BISHOP = MAKE_SCORE(48, 120);
    //P_PAWN_ATK_ROOK = MAKE_SCORE(112, 38);
    //P_PAWN_ATK_QUEEN = MAKE_SCORE(84, 0);
    //B_THREAT_PAWN = MAKE_SCORE(-14, 64);
    //B_THREAT_KNIGHT = MAKE_SCORE(4, 100);
    //B_THREAT_BISHOP = MAKE_SCORE(18, 92);
    //B_THREAT_ROOK = MAKE_SCORE(-18, 72);
    //B_THREAT_QUEEN = MAKE_SCORE(58, 66);
    //B_CHECK_THREAT_KNIGHT = MAKE_SCORE(28, 10);
    //B_CHECK_THREAT_BISHOP = MAKE_SCORE(22, 24);
    //B_CHECK_THREAT_ROOK = MAKE_SCORE(56, 18);
    //B_CHECK_THREAT_QUEEN = MAKE_SCORE(28, 10);
    //// PST
    //PST_P_FILE_OP = 2;
    //PST_P_RANK_EG = -14;
    //PST_P_CENTER = 12;
    //PST_N_BORDER = MAKE_SCORE(4, 12);
    //PST_N_CENTER = MAKE_SCORE(16, 28);
    //PST_B_BORDER = MAKE_SCORE(50, 6);
    //PST_B_DIAGONAL = MAKE_SCORE(84, 20);
    //PST_B_CENTER = MAKE_SCORE(112, 42);
    //PST_B_BASIC = MAKE_SCORE(62, 38);
    //PST_R_CENTER = MAKE_SCORE(4, -6);
    //PST_Q_RANK0_OP = 150;
    //PST_Q_RANKS_OP = 150;
    //PST_Q_BORDER0_EG = 72;
    //PST_Q_BORDER1_EG = 92;
    //PST_Q_BORDER2_EG = 92;
    //PST_Q_BORDER3_EG = 96;
    //PST_K_RANK_OP = 10;
    //PST_K_RANK_EG = 16;
    //PST_K_FILE0_OP = 0;
    //PST_K_FILE1_OP = 94;
    //PST_K_FILE2_OP = 46;
    //PST_K_FILE3_OP = 72;
    //PST_K_FILE0_EG = -12;
    //PST_K_FILE1_EG = 34;
    //PST_K_FILE2_EG = 44;
    //PST_K_FILE3_EG = 54;
}

// END
