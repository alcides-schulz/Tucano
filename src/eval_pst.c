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

#define MS MAKE_SCORE

//------------------------------------------------------------------------------------
//  Piece square tables (PST)
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
//  Pawn PST
//------------------------------------------------------------------------------------
int eval_pst_pawn(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);

    return PST_P_RANK * relative_rank + PST_P_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  Knight PST
//------------------------------------------------------------------------------------
int eval_pst_knight(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);
    return PST_N_RANK[relative_rank] + PST_N_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  Bishop PST
//------------------------------------------------------------------------------------
int eval_pst_bishop(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);
    return PST_B_RANK[relative_rank] + PST_B_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  Rook PST
//------------------------------------------------------------------------------------
int eval_pst_rook(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);
    return PST_R_RANK[relative_rank] + PST_R_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  Queen PST
//------------------------------------------------------------------------------------
int eval_pst_queen(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);
    return PST_Q_RANK[relative_rank] + PST_Q_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  King PST
//------------------------------------------------------------------------------------
int eval_pst_king(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    int relative_rank = get_relative_rank(color, get_rank(pcsq));
    int file = get_file(pcsq);
    int file_center = (file < 4 ? file : 7 - file);
    return PST_K_RANK[relative_rank] + PST_K_FILE[file_center];
}

//------------------------------------------------------------------------------------
//  PST printing
//------------------------------------------------------------------------------------
void eval_pst_print(void)
{
    for (int p = PAWN; p <= KING; p++) {
        char letter = piece_letter(p);
        for (int c = 0; c < 2; c++) {
            printf("%s %c\n", c == WHITE ? "White" : "Black", letter);
            for (int s = 0; s < 64; s++) {
                int v = 0;
                switch (letter) {
                case 'K':
                    v = eval_pst_king(c, s);
                    break;
                case 'Q':
                    v = eval_pst_queen(c, s);
                    break;
                case 'R':
                    v = eval_pst_rook(c, s);
                    break;
                case 'B':
                    v = eval_pst_bishop(c, s);
                    break;
                case 'N':
                    v = eval_pst_knight(c, s);
                    break;
                case 'P':
                    v = eval_pst_pawn(c, s);
                    if (s < 8 || s > 55) v = 0;
                    break;
                }
                int op = OPENING(v);
                int eg = ENDGAME(v);
                int av = op + (eg - op) / 2;
                printf("%4d ", av);
                if (((s + 1) % 8) == 0) printf("\n");
            }
        }
    }
}

// END
