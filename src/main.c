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

// TODO: move xboard to own file.
// TODO: implement popbit functions.
// TODO: review symetry for king/queen file piece square table.
// TODO: fix zkeys board key generation to include all castle rights keys.

#define EXTERN
#include "globals.h"

#define ENGINE "Tucano"
#define AUTHOR "Alcides Schulz"
#define VERSION "8.31"

void        develop_workbench(void);
double      bench(int depth, int print);
void        speed_test(void);
void        settings_init(void);

char        line[MAX_READ];
char        command[MAX_READ] = { '\0' };
char        syzygy_path[1024] = "";

//-------------------------------------------------------------------------------------------------
//  Main loop
//-------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    int         computer = -1;
    int         stop = FALSE;
    char        move_string[20];
    int         perft_depth;
    char        epd_file[1000];
    int         ponder_on = FALSE;
    MOVE        ponder_move = MOVE_NONE;
    THREAD_ID   ponder_thread = 0;

    // Options
    int         threads = 1;    // Number of Threads
    int         hash_size = 64; // Hash Table Size in MB

    printf("%s chess engine by %s - %s (type 'help' for information)\n", ENGINE, AUTHOR, VERSION);

    EVAL_PRINTING = FALSE;
    EVAL_TUNING = FALSE;
    USE_EVAL_TABLE = TRUE;
    USE_PAWN_TABLE = TRUE;

    // Command line options
    for (int i = 0; i < argc; i++) {
        if (!strcmp("-hash", argv[i])) {
            if (++i < argc) hash_size = valid_hash_size(atoi(argv[i]));
        }
        if (!strcmp("-threads", argv[i])) {
            if (++i < argc) threads = valid_threads(atoi(argv[i]));
        }
        if (!strcmp("-ponder", argv[i])) {
            ponder_on = TRUE;
        }
#ifdef EGTB_SYZYGY
        if (!strcmp("-syzygy_path", argv[i])) {
            if (++i < argc) strcpy(syzygy_path, argv[i]);
        }
#endif
    }

    printf("   hash table: %d MB, threads: %d\n", hash_size, threads);

    // Initializations
    srand((UINT)19810505);
    bb_init();
    bb_data_init();
    magic_init();
    eval_param_init();
    book_init();
    tt_init(hash_size);
    threads_init(threads);
#ifdef EGTB_SYZYGY
    if (strlen(syzygy_path) != 0) {
        if (tb_init(syzygy_path)) {
            printf("# using egtb syzygy path=%s TB_LARGEST=%d\n", syzygy_path, TB_LARGEST);
        }

    }
#endif

    settings_init();

    new_game(&main_game, FEN_NEW_GAME);

#ifndef NDEBUG
    printf("\nDEBUG MODE ON (running with asserts)\n");
