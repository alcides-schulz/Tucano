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

#include "globals.h"
#include "eval_nn.h"
//#include "tucanno0012.h"

char *tnn_type_lookup[2] = { "PNBRQK", "pnbrqk" };
void tnn_fen2index(char *fen, S16 index[]);

// Intrinsics compilation for modern cpus, can be defined in the makefile or compile command
//#define TNNAVX2
//#define TNNSSE4

S16 tnn_index(int piece_color, int piece_type, int square)
{
    int piece_index = 0;
    piece_index += piece_color == WHITE ? 0 : 64 * 6;
    piece_index += piece_type * 64;
    piece_index += square;
    return (S16)piece_index;
}

int tnn_eval_full(BOARD *board)
{
    char fen[1000];
    S16 input_index[32];
    S32 hidden_value[TNN_HIDDEN_SIZE];

    util_get_board_fen(board, fen); // TODO: create function tnn_board2index
    tnn_fen2index(fen, input_index);
    
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        hidden_value[i] = TNN_INPUT_BIAS[i];
    }
    for (int i = 0; i < 32 && input_index[i] != -1; i++) {
        int index = input_index[i];
        for (int j = 0; j < TNN_HIDDEN_SIZE; j++) {
            hidden_value[j] += TNN_INPUT_WEIGHT[index][j];
        }
    }
    S32 output_value = TNN_HIDDEN_BIAS;
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        output_value += MAX(0, hidden_value[i]) * TNN_HIDDEN_WEIGHT[i];
    }
    return (int)(output_value / 64 / 64);
}

void tnn_init_hidden_value(BOARD *board)
{
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        board->nn_hidden_value[i] = TNN_INPUT_BIAS[i];
    }
}

void tnn_set_piece(BOARD *board, int piece_color, int piece_type, int square)
{
    int input_index = tnn_index(piece_color, piece_type, square);
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        board->nn_hidden_value[i] += TNN_INPUT_WEIGHT[input_index][i];
    }
}

void tnn_unset_piece(BOARD *board, int piece_color, int piece_type, int square)
{
    int input_index = tnn_index(piece_color, piece_type, square);
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        board->nn_hidden_value[i] -= TNN_INPUT_WEIGHT[input_index][i];
    }
}

#if defined(TNNAVX2)

#include <immintrin.h>

int tnn_eval_incremental(BOARD *board) {
    const size_t WIDTH = sizeof(__m256i) / sizeof(int16_t);
    const size_t CHUNKS = TNN_HIDDEN_SIZE / WIDTH;

    const __m256i zero = _mm256_setzero_si256();
    __m256i accumulator = _mm256_setzero_si256();
    __m256i* hidden_value = (__m256i*)&board->nn_hidden_value;
    __m256i* weights = (__m256i*)TNN_HIDDEN_WEIGHT;

    for (size_t j = 0; j < CHUNKS; j++) {
        const __m256i value = _mm256_max_epi16(hidden_value[j], zero);
        accumulator = _mm256_add_epi32(accumulator, _mm256_madd_epi16(value, weights[j]));
    }

    const __m128i result1 = _mm_add_epi32(_mm256_castsi256_si128(accumulator), _mm256_extractf128_si256(accumulator, 1));
    const __m128i result2 = _mm_add_epi32(result1, _mm_srli_si128(result1, 8));
    const __m128i result3 = _mm_add_epi32(result2, _mm_srli_si128(result2, 4));
    
    int output_value = TNN_HIDDEN_BIAS + _mm_cvtsi128_si32(result3);
    int score = (output_value / 64 / 64);
    return side_on_move(board) == WHITE ? score : -score;
}

#elif defined(TNNSSE4)

#include <immintrin.h>

int tnn_eval_incremental(BOARD *board)
{
    const __m128i zero = _mm_setzero_si128();
    __m128i accumulator = _mm_setzero_si128();

    __m128i* hidden_value = (__m128i*)&board->nn_hidden_value;
    __m128i* weights = (__m128i*)TNN_HIDDEN_WEIGHT;

    for (size_t j = 0; j < TNN_HIDDEN_SIZE / 8; j++) {
        const __m128i value = _mm_max_epi16(hidden_value[j], zero);
        accumulator = _mm_add_epi32(accumulator, _mm_madd_epi16(value, weights[j]));
    }

    const __m128i result1 = _mm_add_epi32(accumulator, _mm_srli_si128(accumulator, 8));
    const __m128i result2 = _mm_add_epi32(result1, _mm_srli_si128(result1, 4));

    int output_value = TNN_HIDDEN_BIAS + _mm_cvtsi128_si32(result2);
    int score = (output_value / 64 / 64);
    return side_on_move(board) == WHITE ? score : -score;
}

#else

int tnn_eval_incremental(BOARD *board)
{
    S32 output_value = TNN_HIDDEN_BIAS;
    for (int i = 0; i < TNN_HIDDEN_SIZE; i++) {
        output_value += MAX(0, board->nn_hidden_value[i]) * TNN_HIDDEN_WEIGHT[i];
    }
    int score = (int)(output_value / 64 / 64);
    return side_on_move(board) == WHITE ? score : -score ;
}

#endif

int tnn_eval(GAME *game)
{
    return tnn_eval_incremental(&game->board);
}

void tnn_fen2index(char *fen, S16 index[])
{
    for (int i = 0; i < TNN_INDEX_SIZE; i++) {
        index[i] = -1;
    }

    int square = 0;
    int input_index = 0;
    int piece_count = 0;

    for (int i = 0; fen[i] && fen[i] != ' '; i++) {
        if (fen[i] == '/') continue;
        if (fen[i] >= '0' && fen[i] <= '9') {
            square += fen[i] - '0';
            continue;
        }
        for (int color = 0; color <= 1; color++) {
            char *type = tnn_type_lookup[color];
            char *piece = strchr(type, fen[i]);
            if (piece != NULL) {
                piece_count++;
                int piece_type = (int)(piece - type);
                if (input_index == TNN_INDEX_SIZE) {
                    printf("error: input_index(%d) equals max size of %d\n", input_index, TNN_INDEX_SIZE);
                    getchar();
                    exit(-1);
                }
                index[input_index++] = tnn_index(color, piece_type, square);
                square += 1;
                break;

            }
        }
    }

    // test
    //printf("%s\n", fen);
    //U64 bb[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ////for (int i = 0; i < 32 && index[i] != -1; i++) {
    ////    int i1 = index[i] / 64;
    ////    int i2 = index[i] % 64;
    ////    printf("i: %d index[i]: %d i1: %d i2: %d\n", i, index[i], i1, i2);
    ////    bb_set_bit(&bb[i1], i2);
    ////}
    //new_game(&temp, fen);
    //for (int i = 0; i < 6; i++) {
    //    board_print(&temp.board, NULL);
    //    printf("type: %c\n", tnn_type[0][i]);
    //    bb_print("1", bb[i]);
    //    bb_print("2", bb[i + 6]);
    //    getchar();
    //}
}

// EOF