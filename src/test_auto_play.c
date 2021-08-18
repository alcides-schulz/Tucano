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

typedef struct s_nndd
{
    char    fen[128];
    char    move[16];
    int     score;
    int     ply;
    int     side;
}   NNDD;

NNDD    nndd[512];

//-------------------------------------------------------------------------------------------------
//  Auto play
//-------------------------------------------------------------------------------------------------
void auto_play(int total_games, SETTINGS *settings)
{
    int         game_count;
    char        move_string[20];
    GAME        *game;

    game = (GAME *)malloc(sizeof(GAME));
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

    free(game);
}

//-------------------------------------------------------------------------------------------------
//  Generate data for NN Training
//-------------------------------------------------------------------------------------------------
void generate_nn_data(int positions_total, int depth, char *output_filename)
{
    int         game_count;
    GAME        *game;
    SETTINGS    settings;
    int         nndd_count = 0;
    int         positions_count = 0;

    game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "auto_play.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    settings.single_move_time = MAX_TIME;
    settings.total_move_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = depth;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;

    FILE *output = fopen(output_filename, "w");

    tt_init(8);

    for (game_count = 1; game_count <= INT_MAX; game_count++) {

        new_game(game, FEN_NEW_GAME);

        int random_moves_count = rand() % 20 + 4;

        //printf("random moves: %d\n", random_moves_count);

        for (int i = 0; i < random_moves_count; i++) {
            make_random_move(game);
        }

        nndd_count = 0;
        while (get_game_result(game) == GR_NOT_FINISH) {
            search_run(game, &settings);
            if (!game->search.best_move) break;

            // got validation errors for en-passant capture in the sf convert validate
            if (unpack_type(game->search.best_move) != MT_EPCAP) {
                util_get_board_fen(&game->board, nndd[nndd_count].fen);
                util_get_move_string(game->search.best_move, nndd[nndd_count].move);
                nndd[nndd_count].ply = get_history_ply(&game->board);
                nndd[nndd_count].side = side_on_move(&game->board);
                nndd[nndd_count].score = game->search.best_score;
                //printf("%s %d\n", nndd[nndd_count].fen, nndd[nndd_count].score);
                nndd_count++;
            }

            make_move(&game->board, game->search.best_move);
            //util_get_move_string(game->search.best_move, move_string);
            //printf("move %s %d\n", move_string, game->search.best_score);
            //board_print(&game->board, "pause");
        }

        int game_result = get_game_result(game);

        for (int i = 0; i < nndd_count; i++) {
            if (ABS(nndd[i].score) > MAX_EVAL) break;
            fprintf(output, "fen %s\n", nndd[i].fen);
            fprintf(output, "move %s\n", nndd[i].move);
            fprintf(output, "score %d\n", nndd[i].score);
            fprintf(output, "ply %d\n", nndd[i].ply);
            switch (game_result) {
            case GR_WHITE_WIN:
                fprintf(output, "result %d\n", nndd[i].side == WHITE ? 1 : -1); 
                break;
            case GR_BLACK_WIN:
                fprintf(output, "result %d\n", nndd[i].side == BLACK ? 1 : -1);
                break;
            default:
                fprintf(output, "result 0\n");
            }
            fprintf(output, "e\n");
            positions_count++;
            if (positions_count >= positions_total) break;
        }

        fflush(output);

        printf("generate NN data -> positions %d of %d (game %d)...\r", positions_count, positions_total, game_count);

        if (positions_count >= positions_total) break;
    }
    printf("\n");

    fclose(output);
    free(game);
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
