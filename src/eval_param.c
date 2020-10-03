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
    SCORE_PAWN = MAKE_SCORE(164, 223);
    SCORE_KNIGHT = MAKE_SCORE(687, 786);
    SCORE_BISHOP = MAKE_SCORE(743, 794);
    SCORE_ROOK = MAKE_SCORE(1040, 1462);
    SCORE_QUEEN = MAKE_SCORE(2375, 2323);
    B_BISHOP_PAIR = MAKE_SCORE(64, 118);
    B_TEMPO = 24;
    // KING
    B_PAWN_PROXIMITY = MAKE_SCORE(-28, 50);
    P_PAWN_SHIELD = MAKE_SCORE(10, 32);
    P_PAWN_STORM = MAKE_SCORE(11, -24);
    // PAWN
    B_CANDIDATE = MAKE_SCORE(7, 16);
    B_CONNECTED[0] = MAKE_SCORE(29, 0);
    B_CONNECTED[1] = MAKE_SCORE(38, 0);
    B_CONNECTED[2] = MAKE_SCORE(38, -19);
    B_CONNECTED[3] = MAKE_SCORE(14, 4);
    B_CONNECTED[4] = MAKE_SCORE(70, 38);
    B_CONNECTED[5] = MAKE_SCORE(88, 51);
    B_CONNECTED_PASSER[0] = MAKE_SCORE(16, -51);
    B_CONNECTED_PASSER[1] = MAKE_SCORE(14, -8);
    B_CONNECTED_PASSER[2] = MAKE_SCORE(-10, 27);
    B_CONNECTED_PASSER[3] = MAKE_SCORE(37, 24);
    B_CONNECTED_PASSER[4] = MAKE_SCORE(51, -11);
    B_CONNECTED_PASSER[5] = MAKE_SCORE(85, 51);
    P_DOUBLED = MAKE_SCORE(42, 12);
    P_ISOLATED = MAKE_SCORE(-16, 22);
    P_ISOLATED_OPEN = MAKE_SCORE(-4, 38);
    P_WEAK = MAKE_SCORE(22, 9);
    B_PAWN_SPACE = MAKE_SCORE(3, 4);
    // PASSED
    B_PASSED_RANK3 = MAKE_SCORE(-11, 13);
    B_PASSED_RANK4 = MAKE_SCORE(-3, 64);
    B_PASSED_RANK5 = MAKE_SCORE(31, 121);
    B_PASSED_RANK6 = MAKE_SCORE(131, 186);
    B_PASSED_RANK7 = MAKE_SCORE(333, 332);
    B_UNBLOCKED_RANK3 = MAKE_SCORE(0, -3);
    B_UNBLOCKED_RANK4 = MAKE_SCORE(12, 31);
    B_UNBLOCKED_RANK5 = MAKE_SCORE(20, 73);
    B_UNBLOCKED_RANK6 = MAKE_SCORE(2, 190);
    B_UNBLOCKED_RANK7 = MAKE_SCORE(43, 263);
    P_KING_FAR_MYC = MAKE_SCORE(-10, 30);
    B_KING_FAR_OPP = MAKE_SCORE(-16, 44);
    // PIECES
    B_ROOK_SEMI_OPEN = MAKE_SCORE(57, -9);
    B_ROOK_FULL_OPEN = MAKE_SCORE(100, -29);
    P_PAWN_BISHOP_SQ = MAKE_SCORE(13, 13);
    // MOBILITY
    B_QUEEN_MOBILITY = MAKE_SCORE(6, 14);
    B_ROOK_MOBILITY = MAKE_SCORE(2, 14);
    B_BISHOP_MOBILITY = MAKE_SCORE(14, 12);
    B_KNIGHT_MOBILITY = MAKE_SCORE(22, 11);
    // KING_ATTACK
    KING_ATTACK_KNIGHT = 6;
    KING_ATTACK_BISHOP = 5;
    KING_ATTACK_ROOK = 4;
    KING_ATTACK_QUEEN = 1;
    KING_ATTACK_MULTI = 6;
    KING_ATTACK_EGPCT = 166;
    B_KING_ATTACK = 51;
    // THREAT
    P_PAWN_ATK_KNIGHT = MAKE_SCORE(123, 66);
    P_PAWN_ATK_BISHOP = MAKE_SCORE(94, 140);
    P_PAWN_ATK_ROOK = MAKE_SCORE(97, 44);
    P_PAWN_ATK_QUEEN = MAKE_SCORE(77, -1);
    B_THREAT_PAWN = MAKE_SCORE(-11, 55);
    B_THREAT_KNIGHT = MAKE_SCORE(25, 80);
    B_THREAT_BISHOP = MAKE_SCORE(28, 81);
    B_THREAT_ROOK = MAKE_SCORE(-5, 58);
    B_THREAT_QUEEN = MAKE_SCORE(75, 91);
    B_CHECK_THREAT_KNIGHT = MAKE_SCORE(35, 23);
    B_CHECK_THREAT_BISHOP = MAKE_SCORE(16, 44);
    B_CHECK_THREAT_ROOK = MAKE_SCORE(90, 10);
    B_CHECK_THREAT_QUEEN = MAKE_SCORE(28, 33);
    // PST
    PST_P_RANK = MAKE_SCORE(7, -1);
    PST_P_FILE[0] = MAKE_SCORE(-14, 27);
    PST_P_FILE[1] = MAKE_SCORE(27, 24);
    PST_P_FILE[2] = MAKE_SCORE(19, 23);
    PST_P_FILE[3] = MAKE_SCORE(30, 3);
    PST_N_RANK[0] = MAKE_SCORE(-28, -32);
    PST_N_RANK[1] = MAKE_SCORE(9, 6);
    PST_N_RANK[2] = MAKE_SCORE(16, 33);
    PST_N_RANK[3] = MAKE_SCORE(61, 60);
    PST_N_RANK[4] = MAKE_SCORE(47, 75);
    PST_N_RANK[5] = MAKE_SCORE(66, 27);
    PST_N_RANK[6] = MAKE_SCORE(53, 37);
    PST_N_RANK[7] = MAKE_SCORE(-103, 27);
    PST_N_FILE[0] = MAKE_SCORE(-11, 11);
    PST_N_FILE[1] = MAKE_SCORE(30, 29);
    PST_N_FILE[2] = MAKE_SCORE(32, 40);
    PST_N_FILE[3] = MAKE_SCORE(48, 80);
    PST_B_RANK[0] = MAKE_SCORE(17, 3);
    PST_B_RANK[1] = MAKE_SCORE(42, 16);
    PST_B_RANK[2] = MAKE_SCORE(54, 44);
    PST_B_RANK[3] = MAKE_SCORE(49, 60);
    PST_B_RANK[4] = MAKE_SCORE(43, 76);
    PST_B_RANK[5] = MAKE_SCORE(94, 55);
    PST_B_RANK[6] = MAKE_SCORE(-20, 89);
    PST_B_RANK[7] = MAKE_SCORE(14, 60);
    PST_B_FILE[0] = MAKE_SCORE(18, 21);
    PST_B_FILE[1] = MAKE_SCORE(54, 54);
    PST_B_FILE[2] = MAKE_SCORE(43, 59);
    PST_B_FILE[3] = MAKE_SCORE(43, 75);
    PST_R_RANK[0] = MAKE_SCORE(34, -1);
    PST_R_RANK[1] = MAKE_SCORE(-25, 11);
    PST_R_RANK[2] = MAKE_SCORE(-39, 28);
    PST_R_RANK[3] = MAKE_SCORE(-20, 57);
    PST_R_RANK[4] = MAKE_SCORE(57, 44);
    PST_R_RANK[5] = MAKE_SCORE(96, 38);
    PST_R_RANK[6] = MAKE_SCORE(50, 53);
    PST_R_RANK[7] = MAKE_SCORE(19, 95);
    PST_R_FILE[0] = MAKE_SCORE(4, 53);
    PST_R_FILE[1] = MAKE_SCORE(5, 44);
    PST_R_FILE[2] = MAKE_SCORE(45, 28);
    PST_R_FILE[3] = MAKE_SCORE(48, 13);
    PST_Q_RANK[0] = MAKE_SCORE(58, -29);
    PST_Q_RANK[1] = MAKE_SCORE(81, -2);
    PST_Q_RANK[2] = MAKE_SCORE(44, 79);
    PST_Q_RANK[3] = MAKE_SCORE(-4, 139);
    PST_Q_RANK[4] = MAKE_SCORE(-3, 137);
    PST_Q_RANK[5] = MAKE_SCORE(9, 132);
    PST_Q_RANK[6] = MAKE_SCORE(-54, 155);
    PST_Q_RANK[7] = MAKE_SCORE(10, 110);
    PST_Q_FILE[0] = MAKE_SCORE(36, 49);
    PST_Q_FILE[1] = MAKE_SCORE(25, 61);
    PST_Q_FILE[2] = MAKE_SCORE(44, 47);
    PST_Q_FILE[3] = MAKE_SCORE(35, 84);
    PST_Q_FILE[4] = MAKE_SCORE(35, 87);
    PST_Q_FILE[5] = MAKE_SCORE(21, 94);
    PST_Q_FILE[6] = MAKE_SCORE(49, 88);
    PST_Q_FILE[7] = MAKE_SCORE(59, 61);
    PST_K_RANK[0] = MAKE_SCORE(25, -72);
    PST_K_RANK[1] = MAKE_SCORE(-2, -7);
    PST_K_RANK[2] = MAKE_SCORE(-24, 15);
    PST_K_RANK[3] = MAKE_SCORE(-29, 40);
    PST_K_RANK[4] = MAKE_SCORE(-27, 101);
    PST_K_RANK[5] = MAKE_SCORE(33, 145);
    PST_K_RANK[6] = MAKE_SCORE(4, 126);
    PST_K_RANK[7] = MAKE_SCORE(15, -32);
    PST_K_FILE[0] = MAKE_SCORE(-39, -11);
    PST_K_FILE[1] = MAKE_SCORE(75, 25);
    PST_K_FILE[2] = MAKE_SCORE(46, 54);
    PST_K_FILE[3] = MAKE_SCORE(-99, 85);
    PST_K_FILE[4] = MAKE_SCORE(7, 55);
    PST_K_FILE[5] = MAKE_SCORE(-57, 72);
    PST_K_FILE[6] = MAKE_SCORE(75, 27);
    PST_K_FILE[7] = MAKE_SCORE(-15, -33);
}

// END
