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
//  Evaluation Tuning
//  -----------------
//  Reference Texel method: https://chessprogramming.wikispaces.com/Texel%27s+Tuning+Method 
//
//  Steps for Tucano (see eval_tune() method):
//  - Create a pgn file containing about 128K games.
//    - Using games played against itself using 3s+0.03 time control.
//  - Run the method select_positions()
//    - will select more than 8M positions from those games
//  - Calculate k than minimizes the error
//    - use calc_min_k function
//  - Run the tunning using exec_tune method
//    - will run for several days
//    - results are saved in tune-results.txt and should be copied to eval_param().
//-------------------------------------------------------------------------------------------------

int TUNE_MATERIAL    = TRUE;
int TUNE_KING        = TRUE;
int TUNE_PAWN        = TRUE;
int TUNE_PASSED      = TRUE;
int TUNE_PIECES      = TRUE;
int TUNE_MOBILITY    = TRUE;
int TUNE_KING_ATTACK = TRUE;
int TUNE_THREAT      = TRUE;
int TUNE_PST_PAWN    = TRUE;
int TUNE_PST_KNIGHT  = TRUE;
int TUNE_PST_BISHOP  = TRUE;
int TUNE_PST_ROOK    = TRUE;
int TUNE_PST_QUEEN   = TRUE;
int TUNE_PST_KING    = TRUE;

enum    {SINGLE_VALUE, OPENING_ENDGAME} LINK_TYPE;

//#define POS_MAX 8000000
#define POS_MAX 8000000
char    position_list[POS_MAX][100];
int     position_count = 0;

#define TUNE_THREADS    16

typedef struct {
    THREAD_ID  thread_id;
    int        start_position;
    int        end_position;
    int        position_count;
    double     k;
    double     error;
    GAME       game;
}   TUNE_THREAD;

TUNE_THREAD     tune_thread[TUNE_THREADS];

#define MAX_PARAM_SIZE 256

typedef struct {
    char    *group_name;
    char    *param_name;
    S32     *eval_param;
    U32     link_type;
    U32     tune_index_sv;
    U32     tune_index_op;
    U32     tune_index_eg;
}   PARAM_LINK;

PARAM_LINK  tune_param_link[MAX_PARAM_SIZE];
int         tune_param_link_count = 0;

char    *tune_param_name[MAX_PARAM_SIZE];
int     tune_param_value[MAX_PARAM_SIZE];
int     tune_param_count = 0;

void    exec_tune(char *results_filename, double k);
void    init_param_list(void);
void    load_positions(char *positions_filename, int start, int end);
void    local_tune(char *results_filename, double k, int param_size, int original[], int initial_guess[], char *param_name[]);
double  calc_e(double k, int tune_param[]);
void    copy_values(int param_size, int from_param[], int to_param[]);
void    print_current_values(char *results_filename, int iteration, UINT time_spent, int param_size, int values[]);
double  calc_min_k(void);
void    select_positions(char *input_pgn, char *output_pos);
double  calc_e_main(double k, int tune_param[], int thread_count, TUNE_THREAD thread_list[]);
void    calc_e_sub(TUNE_THREAD *thread_data);

void eval_tune(void)
{
    //char    *TUNE_GAMES = "tune-games.pgn";
    char    *TUNE_POSITIONS = "tune-positions.txt";
    char    *TUNE_RESULTS = "tune-results.txt";
    double  k = 1.1;

    EVAL_TUNING = TRUE;

    // Step 1: Select positions from pgn file
    //select_positions(TUNE_GAMES, TUNE_POSITIONS);

    // Link parameters from engine to tune process
    init_param_list();

    // Load selected positions to memory
    load_positions(TUNE_POSITIONS, 0, POS_MAX);

    // Step 2: Calculate k that minizes e
    //k = calc_min_k();

    // Step 3: Eval tuning process
    exec_tune(TUNE_RESULTS, k);
}

void exec_tune(char *results_filename, double k)
{
    UINT    start = util_get_time();
    
    printf("k: %f\n", k);
    
    local_tune(results_filename, k, tune_param_count, tune_param_value, tune_param_value, tune_param_name);

    printf("TIME=%u seconds\n", (util_get_time() - start) / 1000);
}

