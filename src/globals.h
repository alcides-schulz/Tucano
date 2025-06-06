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

//-------------------------------------------------------------------------------------------------
// All definitions and declarations.
//-------------------------------------------------------------------------------------------------

// Avoid warnings for microsoft compiler (visual studio)
#define _CRT_SECURE_NO_WARNINGS

// Comment next line to use "assert" functions. "assert" functions are helpfull during debug.
#define NDEBUG

#if defined(_MSC_VER)

#pragma warning (disable : 4706)
#pragma warning (disable : 4711)
#pragma warning (disable : 4255)
#pragma warning (disable : 4820)
#pragma warning (disable : 4005)
#pragma warning (disable : 4710)
#pragma warning (disable : 4668)
#pragma warning (disable : 4214)
#pragma warning (disable : 4206)

#endif

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <memory.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <inttypes.h>

// Multi thread functions - Reference Stockfish
#ifndef _WIN32 // Linux - Unix

#include <pthread.h>

typedef pthread_t THREAD_ID;
typedef void*(*pt_start_fn)(void*);

#define THREAD_CREATE(x,f,t)    pthread_create(&(x),NULL,(pt_start_fn)f,t)
#define THREAD_WAIT(x)          pthread_join(x, NULL)
#define ALIGNED_ALLOC(a, s)     aligned_alloc(a, s)
#define ALIGNED_FREE            free

#else // Windows and MinGW

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

typedef HANDLE THREAD_ID;

#define THREAD_CREATE(x,f,t)    (x = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)f,t,0,NULL))
#define THREAD_WAIT(x)          { WaitForSingleObject(x, INFINITE); CloseHandle(x); }
#define ALIGNED_ALLOC(a, s)     _aligned_malloc(s, a)
#define ALIGNED_FREE            _aligned_free

#endif

// Variables are defined only once in this file.
#ifndef EXTERN
#define EXTERN extern
#endif

// nnue
#include "nnue_defs.h"

// Type definitions
typedef uint64_t        U64; // this is the bitboard
typedef int32_t         S32;
typedef uint32_t        U32;
typedef int8_t          S8;
typedef uint8_t         U8;
typedef int16_t         S16;
typedef uint16_t        U16;
typedef unsigned int    UINT;

#define LOW32(key)        ((U32)key)

#define TRUE        1
#define FALSE       0

// Startup Fen
#define FEN_NEW_GAME        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

// Colors
#define WHITE       0
#define BLACK       1
#define COLORS      2
#define NO_COLOR   -1

#define flip_color(color)   ((color) ^ 1)

// Search values
#define MAX_PLY         256
#define MAX_DEPTH       120
#define MAX_HIST       2048
#define MAX_TIME   10000000

#define MAX_EVAL     20000
#define EGTB_SCORE   25000
#define MATE_SCORE   32000
#define MAX_SCORE    32767
#define WIN_SCORE    (EGTB_SCORE - MAX_PLY) // for MATE and EGTBWIN score

#define POST_NONE       0
#define POST_DEFAULT    1
#define POST_XBOARD     2
#define POST_UCI        3

#define MAX_READ        8196

#define MIN_THREADS     1
#define MAX_THREADS     1024
#define MIN_HASH_SIZE   8
#define MAX_HASH_SIZE   524288

// Parameters
EXTERN S32          gThreads;
EXTERN S32          gHashSize;

// Piece index
#define PAWN        0
#define KNIGHT      1
#define BISHOP      2
#define ROOK        3
#define QUEEN       4
#define KING        5
#define NUM_PIECES  6
#define NO_PIECE    9

// Ranks/Files as represented in this program.
enum    {FILEA, FILEB, FILEC, FILED, FILEE, FILEF, FILEG, FILEH};
enum    {RANK8, RANK7, RANK6, RANK5, RANK4, RANK3, RANK2, RANK1};

#define RANKS       8
#define FILES       8

U64     square_bb(int index);
void    bb_set_bit(U64 *bb, int index);
void    bb_set_bit_rf(U64 *bb, int rank, int file);
void    bb_clear_bit(U64 *bb, int index);
int     bb_is_one(U64 bb, int index);
U64     square_color_bb(int index);

void    bb_init(void);
int     bb_first_index(U64 bb);
int     bb_last_index(U64 bb);
int     bb_bit_count(U64 bb);
int     bb_pop_first_index(U64 *bb);
int     bb_pop_last_index(U64 *bb);

