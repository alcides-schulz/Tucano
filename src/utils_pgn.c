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
//    PGN file parser utility - read cutechess pgn saved files
//-------------------------------------------------------------------------------------------------

char *VALID_RESULT[] = { "White mates", "Black mates", "Draw by" };

int pgn_open(PGN_FILE *game, char *filename) {
    game->file = fopen(filename, "r");
    if (!game->file) return FALSE;
    game->name = filename;
    game->game_number = 0;
    return TRUE;
}

void pgn_close(PGN_FILE *pgn_file) {
    fclose(pgn_file->file);
}

#define ADD_SPACE(s, i)  (i > 0 && s[i - 1] != ' ' ? 1 : 0)

int pgn_is_end_of_game_string(char *g, int p);
int pgn_no_more_moves(char *pgn_moves);
int pgn_number_or_space(char c1);
void pgn_process_move_comments(PGN_GAME *game, PGN_MOVE *pgn_move);

int pgn_next_game(PGN_FILE *pgn, PGN_GAME *game)
{
    int     byte;
    int     move_index = 0;
    int     game_index = 0;
    int     is_comment = FALSE;
    int     is_tag = FALSE;
    char    tag[512];
    int     tag_index = 0;

    memset(game, 0, sizeof(PGN_GAME));
    strcpy(game->initial_fen, FEN_NEW_GAME);

    while ((byte = fgetc(pgn->file)) != EOF) {
        if (game_index + 1 >= PGN_STRING_SIZE) break;
        if (move_index + 1 >= PGN_STRING_SIZE) break;
        
        game->string[game_index++] = (char)byte;

        if (is_comment) {
            game->moves[move_index++] = (char)byte;
            if (byte == '}') is_comment = FALSE;
            continue;
        }
        if (byte == '{') {
            game->moves[move_index++] = (char)byte;
            is_comment = TRUE;
            continue;
        }
        if (is_tag) {
            /*
                [Event "?"]
                [Site "?"]
                [Date "2021.05.04"]
                [Round "1"]
                [White "tucano912"]
                [Black "tucano911"]
                [Result "0-1"]
                [FEN "r1bqkb1r/1ppp1ppp/p1n2n2/4p3/B3P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 1"]
                [PlyCount "136"]
                [SetUp "1"]
                [TimeControl "12+0.12"]
            */
            if (byte == ']') {
                is_tag = 0;
                tag[tag_index] = 0;
                if (!strncmp("White ", tag, 6))  strcpy(game->white, tag);
                if (!strncmp("Black ", tag, 6))  strcpy(game->black, tag);
                if (!strncmp("Result ", tag, 6)) strcpy(game->result, tag);
                if (!strncmp("FEN", tag, 3)) {
                    strcpy(game->initial_fen, &tag[5]); 
                    game->initial_fen[strlen(game->initial_fen)] = '\0';
                }
                continue;
            }
            if (tag_index + 1 < 512)
                tag[tag_index++] = (char)byte;
            continue;
        }
        if (byte == '[') {
            is_tag = TRUE;
            tag_index = 0;
            continue;
        }
        if (byte == '\n') {
            if (ADD_SPACE(game->moves, move_index))
                game->moves[move_index++] = ' ';
            continue;
        }
        if (byte == ' ') {
            if (ADD_SPACE(game->moves, move_index))
                game->moves[move_index++] = ' ';
            continue;
        }
        game->moves[move_index++] = (char)byte;
        if (pgn_is_end_of_game_string(game->moves, move_index))
            break;
    }

    game->string[game_index] = '\0';
    game->moves[move_index] = '\0';
    if (!strcmp(game->moves, " ")) game->moves[0] = '\0';
    pgn->game_number++;

    return (int)strlen(game->moves);
}

int pgn_next_move(PGN_GAME *game, PGN_MOVE *move)
{
    int        msi;

    memset(move, 0, sizeof(PGN_MOVE));
    if (pgn_no_more_moves(&game->moves[game->moves_index])) {
        return FALSE;
    }
    while (pgn_number_or_space(game->moves[game->moves_index])) {
        game->moves_index++;
    }
    msi = 0;
    while (game->moves[game->moves_index] && game->moves[game->moves_index] != ' ' && msi + 1 < PGN_MOVE_SIZE) {
        if (strchr("#+", game->moves[game->moves_index])) {
            game->moves_index++;
            continue;
        }
        move->string[msi++] = game->moves[game->moves_index++];
    }
    move->string[msi] = '\0';

    pgn_process_move_comments(game, move);

    game->move_number++;

    return TRUE;
}