#endif

    signal(SIGINT, SIG_IGN);
    printf("\n");

    while (!stop) {
        fflush(stdout);

        if (side_on_move(&main_game.board) == computer) {

            search_run(&main_game, &game_settings);

            if (!main_game.search.best_move) {
                print_game_result(&main_game);
                computer = -1;
                continue;
            }
            
            make_move(&main_game.board, main_game.search.best_move);
            util_get_move_string(main_game.search.best_move, move_string);
            printf("move %s\n", move_string);
            
            ponder_move = main_game.search.ponder_move;

            if (get_game_result(&main_game) != GR_NOT_FINISH) {
                print_game_result(&main_game);
                continue;
            }

            if (ponder_on && ponder_move != MOVE_NONE && is_valid(&main_game.board, ponder_move)) {
                memcpy(&ponder_game, &main_game, sizeof(GAME));
                make_move(&ponder_game.board, ponder_move);
                if (!is_illegal(&ponder_game.board, ponder_move)) {
                    THREAD_CREATE(ponder_thread, ponder_search, &ponder_game);
                }
            }

            continue;
        }

        if (!fgets(line, MAX_READ, stdin)) return 0;
    
        if (line[0] == '\n') continue;
        
        sscanf(line, "%s", command);

        if (!strcmp(command, "uci")) {
            uci_loop(ENGINE, VERSION, AUTHOR);
            stop = TRUE;
            continue;
        }
        if (ponder_on && ponder_thread != 0) {
            ponder_game.search.abort = TRUE;
            THREAD_WAIT(ponder_thread);
            ponder_thread = 0;
        }
        if (!strcmp(command, "xboard"))  {
            printf("\n");
            continue;
        }
        if (!strcmp(command, "new"))  {
            new_game(&main_game, FEN_NEW_GAME);
            computer = BLACK;
            ponder_move = MOVE_NONE;
            continue;
        }
        if (!strcmp(command, "quit"))  {
            stop = TRUE;
            continue;
        }
        if (!strcmp(command, "force"))  {
            computer = -1;
            continue;
        }
        if (!strcmp(command, "sd"))  {
            sscanf(line, "sd %d", &game_settings.max_depth);
            game_settings.max_depth = MIN(MAX_DEPTH, game_settings.max_depth);
            continue;
        }
        if (!strcmp(command, "st"))  {
            sscanf(line, "st %d", &game_settings.single_move_time);
            game_settings.single_move_time *= 1000;
            game_settings.total_move_time = 0;
            continue;
        }
        if (!strcmp(command, "level"))  {
            // just get the "moves to go". will use "time" command to calculate move time.
            sscanf(line, "level %d", &game_settings.moves_per_level);
            if (game_settings.moves_per_level < 0) game_settings.moves_per_level = 0;
            continue;
        }
        if (!strcmp(command, "time"))  {
            sscanf(line, "time %d", &game_settings.total_move_time);
            game_settings.total_move_time *= 10; // time is in centiseconds
            game_settings.single_move_time = 0;
            continue;
        }
        if (!strcmp(command, "go"))  {
            computer = side_on_move(&main_game.board);
            continue;
        }
        if (!strcmp(command, "hint"))  {
            search_run(&main_game, &game_settings);
            if (!main_game.search.best_move)
                continue;
            util_get_move_string(main_game.search.best_move, move_string);
            printf("Hint: %s\n", move_string);
            continue;
        }
        if (!strcmp(command, "protover")) {
            printf("feature setboard=1\n");
            printf("feature myname=\"tucano_%s\"\n", VERSION);
            printf("feature colors=0\n");
            printf("feature analyze=1\n");
            printf("feature option=\"Hash -spin 64 %d %d\"\n", MIN_HASH_SIZE, MAX_HASH_SIZE);
            printf("feature option=\"Threads -spin 1 %d %d\"\n", MIN_THREADS, MAX_THREADS);
#ifdef EGTB_SYZYGY
            printf("feature option=\"SyzygyPath -path \"\"\"\n");
#endif
            printf("feature done=1\n");
            continue;
        }
        if (!strcmp(command, "option")) {
            if (strstr(line, "Hash")) {
                sscanf(line, "option Hash=%d", &hash_size);
                hash_size = valid_hash_size(hash_size);
                tt_init(hash_size);
            }
            if (strstr(line, "Threads")) {
                sscanf(line, "option Threads=%d", &threads);
                threads = valid_threads(threads);
                threads_init(threads);
            }
#ifdef EGTB_SYZYGY
            if (strstr(line, "SyzygyPath")) {
                strcpy(syzygy_path, &line[strlen("option SyzygyPath=")]);
                if (strlen(syzygy_path) != 0) {
                    if (tb_init(syzygy_path)) {
                        printf("# using egtb syzygy path=%s TB_LARGEST=%d\n", syzygy_path, TB_LARGEST);
                    }
                }
            }
#endif
            continue;
        }
        if (!strcmp(command, "undo")) {
            undo_move(&main_game.board);
            continue;
        }
        if (!strcmp(command, "remove")) {
            undo_move(&main_game.board);
            undo_move(&main_game.board);
            continue;
        }
        if (!strcmp(command, "post")) {
            game_settings.post_flag = POST_XBOARD;
            continue;
        }
        if (!strcmp(command, "nopost")) {
            game_settings.post_flag = POST_NONE;
            continue;
        }
        if (!strcmp(command, "setboard")) {
            if (strlen(line) > 9) {
                new_game(&main_game, &line[9]);
				computer = -1;
            }
            continue;
        }
        if (!strcmp(command, "analyze")) {
            analyze_mode(&main_game);
            continue;
        }
        if (!strcmp(command, "hard")) {
            ponder_on = TRUE;
            continue;
        }
        if (!strcmp(command, "easy")) {
            ponder_on = FALSE;
            continue;
        }

        if (!strcmp(command, "otim"))     continue;
        if (!strcmp(command, "random"))   continue;
        if (!strcmp(command, "computer")) continue;
        if (!strcmp(command, "white"))    continue;
        if (!strcmp(command, "black"))    continue;
        if (!strcmp(command, "accepted")) continue;
        if (!strcmp(command, "rejected")) continue;
        if (!strcmp(command, "result"))   continue;

        //  Special commands (non Xboard)
        if (!strcmp(command, "post1")) {
            //  Different format for showing search information.
            game_settings.post_flag = POST_DEFAULT;
            continue;
        }
        if (!strcmp(command, "d")) {
            //  Display current board and moves.
            board_print(&main_game.board, NULL);
            print_current_moves(&main_game);
            continue;
        }
        if (!strcmp(command, "eval")) {
            //  Display current position evaluation information.
            eval_print(&main_game);
            continue;
        }
        if (!strcmp(command, "perft")) {
            //  Display current position move count.
            perft_depth = 0;
            sscanf(line, "perft %d", &perft_depth);
            if (perft_depth == 0)
                printf("syntax: perft <depth>\n");
            else
                perft(perft_depth);
            continue;
        }
        if (!strcmp(command, "perftx")) {
            //  Display more detailed move count.
            perftx();
            continue;
        }
        if (!strcmp(command, "perfty")) {
            //  Another move count from selected set of positions.
            perfty();
            continue;
        }
        if (!strcmp(command, "perftz")) {
            //  Another move count from selected set of positions.
            perftz();
            continue;
        }
        if (!strcmp(command, "dev")) {
            //  Used during development.
            develop_workbench();
            continue;
        }
        if (!strcmp(command, "auto")) {
            // auto play mode, used for testings.
            int auto_play_count = 1;
            if (strlen(line) > 5) sscanf(line, "auto %d", &auto_play_count);
            if (auto_play_count < 1) auto_play_count = 1;
            auto_play(auto_play_count, &game_settings);
            continue;
        }
        if (!strcmp(command, "fen")) {
            //  Setup a new game from fen position.
            if (strlen(line) > 4) {
                set_fen(&main_game.board, &line[4]);
                computer = -1;
            }
            else
                printf("no fen position entered.\n");
            continue;
        }
        if (!strcmp(command, "book")) {
            //  Toggle book use flag.
            game_settings.use_book = !game_settings.use_book;
            printf("book use is %s\n", (game_settings.use_book ? "ON" : "OFF"));
            continue;
        }
        if (!strcmp(command, "bench")) {
            //  Benchmark
            if (hash_size != 64) {
                printf("'bench' command requires hash size of 64. Current hash size is %d. Use 'option Hash=64'.\n", hash_size);
                continue;
            }
            bench(16, TRUE);
            continue;
        }
        if (!strcmp(command, "speed")) {
            //  Measure engine speed
            if (hash_size != 64) {
                printf("'speed' command requires hash size of 64. Current hash size is %d. Use 'option Hash=64'.\n", hash_size);
                continue;
            }
            speed_test();
            continue;
        }
        if (!strcmp(command, "ttt1")) {
            //  transposition table test 1
            trans_table_test("8/k/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1", "Best move would be a1b1, score 2+.\n");
            continue;
        }
        if (!strcmp(command, "ttt2")) {
            //  transposition table test 2
            trans_table_test("2k5/8/1pP1K3/1P6/8/8/8/8 w - -", "Best move would be c7, score 9+.\n");
            continue;
        }
        if (!strcmp(command, "epd")) {
            //  epd test to locate best move. 
            //  fen should contains "bm" for best move.
            //  e.g. "8/2Q5/2p5/p7/Pk6/2q5/4K3/8 w - - 0 53 bm Qe7;"
            if (strlen(line) < 5) {
                printf("syntax: epd <epd file name>\n");
                continue;
            }
            sscanf(line, "epd %s", epd_file);
            epd(epd_file, &game_settings);
            continue;
        }
        if (!strcmp(command, "egt")) {
            //  end game test
            //  e.g. "8/2Q5/2p5/p7/Pk6/2q5/4K3/8 w - - 0 53"
            if (strlen(line) < 5) {
                printf("syntax: egt <epd file name>\n");
                continue;
            }
            sscanf(line, "egt %s", epd_file);
            epd_search(epd_file, &game_settings);
            continue;
        }
        if (!strcmp(command, "evtest")) {
            //  Verify if evaluation is symetric by fliping/rotating positions.
            if (strlen(line) < 7)  {
                printf("syntax: evtest <epd file name>\n");
                continue;
            }
            sscanf(line, "evtest %s", epd_file);
            eval_test(epd_file);
            continue;
        }
        if (!strcmp(command, "tune")) {
            eval_tune();
            continue;
        }
        if (!strcmp(command, "ppst")) {
            eval_pst_print();
            continue;
        }
        if (!strcmp(command, "help")) {
            printf("Tucano supports XBoard/Winboard or UCI protocols.\n\n");
#if defined(__GNUC__)
            printf("Info: GNUC compile.\n\n");
#elif defined(_WIN64) && defined(_MSC_VER)
            printf("Info: MSC compile.\n\n");
#else
            printf("Info: Generic compile.\n\n");
#endif
            printf("Other commands that can be used:\n\n");
            printf("             d: display current board\n");
            printf("          eval: print evaluation score for current position\n");
            printf("epd <filename>: locate best move for epd positions in the file\n");
            printf("     perft <n>: show perft move count from current position.\n");
            printf("                other perft commands: perftx, perfty, perftz\n");
            printf("          tune: evaluation tuning menu\n");
            printf("          ppst: print current pst values\n");
            printf("\n");
            printf("\n");
            printf("Command line options:\n\n");
            printf(" tucano -hash <MB> -threads <#> -syzygy_path <path>\n");
            printf("   -hash indicates the size of hash table, default = 64 MB, minimum: %d MB, maximum: %d MB.\n", MIN_HASH_SIZE, MAX_HASH_SIZE);
            printf("   -threads indicates how many threads to use during search, minimum: %d, maximum: %d.\n", MIN_THREADS, MAX_THREADS);
            printf("   -syzygy_path indicates the path of Syzygy end game tablebases.\n");
            printf("\n");
            continue;
        }

        // Try to parse a valid move.
        MOVE parsed = util_parse_move(&main_game, command);
        if (parsed == MOVE_NONE || !is_valid(&main_game.board, parsed)) {
            printf("Error (unknown command): %s\n", command);
        }
        else {
            make_move(&main_game.board, parsed);
            print_game_result(&main_game);
        }
    }

    return 0;
}

