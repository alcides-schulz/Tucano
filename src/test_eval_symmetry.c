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
//  Test the evaluation function to make sure is symmetrical (testing).
//-------------------------------------------------------------------------------------------------

void    flip_ranks(BOARD *board);
void    flip_files(BOARD *board);
int     get_piece_color(BOARD *board, int square);
int     get_piece_type(BOARD *board, int square);

//-------------------------------------------------------------------------------------------------
//  Reads positions from epd file and flips position to compare evaluation values.
//-------------------------------------------------------------------------------------------------
void eval_test(char *file)
{
    FILE    *f;
    char    line[1000];
    char    fen[1000];
    int     i;
    int     count = 0;
    int     correct_ranks = 0;
    int     correct_files = 0;
    int     eval_normal;
    int     eval_ranks;
    int     eval_files;
    
    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "eval_test.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    f = fopen(file, "r");
    if (f == NULL) {
        printf("eval_test: cannot open epd file: %s\n", file);
        return;
    }

    while (fgets(line, 1000, f) != NULL) {
        count++;
        i = 0;
        fen[i] = '\0';
        while (i < 1000 && line[i] && line[i] != '\n' && strncmp(&line[i], " bm ", 4)) {
            fen[i] = line[i];
            i++;
            fen[i] = '\0';
        }
        fen[i] = '\0';

        new_game(game, fen);

        fen[50] = '\0';
        printf("position %3d %-50s   ", count, fen);
        
        eval_normal = evaluate(game, -MAX_SCORE, MAX_SCORE);

        flip_ranks(&game->board);
        eval_ranks = evaluate(game, -MAX_SCORE, MAX_SCORE);
        if (eval_normal == eval_ranks) correct_ranks++;

        flip_files(&game->board);
        eval_files = evaluate(game, -MAX_SCORE, MAX_SCORE);
        if (eval_normal == eval_files) correct_files++;

        printf("Eval: %4d FlipRanks: %4d FlipFiles: %4d\n", eval_normal, eval_ranks, eval_files);

        if (eval_normal == eval_ranks && eval_normal == eval_files) 
            continue;

        board_print(&game->board, "Evaluation difference");
    }

    printf("number of tests: %d   ranks: %d  files: %d\n", count, correct_ranks, correct_files);

    free(game);
    fclose(f);
}

void flip_ranks(BOARD *board)
{
    U8 castle_ks = board->state[WHITE].can_castle_ks;
    U8 castle_qs = board->state[WHITE].can_castle_qs;
    board->state[WHITE].can_castle_ks = board->state[BLACK].can_castle_ks;
    board->state[WHITE].can_castle_qs = board->state[BLACK].can_castle_qs;
    board->state[BLACK].can_castle_ks = castle_ks;
    board->state[BLACK].can_castle_qs = castle_qs;

    board->side_on_move = flip_color(board->side_on_move);

    for (int r = 0; r < 4; r++)
    {
        for (int f = 0; f < 8; f++)
        {
            int square_from = r * 8 + f;
            int square_to = (7 - r) * 8 + f;

            int piece_color_from = get_piece_color(board, square_from);
            int piece_type_from = get_piece_type(board, square_from);
            int piece_color_to = get_piece_color(board, square_to);
            int piece_type_to = get_piece_type(board, square_to);

            if (piece_color_from != NO_COLOR) remove_piece(board, piece_color_from, piece_type_from, square_from);
            if (piece_color_to != NO_COLOR)   remove_piece(board, piece_color_to, piece_type_to, square_to);
            if (piece_color_from != NO_COLOR) set_piece(board, flip_color(piece_color_from), piece_type_from, square_to);
            if (piece_color_to != NO_COLOR)   set_piece(board, flip_color(piece_color_to), piece_type_to, square_from);
        }
    }

    board->key = zk_board_key(board);
    board->pawn_key = zk_pawn_key(board);
    board->state[WHITE].king_square = (U8)bb_first_index(board->state[WHITE].piece[KING]);
    board->state[BLACK].king_square = (U8)bb_first_index(board->state[BLACK].piece[KING]);
}

void flip_files(BOARD *board)
{
    board->state[WHITE].can_castle_ks = 0;
    board->state[WHITE].can_castle_qs = 0;

    board->state[BLACK].can_castle_ks = 0;
    board->state[BLACK].can_castle_qs = 0;

    for (int r = 0; r < 8; r++)
    {
        for (int f = 0; f < 4; f++)
        {
            int square_from = r * 8 + f;
            int square_to = r * 8 + (7 - f);

            int piece_color_from = get_piece_color(board, square_from);
            int piece_type_from = get_piece_type(board, square_from);
            int piece_color_to = get_piece_color(board, square_to);
            int piece_type_to = get_piece_type(board, square_to);

            if (piece_color_from != NO_COLOR) remove_piece(board, piece_color_from, piece_type_from, square_from);
            if (piece_color_to != NO_COLOR)   remove_piece(board, piece_color_to, piece_type_to, square_to);
            if (piece_color_from != NO_COLOR) set_piece(board, piece_color_from, piece_type_from, square_to);
            if (piece_color_to != NO_COLOR)   set_piece(board, piece_color_to, piece_type_to, square_from);
        }
    }

    board->key = zk_board_key(board);
    board->pawn_key = zk_pawn_key(board);
    board->state[WHITE].king_square = (U8)bb_first_index(board->state[WHITE].piece[KING]);
    board->state[BLACK].king_square = (U8)bb_first_index(board->state[BLACK].piece[KING]);
}

int get_piece_color(BOARD *board, int square)
{
    if (piece_on_square(board, WHITE, square) != NO_PIECE) return WHITE;
    if (piece_on_square(board, BLACK, square) != NO_PIECE) return BLACK;
    return NO_COLOR;
}

int get_piece_type(BOARD *board, int square)
{
    int type = piece_on_square(board, WHITE, square);
    if (type != NO_PIECE) return type;
    return piece_on_square(board, BLACK, square);
}

//end