int pgn_is_mate(char *comments, PGN_MOVE *pgn_move)
{
    pgn_move->mate = 0;
    int negative = FALSE;
    char *mate_string = strstr(comments, "{+M");
    if (mate_string == NULL) {
        mate_string = strstr(comments, "{-M");
        negative = TRUE;
    }
    if (mate_string == NULL) {
        return FALSE;
    }
    for (int i = 3; isdigit(mate_string[i]); i++) {
        //printf("%d %s %d\n", i, &mate_string[i], mate_string[i]);
        pgn_move->mate = pgn_move->mate * 10 + (mate_string[i] - '0');
    }
    if (negative) {
        pgn_move->mate *= -1;
    }
    return TRUE;
}

void pgn_process_move_comments(PGN_GAME *game, PGN_MOVE *pgn_move)
{
    /*
        2. a3 {book} g4 {book} 
        3. Bb2 {+0.73/13 0.47s} Nf6 {-0.72/12 0.30s}
        80. Kh4 {-M10/20 0.034s} Kf4 {+M9/21 0.028s}
    */
    while (pgn_number_or_space(game->moves[game->moves_index])) {
        game->moves_index++;
    }
    if (game->moves[game->moves_index] != '{') return;

    if (strchr(&game->moves[game->moves_index], '}') == NULL) return;
    char comments[100];
    int i = 0;
    while (game->moves[game->moves_index] != '}' && i < 100) {
        comments[i++] = game->moves[game->moves_index++];
    }
    game->moves_index++;
    comments[i] = '\0';

    if (strstr(comments, "book")) {
        pgn_move->book = TRUE;
        return;
    }
    if (pgn_is_mate(comments, pgn_move)) {
        return;
    }
    char *start_eval = strchr(comments, '{');
    char *end_eval = strchr(comments, '/');
    if (start_eval == NULL || end_eval == NULL) return;
    *end_eval = '\0';
    pgn_move->value = atof(start_eval + 1);
}

MOVE pgn_engine_move(GAME *game, PGN_MOVE *pgn_move)
{
    MOVE_LIST   ml;
    MOVE        move;
    char        desc[128];

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, ml.pins, move)) continue;
        assert(is_valid(&game->board, move));
        pgn_move_desc(move, desc, TRUE, FALSE);
        if (strcmp(pgn_move->string, desc)) pgn_move_desc(move, desc, FALSE, TRUE);
        if (strcmp(pgn_move->string, desc)) pgn_move_desc(move, desc, FALSE, FALSE);
        if (!strcmp(pgn_move->string, desc)) return move;
    }
    return MOVE_NONE;
}

int pgn_no_more_moves(char *pgn_moves)
{
    while (*pgn_moves == ' ') {
        pgn_moves++;
    }
    if (!*pgn_moves) {
        return TRUE;
    }
    if (!strncmp(pgn_moves, "1-0", 3) || !strncmp(pgn_moves, "0-1", 3) || !strncmp(pgn_moves, "1/2-1/2", 7)) {
        return TRUE;
    }
    return FALSE;
}

// 0-1
// 1-0
// 1/2-1/2
// *
int pgn_is_end_of_game_string(char *g, int p)
{
    if (p > 0 && g[p-1] == '*')
        return TRUE;
    if (p > 2 && g[p-1] == '0' && g[p-2] == '-' && g[p-3] == '1')
        return TRUE;
    if (p > 2 && g[p-1] == '1' && g[p-2] == '-' && g[p-3] == '0')
        return TRUE;
    if (p > 6 && g[p-1] == '2' && g[p-2] == '/' && g[p-3] == '1' && g[p-4] == '-' && g[p-5] == '2' && g[p-6] == '/' && g[p-7] == '1')
        return TRUE;
    return FALSE;
}

