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
//  PST functions. Piece square tables.
//------------------------------------------------------------------------------------

int *PST_Q_BORDER_EG[] = {&PST_Q_BORDER0_EG, &PST_Q_BORDER1_EG, &PST_Q_BORDER2_EG, &PST_Q_BORDER3_EG};
int *PST_K_FILE_OP[] = {&PST_K_FILE0_OP, &PST_K_FILE1_OP, &PST_K_FILE2_OP, &PST_K_FILE3_OP};
int *PST_K_FILE_EG[] = {&PST_K_FILE0_EG, &PST_K_FILE1_EG, &PST_K_FILE2_EG, &PST_K_FILE3_EG};

//------------------------------------------------------------------------------------
//  Pawn PST
//------------------------------------------------------------------------------------
int eval_pst_pawn(int color, int pcsq) 
{
    int     op;
    int     eg;
    int     file_center;
    int     file;
    int     relative_rank;

    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);

    file = get_file(pcsq);
    relative_rank = get_relative_rank(color, get_rank(pcsq));

    file_center = (file < 4 ? file : 7 - file) - 2; // -2 to 1;

    op = file_center * PST_P_FILE_OP;
    if (file == FILED || file == FILEE) {
        if (relative_rank == 2  || relative_rank == 4) op = PST_P_CENTER;
        if (relative_rank == 3) op = PST_P_CENTER * 2;
    }

    eg = MAX(relative_rank - 2, 0) * PST_P_RANK_EG;

    return MAKE_SCORE(op, eg);
}

//------------------------------------------------------------------------------------
//  Knight PST
//------------------------------------------------------------------------------------
int eval_pst_knight(int pcsq) 
{
    int file;
    int rank;
    int file_center;
    int rank_center;
    int pst_score;
    int min_file_rank;
    int max_file_rank;

    assert(valid_square(pcsq));

    file = get_file(pcsq); // 0 to 7
    rank = get_rank(pcsq);

    file_center = (file < 4 ? file : 7 - file);
    rank_center = (rank < 4 ? rank : 7 - rank);

    min_file_rank = MIN(file_center, rank_center);
    max_file_rank = MAX(file_center, rank_center);

    pst_score = 0;
    pst_score += PST_N_BORDER * (min_file_rank - 3);
    pst_score += PST_N_CENTER * max_file_rank;

    if (min_file_rank == 3) pst_score += PST_N_CENTER;
    
    return pst_score;
}

//------------------------------------------------------------------------------------
//  Bishop PST
//------------------------------------------------------------------------------------
int eval_pst_bishop(int pcsq) 
{
    int file;
    int rank;
    int file_center;
    int rank_center;

    assert(valid_square(pcsq));


    file = get_file(pcsq); // 0 to 7
    rank = get_rank(pcsq);

    file_center = (file < 4 ? file : 7 - file); // 0 to 3
    rank_center = (rank < 4 ? rank : 7 - rank);

    if (MIN(file_center, rank_center) == 0) return PST_B_BORDER;
    if (file_center == 1 && rank_center == 1) return PST_B_DIAGONAL;
    if (file_center == 2 && rank_center == 2) return PST_B_DIAGONAL;
    if (file_center == 3 && rank_center == 3) return PST_B_CENTER;
    
    return PST_B_BASIC;
}

//------------------------------------------------------------------------------------
//  Rook PST
//------------------------------------------------------------------------------------
int eval_pst_rook(int pcsq) 
{
    int file;
    int file_center;

    file = get_file(pcsq); // 0 to 7
    file_center = (file < 4 ? file : 7 - file); // 0 to 3

    return (file_center + 1) * PST_R_CENTER;
}

//------------------------------------------------------------------------------------
//  Queen PST
//------------------------------------------------------------------------------------
int eval_pst_queen(int color, int pcsq) 
{
    int file;
    int rank;
    int file_center;
    int rank_center;
    int min_file_rank;
    int relative_rank;
    int pst_op;
    int pst_eg;

    assert(valid_square(pcsq));

    file = get_file(pcsq); // 0 to 7
    rank = get_rank(pcsq);

    file_center = (file < 4 ? file : 7 - file); // 0 to 3
    rank_center = (rank < 4 ? rank : 7 - rank);
    relative_rank = get_relative_rank(color, rank); // 0-7 white point of view.

    min_file_rank = MIN(file_center, rank_center);

    pst_op = relative_rank == 0 ? PST_Q_RANK0_OP : PST_Q_RANKS_OP;
    pst_eg = *PST_Q_BORDER_EG[min_file_rank];
    if (file_center == 2 && rank_center == 2) pst_eg--; // previous version table

    return MAKE_SCORE(pst_op, pst_eg);
}

//------------------------------------------------------------------------------------
//  King PST
//------------------------------------------------------------------------------------
int eval_pst_king(int color, int pcsq)
{
    int file;
    int rank;
    int relative_rank;
    int pst_op;
    int pst_eg;
    int file_center;
    int rank_center;

    file = get_file(pcsq);
    rank = get_rank(pcsq);
    relative_rank = get_relative_rank(color, rank);
    file_center = (file < 4 ? file : 7 - file); // 0-3
    rank_center = (rank < 4 ? rank : 7 - rank); // 0-3

    if (relative_rank == 0)
        pst_op = *PST_K_FILE_OP[file_center];
    else
        pst_op = (relative_rank + 1) * PST_K_RANK_OP;

    if (MIN(file_center, rank_center) == 0)
        pst_eg = *PST_K_FILE_EG[0]; // border
    else
        pst_eg = *PST_K_FILE_EG[file_center] + PST_K_RANK_EG * rank_center;

    return MAKE_SCORE(pst_op, pst_eg);
}

//------------------------------------------------------------------------------------
//  PST printing (tests)
//------------------------------------------------------------------------------------
void eval_pst_print(void)
{
    int i;
    int s;
    int c;

    for (c = 0; c < 2; c++) {
        for (i = 0; i < 64; i++) {
            //s = eval_pst_king(c, i);
            //s = eval_pst_rook(i);
            s = eval_pst_queen(c, i);
            //s = eval_pst_pawn(get_file(i), RELATIVE_RANK(c, get_rank(i)));

            //printf("(%3d,%3d) ", OPENING(s), ENDGAME(s));
            if (((i+1) % 8) == 0) printf("\n");
        }
        //printf("\n");
    }
}

// END
