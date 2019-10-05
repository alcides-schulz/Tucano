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
void remove_line_feed(char *line);

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

        remove_line_feed(uci_line);

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
    // Parse "go" parameters
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
        if (!strcmp(token, "searchmoves")) {
            continue; // not supported
        }
    }

    // Setup seach parameters
    game_settings.post_flag = POST_UCI;
    game_settings.max_depth = MAX_DEPTH;
    game_settings.single_move_time = 0;
    game_settings.total_move_time = 0;
    game_settings.moves_to_go = 0;

    int time = side_on_move(&main_game.board) == WHITE ? wtime : btime;

    // note: mate, inc are not used.
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

void remove_line_feed(char *line)
{
    char *p;
    p = strchr(line, '\n');
    if (p != NULL) *p = '\0';
    p = strchr(line, '\r');
    if (p != NULL) *p = '\0';
}

/*
void *uciGo(void *vthreadsgo) {

    // Get our starting time as soon as possible
    double start = getRealTime();

    Limits limits;

    uint16_t bestMove, ponderMove;
    char moveStr[6];

    int depth = 0, infinite = 0;
    double wtime = 0, btime = 0, movetime = 0;
    double winc = 0, binc = 0, mtg = -1;

    char *str = ((ThreadsGo*)vthreadsgo)->str;
    Board *board = ((ThreadsGo*)vthreadsgo)->board;
    Thread *threads = ((ThreadsGo*)vthreadsgo)->threads;

    // Grab the ready lock, as we cannot be ready until we finish this search
    pthread_mutex_lock(&READYLOCK);

    // Reset global signals
    IS_PONDERING = 0;

    // Init the tokenizer with spaces
    char* ptr = strtok(str, " ");

    // Parse any time control and search method information that was sent
    for (ptr = strtok(NULL, " "); ptr != NULL; ptr = strtok(NULL, " ")) {

        if (stringEquals(ptr, "wtime"))
            wtime = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "btime"))
            btime = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "winc"))
            winc = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "binc"))
            binc = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "movestogo"))
            mtg = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "depth"))
            depth = atoi(strtok(NULL, " "));

        else if (stringEquals(ptr, "movetime"))
            movetime = (double)(atoi(strtok(NULL, " ")));

        else if (stringEquals(ptr, "infinite"))
            infinite = 1;

        else if (stringEquals(ptr, "ponder"))
            IS_PONDERING = 1;
    }

    // Initialize limits for the search
    limits.limitedByNone = infinite != 0;
    limits.limitedByTime = movetime != 0;
    limits.limitedByDepth = depth != 0;
    limits.limitedBySelf = !depth && !movetime && !infinite;
    limits.timeLimit = movetime;
    limits.depthLimit = depth;

    // Pick the time values for the colour we are playing as
    limits.start = (board->turn == WHITE) ? start : start;
    limits.time = (board->turn == WHITE) ? wtime : btime;
    limits.inc = (board->turn == WHITE) ? winc : binc;
    limits.mtg = (board->turn == WHITE) ? mtg : mtg;

    // Execute search, return best and ponder moves
    getBestMove(threads, board, &limits, &bestMove, &ponderMove);

    // UCI spec does not want reports until out of pondering
    while (IS_PONDERING);

    // Report best move ( we should always have one )
    moveToString(bestMove, moveStr, board->chess960);
    printf("bestmove %s ", moveStr);

    // Report ponder move ( if we have one )
    if (ponderMove != NONE_MOVE) {
        moveToString(ponderMove, moveStr, board->chess960);
        printf("ponder %s", moveStr);
    }

    // Make sure this all gets reported
    printf("\n"); fflush(stdout);

    // Drop the ready lock, as we are prepared to handle a new search
    pthread_mutex_unlock(&READYLOCK);

    return NULL;
}

void uciSetOption(char *str, int *megabytes, int *chess960, int *nthreads, Thread **threads) {

    // Handle setting UCI options in Ethereal. Options include:
    //   Hash             : Size of the Transposition Table in Megabyes
    //   Threads          : Number of search threads to use
    //   MoveOverhead     : Overhead on time allocation to avoid time losses
    //   SyzygyPath       : Path to Syzygy Tablebases
    //   SyzygyProbeDepth : Minimal Depth to probe the highest cardinality Tablebase
    //   UCI_Chess960     : Set when playing FRC, but not required in order to work

    if (stringStartsWith(str, "setoption name Hash value ")) {
        *megabytes = atoi(str + strlen("setoption name Hash value "));
        initTT(*megabytes); printf("info string set Hash to %dMB\n", *megabytes);
    }

    if (stringStartsWith(str, "setoption name Threads value ")) {
        free(*threads);
        *nthreads = atoi(str + strlen("setoption name Threads value "));
        *threads = createThreadPool(*nthreads);
        printf("info string set Threads to %d\n", *nthreads);
    }

    if (stringStartsWith(str, "setoption name MoveOverhead value ")) {
        MoveOverhead = atoi(str + strlen("setoption name MoveOverhead value "));
        printf("info string set MoveOverhead to %d\n", MoveOverhead);
    }

    if (stringStartsWith(str, "setoption name SyzygyPath value ")) {
        char *ptr = str + strlen("setoption name SyzygyPath value ");
        tb_init(ptr); printf("info string set SyzygyPath to %s\n", ptr);
    }

    if (stringStartsWith(str, "setoption name SyzygyProbeDepth value ")) {
        TB_PROBE_DEPTH = atoi(str + strlen("setoption name SyzygyProbeDepth value "));
        printf("info string set SyzygyProbeDepth to %u\n", TB_PROBE_DEPTH);
    }

    if (stringStartsWith(str, "setoption name UCI_Chess960 value ")) {
        if (stringStartsWith(str, "setoption name UCI_Chess960 value true"))
            printf("info string set UCI_Chess960 to true\n"), *chess960 = 1;
        if (stringStartsWith(str, "setoption name UCI_Chess960 value false"))
            printf("info string set UCI_Chess960 to false\n"), *chess960 = 0;
    }

    fflush(stdout);
}

void uciPosition(char *str, Board *board, int chess960) {

    int size;
    uint16_t moves[MAX_MOVES];
    char *ptr, moveStr[6], testStr[6];
    Undo undo[1];

    // Position is defined by a FEN, X-FEN or Shredder-FEN
    if (stringContains(str, "fen"))
        boardFromFEN(board, strstr(str, "fen") + strlen("fen "), chess960);

    // Position is simply the usual starting position
    else if (stringContains(str, "startpos"))
        boardFromFEN(board, StartPosition, chess960);

    // Position command may include a list of moves
    ptr = strstr(str, "moves");
    if (ptr != NULL)
        ptr += strlen("moves ");

    // Apply each move in the move list
    while (ptr != NULL && *ptr != '\0') {

        // UCI sends moves in long algebraic notation
        for (int i = 0; i < 4; i++) moveStr[i] = *ptr++;
        moveStr[4] = *ptr == '\0' || *ptr == ' ' ? '\0' : *ptr++;
        moveStr[5] = '\0';

        // Generate moves for this position
        size = 0; genAllLegalMoves(board, moves, &size);

        // Find and apply the given move
        for (int i = 0; i < size; i++) {
            moveToString(moves[i], testStr, board->chess960);
            if (stringEquals(moveStr, testStr)) {
                applyMove(board, moves[i], undo);
                break;
            }
        }

        // Reset move history whenever we reset the fifty move rule. This way
        // we can track all positions that are candidates for repetitions, and
        // are still able to use a fixed size for the history array (512)
        if (board->halfMoveCounter == 0)
            board->numMoves = 0;

        // Skip over all white space
        while (*ptr == ' ') ptr++;
    }
}

void uciReport(Thread *threads, int alpha, int beta, int value) {

    // Gather all of the statistics that the UCI protocol would be
    // interested in. Also, bound the value passed by alpha and
    // beta, since Ethereal uses a mix of fail-hard and fail-soft

    int hashfull = hashfullTT();
    int depth = threads->depth;
    int seldepth = threads->seldepth;
    int elapsed = elapsedTime(threads->info);
    int bounded = value = MAX(alpha, MIN(value, beta));
    uint64_t nodes = nodesSearchedThreadPool(threads);
    uint64_t tbhits = tbhitsThreadPool(threads);
    int nps = (int)(1000 * (nodes / (1 + elapsed)));

    // If the score is MATE or MATED in X, convert to X
    int score = bounded >= MATE_IN_MAX ? (MATE - bounded + 1) / 2
        : bounded <= MATED_IN_MAX ? -(bounded + MATE) / 2 : bounded;

    // Two possible score types, mate and cp = centipawns
    char *type = bounded >= MATE_IN_MAX ? "mate"
        : bounded <= MATED_IN_MAX ? "mate" : "cp";

    // Partial results from a windowed search have bounds
    char *bound = bounded >= beta ? " lowerbound "
        : bounded <= alpha ? " upperbound " : " ";

    printf("info depth %d seldepth %d score %s %d%stime %d "
        "nodes %"PRIu64" nps %d tbhits %"PRIu64" hashfull %d pv ",
        depth, seldepth, type, score, bound, elapsed, nodes, nps, tbhits, hashfull);

    // Iterate over the PV and print each move
    for (int i = 0; i < threads->pv.length; i++) {
        char moveStr[6];
        moveToString(threads->pv.line[i], moveStr, threads->board.chess960);
        printf("%s ", moveStr);
    }

    // Send out a newline and flush
    puts(""); fflush(stdout);
}

void uciReportTBRoot(Board *board, uint16_t move, unsigned wdl, unsigned dtz) {

    char moveStr[6];

    // Convert result to a score. We place wins and losses just outside
    // the range of possible mate scores, and move further from them
    // as the depth to zero increases. Draws are of course, zero.
    int score = wdl == TB_LOSS ? -MATE + MAX_PLY + dtz + 1
        : wdl == TB_WIN ? MATE - MAX_PLY - dtz - 1 : 0;

    printf("info depth %d seldepth %d score cp %d time 0 "
        "nodes 0 tbhits 1 nps 0 hashfull %d pv ",
        MAX_PLY - 1, MAX_PLY - 1, score, 0);

    // Print out the given move
    moveToString(move, moveStr, board->chess960);
    puts(moveStr);
    fflush(stdout);
}

void uciReportCurrentMove(Board *board, uint16_t move, int currmove, int depth) {

    char moveStr[6];
    moveToString(move, moveStr, board->chess960);
    printf("info depth %d currmove %s currmovenumber %d\n", depth, moveStr, currmove);
    fflush(stdout);

}

int stringEquals(char *str1, char *str2) {
    return strcmp(str1, str2) == 0;
}

int stringStartsWith(char *str, char *key) {
    return strstr(str, key) == str;
}

int stringContains(char *str, char *key) {
    return strstr(str, key) != NULL;
}

int getInput(char *str) {

    char *ptr;

    if (fgets(str, 8192, stdin) == NULL)
        return 0;

    ptr = strchr(str, '\n');
    if (ptr != NULL) *ptr = '\0';

    ptr = strchr(str, '\r');
    if (ptr != NULL) *ptr = '\0';

    return 1;
}
*/
