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
//  Auto play: play games against itself
//-------------------------------------------------------------------------------------------------

void    make_random_move(GAME *game);

//-------------------------------------------------------------------------------------------------
//  Auto play
//-------------------------------------------------------------------------------------------------
void auto_play(int total_games, SETTINGS *settings)
{
    int         game_count;
    char        move_string[20];
    GAME        *game;

    game = (GAME *)ALIGNED_ALLOC(64, sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "auto_play.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    for (game_count = 1; game_count <= total_games; game_count++) {

        new_game(game, FEN_NEW_GAME);

        make_random_move(game);
        make_random_move(game);
        make_random_move(game);
        make_random_move(game);

        while (get_game_result(game) == GR_NOT_FINISH) {
            search_run(game, settings);
            if (!game->search.best_move)
                break;

            make_move(&game->board, game->search.best_move);
            util_get_move_string(game->search.best_move, move_string);
            printf("move %s\n", move_string);
            board_print(&game->board, NULL);
            printf("auto play (game %d of %d)\n", game_count, total_games);
        }

        print_game_result(game);
    }

    ALIGNED_FREE(game);
}

//-------------------------------------------------------------------------------------------------
//  Make a random move. E.g. in auto play mode randomize the game opening.
//-------------------------------------------------------------------------------------------------
void make_random_move(GAME *game)
{
    MOVE        move;
    MOVE_LIST   ml;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        make_move(&game->board, move);
        if (is_illegal(&game->board, move)) {
            undo_move(&game->board);
            continue;
        }
        if (rand() % 100 >= 75) return;
        undo_move(&game->board);
    }

}

//END
