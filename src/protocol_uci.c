/*-------------------------------------------------------------------------------
Tucano is XBoard chess playing engine developed by Alcides Schulz.
Copyright (C) 2011-present - Alcides Schulz

Tucano is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Tucano is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You can find the GNU General Public License at http://www.gnu.org/licenses/
-------------------------------------------------------------------------------*/

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//    UCI protocol implementation.
//-------------------------------------------------------------------------------------------------

char    uci_line[MAX_READ];
char    go_line[MAX_READ];
int     uci_is_pondering = FALSE;
int     uci_is_infinite = FALSE;

void execute_uci_go(char *line);
void parse_uci_position(char *line);
void remove_line_feed_chars(char *line);

#define HASH_OPTION_STRING "setoption name Hash value "
#define THREADS_OPTION_STRING "setoption name Threads value "
#define SYZYGY_OPTION_STRING "setoption name SyzygyPath value "

//-------------------------------------------------------------------------------------------------
//    UCI main loop.
//-------------------------------------------------------------------------------------------------
void uci_loop(char *engine_name, char *engine_version, char *engine_author) {

    THREAD_ID   go_thread = NULL;

    // UCI initialization
    printf("id name %s %s\n", engine_name, engine_version);
    printf("id author %s\n", engine_author);
    printf("option name Hash type spin default 64 min %d max %d\n", MIN_HASH_SIZE, MAX_HASH_SIZE);
    printf("option name Threads type spin default 1 min %d max %d\n", MIN_THREADS, MAX_THREADS);
    printf("option name SyzygyPath type string default <empty>\n");
    printf("option name Ponder type check default false\n");
    printf("uciok\n");
    
    while (TRUE) {

        fflush(stdout);

        if (!fgets(uci_line, MAX_READ, stdin)) return;

        remove_line_feed_chars(uci_line);

        if (!strcmp(uci_line, "isready")) {
            printf("readyok\n");
            continue;
        }

        if (!strcmp(uci_line, "ucinewgame")) {
            new_game(&main_game, FEN_NEW_GAME);
            continue;
        }

        if (!strncmp(uci_line, HASH_OPTION_STRING, strlen(HASH_OPTION_STRING))) {
            int hash_size = atoi(&uci_line[strlen(HASH_OPTION_STRING)]);
            hash_size = valid_hash_size(hash_size);
            tt_init(hash_size);
            printf("info string Hash set to %d MB\n", hash_size);
            continue;
        }

        if (!strncmp(uci_line, THREADS_OPTION_STRING, strlen(THREADS_OPTION_STRING))) {
            int threads = atoi(&uci_line[strlen(THREADS_OPTION_STRING)]);
            threads = valid_threads(threads);
            threads_init(threads);
            printf("info string Threads set to %d\n", threads);
            continue;
        }

#ifdef EGTB_SYZYGY
        if (!strncmp(uci_line, SYZYGY_OPTION_STRING, strlen(SYZYGY_OPTION_STRING))) {
            char *syzygy_path = &uci_line[strlen(SYZYGY_OPTION_STRING)];
            tb_init(syzygy_path);
            printf("info string SyzygyPath set to %s/%d\n", syzygy_path, TB_LARGEST);
            continue;
        }
#endif

        if (!strncmp(uci_line, "position", 8)) {
            parse_uci_position(uci_line);
            continue;
        }

        if (!strncmp(uci_line, "go", 2)) {
            uci_is_infinite = FALSE;
            uci_is_pondering = FALSE;
            // execute the go command in a new thread
            strcpy(go_line, uci_line);
            THREAD_CREATE(go_thread, execute_uci_go, go_line);
            continue;
        }

        if (!strcmp(uci_line, "ponderhit")) {
            uci_is_pondering = FALSE; // stop pondering but search can continue
            continue;
        }

        if (!strcmp(uci_line, "stop")) {
            main_game.search.abort = TRUE;
            uci_is_infinite = FALSE;
            uci_is_pondering = FALSE;
            THREAD_WAIT(go_thread);
            continue;
        }

        if (!strcmp(uci_line, "quit")) {
            break;
        }
    }
}