void load_positions(char *positions_file_name, int start, int end)
{
    FILE    *f = fopen(positions_file_name, "r");
    char    line[1000];
    int     count = 0;

    if (f)  {
        printf("loading positions from: %s\n", positions_file_name);
    }
    else  {
        fprintf(stderr, "cannot open file: %s\n", positions_file_name);
        exit(-1);
    }
    position_count = 0;
    while (fgets(line, 1000, f)) {
        line[strlen(line) - 1] = 0;
        if (strlen(line) > 100 - 1)
            continue;
        if (count >= start && count < end) {
            strcpy(position_list[position_count], line);
            position_count++;
            if (position_count == POS_MAX)
                break;
            if (position_count % 1000 == 0)
                printf("loading %d...\r", position_count);
        }
        count++;
    }
    printf("loaded %d positions\n", position_count);
}

void local_tune(char *results_filename, double k, int param_size, int original[], int initial_guess[], char *param_name[])
{
    int     i;
    int     improved = TRUE;
    double  best_e;
    double  new_e;
    int     *new_param;
    int     *best_param;
    int     count = 0;
    UINT    start = util_get_time();
    int     iteration = 0;
    UINT    elapsed;
    int     TUNE_INC = 1;

    new_param = (int *)malloc(param_size * sizeof(int));
    best_param = (int *)malloc(param_size * sizeof(int));

    //best_e = calc_e(k, initial_guess);
    best_e = calc_e_main(k, initial_guess, TUNE_THREADS, tune_thread);

    printf("initial e: %1.20f  calc_e time: %u seconds  param count: %d\n\n", best_e, (util_get_time() - start) / 1000, param_size);

    copy_values(param_size, initial_guess, best_param);
    
    while (improved) {
        improved = FALSE;
        count = 0;
        iteration++;
        printf("iteration: %d...\n", iteration);

        for (i = 0; i < param_size; i++) {
            copy_values(param_size, best_param, new_param);
            new_param[i] += TUNE_INC;
            //new_e = calc_e(k, new_param);
            new_e = calc_e_main(k, new_param, TUNE_THREADS, tune_thread);
            if (new_e < best_e) {
                best_e = new_e;
                copy_values(param_size, new_param, best_param);
                improved = TRUE;
                count++;
            }
            else {
                new_param[i] -= (TUNE_INC * 2);
                //new_e = calc_e(k, new_param);
                new_e = calc_e_main(k, new_param, TUNE_THREADS, tune_thread);
                if (new_e < best_e) {
                    best_e = new_e;
                    copy_values(param_size, new_param, best_param);
                    improved = TRUE;
                    count++;
                }
            }

            printf("%03d/%03d) %-30s org: %4d new: %4d bestE: %1.20f %d\n", i+1, param_size, param_name[i], original[i], best_param[i], best_e, count);
        }

        elapsed = (util_get_time() - start) / 1000 / 60;
        printf("\niteration=%d %u minutes\n\n", iteration, elapsed);

        print_current_values(results_filename, iteration, elapsed, param_size, best_param);
    }

    free(new_param);
    free(best_param);
}

int add_param(char *name, int value) 
{
    if (tune_param_count == MAX_PARAM_SIZE - 1)  {
        fprintf(stderr, "tune.add_param: max param limit reached: %d\n", tune_param_count);
        return -1;
    }
    tune_param_name[tune_param_count] = name;
    tune_param_value[tune_param_count] = value;
    tune_param_count++;
    return tune_param_count - 1;
}

int get_value(char *name) 
{
    int i;
    for (i = 0; i < tune_param_count; i++) {
        if (strcmp(tune_param_name[i], name) != 0) return tune_param_value[i];
    }
    return 0;
}

int get_index(char *name) 
{
    int i;
    for (i = 0; i < tune_param_count; i++) {
        if (!strcmp(tune_param_name[i], name)) return i;
    }
    fprintf(stderr, "no parameter found for '%s'\n", name);
    return -1;
}