//-------------------------------------------------------------------------------------------------
//  Initialize settings with default values
//-------------------------------------------------------------------------------------------------
void settings_init(void)
{
    game_settings.single_move_time = 10000; // 10 seconds
    game_settings.total_move_time = 0;
    game_settings.moves_per_level = 0;
    game_settings.max_depth = MAX_DEPTH;
    game_settings.post_flag = POST_DEFAULT;
    game_settings.use_book = FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Search a couple of positions and give the count of nodes searched (signature).
//  It is very useful to test non-functional changes.
//  Reference: discocheck/stockisfh engines)
//-------------------------------------------------------------------------------------------------
double bench(int depth, int print)
{
    char *test[] =
    {
        "3r2k1/pp2b1pp/5p2/4p3/3r4/P2P2BP/1P3PP1/1R1R1K2 b -- - 0 24",
        "rn1qk1nr/pp3pp1/2pbp1p1/8/2pP1BP1/2N5/PP1QPPBP/R3K2R b KQkq - 0 10",
        "r1bq1rk1/ppppbppp/2n5/1B6/4P3/3Q4/PPP2PPP/RNB2RK1 w -- - 0 9",
        "8/2B4k/R6p/1p3pp1/1P6/2P1r1K1/2n5/8 w -- - 0 46",
        "r1b1kbnr/6pp/p1pq1p2/3p4/4P3/5N2/PPP2PPP/RNBQR1K1 w -kq - 0 10",
        "N1bk4/bp1p4/p1nN1p1p/4p3/1PB1P1pq/2PP4/P4PP1/R2Q1RK1 w -- - 0 19",
        "1r2r1k1/3bnppp/p2q4/2RPp3/4P3/6P1/2Q1NPBP/2R3K1 w - -",
        "1qr3k1/p2nbppp/bp2p3/3p4/3P4/1P2PNP1/P2Q1PBP/1N2R1K1 b - -",
        "2b1k2r/2p2ppp/1qp4n/7B/1p2P3/5Q2/PPPr2PP/R2N1R1K b k -",
        "2b5/1p4k1/p2R2P1/4Np2/1P3Pp1/1r6/5K2/8 w - -",
        "2brr1k1/ppq2ppp/2pb1n2/8/3NP3/2P2P2/P1Q2BPP/1R1R1BK1 w - -",
        "2kr2nr/1pp3pp/p1pb4/4p2b/4P1P1/5N1P/PPPN1P2/R1B1R1K1 b - -",
        "2r1k2r/1p1qbppp/p3pn2/3pBb2/3P4/1QN1P3/PP2BPPP/2R2RK1 b k -",
        "2r1r1k1/pbpp1npp/1p1b3q/3P4/4RN1P/1P4P1/PB1Q1PB1/2R3K1 w - -",
        "2r2k2/r4p2/2b1p1p1/1p1p2Pp/3R1P1P/P1P5/1PB5/2K1R3 w - -",
        "2r3k1/5pp1/1p2p1np/p1q5/P1P4P/1P1Q1NP1/5PK1/R7 w - -",
        NULL
    };

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "bench.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return 0;
    }

    SETTINGS settings;
    settings.max_depth = depth;
    settings.moves_per_level = 0;
    settings.post_flag = POST_NONE;
    settings.single_move_time = MAX_TIME;
    settings.total_move_time = MAX_TIME;
    settings.use_book = FALSE;

    if (print) printf("Benchmark (depth=%d)\n", depth);

    int total_tests = 0;
    for (int i = 0; test[i]; i++) total_tests++;

    U64     nodes = 0;
    int     elapsed = 1;

    for (int i = 0; test[i]; i++) {
        if (print) printf("%d/%d) %s\n", i + 1, total_tests, test[i]);

        new_game(game, test[i]);

        search_run(game, &settings);

        nodes += game->search.nodes;
        elapsed += game->search.elapsed_time;
    }

    double nps = 1000.0 * (double)nodes / elapsed;

    if (print) printf("\nSignature: %" PRIu64 "  Elapsed time: %3.2f secs  Nodes/sec: %4.0fk\n", nodes, (double)elapsed / 1000.0, nps / 1000.0);

    free(game);

    return nps;
}