int pgn_number_or_space(char c1)
{
    if ((c1 >= '0' && c1 <= '9') || c1 == '.' || c1 == ' ' || c1 == '\n' || c1 == '\r') {
        return TRUE;
    }
    return FALSE;
}

static const char *FILE_LETTER = "abcdefgh";
static const char *RANK_NUMBER = "87654321";

#define file_letter(sq)    FILE_LETTER[get_file((sq))]
#define rank_number(sq)    RANK_NUMBER[get_rank((sq))]

void pgn_move_desc(MOVE move, char *string, int inc_file, int inc_rank)
{
    char promo_piece;
    char moving_piece[2];

    if (unpack_piece(move) == PAWN) {
        if (move_is_quiet(move)) {
            sprintf(string, "%c%c", file_letter(unpack_to(move)), rank_number(unpack_to(move)));
            return;
        }
        moving_piece[0] = 0;
    }
    else {
        moving_piece[0] = piece_letter(unpack_piece(move));
        moving_piece[1] = 0;
    }
    if (unpack_type(move) == MT_PROMO) {
        promo_piece = promo_letter(unpack_prom_piece(move));
        sprintf(string, "%s%c%c=%c", moving_piece,
                                     file_letter(unpack_to(move)), rank_number(unpack_to(move)),
                                     toupper(promo_piece));
        return;
    }
    if (unpack_type(move) == MT_CPPRM) {
        promo_piece = promo_letter(unpack_prom_piece(move));
        sprintf(string, "%cx%c%c=%c", file_letter(unpack_from(move)),
                                      file_letter(unpack_to(move)), rank_number(unpack_to(move)),
                                      toupper(promo_piece));
        return;
    }
    if (unpack_type(move) == MT_EPCAP) {
        sprintf(string, "%cx%c%c", file_letter(unpack_from(move)), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
        return;
    }
    if (unpack_type(move) == MT_CSWKS || unpack_type(move) == MT_CSBKS) {
        sprintf(string, "O-O");
        return;
    }
    if (unpack_type(move) == MT_CSWQS || unpack_type(move) == MT_CSBQS) {
        sprintf(string, "O-O-O");
        return;
    }
    if (inc_file)
        sprintf(string, "%s%c%s%c%c", moving_piece, file_letter(unpack_from(move)), (unpack_type(move) == MT_CAPPC ? "x" : ""), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
    else
    if (inc_rank)
        sprintf(string, "%s%c%s%c%c", moving_piece, rank_number(unpack_from(move)), (unpack_type(move) == MT_CAPPC ? "x" : ""), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
    else {
        if (unpack_piece(move) == PAWN && unpack_type(move) == MT_CAPPC)
            sprintf(string, "%c%s%c%c", file_letter(unpack_from(move)), (unpack_type(move) == MT_CAPPC ? "x" : ""), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
        else
            sprintf(string, "%s%s%c%c", moving_piece, (unpack_type(move) == MT_CAPPC ? "x" : ""), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
    }
}

void extract_tnn(char *input_pgn)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (!pgn_open(&pgn_file, input_pgn)) {
        fprintf(stderr, "cannot open file: %s\n", input_pgn);
        return;
    }

    int game_count = 0;

    while (pgn_next_game(&pgn_file, &pgn_game)) {

        game_count++;

        if (strstr(pgn_game.string, "loses on time") != NULL) {
            continue;
        }

        if (pgn_file.game_number % 100 == 0) {
            printf("game %3d: %s vs %s: %s                  \r", pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result);
        }

        int move_count = 0;

        new_game(game, pgn_game.initial_fen);

        while (pgn_next_move(&pgn_game, &pgn_move)) {

            MOVE move = pgn_engine_move(game, &pgn_move);

            printf("%s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);

            if (move == MOVE_NONE) {
                printf("%s\n", pgn_game.string);
                printf("PGN_MOVE: [%s]\n", pgn_move.string);
                board_print(&game->board, NULL);
                break;
            }
            
            move_count++;

            if (!pgn_move.book && !pgn_move.mate) {
                char fen[1024];
                util_get_board_fen(&game->board, fen);
                printf("%s;score=%d;move=%s:ply=%d\n", fen, (int)(pgn_move.value * 100), pgn_move.string, move_count);
            }

            make_move(&game->board, move);
        }

        break;
    }

    pgn_close(&pgn_file);

    free(game);

    printf("\ndone.\n");
}

void extract_good_games(char *input_pgn)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (!pgn_open(&pgn_file, input_pgn)) {
        fprintf(stderr, "cannot open file: %s\n", input_pgn);
        return;
    }

    int game_count = 0;

    while (pgn_next_game(&pgn_file, &pgn_game)) {

        game_count++;

        if (strstr(pgn_game.string, "loses on time") != NULL) {
            continue;
        }

        if (pgn_file.game_number % 100 == 0) {
            printf("game %3d: %s vs %s: %s                  \r", pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result);
        }

        if (strstr(pgn_game.result, "1/2-1/2") != NULL) continue;
        if (strstr(pgn_game.result, "0-1") != NULL) continue;

        int valid_game = TRUE;
        int total_diff = 0;
        int move_count = 0;

        new_game(game, pgn_game.initial_fen);

        //printf("%s\n", pgn_game.moves);

        while (pgn_next_move(&pgn_game, &pgn_move)) {

            MOVE move = pgn_engine_move(game, &pgn_move);
            
            move_count++;

            //printf("%s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);

            if (move == MOVE_NONE) {
                valid_game = FALSE;
                printf("%s\n", pgn_game.string);
                printf("GAME: %d PGN_MOVE: %d [%s]\n", game_count, move_count, pgn_move.string);
                board_print(&game->board, "error");
                continue;
            }

            make_move(&game->board, move);

            int diff = 0;
            if (move_count < 50) {
                diff = material_value(&game->board, BLACK) - material_value(&game->board, WHITE);
                if (diff > 0) total_diff += diff;
            }
        }
        if (!valid_game) continue;
        
        if (total_diff <= 0) continue;

        char output_name[1024];
        sprintf(output_name, "d:/temp/games/%05d.%05d.pgn", ABS(total_diff), game_count);
        FILE *out_file = fopen(output_name, "w");
        if (!out_file) {
            fprintf(stderr, "cannot create file: %s\n", output_name);
            pgn_close(&pgn_file);
            return;
        }
        fprintf(out_file, "%s", pgn_game.string);
        fclose(out_file);
    }

    pgn_close(&pgn_file);

    free(game);

    printf("\ndone.\n");
}

int pgn_game_has_valid_result(PGN_GAME *pgn_game)
{
    for (size_t i = 0; i < sizeof(VALID_RESULT) / sizeof(VALID_RESULT[0]); i++) {
        if (strstr(pgn_game->string, VALID_RESULT[i]) != NULL) return TRUE;
    }
    return FALSE;
}

int is_only_one_mate_move(GAME *game, int mate_in, MOVE mate_move)
{
    SETTINGS settings;
    settings.single_move_time = 10000;
    settings.total_move_time = 0;
    settings.moves_per_level = 0;
    settings.max_depth = MAX_DEPTH;
    settings.post_flag = POST_NONE;
    settings.use_book = FALSE;
    settings.max_nodes = 0;
#ifdef TUCANO_COMPOSITION
    settings.exclude = mate_move;
#else
    (void)mate_move;
#endif

    search_run(game, &settings);
    //board_print(&game->board, NULL);
    //util_print_move(game->search.best_move, FALSE);
    //printf(" %d %d\n", game->search.best_score, MATE_VALUE - game->search.best_score);
    //getchar();
    if (MATE_SCORE - game->search.best_score == mate_in) {
        return FALSE;
    }
    return TRUE;
}

int get_move_count(GAME *game, int incheck)
{
    MOVE_LIST   ml;
    MOVE        move;
    int         count = 0;
    select_init(&ml, game, incheck, MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, ml.pins, move)) {
            continue;
        }
        count++;
    }
    return count;
}

void extract_composition(char *input_pgn)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;
    int         MATE_COUNT = 5; // mate in 3 for side on move

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (!pgn_open(&pgn_file, input_pgn)) {
        fprintf(stderr, "cannot open file: %s\n", input_pgn);
        return;
    }

    int game_count = 0;
    FILE *out_file = fopen("d:/temp/composition.csv", "w");
    fprintf(out_file, "fen,move,wvalue,bvalue,moves,quiet,check\n");

    while (pgn_next_game(&pgn_file, &pgn_game)) {

        game_count++;

        if (strstr(pgn_game.string, "loses on time") != NULL) {
            continue;
        } 
        if (strstr(pgn_game.result, "1-0") == NULL) {
            continue;
        }

        //if (pgn_file.game_number % 100 == 0) {
            printf("game %3d: %s vs %s: %s                  \r", pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result);
        //}

        new_game(game, pgn_game.initial_fen);

        //printf("%s\n", pgn_game.moves);

        while (pgn_next_move(&pgn_game, &pgn_move)) {

            MOVE move = pgn_engine_move(game, &pgn_move);

            //printf("%s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);

            if (move == MOVE_NONE) {
                printf("%s\n", pgn_game.string);
                printf("PGN_MOVE: [%s]\n", pgn_move.string);
                board_print(&game->board, NULL);
                continue;
            }

            if (side_on_move(&game->board) == WHITE && pgn_move.mate == MATE_COUNT) {
                int incheck = is_incheck(&game->board, side_on_move(&game->board));
                if (!incheck) {
                    if (is_only_one_mate_move(game, MATE_COUNT, move)) {
                        //printf("\n%s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);
                        char move_string[100];
                        char fen[1000];
                        util_get_move_string(move, move_string);
                        util_get_board_fen(&game->board, fen);
                        int wv = material_value(&game->board, WHITE);
                        int bv = material_value(&game->board, BLACK);
                        int move_count = get_move_count(game, incheck);
                        int quiet = move_is_quiet(move) ? 1 : 0;
                        int check = is_check(&game->board, move);
                        fprintf(out_file, "%s,%s,", fen, move_string);
                        fprintf(out_file, "%d,%d,", wv, bv);
                        fprintf(out_file, "%d,%d,%d", move_count, quiet, check);
                        fprintf(out_file, "\n");
                        fflush(out_file);
                        break;
                    }
                }
            }

            make_move(&game->board, move);
        }

        //break;
    }

    pgn_close(&pgn_file);
    fclose(out_file);

    free(game);

    printf("\ndone.\n");
}

void generate_replay(char *input_pgn, char *output_pgn)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    if (!pgn_open(&pgn_file, input_pgn)) {
        fprintf(stderr, "cannot open file: '%s'\n", input_pgn);
        return;
    }

    FILE *out_file = fopen(output_pgn, "w");
    char fen[1000];
    char move_string[100];

    while (pgn_next_game(&pgn_file, &pgn_game)) {

        printf("game %3d: %s vs %s: %s                  \r", pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result);

        new_game(game, pgn_game.initial_fen);

        util_get_board_fen(&game->board, fen);
        fprintf(out_file, "%s,\n", fen);

        while (pgn_next_move(&pgn_game, &pgn_move)) {

            MOVE move = pgn_engine_move(game, &pgn_move);

            if (move == MOVE_NONE) {
                printf("%s\n", pgn_game.string);
                printf("PGN_MOVE: [%s]\n", pgn_move.string);
                board_print(&game->board, NULL);
                continue;
            }

            make_move(&game->board, move);

            util_get_move_string(move, move_string);
            util_get_board_fen(&game->board, fen);
            fprintf(out_file, "%s,%s\n", fen, move_string);
        }

        break; // only one game at a time.
    }

    pgn_close(&pgn_file);
    fclose(out_file);

    free(game);

    printf("\ndone.\n");
}