void print_current_values(char *results_filename, int iteration, UINT time_spent, int param_size, int values[])
{
    FILE    *out;
    int     i;
    char    prev_group[128];
    
    out = fopen(results_filename, "w");
    if (!out) {
        fprintf(stderr, "could not create file: %s\n", results_filename);
        return;
    }

    fprintf(out, "Copy and paste the following values to eval_param_init.c file.\n\n");

    fprintf(out, "    // Evaluation tuning information: iteration=%d %u minutes param_size: %d\n\n", iteration, time_spent, param_size);
    
    prev_group[0] = '\0';

    for (i = 0; i < tune_param_link_count; i++) {

        if (strcmp(prev_group, tune_param_link[i].group_name))
            fprintf(out, "    // %s\n", tune_param_link[i].group_name);
        strcpy(prev_group, tune_param_link[i].group_name);

        fprintf(out, "    %-20s = ", tune_param_link[i].param_name);
        if (tune_param_link[i].link_type == SINGLE_VALUE)
            fprintf(out, "%d;\n", values[tune_param_link[i].tune_index_sv]);
        else
            fprintf(out, "MAKE_SCORE(%d, %d);\n", values[tune_param_link[i].tune_index_op], values[tune_param_link[i].tune_index_eg]);
    }

    fclose(out);
}

void init_param(char *op_name, char *eg_name, int value)
{
    add_param(op_name, OPENING(value));
    add_param(eg_name, ENDGAME(value));
}

void create_link(char *group, char *name, S32 *eval_param, int link_type)
{
    if (tune_param_link_count == MAX_PARAM_SIZE - 1) {
        fprintf(stderr, "maximum param size reached: %d, tune stoped.\n", tune_param_link_count);
        exit(0);
    }
    tune_param_link[tune_param_link_count].group_name = group;
    tune_param_link[tune_param_link_count].param_name = name;
    tune_param_link[tune_param_link_count].eval_param = eval_param;
    tune_param_link[tune_param_link_count].link_type = link_type;
    if (link_type == SINGLE_VALUE) {
        tune_param_link[tune_param_link_count].tune_index_sv = add_param(name, *eval_param);
    }
    else {
        tune_param_link[tune_param_link_count].tune_index_op = add_param(name, OPENING(*eval_param));
        tune_param_link[tune_param_link_count].tune_index_eg = add_param(name, ENDGAME(*eval_param));
    }
    tune_param_link_count++;
}