U64     king_moves_bb(int from_square);
U64     knight_moves_bb(int from_square);
U64     west_moves_bb(int from_square);
U64     east_moves_bb(int from_square);
U64     north_moves_bb(int from_square);
U64     south_moves_bb(int from_square);
U64     se_moves_bb(int from_square);
U64     sw_moves_bb(int from_square);
U64     ne_moves_bb(int from_square);
U64     nw_moves_bb(int from_square);
U64     rankfile_moves_bb(int from_square);
U64     diagonal_moves_bb(int from_square);
U64     from_to_path_bb(int from_square, int to_square);
U64     pawn_attack_bb(int color, int square);
U64     forward_path_bb(int color, int from_square);
U64     backward_path_bb(int color, int from_square);
U64     passed_mask_bb(int color, int from_square);
U64     weak_mask_bb(int color, int from_square);
U64     connected_mask_bb(int from_square);
U64     isolated_mask_bb(int from_square);

void    magic_init(void);
U64     bb_rook_attacks(int sq, U64 occup);
U64     bb_bishop_attacks(int sq, U64 occup);
void    bb_print(char *msg, U64 bb);

// move types
#define MT_QUIET    0
#define MT_CAPPC    1
#define MT_PROMO    2
#define MT_CPPRM    3
#define MT_PAWN2    4
#define MT_EPCAP    5
#define MT_CSWKS    6
#define MT_CSWQS    7
#define MT_CSBKS    8
#define MT_CSBQS    9
#define MT_NULL     10
#define MT_SIZE     11

typedef U32         MOVE;

#define MOVE_NONE   ((MOVE)0)
#define NULL_MOVE   ((MOVE)(MT_NULL << 12))

MOVE    pack_quiet(int moving_piece, int from_square, int to_square);
MOVE    pack_pawn_2square(int from_square, int to_square, int ep_square);
MOVE    pack_castle(int from_square, int to_square, int castle);
MOVE    pack_capture(int moving_piece, int captured_piece, int from_square, int to_square);
MOVE    pack_en_passant_capture(int from_square, int to_square, int pawn_square);
MOVE    pack_promotion(int from_square, int to_square, int prom_piece);
MOVE    pack_capture_promotion(int captured_piece, int from_square, int to_square, int prom_piece);

int     unpack_from(MOVE move);
int     unpack_to(MOVE move);
int     unpack_type(MOVE move);
int     unpack_piece(MOVE move);
int     unpack_capture(MOVE move);
int     unpack_prom_piece(MOVE move);
int     unpack_ep_square(MOVE move);
int     unpack_ep_pawn_square(MOVE move);
int     move_is_quiet(MOVE move);
int     move_is_castle(MOVE move);
int     move_is_promotion(MOVE move);
int     move_is_en_passant(MOVE move);
int     move_is_capture(MOVE move);

// General use macros.
#define ABS(a)       ((a) < 0 ? -(a) : (a))
#define MAX(a, b)    ((a) > (b) ? (a) : (b))
#define MIN(a, b)    ((a) < (b) ? (a) : (b))

void    bb_data_init(void);
int     get_rank(int square);
int     get_file(int square);
int     get_square(int rank, int file);
int     get_front_square(int color, int pcsq);
int     get_relative_rank(int color, int rank);
int     get_relative_square(int color, int pcsq);
int     is_rank_valid(int rank);
int     is_file_valid(int file);

#define SQ_NE(sq)   ((sq) - 7)
#define SQ_NW(sq)   ((sq) - 9)
#define SQ_SE(sq)   ((sq) + 9)
#define SQ_SW(sq)   ((sq) + 7)
#define SQ_N(sq)    ((sq) - 8)
#define SQ_S(sq)    ((sq) + 8)
#define SQ_E(sq)    ((sq) + 1)
#define SQ_W(sq)    ((sq) - 1)

// Principal Variation line.
typedef struct s_pv_line {
    MOVE    line[MAX_PLY][MAX_PLY];
    int     size[MAX_PLY];
}   PV_LINE;

// Piece Values
#define VALUE_PAWN      180
#define VALUE_KNIGHT    640
#define VALUE_BISHOP    640
#define VALUE_ROOK      1000
#define VALUE_QUEEN     2000
#define VALUE_KING      0