//-------------------------------------------------------------------------------------------------
//  Automated speed test.
//  Run bench command 5 times and collect nodes per second.
//  Excludes min and max values, and print an average of 3 values.
//-------------------------------------------------------------------------------------------------
void speed_test(void)
{
    double  nps[5];
    int     min_nps = 0;
    int     max_nps = 0;

    for (int i = 0; i < 5; i++) {
        printf("running speed test %d of 5...\n", i + 1);
        nps[i] = bench(14, FALSE);
        if (nps[i] < nps[min_nps]) min_nps = i;
        if (nps[i] > nps[max_nps]) max_nps = i;
    }

    if (min_nps == max_nps) {
        min_nps = 0;
        max_nps = 4;
    }

    double total_3_nps = 0;
    for (int i = 0; i < 5; i++) {
        if (i == min_nps || i == max_nps) continue;
        total_3_nps += nps[i];
    }

    printf("\nSpeed test: nodes per second average = %4.0fk\n", total_3_nps / 3.0 / 1000.0);
}

//-------------------------------------------------------------------------------------------------
//  Validate Threads option
//-------------------------------------------------------------------------------------------------
int valid_threads(int threads) {
    if (threads < MIN_THREADS) threads = MIN_THREADS;
    if (threads > MAX_THREADS) threads = MAX_THREADS;
    return threads;
}

//-------------------------------------------------------------------------------------------------
//  Validate Threads option
//-------------------------------------------------------------------------------------------------
int valid_hash_size(int hash_size) {
    if (hash_size < MIN_HASH_SIZE) hash_size = MIN_HASH_SIZE;
    if (hash_size > MAX_HASH_SIZE) hash_size = MAX_HASH_SIZE;
    return hash_size;
}

//-------------------------------------------------------------------------------------------------
//  Used for development tests.
//-------------------------------------------------------------------------------------------------
typedef struct strans_record
{
    U32     key;
    MOVE    best_move;
    S32     search_score;
    S16     age;
    S8      depth;
    S8      flag;
}   TT_REC;

typedef struct trans_entry
{
    TT_REC  record[4];
}   TT_ENTRY;
void tt_initxx(size_t size_mb)
{
    assert(sizeof(TT_REC) == 16);

    size_t trans_size = 2;
    while (trans_size * 2 <= size_mb) {
        trans_size *= 2;
    }
    trans_size = trans_size * 1024 * 1024;
    int trans_entries = (int)(trans_size / sizeof(TT_ENTRY));

    printf("%d\n%d\n", trans_entries, INT_MAX);

}

void develop_workbench(void)
{
}

//END