void init_param_list(void)
{
    if (TUNE_MATERIAL)  {
        create_link("MATERIAL", "SCORE_PAWN",    &SCORE_PAWN,    OPENING_ENDGAME);
        create_link("MATERIAL", "SCORE_KNIGHT",  &SCORE_KNIGHT,  OPENING_ENDGAME);
        create_link("MATERIAL", "SCORE_BISHOP",  &SCORE_BISHOP,  OPENING_ENDGAME);
        create_link("MATERIAL", "SCORE_ROOK",    &SCORE_ROOK,    OPENING_ENDGAME);
        create_link("MATERIAL", "SCORE_QUEEN",   &SCORE_QUEEN,   OPENING_ENDGAME);
        create_link("MATERIAL", "B_BISHOP_PAIR", &B_BISHOP_PAIR, OPENING_ENDGAME);
    }
    if (TUNE_KING) {
        create_link("KING", "B_PAWN_PROXIMITY",  &B_PAWN_PROXIMITY, OPENING_ENDGAME);
        create_link("KING", "P_PAWN_SHIELD",     &P_PAWN_SHIELD,    OPENING_ENDGAME);
        create_link("KING", "P_PAWN_STORM",      &P_PAWN_STORM,     OPENING_ENDGAME);
    }
    if (TUNE_PAWN) {
        create_link("PAWN", "B_CANDIDATE",     &B_CANDIDATE,     OPENING_ENDGAME);
        create_link("PAWN", "B_CONNECTED",     &B_CONNECTED,     OPENING_ENDGAME);
        create_link("PAWN", "P_DOUBLED",       &P_DOUBLED,       OPENING_ENDGAME);
        create_link("PAWN", "P_ISOLATED",      &P_ISOLATED,      OPENING_ENDGAME);
        create_link("PAWN", "P_ISOLATED_OPEN", &P_ISOLATED_OPEN, OPENING_ENDGAME);
        create_link("PAWN", "P_BACKWARD",      &P_BACKWARD,      OPENING_ENDGAME);
        create_link("PAWN", "P_BACKWARD_OPEN", &P_BACKWARD_OPEN, OPENING_ENDGAME);
    }
    if (TUNE_PASSED) {
        create_link("PASSED", "B_PASSED_RANK3",     &B_PASSED_RANK3,     OPENING_ENDGAME);
        create_link("PASSED", "B_PASSED_RANK4",     &B_PASSED_RANK4,     OPENING_ENDGAME);
        create_link("PASSED", "B_PASSED_RANK5",     &B_PASSED_RANK5,     OPENING_ENDGAME);
        create_link("PASSED", "B_PASSED_RANK6",     &B_PASSED_RANK6,     OPENING_ENDGAME);
        create_link("PASSED", "B_PASSED_RANK7",     &B_PASSED_RANK7,     OPENING_ENDGAME);
        create_link("PASSED", "B_UNBLOCKED_RANK3",  &B_UNBLOCKED_RANK3,  OPENING_ENDGAME);
        create_link("PASSED", "B_UNBLOCKED_RANK4",  &B_UNBLOCKED_RANK4,  OPENING_ENDGAME);
        create_link("PASSED", "B_UNBLOCKED_RANK5",  &B_UNBLOCKED_RANK5,  OPENING_ENDGAME);
        create_link("PASSED", "B_UNBLOCKED_RANK6",  &B_UNBLOCKED_RANK6,  OPENING_ENDGAME);
        create_link("PASSED", "B_UNBLOCKED_RANK7",  &B_UNBLOCKED_RANK7,  OPENING_ENDGAME);
        create_link("PASSED", "P_KING_FAR_MYC",     &P_KING_FAR_MYC,     OPENING_ENDGAME);
        create_link("PASSED", "B_KING_FAR_OPP",     &B_KING_FAR_OPP,     OPENING_ENDGAME);
    }
    if (TUNE_PIECES) {
        create_link("PIECES", "B_ROOK_SEMI_OPEN",   &B_ROOK_SEMI_OPEN,   OPENING_ENDGAME);
        create_link("PIECES", "B_ROOK_FULL_OPEN",   &B_ROOK_FULL_OPEN,   OPENING_ENDGAME);
        create_link("PIECES", "B_ROOK_RANK_8",      &B_ROOK_RANK_8,      OPENING_ENDGAME);
        create_link("PIECES", "B_ROOK_RANK_7",      &B_ROOK_RANK_7,      OPENING_ENDGAME);
        create_link("PIECES", "P_ROOK_TRAP",        &P_ROOK_TRAP,        OPENING_ENDGAME);
        create_link("PIECES", "B_DOUBLE_RANK_7",    &B_DOUBLE_RANK_7,    OPENING_ENDGAME);
        create_link("PIECES", "P_MINOR_BLOCK_PAWN", &P_MINOR_BLOCK_PAWN, OPENING_ENDGAME);
        create_link("PIECES", "B_QUEEN_RANK_7",     &B_QUEEN_RANK_7,     OPENING_ENDGAME);
        create_link("PIECES", "P_BISHOP_TRAP1",     &P_BISHOP_TRAP1,     OPENING_ENDGAME);
        create_link("PIECES", "P_BISHOP_TRAP2",     &P_BISHOP_TRAP2,     OPENING_ENDGAME);
    }
    if (TUNE_MOBILITY) {
        create_link("MOBILITY", "B_QUEEN_MOBILITY",  &B_QUEEN_MOBILITY,  OPENING_ENDGAME);
        create_link("MOBILITY", "B_ROOK_MOBILITY",   &B_ROOK_MOBILITY,   OPENING_ENDGAME);
        create_link("MOBILITY", "B_BISHOP_MOBILITY", &B_BISHOP_MOBILITY, OPENING_ENDGAME);
        create_link("MOBILITY", "B_KNIGHT_MOBILITY", &B_KNIGHT_MOBILITY, OPENING_ENDGAME);
    }
    if (TUNE_KING_ATTACK) {
        create_link("KING_ATTACK", "KING_ATTACK_KNIGHT", &KING_ATTACK_KNIGHT, SINGLE_VALUE);
        create_link("KING_ATTACK", "KING_ATTACK_BISHOP", &KING_ATTACK_BISHOP, SINGLE_VALUE);
        create_link("KING_ATTACK", "KING_ATTACK_ROOK",   &KING_ATTACK_ROOK,   SINGLE_VALUE);
        create_link("KING_ATTACK", "KING_ATTACK_QUEEN",  &KING_ATTACK_QUEEN,  SINGLE_VALUE);
        create_link("KING_ATTACK", "KING_ATTACK_MULTI",  &KING_ATTACK_MULTI,  SINGLE_VALUE);
        create_link("KING_ATTACK", "B_KING_ATTACK",      &B_KING_ATTACK,      SINGLE_VALUE);
    }
    if (TUNE_THREAT) {
        create_link("THREAT", "P_PAWN_ATK_KING",   &P_PAWN_ATK_KING,   OPENING_ENDGAME);
        create_link("THREAT", "P_PAWN_ATK_KNIGHT", &P_PAWN_ATK_KNIGHT, OPENING_ENDGAME);
        create_link("THREAT", "P_PAWN_ATK_BISHOP", &P_PAWN_ATK_BISHOP, OPENING_ENDGAME);
        create_link("THREAT", "P_PAWN_ATK_ROOK",   &P_PAWN_ATK_ROOK,   OPENING_ENDGAME);
        create_link("THREAT", "P_PAWN_ATK_QUEEN",  &P_PAWN_ATK_QUEEN,  OPENING_ENDGAME);
        create_link("THREAT", "B_THREAT_PAWN",     &B_THREAT_PAWN,     OPENING_ENDGAME);
        create_link("THREAT", "B_THREAT_KNIGHT",   &B_THREAT_KNIGHT,   OPENING_ENDGAME);
        create_link("THREAT", "B_THREAT_BISHOP",   &B_THREAT_BISHOP,   OPENING_ENDGAME);
        create_link("THREAT", "B_THREAT_ROOK",     &B_THREAT_ROOK,     OPENING_ENDGAME);
        create_link("THREAT", "B_THREAT_QUEEN",    &B_THREAT_QUEEN,    OPENING_ENDGAME);
    }
    if (TUNE_PST_PAWN) {
        create_link("PST", "PST_P_FILE_OP", &PST_P_FILE_OP, SINGLE_VALUE);
        create_link("PST", "PST_P_RANK_EG", &PST_P_RANK_EG, SINGLE_VALUE);
        create_link("PST", "PST_P_CENTER",  &PST_P_CENTER,  SINGLE_VALUE);
    }
    if (TUNE_PST_KNIGHT) {
        create_link("PST", "PST_N_BORDER", &PST_N_BORDER, OPENING_ENDGAME);
        create_link("PST", "PST_N_CENTER", &PST_N_CENTER, OPENING_ENDGAME);
    }
    if (TUNE_PST_BISHOP) {
        create_link("PST", "PST_B_BORDER",   &PST_B_BORDER,   OPENING_ENDGAME);
        create_link("PST", "PST_B_DIAGONAL", &PST_B_DIAGONAL, OPENING_ENDGAME);
        create_link("PST", "PST_B_CENTER",   &PST_B_CENTER,   OPENING_ENDGAME);
        create_link("PST", "PST_B_BASIC",    &PST_B_BASIC,    OPENING_ENDGAME);
    }
    if (TUNE_PST_ROOK) {
        create_link("PST", "PST_R_CENTER", &PST_R_CENTER, OPENING_ENDGAME);
    }
    if (TUNE_PST_QUEEN) {
        create_link("PST", "PST_Q_RANK0_OP",   &PST_Q_RANK0_OP,   SINGLE_VALUE);
        create_link("PST", "PST_Q_RANKS_OP",   &PST_Q_RANKS_OP,   SINGLE_VALUE);
        create_link("PST", "PST_Q_BORDER0_EG", &PST_Q_BORDER0_EG, SINGLE_VALUE);
        create_link("PST", "PST_Q_BORDER1_EG", &PST_Q_BORDER1_EG, SINGLE_VALUE);
        create_link("PST", "PST_Q_BORDER2_EG", &PST_Q_BORDER2_EG, SINGLE_VALUE);
        create_link("PST", "PST_Q_BORDER3_EG", &PST_Q_BORDER3_EG, SINGLE_VALUE);
    }
    if (TUNE_PST_KING) {
        create_link("PST", "PST_K_RANK_OP",  &PST_K_RANK_OP,  SINGLE_VALUE);
        create_link("PST", "PST_K_RANK_EG",  &PST_K_RANK_EG,  SINGLE_VALUE);
        create_link("PST", "PST_K_FILE0_OP", &PST_K_FILE0_OP, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE1_OP", &PST_K_FILE1_OP, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE2_OP", &PST_K_FILE2_OP, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE3_OP", &PST_K_FILE3_OP, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE0_EG", &PST_K_FILE0_EG, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE1_EG", &PST_K_FILE1_EG, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE2_EG", &PST_K_FILE2_EG, SINGLE_VALUE);
        create_link("PST", "PST_K_FILE3_EG", &PST_K_FILE3_EG, SINGLE_VALUE);
    }
}

