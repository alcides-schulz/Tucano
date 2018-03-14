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

//-------------------------------------------------------------------------------------------------
//  Opening book generation for tests.
//-------------------------------------------------------------------------------------------------

void test_open_gen(GAME *game, int depth);
//void test_open_eval(void);
void test_open_conv(char *from_file, char *prefix);

#define MAX_OPEN_LINES  1000000
#define MAX_OPEN_MOVES    4

MOVE    open_moves[MAX_OPEN_LINES][MAX_OPEN_MOVES];
int        open_count = 0;

void test_open(void)
{

    //set_ply(0);
    //test_open_gen(MAX_OPEN_MOVES);
    //test_open_eval();
    //test_open_conv("open.csv", "tucano_open_book_");
}

void test_open_conv(char *from_file, char *prefix) 
{
    FILE *ff = fopen(from_file, "r");
    FILE *ob = NULL;
    int      fn = 0;
    int   cc = 0;
    char  line[1024];
    char  name[100];

//[Event "0x24de8256"]
//[Site "?"]
//[Date "????.??.??"]
//[Round "?"]
//[White "?"]
//[Black "?"]
//[Result " *"]

    while (fgets(line, 1024, ff) != NULL)  {
        line[strlen(line) - 1] = '\0';

        if (cc == 0) {
            fn++;
            sprintf(name, "%s%02d.pgn", prefix, fn);
            ob = fopen(name, "w");
            printf("%s\n", name);
        }
        cc++;
        fprintf(ob, "[Event \"%02d.%04d\"]\n", fn, cc);
        fprintf(ob, "[Site \"?\"]\n");
        fprintf(ob, "[Date \"????.??.??\"]\n");
        fprintf(ob, "[Round \"?\"]\n");
        fprintf(ob, "[White \"?\"]\n");
        fprintf(ob, "[Black \"?\"]\n");
        fprintf(ob, "[Result \"*\"]\n");
        fprintf(ob, "\n%s *\n\n", line);
        if (cc == 1000) {
            fclose(ob);
            ob = NULL;
            cc = 0;
        }
        if (fn == 98) break;
    }
    fclose(ff);
    if (ob != NULL) fclose(ob);
}


void test_open_gen(GAME *game, int depth)
{
    int         move;
    MOVE_LIST   ml;

    if (depth == 0) {
        get_history_moves(&game->board, open_moves[open_count], MAX_OPEN_MOVES);
        open_count++;
        printf("open_count: %d\n", open_count);
        //board_print("open");
        return;
    }

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), 0, 0);
    
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, ml.pins, move))
            continue;
        make_move(&game->board, move);
        test_open_gen(game, depth - 1);
        undo_move(&game->board);
    }
}

//void test_open_eval(void) 
//{
//    int        i;
//    int        j;
//    int        e;
//    char    l[1024];
//    char    m[100];
//    char    *n[MAX_OPEN_MOVES] = {"1.", " ", " 2.", " "};
//
//    FILE    *o = fopen("open.csv", "w");
//
//    for (i = 0; i < open_count; i++)  {
//        set_fen(FEN_NEW_GAME);
//        l[0] = '\0';
//        for (j = 0; j < MAX_OPEN_MOVES; j++)  {
//            make_move(open_moves[i][j]);
//            util_get_move_desc(open_moves[i][j], m, FALSE);
//            strcat(l, n[j]);
//            strcat(l, m);
//        }
//        e = quiesce(is_incheck(side_on_move(board)), -MAX_SCORE, MAX_SCORE, 0, 0);
//        fprintf(o, "%d,\"%s\"\n", e, l);
//        printf("%d,\"%s\"\n", e, l);
//        //board_print("eval");
//    }
//    fclose(o);
//}

//END