void generate_plain_files(char *pgn_file_list)
{
    PGN_FILE    pgn_file;
    PGN_GAME    pgn_game;
    PGN_MOVE    pgn_move;

    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "select_positions.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }

    FILE *file_list = fopen(pgn_file_list, "r");
    if (!file_list) {
        fprintf(stderr, "cannot open pgn file list: %s\n", pgn_file_list);
        return;
    }

    int MAX_PLAIN_POSITIONS = 100000000;
    int plain_position_count = 0;
    int plain_file_number = 0;
    char plain_file_name[512];
    FILE *plain_file = NULL;

    char input_pgn[1024];
    
    while (fgets(input_pgn, 1000, file_list) != NULL) {

        char *new_line_char = strchr(input_pgn, '\n');
        if (new_line_char != NULL) {
            *new_line_char = '\0';
        }

        if (!pgn_open(&pgn_file, input_pgn)) {
            fprintf(stderr, "cannot open file: %s\n", input_pgn);
            continue;
        }

        int game_count = 0;

        while (pgn_next_game(&pgn_file, &pgn_game)) {

            game_count++;

            if (strstr(pgn_game.string, "loses on time") != NULL) {
                continue;
            }
            if (strstr(pgn_game.result, "1-0") == NULL) {
                if (strstr(pgn_game.result, "0-1") == NULL) {
                    if (strstr(pgn_game.result, "1/2-1/2") == NULL) {
                        //printf("\ninvalid result: %s\n", pgn_game.result);
                        continue;
                    }
                }
            }

            if (pgn_file.game_number % 100 == 0) {
                printf("file %s game %3d: %s vs %s: %s      \r", 
                    input_pgn, pgn_file.game_number, pgn_game.white, pgn_game.black, pgn_game.result);
            }

            int move_count = 0;
            int white_win = strstr(pgn_game.result, "1-0") != NULL;
            int black_win = strstr(pgn_game.result, "0-1") != NULL;
 
            new_game(game, pgn_game.initial_fen);

            //printf("%s\n", pgn_game.moves);

            while (pgn_next_move(&pgn_game, &pgn_move)) {

                MOVE move = pgn_engine_move(game, &pgn_move);

                move_count++;

                //printf("%s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);
                //getchar();

                if (move == MOVE_NONE) {
                    //printf("file name: %s\n", input_pgn);
                    //printf("%s\n", pgn_game.string);
                    //printf("GAME: %d PGN_MOVE: %d [%s]\n", game_count, move_count, pgn_move.string);
                    //board_print(&game->board, "error");
                    continue;
                }

                if (!pgn_move.book && !pgn_move.mate && !move_is_en_passant(move)) {

                    char fen[512];
                    util_get_board_fen(&game->board, fen);

                    char move_string[100];
                    util_get_move_string(move, move_string);

                    int score = (int)(pgn_move.value * 100);

                    int result = 0;
                    if (white_win) {
                        result = side_on_move(&game->board) == WHITE ? 1 : -1;
                    }
                    if (black_win) {
                        result = side_on_move(&game->board) == WHITE ? -1 : 1;
                    }

                    int ply = move_count;

                    //fen 8/7p/4nk2/7P/6K1/8/8/8 w - -
                    //move g4f3
                    //score -807
                    //ply 118
                    //result -1
                    //e
                    if (plain_file == NULL || plain_position_count > MAX_PLAIN_POSITIONS) {
                        plain_position_count = 0;
                        plain_file_number++;
                        sprintf(plain_file_name, "data%04d.plain", plain_file_number);
                        if (plain_file != NULL) {
                            fclose(plain_file);
                        }
                        plain_file = fopen(plain_file_name, "w");
                        if (!plain_file) {
                            fprintf(stderr, "could not create plain file\n");
                            fclose(file_list);
                            free(game);
                            return;
                        }
                        printf("\nplain file: %s\n", plain_file_name);
                    }
                    //fprintf(plain_file, "MOVE: %s %d %d %f\n", pgn_move.string, pgn_move.book, pgn_move.mate, pgn_move.value);
                    fprintf(plain_file, "fen %s\n", fen);
                    fprintf(plain_file, "move %s\n", move_string);
                    fprintf(plain_file, "score %d\n", score);
                    fprintf(plain_file, "ply %d\n", ply);
                    fprintf(plain_file, "result %d\n", result);
                    fprintf(plain_file, "e\n");
                    plain_position_count++;
                    //if (position_count == 1000) {
                    //    pgn_close(&pgn_file);
                    //    goto end_processing;
                    //}

                }
                make_move(&game->board, move);
            }
        }

        pgn_close(&pgn_file);

    }
//end_processing:
    fclose(plain_file);
    fclose(file_list);
    free(game);

    printf("\ndone.\n");
}

//END
