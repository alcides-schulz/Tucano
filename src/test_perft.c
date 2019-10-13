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
//  perft test: count the moves from start position to test move generation.
//  depth 6 should show 119,060,324 nodes.
//-------------------------------------------------------------------------------------------------

U64 perft_nodes(GAME *game, int depth);

void perft(int depth)
{
    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "perft.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    printf("Depth     Nodes Secs Nodes/Sec\n");

    new_game(game, FEN_NEW_GAME);

    for (int d = 1; d <= depth; d++)  {
        UINT start = util_get_time();
        U64 nodes = perft_nodes(game, d);
        UINT finish = util_get_time();

        UINT duration = (finish - start);
        if (duration == 0) duration = 1;
        double nodes_per_second = (double)nodes / (double)duration * 1000.0;
        double seconds = (double)duration / 1000.0;

        printf("%5d ", d);
        printf("%9" PRIu64 " ", nodes);
        printf("%4.1f ", seconds);
        printf("%9.0f ", nodes_per_second);
        printf("\n");
    }

    free(game);

    printf("perft completed.\n");
}

U64 perft_nodes(GAME *game, int depth) {
    MOVE_LIST   move_list;
    U64         nodes = 0;
    MOVE        move;

    if (depth == 0) return 1;

    select_init(&move_list, game, is_incheck(&game->board, side_on_move(&game->board)), 0, 0);
    
    while ((move = next_move(&move_list)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, move_list.pins, move))
            continue;
        make_move(&game->board, move);
        nodes += perft_nodes(game, depth - 1);
        undo_move(&game->board);
    }

    return nodes;
}

//END
