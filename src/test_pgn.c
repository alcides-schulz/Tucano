#include "globals.h"

int read_game(FILE *pgn, char *pgn_moves, char *pgn_game, char *white, char *black, char *result);
int pgn_is_end_game(char *pgn_moves);
int pgn_is_number_or_space(char c1);
MOVE pgn_get_curr_move(BOARD *board, char *pgn_move);
void util_pgn_desc(MOVE move, char *string, int inc_file, int inc_rank);

void test_pgn(char *pgn_filename)
{
    FILE    *pgn_file = fopen(pgn_filename, "r");
    char    pgn_moves[16384];
    char    pgn_move[100];
    char    pgn_game[16384];
    int     pl;
    int     pm;
    MOVE    curr_move;
    int     game_count = 0;
    int     move_count;
    FILE    *out = fopen("tune-positions.txt", "w");
    char    fen[1000];
    char    res;
    char    white[100];
    char    black[100];
    char    result[100];
    int     es;
    int     qs = 0;
    GAME game;

    int        total_positions = 0;
    int        selected_positions = 0;

    tt_init(8);

    while (read_game(pgn_file, pgn_moves, pgn_game, white, black, result)) {
        assert(strlen(pgn_moves) < 16384);

        new_game(&game, FEN_NEW_GAME);

        game_count++;
        move_count = 0;

        //printf("game %d while=%s black=%s result=%s\n", game_count, white, black, result);

        pl = 0;
        while (!pgn_is_end_game(&pgn_moves[pl])) {
            while (pgn_is_number_or_space(pgn_moves[pl]))
                pl++;
            pm = 0;
            while (pgn_moves[pl] && pgn_moves[pl] != ' ' && pm < 100) {
                if (pgn_moves[pl] == '#' || pgn_moves[pl] == '+') {
                    pl++;
                    continue;
                }
                pgn_move[pm++] = pgn_moves[pl++];
            }
            pgn_move[pm] = 0;

            move_count++;
            curr_move = pgn_get_curr_move(&game.board, pgn_move);

            if (curr_move == MOVE_NONE) {
                printf("\n\nNOT FOUND: %s\n\n", pgn_move);
                print_current_moves(&game);
                board_print(&game.board, "move");
                break;
            }

            assert(is_valid(&game.board, curr_move));

            make_move(&game.board, curr_move);
            total_positions++;

            if (side_on_move(&game.board) == BLACK) continue;

            if (move_count <= 16) continue; // book moves

            //if (move_count >= 50) continue;

            if (is_incheck(&game.board, WHITE)) continue;

            es = tnn_eval_incremental(&game.board);
            //qs = quiesce(FALSE, -MAX_SCORE, MAX_SCORE, 0, -1);

            if (es != qs) continue;
            if (es < -500 || es > 500) continue;

            selected_positions++;
            //printf("%d: [%s] eval: %d total: %d selected: %d\n", move_count, pgn_move, es, total_positions, selected_positions);
            
            util_get_board_fen(&game.board, fen);
            if (strstr(result, "1-0")) 
                res = 'w';
            else
                if (strstr(result, "0-1")) 
                    res = 'l';
                else
                    res = 'd';

            fprintf(out, "%c %s\n", res, fen);

        }

        printf("%d) total: %d selected: %d\n", game_count, total_positions, selected_positions);
    }


    fclose(out);
    fclose(pgn_file);
}

MOVE pgn_get_curr_move(BOARD *board, char *pgn_move)
{
    MOVE_LIST       ml;
    MOVE        move;
    char        desc[40];

    //select_init(board, &sh, &ml, is_incheck(board, side_on_move(board)), MOVE_NONE, 0);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(board, ml.pins, move))
            continue;
        assert(is_valid(board, move));
        util_pgn_desc(move, desc, 1, 0);
        if (strcmp(pgn_move, desc) != 0)
            util_pgn_desc(move, desc, 0, 1);
        if (strcmp(pgn_move, desc) != 0)
            util_pgn_desc(move, desc, 0, 0);
        if (!strcmp(pgn_move, desc))
            return move;
    }
    return MOVE_NONE;
}

int pgn_is_number_or_space(char c1)
{
    return ((c1 >= '0' && c1 <= '9') || c1 == '.' || c1 == ' ') ? TRUE : FALSE;
}

int pgn_is_end_game(char *pgn_moves)
{
    while (*pgn_moves == ' ')
        pgn_moves++;
    if (!*pgn_moves)
        return 1;
    if (!strncmp(pgn_moves, "1-0", 3) || !strncmp(pgn_moves, "0-1", 3) || !strncmp(pgn_moves, "1/2-1/2", 7))
        return 1;
    return 0;
}

#define ADD_SPACE(s, i)  (i > 0 && s[i - 1] != ' ' ? 1 : 0)

// 0-1
// 1-0
// 1/2-1/2
// *
int is_end_game(char *g, int p)
{
    if (p > 0 && g[p-1] == '*')
        return 1;
    if (p > 2 && g[p-1] == '0' && g[p-2] == '-' && g[p-3] == '1')
        return 1;
    if (p > 2 && g[p-1] == '1' && g[p-2] == '-' && g[p-3] == '0')
        return 1;
    if (p > 6 && g[p-1] == '2' && g[p-2] == '/' && g[p-3] == '1' && g[p-4] == '-' && g[p-5] == '2' && g[p-6] == '/' && g[p-7] == '1')
        return 1;
    return 0;
}

int read_game(FILE *pgn, char *moves, char *game, char *white, char *black, char *result)
{
    int     byte;
    int     pos = 0;
    int     gp = 0;
    int     is_comm = 0;
    int     is_tag = 0;
    char    tag[100];
    int     tag_pos = 0;

    white[0] = black[0] = result[0] = 0;

    moves[0] = '\0';
    game[0] = '\0';

    while ((byte = fgetc(pgn)) != EOF) {
        game[gp++] = (char)byte;
        if (is_comm) {
            if (byte == '}') {
                is_comm = 0;
            }
            continue;
        }
        if (is_tag) {
            if (byte == ']') {
                is_tag = 0;
                tag[tag_pos] = '\0';
                if (!strncmp("White", tag, 5))
                    strcpy(white, tag);
                if (!strncmp("Black", tag, 5))
                    strcpy(black, tag);
                if (!strncmp("Result", tag, 6))
                    strcpy(result, tag);
                continue;
            }
            if (tag_pos + 1 < 100)
                tag[tag_pos++] = (char)byte;
            continue;
        }
        if (byte == '{') {
            is_comm = 1;
            continue;
        }
        if (byte == '[') {
            is_tag = 1;
            tag_pos = 0;
            continue;
        }
        if (byte == '\n') {
            if (ADD_SPACE(moves, pos))
                moves[pos++] = ' ';
            continue;
        }
        if (byte == ' ') {
            if (ADD_SPACE(moves, pos))
                moves[pos++] = ' ';
            continue;
        }
        moves[pos++] = (char)byte;
        if (is_end_game(moves, pos))
            break;
    }

    game[gp] = '\0';
    moves[pos] = 0;
    if (!strcmp(moves, " "))
        moves[0] = 0;
    return (int)strlen(moves);
}

static const char *FILE_LETTER = "abcdefgh";
static const char *RANK_NUMBER = "87654321";

#define file_letter(sq)    FILE_LETTER[get_file((sq))]
#define rank_number(sq)    RANK_NUMBER[get_rank((sq))]

void util_pgn_desc(MOVE move, char *string, int inc_file, int inc_rank)
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

//END