void send_param_to_engine(int values[]) 
{
    for (int i = 0; i < tune_param_link_count; i++) {
        if (tune_param_link[i].link_type == SINGLE_VALUE)
            *tune_param_link[i].eval_param = values[tune_param_link[i].tune_index_sv];
        else
            *tune_param_link[i].eval_param = MAKE_SCORE(values[tune_param_link[i].tune_index_op], values[tune_param_link[i].tune_index_eg]);
    }
}

double calc_e_main(double k, int tune_param[], int thread_count, TUNE_THREAD thread_list[])
{
    if (position_count % thread_count != 0) {
        fprintf(stderr, "calc_e_main: wrong number of threads %d for position count of %d\n", thread_count, position_count);
        exit(-1);
    }
    int positions_per_thread = position_count / thread_count;

    send_param_to_engine(tune_param);

    int position_index = 0;
    for (int i = 0; i < thread_count; i++) {
        thread_list[i].start_position = position_index;
        thread_list[i].end_position = position_index + positions_per_thread - 1;
        thread_list[i].position_count = 0;
        thread_list[i].error = 0;
        thread_list[i].k = k;
        THREAD_CREATE(thread_list[i].thread_id, calc_e_sub, &thread_list[i]);

        position_index += positions_per_thread;
    }

    double  error = 0;

    for (int i = 0; i < thread_count; i++) {
        THREAD_WAIT(thread_list[i].thread_id);

        error += thread_list[i].error;

        if (thread_list[i].position_count != positions_per_thread) {
            printf("error: thread %d position_count: %d positions_per_thread: %d\n", i, thread_list[i].position_count, positions_per_thread);
        }
    }

    error /= (double)position_count;

    return error;
}

