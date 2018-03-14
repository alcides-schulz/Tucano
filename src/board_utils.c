/*-------------------------------------------------------------------------------
tucano is a XBoard chess playing engine developed by Alcides Schulz.
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

//-------------------------------------------------------------------------------------------------
//  Board Utilies - square math, printing, information, etc.
//-------------------------------------------------------------------------------------------------

static const char *FILE_LETTER = "abcdefgh";
static const char *RANK_NUMBER = "87654321";

static const char PIECE_LETTER[NUM_PIECES] = { 'P', 'N', 'B', 'R', 'Q', 'K' };
static const char PROMO_LETTER[NUM_PIECES] = { ' ', 'n', 'b', 'r', 'q', ' ' };

static const char SQUARE_LABEL[64][3] =
{
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

#define file_letter(sq)    FILE_LETTER[get_file((sq))]
#define rank_number(sq)    RANK_NUMBER[get_rank((sq))]

//-------------------------------------------------------------------------------------------------
//    Piece letter
//-------------------------------------------------------------------------------------------------
char piece_letter(int piece)
{
    assert(piece > 0 && piece < NUM_PIECES);
    return PIECE_LETTER[piece];
}

//-------------------------------------------------------------------------------------------------
//    Piece letter for promotion (N, B, R, Q)
//-------------------------------------------------------------------------------------------------
char promo_letter(int piece)
{
    assert(piece == KNIGHT || piece == BISHOP || piece == ROOK || piece == QUEEN);
    return PROMO_LETTER[piece];
}

//-------------------------------------------------------------------------------------------------
//  Return rank (0-7) from a square (0-63)
//-------------------------------------------------------------------------------------------------
int get_rank(int square)
{
    assert(square >= 0 && square <= 63);
    return square >> 3;
}

//-------------------------------------------------------------------------------------------------
//  Return file (0-7) from a square (0-63)
//-------------------------------------------------------------------------------------------------
int get_file(int square)
{
    assert(square >= 0 && square <= 63);
    return square & 0x07;
}

//-------------------------------------------------------------------------------------------------
//  Return square (0-63) from rank (0-7) and file (0-7)
//-------------------------------------------------------------------------------------------------
int get_square(int rank, int file)
{
    assert(rank >= 0 && rank <= 7);
    assert(file >= 0 && file <= 7);
    return rank * 8 + file;
}

//-------------------------------------------------------------------------------------------------
//  Return the rank from black/white point of view: 0-7
//-------------------------------------------------------------------------------------------------
int get_relative_rank(int color, int rank)
{
    assert(color == WHITE || color == BLACK);
    assert(rank >= 0 && rank <= 7);
    return (7 - rank) ^ (color * 7);
}

//-------------------------------------------------------------------------------------------------
//  Return the square number from black/white point of view: 0-63
//-------------------------------------------------------------------------------------------------
int get_relative_square(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 0 && pcsq <= 63);
    return color == WHITE ? pcsq : pcsq ^ 070;
}

//-------------------------------------------------------------------------------------------------
//  Return TRUE if rank is valid: between 0 and 7
//-------------------------------------------------------------------------------------------------
int is_rank_valid(int rank)
{
    return (rank) >= 0 && (rank) <= 7 ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Return TRUE if file is valid: between 0 and 7
//-------------------------------------------------------------------------------------------------
int is_file_valid(int file)
{
    return (file) >= 0 && (file) <= 7 ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Return the square in front from color point of view.
//-------------------------------------------------------------------------------------------------
int get_front_square(int color, int pcsq)
{
    assert(color == WHITE || color == BLACK);
    assert(pcsq >= 8 && pcsq <= 55); // does not work on last or first ranks

    return pcsq + (color == WHITE ? -8 : 8);
}

//-------------------------------------------------------------------------------------------------
//    Print move.
//-------------------------------------------------------------------------------------------------
void util_print_move(MOVE move, int new_line)
{
    char str[50];
    util_get_move_string(move, str);
    printf("%s", str);
    if (new_line)
        printf("\n");
}

//-------------------------------------------------------------------------------------------------
//    Return move notation.
//-------------------------------------------------------------------------------------------------
void util_get_move_string(MOVE move, char *string)
{
    char promo_piece;

    if (unpack_type(move) == MT_PROMO || unpack_type(move) == MT_CPPRM) {
        promo_piece = promo_letter(unpack_prom_piece(move));
        sprintf(string, "%c%c%c%c%c", file_letter(unpack_from(move)), rank_number(unpack_from(move)),
            file_letter(unpack_to(move)), rank_number(unpack_to(move)),
            promo_piece);
    }
    else
        sprintf(string, "%c%c%c%c", file_letter(unpack_from(move)), rank_number(unpack_from(move)),
            file_letter(unpack_to(move)), rank_number(unpack_to(move)));
}

//-------------------------------------------------------------------------------------------------
//        Return square label: "a8" to "h1"
//-------------------------------------------------------------------------------------------------
const char *square_label(int square)
{
    assert(square >= 0 && square <= 63);
    return SQUARE_LABEL[square];
}

//-------------------------------------------------------------------------------------------------
//    Return move description.
//-------------------------------------------------------------------------------------------------
void util_get_move_desc(MOVE move, char *string, int inc_file)
{
    char promo_piece;
    char moving_piece[2];

    if (unpack_piece(move) == PAWN)
        moving_piece[0] = 0;
    else {
        moving_piece[0] = piece_letter(unpack_piece(move));
        moving_piece[1] = 0;
    }
    if (unpack_type(move) == MT_PROMO || unpack_type(move) == MT_CPPRM) {
        promo_piece = promo_letter(unpack_prom_piece(move));
        sprintf(string, "%s%c%c%c", moving_piece,
            file_letter(unpack_to(move)), rank_number(unpack_to(move)),
            promo_piece);
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
        sprintf(string, "%s%c%c%c", moving_piece, file_letter(unpack_from(move)), file_letter(unpack_to(move)), rank_number(unpack_to(move)));
    else
        sprintf(string, "%s%c%c", moving_piece, file_letter(unpack_to(move)), rank_number(unpack_to(move)));
}

//-------------------------------------------------------------------------------------------------
//  Parse a move from string and create a MOVE.
//-------------------------------------------------------------------------------------------------
MOVE util_parse_move(GAME *game, char *move_string)
{
    int         from;
    int         to;
    MOVE        move;
    int         promo_piece;
    int         temp_piece;
    MOVE_LIST   ml;

    if (strlen(move_string) < 4)
        return MOVE_NONE;
    if (move_string[0] < 'a' || move_string[0] > 'h' || move_string[1] < '0' || move_string[1] > '8')
        return MOVE_NONE;
    if (move_string[2] < 'a' || move_string[2] > 'h' || move_string[3] < '0' || move_string[3] > '8')
        return MOVE_NONE;

    from = ('8' - move_string[1]) * 8 + (move_string[0] - 'a');
    to = ('8' - move_string[3]) * 8 + (move_string[2] - 'a');

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, FALSE);
    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (unpack_from(move) == from && unpack_to(move) == to) {
            if (unpack_type(move) == MT_PROMO || unpack_type(move) == MT_CPPRM) {
                promo_piece = promo_letter(unpack_prom_piece(move));
                if (strlen(move_string) < 5)
                    temp_piece = promo_letter(QUEEN);
                else
                    temp_piece = tolower(move_string[4]);
                if (promo_piece != temp_piece)
                    continue;
            }
            return move;
        }
    }

    return MOVE_NONE;
}
//-------------------------------------------------------------------------------------------------
//  Utility to print a bitboard to screen.
//-------------------------------------------------------------------------------------------------
void bb_print(char *msg, U64 b)
{
    int     i, j;
    char    s1[200];
    char    s2[200];
    int        li;
    int     fi;
    int     bc;
    BBIX    bbix;

    bbix.u64 = b;

    li = bb_last(bbix);
    fi = bb_first(bbix);
    bc = bb_count(bbix);

    memset(s1, 0, sizeof(s1));
    memset(s2, 0, sizeof(s2));

    for (i = 0; i < 64; i++) {
        if (b & 1)
            s1[63 - i] = '1';
        else
            s1[63 - i] = '0';
        b >>= 1;
    }

    for (i = j = 0; i < 64; i++, j++) {
        s2[j] = s1[i];
        if (i > 0 && i < 56 && (((i + 1) % 8) == 0))
            s2[++j] = '\n';
    }

    //printf("%s %s fi=%d li=%d\n", m, s2, fi, li);
    printf("%s firstindex=%d lastindex=%d  bitcount=%d Hex: ((U64)0x%016llX)\n", msg, fi, li, bc, bbix.u64);
    printf("%s\n", s2);
}

//-------------------------------------------------------------------------------------------------
//  Print current board.
//-------------------------------------------------------------------------------------------------
void board_print(BOARD *board, char *text)
{
    if (text != NULL) printf("%s\n", text);

    util_draw_board(board);
    
    printf("\n");

    if (side_on_move(board) == WHITE)
        printf("White to move  ");
    else
        printf("Black to move  ");

    printf("Ply: %d HistPly: %d", get_ply(board), get_history_ply(board));
    printf("     Castle ");
    printf("%c", can_castle_ks_flag(board, WHITE) ? 'K' : ' ');
    printf("%c", can_castle_qs_flag(board, WHITE) ? 'Q' : ' ');
    printf("%c", can_castle_ks_flag(board, BLACK) ? 'k' : ' ');
    printf("%c", can_castle_qs_flag(board, BLACK) ? 'q' : ' ');
    printf("    InCheck?: %s ", is_incheck(board, side_on_move(board)) ? "Yes" : "No");
    printf("\n");
    char fen[1024];
    util_get_board_fen(board, fen);
    printf("%s\n", fen);
    printf("-------------------------------------------------------------------------------\n");

    if (text != NULL) while (getchar() != '\n');
}

//-------------------------------------------------------------------------------------------------
//  Print current move list.
//-------------------------------------------------------------------------------------------------
void print_current_moves(GAME *game)
{
    int         i = 1;
    int         j = 0;
    MOVE        move;
    MOVE_LIST   ml;

    select_init(&ml, game, is_incheck(&game->board, side_on_move(&game->board)), MOVE_NONE, 0);

    while ((move = next_move(&ml)) != MOVE_NONE) {
        if (!is_pseudo_legal(&game->board, ml.pins, move))
            continue;
        if (!is_valid(&game->board, move)) {
            util_print_move(move, 1);
            board_print(&game->board, "invalid_move");
        }
        printf("%2d) %s", i, square_label(unpack_from(move)));
        if (unpack_type(move) == MT_CAPPC || unpack_type(move) == MT_EPCAP)
            printf("x");
        else
            printf("-");
        printf("%s %9d", square_label(unpack_to(move)), 0);
        if (j == 3) {
            printf("\n");
            j = -1;
        }
        else
            printf(" ");
        i++;
        j++;
    }
    printf("\n");
}

//-------------------------------------------------------------------------------------------------
//  Print move list.
//-------------------------------------------------------------------------------------------------
void print_moves(BOARD *board, MOVE_LIST *ml)
{
    int     i = 1;
    int     j = 0;
    int     m;
    MOVE    move;

    for (m = 0; m < ml->count; m++) {
        move = ml->moves[m];
        if (!is_valid(board, move)) {
            util_print_move(move, 1);
            board_print(board, "invalid_move");
        }
        printf("%2d) %s", i, square_label(unpack_from(move)));
        if (unpack_type(move) == MT_CAPPC || unpack_type(move) == MT_EPCAP)
            printf("x");
        else
            printf("-");
        printf("%s %9d", square_label(unpack_to(move)), 0);
        if (j == 3) {
            printf("\n");
            j = -1;
        }
        else
            printf(" ");
        i++;
        j++;
    }
    printf("\n");
}

//-------------------------------------------------------------------------------------------------
//  Create the fen represtation of current position
//-------------------------------------------------------------------------------------------------
void util_get_board_fen(BOARD *board, char *fen)
{
    int     i;
    int     f;
    int     r;
    char    b[64];
    int     idx;

    util_get_board_chars(board, b);

    f = idx = 0;
    for (i = 0, r = 0; i < 64; i++, r++) {
        if (b[i] == ' ')
            f++;
        else {
            if (f)
                fen[idx++] = '0' + (char)f;
            fen[idx++] = b[i];
            f = 0;
        }
        if (r == 7) {
            if (f)
                fen[idx++] = '0' + (char)f;
            f = 0;
            if (i != 63)
                fen[idx++] = '/';
            r = -1;
        }
    }
    fen[idx++] = ' ';
    if (side_on_move(board) == WHITE)
        fen[idx++] = 'w';
    else
        fen[idx++] = 'b';
    fen[idx++] = ' ';
    if (can_castle_ks_flag(board, WHITE) || can_castle_qs_flag(board, WHITE) || can_castle_ks_flag(board, BLACK) || can_castle_qs_flag(board, BLACK)) {
        if (can_castle_ks_flag(board, WHITE))
            fen[idx++] = 'K';
        if (can_castle_qs_flag(board, WHITE))
            fen[idx++] = 'Q';
        if (can_castle_ks_flag(board, BLACK))
            fen[idx++] = 'k';
        if (can_castle_qs_flag(board, BLACK))
            fen[idx++] = 'q';
    }
    else {
        fen[idx++] = '-';
    }
    fen[idx++] = ' ';
    if (ep_square_bb(board)) {
        fen[idx++] = '-';
        fen[idx++] = file_letter(ep_square(board));
        fen[idx++] = rank_number(ep_square(board));
    }
    else {
        fen[idx++] = '-';
    }

    fen[idx] = 0;
}

//-------------------------------------------------------------------------------------------------
//  Create a list of chars representing current board position.
//-------------------------------------------------------------------------------------------------
void util_get_board_chars(BOARD *board, char b[64])
{
    int     index;
    BBIX    piece;

    memset(b, ' ', 64);

    if (king_bb(board, BLACK))
        b[king_square(board, BLACK)] = 'k';
    if (king_bb(board, WHITE))
        b[king_square(board, WHITE)] = 'K';

    piece.u64 = knight_bb(board, BLACK);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'n';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = rook_bb(board, BLACK);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'r';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = bishop_bb(board, BLACK);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'b';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = queen_bb(board, BLACK);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'q';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = pawn_bb(board, BLACK);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'p';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = knight_bb(board, WHITE);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'N';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = rook_bb(board, WHITE);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'R';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = bishop_bb(board, WHITE);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'B';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = queen_bb(board, WHITE);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'Q';
        bb_clear_bit(&piece.u64, index);
    }
    piece.u64 = pawn_bb(board, WHITE);
    while (piece.u64) {
        index = bb_first(piece);
        b[index] = 'P';
        bb_clear_bit(&piece.u64, index);
    }
}

//END