enum e_squares
{
    A8, B8, C8, D8, E8, F8, G8, H8,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A1, B1, C1, D1, E1, F1, G1, H1
};

#define BB_NO_AFILE ((U64)0x7F7F7F7F7F7F7F7F)
#define BB_NO_HFILE ((U64)0xFEFEFEFEFEFEFEFE)
#define BB_RANK_6   ((U64)0x0000FF0000000000)
#define BB_RANK_3   ((U64)0x0000000000FF0000)
#define BB_RANK_8   ((U64)0xFF00000000000000)
#define BB_RANK_7   ((U64)0x00FF000000000000)
#define BB_RANK_2   ((U64)0x000000000000FF00)
#define BB_RANK_1   ((U64)0x00000000000000FF)
#define BB_CENTER   ((U64)0x0000001818000000)
#define BB_LIGHT_SQ ((U64)0xAA55AA55AA55AA55)
#define BB_DARK_SQ  ((U64)0x55AA55AA55AA55AA)
#define BB_FILES_QS ((U64)0xF0F0F0F0F0F0F0F0)
#define BB_FILES_KS ((U64)0x0F0F0F0F0F0F0F0F)

//  Search data: move, nodes, time control, etc.
//  Times are in milliseconds.
typedef struct s_search
{
    U64     nodes;                  // visited nodes
    U64     tbhits;                 // end game table base hits
    int     post_flag;              // output information
    int     use_book;               // use opening book
    int     max_depth;              // max depth 
    int     is_single_move_time;    // flag to indicate specific time for a move
    UINT    normal_move_time;       // max time for move
    UINT    extended_move_time;     // max time for long search
    UINT    normal_finish_time;     // calculated time to finish
    UINT    extended_finish_time;   // extended time to finish
    U64     max_nodes;              // maximum number of nodes to seach
    UINT    start_time;             // recorded start time
    UINT    end_time;               // recorded end time
    UINT    elapsed_time;           // search duration
    MOVE    best_move;              // best move found
    int     best_score;             // best move score
    MOVE    ponder_move;            // pondering move
    int     cur_depth;              // current depth
    int     score_drop;             // controls when score drops
    int     abort;                  // indicates end of search
    int     root_move_count;        // number of moves at root node, used by xboard analysis
    int     root_move_search;       // number of move searched at root node, used by xboard analysis
#ifdef TUCANO_COMPOSITION
    MOVE    exclude;                // used for composition function, non-playing feature
#endif
}   SEARCH;

// To verify game results
#define GR_NOT_FINISH   0
#define GR_WHITE_WIN    1
#define GR_BLACK_WIN    2
#define GR_DRAW         3

//  Move history information
typedef struct s_move_history
{
    U64     board_key;
    U64     pawn_key;
    MOVE    move;
    U8      fifty_move_rule;
    U8      ep_square;
    U8      can_castle_ks;
    U8      can_castle_qs;
}   MOVE_HIST;

//  Move ordering data: cutoff history heuristic, killers, counter moves
typedef struct s_cutoff_history {
    U16     search_count;
    U16     cutoff_count;
}   CUTOFF_HISTORY;

typedef struct s_move_ordering {
    CUTOFF_HISTORY  cutoff_history[COLORS][NUM_PIECES][64];
    MOVE            killers[MAX_PLY][COLORS][2];
    MOVE            counter_move[COLORS][NUM_PIECES][64][2];
}   MOVE_ORDER;

//  Board representation (bitboard based)
typedef struct s_board
{
    struct s_state
    {
        U64     all_pieces;     // bb for all pieces combined
        U64     piece[6];       // bb for each piece type
        U8      count[6];       // piece count
        U8      can_castle_ks;  // king side
        U8      can_castle_qs;  // queen side
        U8      king_square;    // king square
		U8		square[64];		// piece type per square
    }           state[COLORS];
    U64         key;
    U64         pawn_key;
    U16         ply;
    U16         histply;
    U8          side_on_move;
    U8          fifty_move_rule;
    U8          ep_square;
    U16         selective_depth;
    MOVE_HIST   history[MAX_HIST];
    NNUE_DATA   nnue_data[MAX_HIST];
}   BOARD;

typedef struct s_eval_table
{
    U64     key;
    S32     score;
}   EVAL_TABLE;

#define EVAL_TABLE_SIZE 64536

