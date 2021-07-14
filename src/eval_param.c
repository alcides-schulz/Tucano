/*-------------------------------------------------------------------------------
tucano is a chess playing engine developed by Alcides Schulz.
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
    SCORE_PAWN = MAKE_SCORE(133, 255);
    SCORE_KNIGHT = MAKE_SCORE(640, 844);
    SCORE_BISHOP = MAKE_SCORE(680, 867);
    SCORE_ROOK = MAKE_SCORE(984, 1535);
    SCORE_QUEEN = MAKE_SCORE(2448, 2396);
    B_BISHOP_PAIR = MAKE_SCORE(40, 163);
    B_TEMPO = 28;
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-8, 41);
    P_PAWN_SHIELD = MAKE_SCORE(11, 13);
    P_PAWN_STORM = MAKE_SCORE(1, 9);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(2, 15);
    B_CONNECTED[0] = MAKE_SCORE(24, 9);
    B_CONNECTED[1] = MAKE_SCORE(36, 7);
    B_CONNECTED[2] = MAKE_SCORE(20, -17);
    B_CONNECTED[3] = MAKE_SCORE(35, 4);
    B_CONNECTED[4] = MAKE_SCORE(41, 101);
    B_CONNECTED[5] = MAKE_SCORE(155, 12);
    B_CONNECTED_PASSER[0] = MAKE_SCORE(10, -59);
    B_CONNECTED_PASSER[1] = MAKE_SCORE(-1, -7);
    B_CONNECTED_PASSER[2] = MAKE_SCORE(-11, 38);
    B_CONNECTED_PASSER[3] = MAKE_SCORE(25, 38);
    B_CONNECTED_PASSER[4] = MAKE_SCORE(85, -36);
    B_CONNECTED_PASSER[5] = MAKE_SCORE(149, 17);
    P_DOUBLED = MAKE_SCORE(19, 22);
    P_ISOLATED = MAKE_SCORE(-7, 20);
    P_ISOLATED_OPEN = MAKE_SCORE(7, 26);
    P_WEAK = MAKE_SCORE(9, 10);
    B_PAWN_SPACE = MAKE_SCORE(3, 4);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-21, 0);
    B_PASSED_RANK4 = MAKE_SCORE(-33, 39);
    B_PASSED_RANK5 = MAKE_SCORE(14, 89);
    B_PASSED_RANK6 = MAKE_SCORE(88, 187);
    B_PASSED_RANK7 = MAKE_SCORE(297, 356);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(7, 6);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(22, 44);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(20, 88);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(0, 202);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(36, 226);
    P_KING_FAR_MYC = MAKE_SCORE(-10, 28);
    B_KING_FAR_OPP = MAKE_SCORE(-12, 42);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(34, 1);
    B_ROOK_FULL_OPEN = MAKE_SCORE(76, -6);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(9, 12);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(4, 11);
    B_ROOK_MOBILITY = MAKE_SCORE(1, 13);
    B_BISHOP_MOBILITY = MAKE_SCORE(10, 13);
    B_KNIGHT_MOBILITY = MAKE_SCORE(16, 14);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 7;
    KING_ATTACK_BISHOP = 3;
    KING_ATTACK_ROOK = 6;
    KING_ATTACK_QUEEN = 4;
    KING_ATTACK_MULTI = 10;
    KING_ATTACK_EGPCT = 162;
    B_KING_ATTACK = 48;
    B_KING_DEFENDER = 9;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(86, 125);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(76, 190);
    P_PAWN_ATK_ROOK = MAKE_SCORE(81, 23);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(57, 39);
    B_THREAT_PAWN = MAKE_SCORE(-8, 31);
    B_THREAT_KNIGHT = MAKE_SCORE(19, 76);
    B_THREAT_BISHOP = MAKE_SCORE(23, 69);
    B_THREAT_ROOK = MAKE_SCORE(-8, 49);
    B_THREAT_QUEEN = MAKE_SCORE(43, 46);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(34, 15);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(15, 24);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(46, 13);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(19, 56);
    // PST
    PST_P_RANK = MAKE_SCORE(5, -2);
    PST_P_FILE[0] = MAKE_SCORE(-6, 35);
    PST_P_FILE[1] = MAKE_SCORE(6, 36);
    PST_P_FILE[2] = MAKE_SCORE(12, 31);
    PST_P_FILE[3] = MAKE_SCORE(26, 13);
    PST_N_RANK[0] = MAKE_SCORE(-30, -28);
    PST_N_RANK[1] = MAKE_SCORE(-8, 10);
    PST_N_RANK[2] = MAKE_SCORE(10, 31);
    PST_N_RANK[3] = MAKE_SCORE(42, 91);
    PST_N_RANK[4] = MAKE_SCORE(49, 100);
    PST_N_RANK[5] = MAKE_SCORE(71, 62);
    PST_N_RANK[6] = MAKE_SCORE(46, 40);
    PST_N_RANK[7] = MAKE_SCORE(-176, 51);
    PST_N_FILE[0] = MAKE_SCORE(0, -2);
    PST_N_FILE[1] = MAKE_SCORE(18, 46);
    PST_N_FILE[2] = MAKE_SCORE(22, 69);
    PST_N_FILE[3] = MAKE_SCORE(34, 99);
    PST_B_RANK[0] = MAKE_SCORE(14, 10);
    PST_B_RANK[1] = MAKE_SCORE(35, 23);
    PST_B_RANK[2] = MAKE_SCORE(42, 48);
    PST_B_RANK[3] = MAKE_SCORE(43, 78);
    PST_B_RANK[4] = MAKE_SCORE(49, 99);
    PST_B_RANK[5] = MAKE_SCORE(72, 93);
    PST_B_RANK[6] = MAKE_SCORE(-12, 99);
    PST_B_RANK[7] = MAKE_SCORE(-59, 107);
    PST_B_FILE[0] = MAKE_SCORE(28, 39);
    PST_B_FILE[1] = MAKE_SCORE(55, 68);
    PST_B_FILE[2] = MAKE_SCORE(32, 73);
    PST_B_FILE[3] = MAKE_SCORE(30, 85);
    PST_R_RANK[0] = MAKE_SCORE(13, 40);
    PST_R_RANK[1] = MAKE_SCORE(-14, 23);
    PST_R_RANK[2] = MAKE_SCORE(-19, 45);
    PST_R_RANK[3] = MAKE_SCORE(-17, 86);
    PST_R_RANK[4] = MAKE_SCORE(43, 88);
    PST_R_RANK[5] = MAKE_SCORE(65, 91);
    PST_R_RANK[6] = MAKE_SCORE(58, 111);
    PST_R_RANK[7] = MAKE_SCORE(52, 131);
    PST_R_FILE[0] = MAKE_SCORE(7, 65);
    PST_R_FILE[1] = MAKE_SCORE(19, 71);
    PST_R_FILE[2] = MAKE_SCORE(27, 76);
    PST_R_FILE[3] = MAKE_SCORE(34, 67);
    PST_Q_RANK[0] = MAKE_SCORE(74, 34);
    PST_Q_RANK[1] = MAKE_SCORE(81, 57);
    PST_Q_RANK[2] = MAKE_SCORE(60, 120);
    PST_Q_RANK[3] = MAKE_SCORE(37, 164);
    PST_Q_RANK[4] = MAKE_SCORE(14, 179);
    PST_Q_RANK[5] = MAKE_SCORE(-4, 166);
    PST_Q_RANK[6] = MAKE_SCORE(-57, 221);
    PST_Q_RANK[7] = MAKE_SCORE(1, 170);
    PST_Q_FILE[0] = MAKE_SCORE(63, 112);
    PST_Q_FILE[1] = MAKE_SCORE(67, 120);
    PST_Q_FILE[2] = MAKE_SCORE(62, 113);
    PST_Q_FILE[3] = MAKE_SCORE(55, 148);
    PST_K_RANK[0] = MAKE_SCORE(19, -55);
    PST_K_RANK[1] = MAKE_SCORE(4, 9);
    PST_K_RANK[2] = MAKE_SCORE(-24, 33);
    PST_K_RANK[3] = MAKE_SCORE(1, 58);
    PST_K_RANK[4] = MAKE_SCORE(-8, 94);
    PST_K_RANK[5] = MAKE_SCORE(53, 106);
    PST_K_RANK[6] = MAKE_SCORE(24, 89);
    PST_K_RANK[7] = MAKE_SCORE(78, -61);
    PST_K_FILE[0] = MAKE_SCORE(32, -29);
    PST_K_FILE[1] = MAKE_SCORE(62, 47);
    PST_K_FILE[2] = MAKE_SCORE(-27, 82);
    PST_K_FILE[3] = MAKE_SCORE(-27, 80);
}

// END