void calc_e_sub(TUNE_THREAD *thread_data)
{
    char    *line;
    double  result;
    double  x;

    for (int i = thread_data->start_position; i <= thread_data->end_position; i++) {
        line = position_list[i];
        thread_data->position_count++;

        if (line[0] == 'w')
            result = 1.0;
        else
            if (line[0] == 'l')
                result = 0.0;
            else
                result = 0.5;

        new_game(&thread_data->game, &line[2]);

        thread_data->game.search.max_depth = MAX_DEPTH;
        thread_data->game.search.normal_move_time = 20000;
        thread_data->game.search.extended_move_time = 20000;
        thread_data->game.search.post_flag = POST_DEFAULT;

        double eval = (double)quiesce(&thread_data->game, is_incheck(&thread_data->game.board, side_on_move(&thread_data->game.board)), -MAX_SCORE, MAX_SCORE, 0, 0);

        x = -(thread_data->k * eval / 400.0);
        x = 1.0 / (1.0 + pow(10, x));
        x = pow(result - x, 2);

        thread_data->error += x;
    }
}

double calc_e(double k, int tune_param[])
{
    char    *line;
    double  result;
    char    *fen;
    double  err;
    double  eval = 0;
    double  x;
    GAME    game;

    int         count = 0;

    send_param_to_engine(tune_param);

    err = 0;

    while (count < position_count) {
        line = position_list[count++];

        if (count % 10000 == 0)
            printf("%d        \r", count);

        if (line[0] == 'w')
            result = 1.0;
        else
            if (line[0] == 'l')
                result = 0.0;
            else
                result = 0.5;

        fen = &line[2];

        new_game(&game, fen);

        game.search.max_depth = MAX_DEPTH;
        game.search.normal_move_time = 20000;
        game.search.extended_move_time = 20000;
        game.search.post_flag = POST_DEFAULT;

        eval = (double)quiesce(&game, is_incheck(&game.board, side_on_move(&game.board)), -MAX_SCORE, MAX_SCORE, 0, 0);

        x = -(k * eval / 400.0);
        x = 1.0 / (1.0 + pow(10, x));
        x = pow(result - x, 2);
        err += x;
    }

    err = err / (double)position_count;

    return err;
}