//  Game Data
typedef struct s_game {
    SEARCH      search;
    BOARD       board;
    PV_LINE     pv_line;
    MOVE_ORDER  move_order;
    EVAL_TABLE  eval_table[EVAL_TABLE_SIZE];
    int         eval_hist[MAX_PLY];
    int         reductions[MAX_PLY];
    int         is_main_thread;
    THREAD_ID   thread_handle;
    int         thread_number;
}   GAME;

// Move generation and selection
#define MAX_MOVE   128

typedef struct s_move_list
{
    MOVE        moves[MAX_MOVE];
    int         score[MAX_MOVE];
    int         count;
    int         incheck;
    int         caps;
    MOVE        ttm;
    int         next;
    int         phase;
    MOVE        late_moves[MAX_MOVE];
    int         late_moves_count;
    int         late_moves_next;
    int         sort;
    U64         pins;
    BOARD       *board;
    MOVE_ORDER  *move_order;
}   MOVE_LIST;


// Transposition Table entry type
#define TT_UPPER    0x01
#define TT_LOWER    0x02
#define TT_EXACT    0x03

typedef union s_trans_record {
    struct {
        MOVE    move;
        S16     score;
        S8      depth;
        U8      age : 6, flag : 2;
    }   info;
    U64 data;
}   TT_RECORD;

#define TIME_CHECK  4095

// Search Reduction Table
EXTERN int reduction_table[MAX_DEPTH][MAX_MOVE];

void    search_tables_init(void);

//  Settings: default time, max depth, etc
typedef struct s_settings {
    UINT    single_move_time;       // set by st command. default is 10 seconds.
    UINT    total_time;             // set by time command.
    UINT    increment_time;         // set by winc/binc
    int     moves_per_level;        // set by level command in XBoard mode
    int     moves_to_go;            // set by movestogo option in UCI mode.
    int     max_depth;              // set by sd command.
    int     post_flag;              // post format.
    int     use_book;               // opening book use.
    U64     max_nodes;              // maximum nodes per search, 0 = no limit.
#ifdef TUCANO_COMPOSITION
    MOVE    exclude;                // used for composition functionally, non-playing.
#endif
}   SETTINGS;

// Zobrish Keys (hash)
U64     zk_board_key(BOARD *board);
U64     zk_pawn_key(BOARD *board);
U64     zk_color(void);
U64     zk_ep(int square);
U64     zk_ks(int color, int flag);
U64     zk_qs(int color, int flag);
U64     zk_square(int color, int piece, int square);

// Game
EXTERN GAME        main_game;
EXTERN SETTINGS    game_settings;
EXTERN GAME        ponder_game;

void    print_game_result(GAME *game);
void    trans_table_test(char *fen, char *desc);
void    auto_play(int total_games, SETTINGS *settings);
void    new_game(GAME *game, char *fen);
int     valid_threads(int threads);
int     valid_hash_size(int hash_size);
int     get_game_result(GAME *game);

// Interface protocols. Communication between engine and GUI.
void uci_loop(char *engine_name, char *engine_version, char *engine_author);
    
// Book
void    book_init(void);
MOVE    book_next_move(GAME *game);

// Attacks/Checks
int     is_illegal(BOARD *board, MOVE move);
int     is_incheck(BOARD *board, int color);
int     is_pseudo_legal(BOARD *board, U64 pins, MOVE move);
U64     find_pins(BOARD *board);
int     is_square_attacked(BOARD *board, int square, int by_color, U64 occup);

void    gen_moves(BOARD *board, MOVE_LIST *ml);
void    gen_caps(BOARD *board, MOVE_LIST *ml);
void    gen_check_evasions(BOARD *board, MOVE_LIST *ml);
void    print_current_moves(GAME *game);
void    print_moves(BOARD *board, MOVE_LIST *ml);

void    select_init(MOVE_LIST *ml, GAME *game, int incheck, MOVE ttm, int caps);
void    add_move(MOVE_LIST *ml, MOVE move);
void    add_all_promotions(MOVE_LIST *ml, int from_square, int to_square);
void    add_all_capture_promotions(MOVE_LIST *ml, int from_square, int to_square, int captured_piece);
MOVE    next_move(MOVE_LIST *ml);
MOVE    prev_move(MOVE_LIST *ml);
int     is_late_moves(MOVE_LIST *ml);
int     is_bad_capture(MOVE_LIST *ml);
int     is_mate_score(int score);
int     is_eval_score(int score);
int     is_winning_score(int score);
int     is_losing_score(int score);

