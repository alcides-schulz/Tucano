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
    SCORE_PAWN = MAKE_SCORE(169, 218);
    SCORE_KNIGHT = MAKE_SCORE(671, 771);
    SCORE_BISHOP = MAKE_SCORE(733, 775);
    SCORE_ROOK = MAKE_SCORE(1039, 1418);
    SCORE_QUEEN = MAKE_SCORE(2353, 2274);
    B_BISHOP_PAIR = MAKE_SCORE(80, 108);
    B_TEMPO = 20;
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-7, 49);
    P_PAWN_SHIELD = MAKE_SCORE(10, 28);
    P_PAWN_STORM = MAKE_SCORE(4, -10);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(9, 14);
    B_CONNECTED = MAKE_SCORE(35, -3);
    P_DOUBLED = MAKE_SCORE(43, 8);
    P_ISOLATED = MAKE_SCORE(-10, 13);
    P_ISOLATED_OPEN = MAKE_SCORE(-3, 39);
    P_WEAK = MAKE_SCORE(19, 19);
    B_PAWN_SPACE = MAKE_SCORE(3, 3);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-22, -2);
    B_PASSED_RANK4 = MAKE_SCORE(-12, 62);
    B_PASSED_RANK5 = MAKE_SCORE(52, 124);
    B_PASSED_RANK6 = MAKE_SCORE(166, 190);
    B_PASSED_RANK7 = MAKE_SCORE(355, 325);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(19, 4);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(17, 21);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(4, 72);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(5, 186);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(47, 302);
    P_KING_FAR_MYC = MAKE_SCORE(-11, 28);
    B_KING_FAR_OPP = MAKE_SCORE(-18, 45);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(63, -11);
    B_ROOK_FULL_OPEN = MAKE_SCORE(110, -17);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(13, 14);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(6, 12);
    B_ROOK_MOBILITY = MAKE_SCORE(4, 12);
    B_BISHOP_MOBILITY = MAKE_SCORE(16, 11);
    B_KNIGHT_MOBILITY = MAKE_SCORE(30, 6);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 7;
    KING_ATTACK_BISHOP = 6;
    KING_ATTACK_ROOK = 4;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 6;
    KING_ATTACK_EGPCT = 164;
    B_KING_ATTACK = 51;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(129, 49);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(83, 158);
    P_PAWN_ATK_ROOK = MAKE_SCORE(117, 45);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(87, -28);
    B_THREAT_PAWN = MAKE_SCORE(-15, 61);
    B_THREAT_KNIGHT = MAKE_SCORE(12, 97);
    B_THREAT_BISHOP = MAKE_SCORE(23, 95);
    B_THREAT_ROOK = MAKE_SCORE(-17, 61);
    B_THREAT_QUEEN = MAKE_SCORE(52, 89);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(39, 45);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(15, 38);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(89, 6);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(35, 23);
    // PST
    PST_P_RANK = MAKE_SCORE(9, -1);
    PST_P_FILE[0] = MAKE_SCORE(-20, 36);
    PST_P_FILE[1] = MAKE_SCORE(31, 19);
    PST_P_FILE[2] = MAKE_SCORE(18, 21);
    PST_P_FILE[3] = MAKE_SCORE(38, 6);
    PST_N_RANK[0] = MAKE_SCORE(-28, -36);
    PST_N_RANK[1] = MAKE_SCORE(-6, 0);
    PST_N_RANK[2] = MAKE_SCORE(2, 29);
    PST_N_RANK[3] = MAKE_SCORE(55, 53);
    PST_N_RANK[4] = MAKE_SCORE(54, 69);
    PST_N_RANK[5] = MAKE_SCORE(94, 24);
    PST_N_RANK[6] = MAKE_SCORE(71, 26);
    PST_N_RANK[7] = MAKE_SCORE(-49, -2);
    PST_N_FILE[0] = MAKE_SCORE(-14, 4);
    PST_N_FILE[1] = MAKE_SCORE(21, 31);
    PST_N_FILE[2] = MAKE_SCORE(26, 42);
    PST_N_FILE[3] = MAKE_SCORE(45, 71);
    PST_B_RANK[0] = MAKE_SCORE(12, 1);
    PST_B_RANK[1] = MAKE_SCORE(32, 18);
    PST_B_RANK[2] = MAKE_SCORE(43, 37);
    PST_B_RANK[3] = MAKE_SCORE(46, 55);
    PST_B_RANK[4] = MAKE_SCORE(49, 61);
    PST_B_RANK[5] = MAKE_SCORE(96, 45);
    PST_B_RANK[6] = MAKE_SCORE(5, 61);
    PST_B_RANK[7] = MAKE_SCORE(56, 17);
    PST_B_FILE[0] = MAKE_SCORE(25, 12);
    PST_B_FILE[1] = MAKE_SCORE(51, 48);
    PST_B_FILE[2] = MAKE_SCORE(38, 56);
    PST_B_FILE[3] = MAKE_SCORE(41, 66);
    PST_R_RANK[0] = MAKE_SCORE(22, 1);
    PST_R_RANK[1] = MAKE_SCORE(-25, -5);
    PST_R_RANK[2] = MAKE_SCORE(-26, 9);
    PST_R_RANK[3] = MAKE_SCORE(-17, 46);
    PST_R_RANK[4] = MAKE_SCORE(70, 11);
    PST_R_RANK[5] = MAKE_SCORE(92, 29);
    PST_R_RANK[6] = MAKE_SCORE(72, 37);
    PST_R_RANK[7] = MAKE_SCORE(24, 73);
    PST_R_FILE[0] = MAKE_SCORE(-2, 51);
    PST_R_FILE[1] = MAKE_SCORE(-5, 34);
    PST_R_FILE[2] = MAKE_SCORE(42, 14);
    PST_R_FILE[3] = MAKE_SCORE(53, -5);
    PST_Q_RANK[0] = MAKE_SCORE(57, -38);
    PST_Q_RANK[1] = MAKE_SCORE(69, -2);
    PST_Q_RANK[2] = MAKE_SCORE(32, 69);
    PST_Q_RANK[3] = MAKE_SCORE(6, 116);
    PST_Q_RANK[4] = MAKE_SCORE(-2, 109);
    PST_Q_RANK[5] = MAKE_SCORE(10, 101);
    PST_Q_RANK[6] = MAKE_SCORE(-47, 131);
    PST_Q_RANK[7] = MAKE_SCORE(52, 82);
    PST_Q_FILE[0] = MAKE_SCORE(23, 51);
    PST_Q_FILE[1] = MAKE_SCORE(34, 57);
    PST_Q_FILE[2] = MAKE_SCORE(41, 46);
    PST_Q_FILE[3] = MAKE_SCORE(31, 66);
    PST_Q_FILE[4] = MAKE_SCORE(33, 69);
    PST_Q_FILE[5] = MAKE_SCORE(22, 72);
    PST_Q_FILE[6] = MAKE_SCORE(38, 64);
    PST_Q_FILE[7] = MAKE_SCORE(40, 82);
    PST_K_RANK[0] = MAKE_SCORE(21, -67);
    PST_K_RANK[1] = MAKE_SCORE(7, -7);
    PST_K_RANK[2] = MAKE_SCORE(-27, 25);
    PST_K_RANK[3] = MAKE_SCORE(-43, 54);
    PST_K_RANK[4] = MAKE_SCORE(-31, 85);
    PST_K_RANK[5] = MAKE_SCORE(27, 114);
    PST_K_RANK[6] = MAKE_SCORE(44, 107);
    PST_K_RANK[7] = MAKE_SCORE(54, -8);
    PST_K_FILE[0] = MAKE_SCORE(-55, -12);
    PST_K_FILE[1] = MAKE_SCORE(85, 25);
    PST_K_FILE[2] = MAKE_SCORE(25, 53);
    PST_K_FILE[3] = MAKE_SCORE(-93, 79);
    PST_K_FILE[4] = MAKE_SCORE(-4, 55);
    PST_K_FILE[5] = MAKE_SCORE(-35, 65);
    PST_K_FILE[6] = MAKE_SCORE(80, 22);
    PST_K_FILE[7] = MAKE_SCORE(-14, -34);
    
}

// END
