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

#define TRANS_SIZE_GEN 8

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
    int     depth;
}   NNDD;

NNDD    nndd[1024];

//-------------------------------------------------------------------------------------------------
//  Generate data for NN Training
//  output_filename extension determines file format:
//      *.tnn -> format for tucano nn trainer.
//      *.plain or other -> plain format compatible with nodchip nnue trainer
//-------------------------------------------------------------------------------------------------
void generate_nn_data(int fen_total, int max_depth, char *output_filename, int log_pgn, int log_fen)
{
    GAME        *game;
    SETTINGS    settings;
    int         tnn_format = FALSE;
    int         plain_format = FALSE;

    game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "generate_nn_data.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (strlen(output_filename) > 4 && !strcmp(&output_filename[strlen(output_filename) - 4], ".tnn")) {
        tnn_format = TRUE;
    }
    if (strlen(output_filename) > 6 && !strcmp(&output_filename[strlen(output_filename) - 6], ".plain")) {
        plain_format = TRUE;
    }

    if (tnn_format == FALSE && plain_format == FALSE) {
        printf("generation file extension should be .tnn or .plain, cannot generate for %s\n", output_filename);
        return;
    }

    settings.single_move_time = MAX_TIME;
    settings.total_move_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = max_depth;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;
    settings.max_nodes = 0;

    FILE *output = fopen(output_filename, "w");

    tt_init(TRANS_SIZE_GEN);

    UINT start_time = util_get_time();
    int fen_count = 0;
    int game_count;

    FILE *fen_file = NULL;
    if (log_fen) {
        fen_file = fopen("d:/temp/fen.txt", "w");
    }

    for (game_count = 1; game_count <= INT_MAX; game_count++) {

        new_game(game, FEN_NEW_GAME);

        int random_moves_count = (rand() % 16) + 4;

        for (int i = 0; i < random_moves_count; i++) {
            make_random_move_nn(game);
        }

        if (log_fen) {
            char fen[100];
            util_get_board_fen(&game->board, fen);
            fprintf(fen_file, "%s\n", fen);
            fflush(fen_file);
        }

        int nndd_count = 0;
        while (get_game_result(game) == GR_NOT_FINISH) {

            search_run(game, &settings);
            if (!game->search.best_move) break;

            if (!is_valid(&game->board, game->search.best_move)) {
                util_get_board_fen(&game->board, nndd[nndd_count].fen);
                util_get_move_string(game->search.best_move, nndd[nndd_count].move);
                printf("\ninvalid move: [%s] fen: [%s]\n", nndd[nndd_count].move, nndd[nndd_count].fen);
                break;
            }
            else {
                if (!move_is_en_passant(game->search.best_move)) {
                    util_get_board_fen(&game->board, nndd[nndd_count].fen);
                    util_get_move_string(game->search.best_move, nndd[nndd_count].move);
                    nndd[nndd_count].ply = get_history_ply(&game->board);
                    nndd[nndd_count].side = side_on_move(&game->board);
                    nndd[nndd_count].score = game->search.best_score;
                    nndd[nndd_count].depth = settings.max_depth;
                    nndd_count++;
                }
            }

            make_move(&game->board, game->search.best_move);
        }

        int game_result = get_game_result(game);
        if (game_result == GR_NOT_FINISH) continue;
        
        if (log_pgn) {
            char pgn_name[1024];
            sprintf(pgn_name, "d:/temp/gg/game%04d.pgn", game_count);
            save_board_pgn(game, pgn_name, random_moves_count);
        }

        for (int i = 0; i < nndd_count; i++) {
            if (tnn_format) {
                //rnbqkbr1/1p1pppp1/2p4p/p4n2/1PPP4/B5PN/P3PP1P/RN1QKB1R b - -;score=12;[1-0];move=e2e4
                fprintf(output, "%s;", nndd[i].fen);
                fprintf(output, "score=%d;", nndd[i].side == WHITE ? nndd[i].score : -nndd[i].score);
                switch (game_result) {
                case GR_WHITE_WIN: fprintf(output, "[1-0]"); break;
                case GR_BLACK_WIN: fprintf(output, "[0-1]"); break;
                default: fprintf(output, "[1/2]"); break;
                }
                fprintf(output, ";depth=%d", nndd[i].depth);
                fprintf(output, ";move=%s", nndd[i].move);
                fprintf(output, "\n");
            }
            if (plain_format) { // NNUE plain format
            /*
                fen rnb1k2r/p4ppp/1qp1pn2/1p1pN3/1b1P1B1P/P3P3/1PPN1PP1/R2QKB1R w KQkq - 1 9
                move a3b4
                score 1340
                ply 16
                result 1
                e
                fen rnb1k2r/p4ppp/1qp1pn2/1p1pN3/1P1P1B1P/4P3/1PPN1PP1/R2QKB1R b KQkq - 0 9
                move a7a5
                score -1346
                ply 17
                result -1
                e
            */
                fprintf(output, "fen %s\n", nndd[i].fen);
                fprintf(output, "move %s\n", nndd[i].move);
                fprintf(output, "score %d\n", nndd[i].score);
                fprintf(output, "ply %d\n", nndd[i].ply);
                switch (game_result) {
                case GR_WHITE_WIN: fprintf(output, "result %d\n", nndd[i].side == WHITE ? 1 : -1); break;
                case GR_BLACK_WIN: fprintf(output, "result %d\n", nndd[i].side == BLACK ? 1 : -1); break;
                default: fprintf(output, "result 0\n"); break;
                }
                fprintf(output, "e\n");
            }
            fflush(output);
            fen_count++;
            if (fen_count >= fen_total) break;
        }

        fflush(output);
        if (fen_file) fclose(fen_file);

        if (game_count % 10 == 0) {
            UINT elapsed_seconds = (util_get_time() - start_time) / 1000;
            elapsed_seconds = MAX(1, elapsed_seconds);
            int fen_per_sec = fen_count / elapsed_seconds;
            int fen_per_day = fen_per_sec * 3600 * 24;
            printf("file: %s -> %d of %d (game %d, fen/sec: %u, fen/day: %d)...\r",
                output_filename, fen_count, fen_total, game_count, fen_per_sec, fen_per_day);
            fflush(stdout);
        }

        if (fen_count >= fen_total) break;
    }
    printf("\n");

    fclose(output);
    free(game);
    if (log_fen) {
        fclose(fen_file);
    }
}

