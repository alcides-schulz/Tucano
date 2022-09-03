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

    if (pgn_no_more_moves(&game->moves[game->moves_index])) return FALSE;

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
    if (strstr(comments, "{M")) {
        pgn_move->mate = TRUE;
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
    while (*pgn_moves == ' ') pgn_moves++;
    if (!*pgn_moves) return TRUE;
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
    if ((c1 >= '0' && c1 <= '9') || c1 == '.' || c1 == ' ')
        return TRUE;
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

void extract_games(char *input_pgn)
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

        new_game(game, pgn_game.initial_fen);

        int valid_game = TRUE;
        int total_diff = 0;
        int move_count = 0;
        int white_win = strstr(pgn_game.result, "1-0") != NULL;

        while (pgn_next_move(&pgn_game, &pgn_move)) {

            MOVE move = pgn_engine_move(game, &pgn_move);

            if (move == MOVE_NONE) {
                valid_game = FALSE;
                printf("%s\n", pgn_game.string);
                printf("PGN_MOVE: [%s]\n", pgn_move.string);
                board_print(&game->board, NULL);
                continue;
            }

            make_move(&game->board, move);
            move_count++;

            int diff = 0;
            if (move_count < 40) {
                if (white_win) {
                    diff = material_value(&game->board, BLACK) - material_value(&game->board, WHITE);
                }
                else {
                    diff = material_value(&game->board, WHITE) - material_value(&game->board, BLACK);
                }
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

//END