void copy_values(int param_size, int from_param[], int to_param[]) 
{
    for (int i = 0; i < param_size; i++) 
        to_param[i] = from_param[i];
}

double calc_min_k(void) 
{
    double k = 0;
    double i;
    double e;
    double s = 9999;

    for (i = -1; i <= 2; i += 0.1)  {
        //e = calc_e(i, tune_param_value);
        e = calc_e_main(i, tune_param_value, TUNE_THREADS, tune_thread);
        if (e < s) {
            k = i;
            s = e;
        }
        printf("i=%3.8f e=%2.8f k=%3.8f\n", i, e, k);
    }
    
    printf("\nk=%1.8f\n", k);

    return k;
}

char tune_result_letter(char *result) {
    if (strstr(result, "1-0")) return 'w';
    if (strstr(result, "0-1")) return 'l';
    return 'd';
}

void select_positions(char *input_pgn, char *output_pos)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;
    MOVE        move;
    int         score = 0;
    char        fen[1024];
    FILE        *out_file;
    int         count = 0;
    int         in_check;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (!pgn_open(&pgn_file, input_pgn))  {
        fprintf(stderr, "cannot open file: %s\n", input_pgn);
        return;
    }
    out_file = fopen(output_pos, "w");
    if (!out_file) {
        fprintf(stderr, "cannot create file: %s\n", output_pos);
        pgn_close(&pgn_file);
        return;
    }

    printf("select_positions: [%s] -> [%s]\n", input_pgn, output_pos);

    while (pgn_next_game(&pgn_file, &pgn_game))  {
        printf("game %3d: %s vs %s: %s %c\n", pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result, tune_result_letter(pgn_game.result));
        
        new_game(game, FEN_NEW_GAME);

        while (pgn_next_move(&pgn_game, &pgn_move))  {

            move = pgn_engine_move(game, &pgn_move);

            if (move == MOVE_NONE) {
                fprintf(stderr, "move not valid: %s\n", pgn_move.string);
                continue; // TODO: review not valid moves. e.g. Qe1e3
            }

            make_move(&game->board, move);
            if (pgn_game.move_number <= 8) continue;
            if (side_on_move(&game->board) != WHITE) continue;
            in_check = is_incheck(&game->board, side_on_move(&game->board));
            score = quiesce(game, in_check, -MAX_SCORE, MAX_SCORE, 0, 0);
            //if (ABS(score) > VALUE_ROOK) continue;
            if (is_mate_score(score)) continue;
            util_get_board_fen(&game->board, fen);

            fprintf(out_file, "%c %s\n", tune_result_letter(pgn_game.result), fen);
            count++;
            
            //board_print("pgn");
        }
        //printf("\n");
    }

    pgn_close(&pgn_file);

    fclose(out_file);
    free(game);

    printf("select_positions saved: [%s] -> [%s]  %d positions\n", input_pgn, output_pos, count);
}

//END
