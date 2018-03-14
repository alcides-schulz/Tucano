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
//  Utilities used by evaluation.
//------------------------------------------------------------------------------------

void eval_print_item(char *desc, int item[2]);
void eval_print_value(double value);

//------------------------------------------------------------------------------------
//  Clear tables for a new game.
//------------------------------------------------------------------------------------
void clear_eval_table(GAME *game)
{
    memset(game->eval_table, 0, sizeof(game->eval_table));
    memset(game->pawn_table, 0, sizeof(game->pawn_table));
}

//------------------------------------------------------------------------------------
//  Calculate and print evaluation of current board position. 
//------------------------------------------------------------------------------------
void eval_print(GAME *game) 
{
    EVAL_PRINTING = TRUE;
    int score = evaluate(game, -MAX_SCORE, MAX_SCORE);
    printf("\n%-16s: %d\n", "Final score", score);
    EVAL_PRINTING = FALSE;
}

//------------------------------------------------------------------------------------
//  Print evaluation values
//------------------------------------------------------------------------------------
void eval_print_values(EVALUATION *eval_values)
{
    printf("                       White            Black            Final\n");
    printf("Evaluation Term     Open    End      Open    End      Open    End\n");
    printf("---------------    -----  -----     -----  -----     -----  -----\n");
    eval_print_item("Material",     eval_values->material);
    eval_print_item("Pawn",         eval_values->pawn);
    eval_print_item("Pieces",       eval_values->pieces);
    eval_print_item("Mobility",     eval_values->mobility);
    eval_print_item("Passed pawns", eval_values->passed);
    eval_print_item("King",         eval_values->king);

    printf("\nDraw Adjustment : %d (64 = no adjustment, 0 = draw)\n", eval_values->draw_adjust);

}

//------------------------------------------------------------------------------------
//  Print evaluation item.
//------------------------------------------------------------------------------------
void eval_print_item(char *desc, int item[2])
{
    double  value[2][2];
    double  total[2];
    int     op[2];
    int     eg[2];

    op[WHITE] = OPENING(item[WHITE]);
    eg[WHITE] = ENDGAME(item[WHITE]);
    op[BLACK] = OPENING(item[BLACK]);
    eg[BLACK] = ENDGAME(item[BLACK]);

    value[WHITE][OP] = op[WHITE] / 100.0;
    value[WHITE][EG] = eg[WHITE] / 100.0;
    value[BLACK][OP] = op[BLACK] / 100.0;
    value[BLACK][EG] = eg[BLACK] / 100.0;

    total[OP] = value[WHITE][OP] - value[BLACK][OP];
    total[EG] = value[WHITE][EG] - value[BLACK][EG];

    printf("%-16s: ", desc);
    eval_print_value(value[WHITE][OP]);
    eval_print_value(value[WHITE][EG]);
    printf(" | ");
    eval_print_value(value[BLACK][OP]);
    eval_print_value(value[BLACK][EG]);
    printf(" | ");
    eval_print_value(total[OP]);
    eval_print_value(total[EG]);
    printf("|\n");
}

//------------------------------------------------------------------------------------
//  Format score value.
//------------------------------------------------------------------------------------
void eval_print_value(double value)
{
    char    v[100];

    sprintf(v, "%3.2f", value);
    printf("%6s ", v);
}

//END

