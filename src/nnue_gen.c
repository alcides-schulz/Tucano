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
    int     nodes;
}   NNDD;

NNDD    nndd[1024];

//-------------------------------------------------------------------------------------------------
//  Generate data for NN Training
//  output_filename extension determines file format:
//      *.tnn -> format for tucano nn trainer.
//      *.plain or other -> plain format compatible with nodchip nnue trainer
//-------------------------------------------------------------------------------------------------
void generate_nn_data(int fen_total, int max_depth, int max_nodes, char *output_filename, int log_pgn, int log_fen)
{
    GAME        *game;
    SETTINGS    settings;
    int         tnn_format = FALSE;
    int         plain_format = FALSE;

    game = (GAME *)ALIGNED_ALLOC(64, sizeof(GAME));
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
    settings.total_time = 0;
    settings.increment_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = max_depth;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;
    settings.max_nodes = max_nodes;

    FILE *output = fopen(output_filename, "w");

    gHashSize = TRANS_SIZE_GEN;
    tt_init();

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

            if (move_is_quiet(game->search.best_move)) {
                if (!is_incheck(&game->board, side_on_move(&game->board))) {
                    util_get_board_fen(&game->board, nndd[nndd_count].fen);
                    util_get_move_string(game->search.best_move, nndd[nndd_count].move);
                    nndd[nndd_count].ply = get_history_ply(&game->board);
                    nndd[nndd_count].side = side_on_move(&game->board);
                    nndd[nndd_count].score = game->search.best_score;
                    nndd[nndd_count].depth = settings.max_depth;
                    nndd[nndd_count].nodes = (int)game->search.nodes;
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
    ALIGNED_FREE(game);
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
        if (!is_pseudo_legal(&game->board, ml.pins, move)) {
            continue;
        }
        if (!is_valid(&game->board, move)) {
            //util_print_move(move, TRUE);
            //board_print(&game->board, NULL);
            continue;
        }
        if (count < 100) {
            move_list[count++] = move;
        }
    }
    if (count) {
        int index = rand() % count;
        make_move(&game->board, move_list[index]);
    }
}

void generate_nn_files(char *to_file_mask, int total_positions, int max_depth, int max_nodes)
{
    char to_file_name[1000];

    for (int i = 1; i <= 100; i++) {
        sprintf(to_file_name, to_file_mask, max_depth, max_nodes, i);
        FILE *tf = fopen(to_file_name, "r");
        if (tf != NULL) {
            fclose(tf);
            continue;
        }
        generate_nn_data(total_positions, max_depth, max_nodes, to_file_name, FALSE, FALSE);
        break;
    }
}

void tnn_generate_menu()
{
#ifdef _MSC_VER
    char *to_file_mask_tnn = "d:/temp/data/data_d%d_n%d_%04d.tnn";
    char *to_file_mask_plain = "d:/temp/data/data_d%d_n%d_%04d.plain";
#else
    char *to_file_mask_tnn = "./data/data_d%d_n%d_%04d.tnn";
    char *to_file_mask_plain = "./data/data_d%d_n%d_%04d.plain";
#endif
    int total_positions = 100000000;
    int max_depth = 10;
    int max_nodes = 0;
    char resp[100];

    while (TRUE) {
        printf("NN data generation menu\n\n");
        
        printf("Settings:\n");
        printf("\t.tnn file mask..: %s\n", to_file_mask_tnn);
        printf("\t.plain file mask: %s\n", to_file_mask_plain);
        printf("\tTotal positions.: %d\n", total_positions);
        printf("\tMax depth.......: %d\n", max_depth);
        printf("\tMax nodes.......: %d\n", max_nodes);
        printf("\n");

        printf("\t1. Generate training data .tnn\n");
        printf("\t2. Generate training data .plain (stockfish trainer)\n");
        printf("\tx. Exit\n\n");
        printf("\t--> ");
        fgets(resp, 100, stdin);
        printf("\n");
        if (!strncmp(resp, "1", 1)) generate_nn_files(to_file_mask_tnn, total_positions, max_depth, max_nodes);
        if (!strncmp(resp, "2", 1)) generate_nn_files(to_file_mask_plain, total_positions, max_depth, max_nodes);
        if (!strncmp(resp, "x", 1)) break;
    }
}

void tnn_consolidate_files()
{
    /*
        fen rnb1k2r/p4ppp/1qp1pn2/1p1pN3/1b1P1B1P/P3P3/1PPN1PP1/R2QKB1R w KQkq - 1 9
        move a3b4
        score 1340
        ply 16
        result 1
        e
    */
#ifdef _MSC_VER
    char *from_file_mask_plain = "d:/temp/temp/data_d%d_n%d_%04d.plain";
    char *to_file_mask_plain = "d:/temp/data/data_d%d_n%d_%04d.plain";
#else
    char *from_file_mask_plain = "./temp/data_d%d_n%d_%04d.plain";
    char *to_file_mask_plain = "./data/data_d%d_n%d_%04d.plain";
#endif
    int total_positions = 100000000;
    char from_file_name[1000];
    char to_file_name[1000];
    FILE *to_file = NULL;

    char fen[500];
    char move[100];
    char score[100];
    char ply[100];
    char result[100];
    char e[100];

    int write_count = 0;
    int write_file = 0;

    for (int i = 0; i < 100; i++) {
        sprintf(from_file_name, from_file_mask_plain, 10, 0, i);
        FILE *from_file = fopen(from_file_name, "r");
        if (from_file == NULL) continue;
        printf("reading %s...\n", from_file_name);
        while (TRUE) {
            if (!fgets(fen, 500, from_file)) {
                break;
            }
            if (strncmp(fen, "fen ", 4)) {
                printf("error reading 'fen ': %s\n", fen);
                break;
            }
            if (!fgets(move, 100, from_file) || strncmp(move, "move ", 5)) {
                printf("error reading 'move ': %s\n", move);
                break;
            }
            if (!fgets(score, 100, from_file) || strncmp(score, "score ", 6)) {
                printf("error reading 'score ': %s\n", score);
                break;
            }
            if (!fgets(ply, 100, from_file) || strncmp(ply, "ply ", 4)) {
                printf("error reading 'ply ': %s\n", ply);
                break;
            }
            if (!fgets(result, 100, from_file) || strncmp(result, "result ", 7)) {
                printf("error reading 'result ': %s\n", result);
                break;
            }
            if (!fgets(e, 100, from_file) || strncmp(e, "e", 1)) {
                printf("error reading 'e': %s\n", e);
                break;
            }
            if (write_count == 0 || write_count == total_positions) {
                if (to_file != NULL) fclose(to_file);
                sprintf(to_file_name, to_file_mask_plain, 10, 0, ++write_file);
                to_file = fopen(to_file_name, "w");
                if (to_file == NULL) {
                    printf("error opening file: %s\n", to_file_name);
                    exit(0);
                }
                printf("\twriting %s\n", to_file_name);
                write_count = 0;
            }
            fputs(fen, to_file);
            fputs(move, to_file);
            fputs(score, to_file);
            fputs(ply, to_file);
            fputs(result, to_file);
            fputs(e, to_file);
            write_count++;
        }
        fclose(from_file);
    }
    if (to_file != NULL) fclose(to_file);
}

//END