//  Move ordering
void    save_beta_cutoff_data(MOVE_ORDER *move_order, int color, int ply, MOVE best_move, MOVE_LIST *ml, MOVE previous_move);
int     get_beta_cutoff_percent(MOVE_ORDER *move_order, int color, MOVE move);
int     get_pruning_margin(MOVE_ORDER *move_order, int color, MOVE move);
int     get_has_bad_history(MOVE_ORDER *move_order, int color, MOVE move);
int     is_killer_move(MOVE_ORDER *move_order, int color, int ply, MOVE move);
int     is_counter_move(MOVE_ORDER *move_order, int prev_color, MOVE previous_move, MOVE current_move);

// Search
void    prepare_search(GAME *game, SETTINGS *settings);
void    threads_init(void);
void    search_run(GAME *game, SETTINGS *settings);
U64     get_additional_threads_nodes(void);
U64     get_additional_threads_tbhits(void);
void    *ponder_search(void *game);
void    update_pv(PV_LINE *pv_line, int ply, MOVE move);
int     piece_value(int piece);
int     get_best_capture(BOARD *board);
int     is_free_passer(BOARD *board, int color, MOVE move);
int     has_pawn_on_rank7(BOARD *board, int color);
int     is_pawn_to_rank78(int turn, MOVE move);
void    check_time(GAME *game);
int     search(GAME *game, UINT incheck, int alpha, int beta, int depth, MOVE exclude_move);
int     quiesce(GAME *game, UINT incheck, int alpha, int beta, int depth);
void    post_info(GAME *game, int score, int depth);
int     is_check(BOARD *board, MOVE move);
S16     score_to_tt(int score, int ply);
int     score_from_tt(int score, int ply);

// see
int     see_move(BOARD *board, MOVE move);
int     piece_value_see(int piece);

// transposition table
void    tt_age(void);
void    tt_init(void);
void    tt_clear(void);
void    tt_save(U64 key, TT_RECORD *record);
void    tt_read(U64 key, TT_RECORD *record);

// Analyze Mode
void    analyze_mode(GAME *game);

// Evaluation
int     evaluate(GAME *game);

// Board
void    new_game(GAME *game, char *fen);
void    set_fen(BOARD *board, char *fen);
void    make_move(BOARD *board, MOVE move);
void    undo_move(BOARD *board);
int     is_draw(BOARD *board);
int     insufficient_material(BOARD *board);
int     reached_fifty_move_rule(BOARD *board);
int     is_threefold_repetition(BOARD *board);
int     material_value(BOARD *board, int color);
int     pieces_count(BOARD *board, int color);
int     is_valid(BOARD *board, MOVE move);
int     square_distance(int square1, int square2);
void    board_print(BOARD *board, char *text);
U64     king_bb(BOARD *board, int color);
U64     queen_bb(BOARD *board, int color);
U64     rook_bb(BOARD *board, int color);
U64     bishop_bb(BOARD *board, int color);
U64     knight_bb(BOARD *board, int color);
U64     pawn_bb(BOARD *board, int color);
U64     queen_rook_bb(BOARD *board, int color);
U64     queen_bishop_bb(BOARD *board, int color);
int     king_count(BOARD *board, int color);
int     queen_count(BOARD *board, int color);
int     rook_count(BOARD *board, int color);
int     bishop_count(BOARD *board, int color);
int     knight_count(BOARD *board, int color);
int     pawn_count(BOARD *board, int color);
U64     occupied_bb(BOARD *board);
U64     empty_bb(BOARD *board);
int     king_square(BOARD *board, int color);
U8      side_on_move(BOARD *board);
MOVE    get_last_move_made(BOARD *board);
U64     board_key(BOARD *board);
U64     board_pawn_key(BOARD *board);
int     piece_on_square(BOARD *board, int color, int square);
U64     all_pieces_bb(BOARD *board, int color);
U64     qrnb_bb(BOARD *board, int color);
U64     piece_bb(BOARD *board, int color, int piece);
int     has_pieces(BOARD *board, int color);
int     has_recent_null_move(BOARD *board);
U64     ep_square_bb(BOARD *board);
int     ep_square(BOARD *board);
int     can_castle_ks_flag(BOARD *board, int color);
int     can_castle_qs_flag(BOARD *board, int color);
int     can_generate_castle_ks(BOARD *board, int color);
int     can_generate_castle_qs(BOARD *board, int color);
void    set_ply(BOARD *board, U16 value);
U16     get_ply(BOARD *board);
int     get_played_moves(BOARD *board, char *line, size_t max_chars);
int     get_history_moves(BOARD *board, MOVE move[], int max_moves);
U16     get_history_ply(BOARD *board);
void    init_seldepth(BOARD *board);
int     get_played_moves_count(BOARD *board, int color);
void    move_piece(BOARD *board, int color, int type, int frsq, int tosq);
void    set_piece(BOARD *board, int color, int type, int tosq);
void    init_piece(BOARD *board, int color, int type, int tosq);
void    remove_piece(BOARD *board, int color, int type, int frsq);
void    move_piece_undo(BOARD *board, int color, int type, int frsq, int tosq);
void    set_piece_undo(BOARD *board, int color, int type, int index);
void    remove_piece_undo(BOARD *board, int color, int type, int index);

