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
//  Small opening book. Based on tscp source.
//  Provide some opening variety when playing from starting position.
//-------------------------------------------------------------------------------------------------

char    *book_lines[1000];
int     book_count;
size_t  book_max_line;
char    current_line[1024];

//-------------------------------------------------------------------------------------------------
//  Return the next book move for current position or MOVE_NONE if there's no match.
//-------------------------------------------------------------------------------------------------
MOVE book_next_move(GAME *game)
{
    char    moves[100][10];

    if (!get_played_moves(&game->board, current_line, 1024))
        return MOVE_NONE;

    int options = 0;
    for (int i = 0; i < book_count && options < 100; i++) {
        char *book_line = book_lines[i];
        if (strlen(book_line) <= strlen(current_line)) continue;
        if (!strncmp(current_line, book_line, strlen(current_line))) {
            size_t start = strlen(current_line);
            while (book_line[start] && book_line[start] == ' ')
                start++;
            strncpy(moves[options], &book_line[start], 4);
            moves[options][4] = '\0';
            options++;
        }
    }

    if (options == 0) return MOVE_NONE;

    int choice = rand() % options;
    MOVE move = util_parse_move(game, moves[choice]);
    if (!is_valid(&game->board, move)) move = MOVE_NONE;

    return move;
}

//-------------------------------------------------------------------------------------------------
//  Add a move line to the book.
//-------------------------------------------------------------------------------------------------
void book_add_line(char *line)
{
    book_lines[book_count++] = line;
    if (strlen(line) > book_max_line)
        book_max_line = strlen(line);
}

//-------------------------------------------------------------------------------------------------
//  Init the book lines.
//-------------------------------------------------------------------------------------------------
void book_init(void)
{
    srand((unsigned int)time(0));
    book_count = 0;
    book_max_line = 0;

    book_add_line("e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4");
    book_add_line("e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5c6");
    book_add_line("g1f3 g8f6 g2g3 d7d5 f1g2");
    book_add_line("d2d4 d7d5 c2c4 c7c6");
    book_add_line("d2d4 d7d5 c2c4 e7e6");
    book_add_line("e2e4 c7c6 d2d4 d7d5 e4e5");
    book_add_line("e2e4 c7c6 d2d4 d7d5 b1c3");
    book_add_line("e2e4 e7e5 g1f3 b8c6 f1c4 f8c5");
    book_add_line("e2e4 e7e5 g1f3 b8c6 f1c4 g8f6");
    book_add_line("e2e4 c7c5 g1f3 d7d6");
    book_add_line("e2e4 c7c5 g1f3 e7e6");
    book_add_line("e2e4 c7c5 g1f3 b8c6");
    book_add_line("e2e4 e7e6 d2d4 d7d5 e4e5");
    book_add_line("e2e4 e7e6 d2d4 d7d5 e4d5");
    book_add_line("d2d4 g8f6 c2c4 e7e6");
    book_add_line("d2d4 g8f6 c2c4 g7g6");
    book_add_line("d2d4 g8f6 c2c4 c7c5");
    book_add_line("c2c4 c7c5");
    book_add_line("c2c4 e7e5");
}

//end
