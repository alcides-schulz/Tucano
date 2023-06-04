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

//-------------------------------------------------------------------------------------------------
//    More perf testing. Posted on talkchess forum.
//-------------------------------------------------------------------------------------------------

U64  perftz_nodes(GAME *game, int depth);
void perftz_pos(GAME *game, char *fen, int depth, U64 expected);

void perftz(void)
{
    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "perft.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    printf("\nperftz: count number of moves for special cases");
    printf("\n-----------------------------------------------\n\n");

    printf("\nAvoid illegal ep:\n");
    perftz_pos(game, "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888);
    perftz_pos(game, "8/8/8/8/k1p4R/8/3P4/3K4 w - - 0 1", 6, 1134888);
    printf("\nEn passant capture checks opponent\n");
    perftz_pos(game, "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467);
    perftz_pos(game, "8/5k2/8/2Pp4/2B5/1K6/8/8 w - d6 0 1", 6, 1440467);
    printf("\nShort castling gives check\n");
    perftz_pos(game, "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072);
    perftz_pos(game, "4k2r/8/8/8/8/8/8/5K2 b k - 0 1", 6, 661072);
    printf("\nLong castling gives check\n");
    perftz_pos(game, "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711);
    perftz_pos(game, "r3k3/8/8/8/8/8/8/3K4 b q - 0 1", 6, 803711);
    printf("\nCastling (including losing rights due to rook capture)\n"); 
    perftz_pos(game, "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206);
    perftz_pos(game, "r3k2r/7b/8/8/8/8/1B4BQ/R3K2R b KQkq - 0 1", 4, 1274206);
    printf("\nCastling prevented\n");
    perftz_pos(game, "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476);
    perftz_pos(game, "r3k2r/8/5Q2/8/8/3q4/8/R3K2R w KQkq - 0 1", 4, 1720476);
    printf("\nPromote out of check\n");
    perftz_pos(game, "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001);
    perftz_pos(game, "3K4/8/8/8/8/8/4p3/2k2R2 b - - 0 ", 6, 3821001);
    printf("\nDiscovered check\n");
    perftz_pos(game, "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 ", 5, 1004658);
    perftz_pos(game, "5K2/8/1Q6/2N5/8/1p2k3/8/8 w - - 0 ", 5, 1004658);
    printf("\nPromote to give check\n");
    perftz_pos(game, "4k3/1P6/8/8/8/8/K7/8 w - - 0 ", 6, 217342);
    perftz_pos(game, "8/k7/8/8/8/8/1p6/4K3 b - - 0 ", 6, 217342);
    printf("\nUnderpromote to check\n");
    perftz_pos(game, "8/P1k5/K7/8/8/8/8/8 w - - 0 ", 6, 92683);
    perftz_pos(game, "8/8/8/8/8/k7/p1K5/8 b - - 0 ", 6, 92683);
    printf("\nSelf stalemate\n");
    perftz_pos(game, "K1k5/8/P7/8/8/8/8/8 w - - 0 ", 6, 2217);
    perftz_pos(game, "8/8/8/8/8/p7/8/k1K5 b - - 0 ", 6, 2217);
    printf("\nStalemate/checkmate\n");
    perftz_pos(game, "8/k1P5/8/1K6/8/8/8/8 w - - 0 ", 7, 567584);
    perftz_pos(game, "8/8/8/8/1k6/8/K1p5/8 b - - 0 ", 7, 567584);
    printf("\nDouble check\n");
    perftz_pos(game, "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 ", 4, 23527);
    perftz_pos(game, "8/5k2/8/5N2/5Q2/2K5/8/8 w - - 0 ", 4, 23527);

    free(game);

    printf("\nperftz completed.\n");
}

void perftz_pos(GAME *game, char *fen, int depth, U64 expected)
{
    U64     nodes = 0;
    int     d;

    new_game(game, fen);

    for (d = 0; d < depth; d++) {
        nodes = perftz_nodes(game, d + 1);
    }
    
    printf("[%s] depth: %d count: %" PRIu64 " found: %" PRIu64 "\n", fen, depth, expected, nodes);

    if (expected != nodes) {
        printf(" ERROR ****************** \n");
        board_print(&game->board, "perftz error");
    }
}

U64 perftz_nodes(GAME *game, int depth) 
{
    U64         nodes = 0;
    MOVE        move;
    MOVE_LIST   ml;

    if (depth == 0) return 1;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, ml.pins, move))
            continue;
        make_move(&game->board, move);
        nodes += perftz_nodes(game, depth - 1);
        undo_move(&game->board);
    }

    return nodes;
}




