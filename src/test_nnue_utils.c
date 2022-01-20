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

#ifdef TUCANNUE
#include "nnue/nnue.h"
#endif

#define TRANS_SIZE_GEN 16

//-------------------------------------------------------------------------------------------------
//  Couple of utilities used to generate nn data.
//-------------------------------------------------------------------------------------------------

void    make_random_move_nn(GAME *game);

typedef struct s_nndd
{
    char    fen[128];
    char    move[16];
    int     score;
    int     ply;
    int     side;
}   NNDD;

NNDD    nndd[1024];

//-------------------------------------------------------------------------------------------------
//  Generate data for NN Training
//  output_filename extension determines file format:
//      *.bin -> binary format for tucano nn trainer.
//      *.plain or other -> plain format compatible with nodchip nnue trainer
//-------------------------------------------------------------------------------------------------
void generate_nn_data(int positions_total, int depth, char *output_filename)
{
    GAME        *game;
    SETTINGS    settings;
    int         binary_format = FALSE;

    game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "generate_nn_data.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (strlen(output_filename) > 4 && !strcmp(&output_filename[strlen(output_filename) - 4], ".bin")) {
        binary_format = TRUE;
    }

    settings.single_move_time = MAX_TIME;
    settings.total_move_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = depth;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;

    FILE *output = fopen(output_filename, binary_format ? "wb" : "w");

    tt_init(TRANS_SIZE_GEN);

    UINT start_time = util_get_time();
    int positions_count = 0;
    int game_count;

    for (game_count = 1; game_count <= INT_MAX; game_count++) {

        new_game(game, FEN_NEW_GAME);

        int random_moves_count = (rand() % 10) + 4;

        for (int i = 0; i < random_moves_count; i++) {
            make_random_move_nn(game);
        }

        int nndd_count = 0;
        while (get_game_result(game) == GR_NOT_FINISH) {
            search_run(game, &settings);
            if (!game->search.best_move) break;

            if (ABS(game->search.best_score) <= MAX_EVAL && unpack_type(game->search.best_move) != MT_EPCAP) {
                util_get_board_fen(&game->board, nndd[nndd_count].fen);
                util_get_move_string(game->search.best_move, nndd[nndd_count].move);
                nndd[nndd_count].ply = get_history_ply(&game->board);
                nndd[nndd_count].side = side_on_move(&game->board);
                nndd[nndd_count].score = game->search.best_score;
                nndd_count++;
            }

            make_move(&game->board, game->search.best_move);
        }

        int game_result = get_game_result(game);

        if (game_result != GR_NOT_FINISH) {
            for (int i = 0; i < nndd_count; i++) {
                if (binary_format) {
                    //tnn_write_record(output, nndd[i].fen, nndd[i].score);
                }
                else {
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
                        break;
                    }
                    fprintf(output, "e\n");
                }
                fflush(output);
                positions_count++;
                if (positions_count >= positions_total) break;
            }
        }

        fflush(output);

        UINT elapsed_seconds = (util_get_time() - start_time) / 1000;
        elapsed_seconds = MAX(1, elapsed_seconds);
        int positions_per_second = positions_count / elapsed_seconds;
        printf("file: %s -> positions %d of %d (game %d, positions per second: %u)...\r", output_filename, positions_count, positions_total, game_count, positions_per_second);

        if (positions_count >= positions_total) break;
    }
    printf("\n");

    fclose(output);
    free(game);
}

//-------------------------------------------------------------------------------------------------
//  Make a random move. E.g. in auto play mode randomize the game opening.
//-------------------------------------------------------------------------------------------------
void make_random_move_nn(GAME *game)
{
    MOVE        move;
    MOVE_LIST   ml;
    MOVE        move_list[10];
    int         score_list[10];
    int         count = 0;
    SETTINGS    settings;

    settings.single_move_time = MAX_TIME;
    settings.total_move_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = 4;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        make_move(&game->board, move);
        if (is_illegal(&game->board, move)) {
            undo_move(&game->board);
            continue;
        }
        search_run(game, &settings);
        undo_move(&game->board);

        int score = ABS(game->search.best_score);

        if (count < 10) {
            score_list[count] = score;
            move_list[count] = move;
            count++;
        }
        else {
            int max_score = score_list[0];
            int max_index = 0;
            for (int i = 1; i < 10; i++) {
                if (score_list[i] > max_score) {
                    max_score = score_list[i];
                    max_index = i;
                }
            }
            if (score < score_list[max_index]) {
                score_list[max_index] = score;
                move_list[max_index] = move;
            }
        }
    }

    if (count) {
        int index = rand() % count;
        make_move(&game->board, move_list[index]);
    }

}

void generate_nn_files()
{
    char to_file[1000];

    for (int i = 1; i <= 100; i++) {
#ifdef _MSC_VER
        sprintf(to_file, "d:/temp/d%03d.plain", i);
#else
        sprintf(to_file, "./data/d%03d.plain", i);
#endif
        FILE *tf = fopen(to_file, "r");
        if (tf != NULL) {
            fclose(tf);
            continue;
        }
        generate_nn_data(100000000, 8, to_file);
        break;
    }
}

//END
