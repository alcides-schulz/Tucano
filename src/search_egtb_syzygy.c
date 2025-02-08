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

#ifdef _MSC_VER
#pragma warning (disable : 4206)
#endif

#ifdef EGTB_SYZYGY

//#define EGTB_TRACE

#include "globals.h"

//-------------------------------------------------------------------------------------------------
//  End Game Table Base - Syzygy
//-------------------------------------------------------------------------------------------------

unsigned TB_PROBE_DEPTH = 10;
extern unsigned TB_LARGEST;

U64 convert_to_fathom(U64 bitboard);
int has_repeated(BOARD *board);
int convert_from_fathom(int square);

//-------------------------------------------------------------------------------------------------
//  Probe a position
//-------------------------------------------------------------------------------------------------
U32 egtb_probe_wdl(BOARD *board, int depth, int ply)
{
    int can_castle = FALSE;

    if (board->state[WHITE].can_castle_ks || board->state[WHITE].can_castle_qs) can_castle = TRUE;
    if (board->state[BLACK].can_castle_ks || board->state[BLACK].can_castle_qs) can_castle = TRUE;

    if (board->fifty_move_rule != 0 || can_castle || board->ep_square != 0 || ply == 0) {
        return TB_RESULT_FAILED;
    }

    int piece_count = bb_bit_count(all_pieces_bb(board, WHITE) | all_pieces_bb(board, BLACK));

    if (piece_count > (int)TB_LARGEST || (piece_count == (int)TB_LARGEST && depth < (int)TB_PROBE_DEPTH)) {
        return TB_RESULT_FAILED;
    }

    U64 white = convert_to_fathom(all_pieces_bb(board, WHITE));
    U64 black = convert_to_fathom(all_pieces_bb(board, BLACK));
    U64 kings = convert_to_fathom(king_bb(board, WHITE) | king_bb(board, BLACK));
    U64 queens = convert_to_fathom(queen_bb(board, WHITE) | queen_bb(board, BLACK));
    U64 rooks = convert_to_fathom(rook_bb(board, WHITE) | rook_bb(board, BLACK));
    U64 bishops = convert_to_fathom(bishop_bb(board, WHITE) | bishop_bb(board, BLACK));
    U64 knights = convert_to_fathom(knight_bb(board, WHITE) | knight_bb(board, BLACK));
    U64 pawns = convert_to_fathom(pawn_bb(board, WHITE) | pawn_bb(board, BLACK));

    return tb_probe_wdl(white, black, kings, queens, rooks, bishops, knights, pawns, 0, 0, 0, side_on_move(board) == WHITE ? 1 : 0);
}