//-------------------------------------------------------------------------------------------------
//  Parse UCI "go" command:
//    * searchmoves <move1> .... <movei>
//    * ponder
//    * wtime <x>
//    * btime <x>
//    * winc <x>
//    * binc <x>
//    * movestogo <x>
//    * depth <x>
//    * nodes <x>
//    * mate <x>
//    * movetime <x>
//    * infinite
//-------------------------------------------------------------------------------------------------
void execute_uci_go(char *line)
{
    int infinite = FALSE;
    int ponder = FALSE;
    int depth = -1;
    int moves_to_go = -1;
    int wtime = -1;
    int btime = -1;
    int move_time = -1;

    char *token = strtok(line, " "); // skip "go "

    for (token = strtok(NULL, " "); token != NULL; token = strtok(NULL, " ")) {
        if (!strcmp(token, "wtime")) {
            wtime = atoi(strtok(NULL, " "));
            continue;
        }
        if (!strcmp(token, "btime")) {
            btime = atoi(strtok(NULL, " "));
            continue;
        }
        if (!strcmp(token, "depth")) {
            depth = atoi(strtok(NULL, " "));
            continue;
        }
        if (!strcmp(token, "infinite")) {
            infinite = TRUE;
            continue;
        }
        if (!strcmp(token, "ponder")) {
            ponder = TRUE;
            continue;
        }
        if (!strcmp(token, "movestogo")) {
            moves_to_go = atoi(strtok(NULL, " "));
            continue;
        }
        if (!strcmp(token, "movetime")) {
            move_time = atoi(strtok(NULL, " "));
            continue;
        }
    }

    // Setup seach parameters
    game_settings.post_flag = POST_UCI;
    game_settings.max_depth = MAX_DEPTH;
    game_settings.single_move_time = 0;
    game_settings.total_move_time = 0;
    game_settings.moves_to_go = 0;

    int time = side_on_move(&main_game.board) == WHITE ? wtime : btime;

    if (depth != -1) game_settings.max_depth = MAX(1, MIN(depth, MAX_DEPTH));
    if (time != -1) game_settings.total_move_time = time;
    if (move_time != -1) game_settings.single_move_time = move_time;
    if (moves_to_go != -1) game_settings.moves_to_go = moves_to_go;
    if (ponder) uci_is_pondering = TRUE;
    if (infinite) uci_is_infinite = TRUE;
    if (uci_is_infinite) game_settings.single_move_time = MAX_TIME;

    // search
    search_run(&main_game, &game_settings);

    // Ponder: if search finish early have to wait for stop or ponderhit commands from uci
    while (uci_is_pondering);

    // Infinite: if search finish early have to wait for stop command from uci
    if (uci_is_infinite) while (!main_game.search.abort);

    // print move
    char move_string[10];
    make_move(&main_game.board, main_game.search.best_move);
    util_get_move_string(main_game.search.best_move, move_string);
    printf("bestmove %s ", move_string);

    if (main_game.search.ponder_move != MOVE_NONE) {
        util_get_move_string(main_game.search.ponder_move, move_string);
        printf("ponder %s", move_string);
    }

    printf("\n"); 
    fflush(stdout);
}

//-------------------------------------------------------------------------------------------------
//  Parse UCI "position" command:
//          position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
//-------------------------------------------------------------------------------------------------
void parse_uci_position(char *line)
{
    char *fen_option = strstr(line, "fen");
    char *startpos_option = strstr(line, "startpos");
    char *moves_option = strstr(line, "moves");

    if (fen_option != NULL) {
        char *fen_start = fen_option + 4;
        if (moves_option != NULL) {
            char *fen_end = moves_option - 1;
            fen_end[0] = '\0';
        }
        set_fen(&main_game.board, fen_start);
    }
    if (startpos_option != NULL) {
        set_fen(&main_game.board, FEN_NEW_GAME);
    }
    if (moves_option != NULL) {
        char *move_string = strtok(moves_option, " ");
        move_string = strtok(NULL, " "); // skip "moves" string
        while (move_string != NULL) {
            MOVE move = util_parse_move(&main_game, move_string);
            if (move == MOVE_NONE) break;
            make_move(&main_game.board, move);
            move_string = strtok(NULL, " ");
        }
    }
}

void remove_line_feed_chars(char *line)
{
    char *p;
    p = strchr(line, '\n');
    if (p != NULL) *p = '\0';
    p = strchr(line, '\r');
    if (p != NULL) *p = '\0';
}

//EOF