// Utils
UINT    util_get_time(void);
void    util_sleep(int milliseconds);
void    util_get_move_string(MOVE move, char *string);
void    util_get_move_desc(MOVE move, char *string, int inc_file);
MOVE    util_parse_move(GAME *game, char *move_string);
void    util_print_move(MOVE move, int new_line);
void    util_get_board_fen(BOARD *board, char *fen);
void    util_get_board_chars(BOARD *board, char b[64]);
char    piece_letter(int piece);
char    promo_letter(int piece);
void    util_draw_board(BOARD *board);
void    save_board_pgn(GAME *game, char *file_name, int book_moves);

// PGN utils
#define PGN_STRING_SIZE 32767
#define PGN_TAG_SIZE    128
#define PGN_MOVE_SIZE   32
#define PGN_FEN_SIZE    512

typedef struct s_pgn {
    char    *name;
    FILE    *file;
    int     game_number;
}   PGN_FILE;

typedef struct s_pgn_game {
    char    string[PGN_STRING_SIZE];
    char    moves[PGN_STRING_SIZE];
    char    white[PGN_TAG_SIZE];
    char    black[PGN_TAG_SIZE];
    char    result[PGN_TAG_SIZE];
    char    initial_fen[PGN_FEN_SIZE];
    int     moves_index;
    int     move_number;
    int     loses_on_time;
}   PGN_GAME;

typedef struct s_pgn_move {
    char    string[PGN_MOVE_SIZE];
    int     book;
    int     mate;
    double  value;
}   PGN_MOVE;

int     pgn_open(PGN_FILE *game, char *filename);
void    pgn_close(PGN_FILE *pgn_file);
int     pgn_next_game(PGN_FILE *pgn, PGN_GAME *game);
int     pgn_next_move(PGN_GAME *game, PGN_MOVE *move);
void    pgn_move_desc(MOVE move, char *string, int inc_file, int inc_rank);
MOVE    pgn_engine_move(GAME *game, PGN_MOVE *pgn_move);
int     pgn_game_has_valid_result(PGN_GAME *pgn_game);

// Perf
void    perft(int depth);
void    perftx(void);
void    perfty(void);
void    perftz(void);

// Tests
void    epd(char *file_name, SETTINGS *settings);
void    epd_search(char *file, SETTINGS *settings);
void    eval_test(char *file_name);

#ifndef NDEBUG
// Assert functions.
int     valid_material(BOARD *board);
int     valid_rank_file(void);
int     valid_square(int square);
int     valid_color(int color);
int     board_state_is_ok(BOARD *board);
int     valid_piece(BOARD *board, int pcsq, int color, int type);
int     valid_is_legal(BOARD *board, MOVE move);
int     is_move_in_list(GAME *game, MOVE test_move);
int     pawn_is_doubled(BOARD *board, int pcsq, int color);
int     pawn_is_connected(BOARD *board, int pcsq, int color);
int     pawn_is_weak(BOARD *board, int pcsq, int color);
int     pawn_is_passed(BOARD *board, int pcsq, int color);
int     pawn_is_isolated(BOARD *board, int pcsq, int color);
int     pawn_is_candidate(BOARD *board, int pcsq, int color);
#endif

#ifdef EGTB_SYZYGY

#define TB_NO_STDBOOL
#include "fathom/tbprobe.h"

U32 egtb_probe_wdl(BOARD *board, int depth, int ply);

#endif

//End