//-------------------------------------------------------------------------------------------------
//  Probe the root position and rank the moves using the egtb score
//-------------------------------------------------------------------------------------------------
void egtb_rank_root_moves(GAME *game)
{
    BOARD *board = &game->board;
    S32 move_score[64][64] = { 0 };
    game->is_egtb_position = FALSE;
    int piece_count = bb_bit_count(all_pieces_bb(board, WHITE) | all_pieces_bb(board, BLACK));
    if (piece_count > (int)TB_LARGEST) {
        return;
    }
    if (board->state[WHITE].can_castle_ks || board->state[WHITE].can_castle_qs) {
        return;
    }
    if (board->state[BLACK].can_castle_ks || board->state[BLACK].can_castle_qs) {
        return;
    }
    U64 white = convert_to_fathom(all_pieces_bb(board, WHITE));
    U64 black = convert_to_fathom(all_pieces_bb(board, BLACK));
    U64 kings = convert_to_fathom(king_bb(board, WHITE) | king_bb(board, BLACK));
    U64 queens = convert_to_fathom(queen_bb(board, WHITE) | queen_bb(board, BLACK));
    U64 rooks = convert_to_fathom(rook_bb(board, WHITE) | rook_bb(board, BLACK));
    U64 bishops = convert_to_fathom(bishop_bb(board, WHITE) | bishop_bb(board, BLACK));
    U64 knights = convert_to_fathom(knight_bb(board, WHITE) | knight_bb(board, BLACK));
    U64 pawns = convert_to_fathom(pawn_bb(board, WHITE) | pawn_bb(board, BLACK));
    unsigned ep_index = 0;
    if (board->ep_square) {
        ep_index = bb_first_index(convert_to_fathom(square_bb(board->ep_square)));
    }
    struct TbRootMoves egtb_moves;
    int egtb_hit = tb_probe_root_dtz(white, black, kings, queens, rooks, bishops, knights, pawns,
                                     board->fifty_move_rule, FALSE, ep_index, 
                                     side_on_move(board) == WHITE, has_repeated(board), 1, &egtb_moves);

    if (egtb_hit) {
        // Generate and score the root move list to be used by the search, instead of the usual gen/sort.
        for (int i = 0; i < egtb_moves.size; i++) {
#if defined(EGTB_TRACE)
            printf("%2d) FROM: %2d TO: %2d promotion: %d score: %6d rank: %6d --> FROM: %2d TO: %2d value: %12d\n", i,
                TB_MOVE_FROM(egtb_moves.moves[i].move),
                TB_MOVE_TO(egtb_moves.moves[i].move),
                TB_MOVE_PROMOTES(egtb_moves.moves[i].move),
                egtb_moves.moves[i].tbScore,
                egtb_moves.moves[i].tbRank,
                convert_from_fathom(TB_MOVE_FROM(egtb_moves.moves[i].move)),
                convert_from_fathom(TB_MOVE_TO(egtb_moves.moves[i].move)),
                egtb_moves.moves[i].tbRank * 100000 + egtb_moves.moves[i].tbScore
            );
#endif
            // For promotions only keeps the queen promotion score
            int promotion = TB_MOVE_PROMOTES(egtb_moves.moves[i].move);
            if (promotion != 0 && promotion != TB_PROMOTES_QUEEN) {
                continue;
            }
            int from = convert_from_fathom(TB_MOVE_FROM(egtb_moves.moves[i].move));
            int to = convert_from_fathom(TB_MOVE_TO(egtb_moves.moves[i].move));
            move_score[from][to] = egtb_moves.moves[i].tbRank * 100000 + egtb_moves.moves[i].tbScore;
        }
        int incheck = is_incheck(&game->board, side_on_move(&game->board));
        select_init(&game->root_moves, game, incheck, MOVE_NONE, FALSE);
        if (incheck) {
            gen_check_evasions(&game->board, &game->root_moves);
        }
        else {
            gen_caps(&game->board, &game->root_moves);
            gen_moves(&game->board, &game->root_moves);
        }
        for (int i = 0; i < game->root_moves.count; i++) {
            if (move_is_promotion(game->root_moves.moves[i]) && unpack_prom_piece(game->root_moves.moves[i]) != QUEEN) {
                continue;
            }
            game->root_moves.score[i] = move_score[unpack_from(game->root_moves.moves[i])][unpack_to(game->root_moves.moves[i])];
#if defined(EGTB_TRACE)
            util_print_move(game->root_moves.moves[i], FALSE);
            if (!move_is_promotion(game->root_moves.moves[i]))
                printf(" ");
            printf(" FROM: %2d TO: %2d SCORE: %12d\n", unpack_from(game->root_moves.moves[i]), unpack_to(game->root_moves.moves[i]), game->root_moves.score[i]);
#endif
        }
#if defined(EGTB_TRACE)
        board_print(board, "egtb_hit");
#endif
        game->is_egtb_position = TRUE;
        set_root_node(&game->root_moves);
    }
}

int has_repeated(BOARD *board)
{
    for (int i = board->histply - 2; i >= 0; i -= 2) {
        if (board->history[i].board_key == board->key) {
            return TRUE;
        }
    }
    return FALSE;
}

int convert_from_fathom(int square)
{
    int rank = 7 - get_rank(square);
    int file = get_file(square);
    return rank * 8 + file;
}

U64 convert_to_fathom(U64 bitboard)
{
    U64 fathom_bb = 0;
    while (bitboard) {
        int square = bb_first_index(bitboard);
        int rank = 7 - get_rank(square);
        int file = get_file(square);
        int fathom_square = rank * 8 + file;
        fathom_bb |= (U64)1 << fathom_square;
        bb_clear_bit(&bitboard, square);
    }
    return fathom_bb;
}

void egtb_test(char *file_name)
{
    GAME *game = (GAME *)malloc(sizeof(GAME));
    if (game == NULL) {
        fprintf(stderr, "egtb_test.malloc: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        return;
    }
    char line[1000];
    FILE *f = fopen(file_name, "r");
    if (f == NULL) {
        fprintf(stderr, "egtb_test: not enough memory for %d bytes.\n", (int)sizeof(GAME));
        free(game);
        return;
    }
    while (fgets(line, 1000, f) != NULL) {
        char *pos = strchr(line, '\n');
        if (pos != NULL) {
            *pos = '\0';
        }
        new_game(game, line);
        egtb_rank_root_moves(game);
        printf("%s %d\n", line, game->is_egtb_position);
    }
    fclose(f);
    free(game);
}

#endif