//-------------------------------------------------------------------------------------------------
//  Make a random move.
//-------------------------------------------------------------------------------------------------
void make_random_move_nn(GAME *game)
{
    MOVE        move;
    MOVE_LIST   ml;
    MOVE        move_list[100];
    int         count = 0;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        make_move(&game->board, move);
        if (is_illegal(&game->board, move)) {
            undo_move(&game->board);
            continue;
        }
        undo_move(&game->board);

        if (count < 100) {
            move_list[count++] = move;
        }
    }

    if (count) {
        int index = rand() % count;
        make_move(&game->board, move_list[index]);
    }
}

void generate_nn_files(char *to_file_mask, int total_positions, int max_depth)
{
    char to_file_name[1000];

    for (int i = 1; i <= 100; i++) {
        sprintf(to_file_name, to_file_mask, i);
        FILE *tf = fopen(to_file_name, "r");
        if (tf != NULL) {
            fclose(tf);
            continue;
        }
        generate_nn_data(total_positions, max_depth, to_file_name, FALSE, FALSE);
        break;
    }
}

void tnn_generate_menu()
{
#ifdef _MSC_VER
    char *to_file_mask_tnn = "d:/temp/data/d%04d.tnn";
    char *to_file_mask_plain = "d:/temp/data/d%04d.plain";
    int total_positions = 100000;
    int max_depth = 4;
#else
    char *to_file_mask_tnn = "./data/d%04d.tnn";
    char *to_file_mask_plain = "./data/d%04d.plain";
    int total_positions = 100000000;
    int max_depth = 7;
#endif
    char resp[100];

    while (TRUE) {
        printf("NN data generation menu\n\n");
        
        printf("Settings:\n");
        printf("\t.tnn file mask..: %s\n", to_file_mask_tnn);
        printf("\t.plain file mask: %s\n", to_file_mask_plain);
        printf("\tTotal positions.: %d\n", total_positions);
        printf("\tMax depth.......: %d\n", max_depth);
        printf("\n");

        printf("\t1. Generate training data .tnn\n");
        printf("\t2. Generate training data .plain (stockfish trainer)\n");
        printf("\tx. Exit\n\n");
        printf("\t--> ");
        fgets(resp, 100, stdin);
        printf("\n");
        if (!strncmp(resp, "1", 1)) generate_nn_files(to_file_mask_tnn, total_positions, max_depth);
        if (!strncmp(resp, "2", 1)) generate_nn_files(to_file_mask_plain, total_positions, max_depth);
        if (!strncmp(resp, "x", 1)) break;
    }
}

//END
