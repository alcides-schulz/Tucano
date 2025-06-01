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
//  Test transposition table by searching a special position.
//-------------------------------------------------------------------------------------------------
void trans_table_test(char *fen, char *desc)
{
    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "trans_table_test.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    new_game(game, fen);

    SETTINGS settings;
    settings.max_depth = MAX_DEPTH;
    settings.moves_per_level = 0;
    settings.post_flag = POST_DEFAULT;
    settings.single_move_time = 10000; 
    settings.total_time = 0;
    settings.increment_time = 0;
    settings.use_book = FALSE;
    settings.max_nodes = 0;

    search_run(game, &settings);

    printf("\nmove found is ");
    util_print_move(game->search.best_move, TRUE);

    printf("\nTransposition table expected result: %s\n", desc);

    free(game);
}

//END