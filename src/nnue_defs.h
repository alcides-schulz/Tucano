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

#pragma once

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

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#  pragma warning (disable: 4996)
#endif

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/mman.h>
#endif

// NNUE Constants and defines
#ifdef _WIN32
typedef HANDLE FD;
#define FD_ERR INVALID_HANDLE_VALUE
typedef HANDLE map_t;
#else /* Unix */
typedef int FD;
#define FD_ERR -1
typedef size_t map_t;
#endif

static const uint32_t NNUE_VERSION = 0x7AF32F16u;

/**
* Internal piece representation
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12
*/
enum NNUE_COLORS {
    NNUE_WHITE, NNUE_BLACK
};

enum NNUE_PIECES {
    blank = 0, wking, wqueen, wrook, wbishop, wknight, wpawn,
    bking, bqueen, brook, bbishop, bknight, bpawn
};

enum NNUE_INDEXES {
    PS_W_PAWN = 1,
    PS_B_PAWN = 1 * 64 + 1,
    PS_W_KNIGHT = 2 * 64 + 1,
    PS_B_KNIGHT = 3 * 64 + 1,
    PS_W_BISHOP = 4 * 64 + 1,
    PS_B_BISHOP = 5 * 64 + 1,
    PS_W_ROOK = 6 * 64 + 1,
    PS_B_ROOK = 7 * 64 + 1,
    PS_W_QUEEN = 8 * 64 + 1,
    PS_B_QUEEN = 9 * 64 + 1,
    PS_END = 10 * 64 + 1
};

enum NNUE_STRUCTURE {
    FV_SCALE = 16,
    SHIFT = 6,
    KHALF_DIMENSIONS = 256,
    FT_IN_DIMS = 64 * PS_END,
    FT_OUT_DIMS = KHALF_DIMENSIONS * 2,
    TRANSFORMER_START = 3 * 4 + 177,
    NETWORK_START = TRANSFORMER_START + 4 + 2 * 256 + 2 * 256 * 64 * 641
};

#define NNUE_KING(c)    ( (c) ? bking : wking )
#define NNUE_IS_KING(p) ( ((p) == wking) || ((p) == bking) )

// Input feature converter
typedef int8_t clipped_t;
typedef int8_t weight_t;

// Align options for MSC and GCC compilers
#ifdef _MSC_VER
#define AL64    __declspec(align(64))
#define AL08    __declspec(align(8))
#pragma warning (disable : 4324)
#else
#define AL64    __attribute__((aligned(64)))
#define AL08    __attribute__((aligned(8)))
#endif

// InputLayer = InputSlice<256 * 2>
// out: 512 x clipped_t

// Hidden1Layer = ClippedReLu<AffineTransform<InputLayer, 32>>
// 512 x clipped_t -> 32 x int32_t -> 32 x clipped_t

// Hidden2Layer = ClippedReLu<AffineTransform<hidden1, 32>>
// 32 x clipped_t -> 32 x int32_t -> 32 x clipped_t

// OutputLayer = AffineTransform<HiddenLayer2, 1>
// 32 x clipped_t -> 1 x int32_t

typedef struct s_nnue_value {
#ifdef _MSC_VER
    int16_t         ft_biases[KHALF_DIMENSIONS];
    int16_t         ft_weights[KHALF_DIMENSIONS * FT_IN_DIMS];
    AL64 weight_t   hidden1_weights[32 * 512];
    AL64 weight_t   hidden2_weights[32 * 32];
    AL64 weight_t   output_weights[1 * 32];
    AL64 int32_t    hidden1_biases[32];
    AL64 int32_t    hidden2_biases[32];
    int32_t         output_biases[1];
#else
    // using align options for GCC
    int16_t         ft_biases[KHALF_DIMENSIONS];
    int16_t         ft_weights[KHALF_DIMENSIONS * FT_IN_DIMS];
    weight_t        hidden1_weights[32 * 512] AL64;
    weight_t        hidden2_weights[32 * 32] AL64;
    weight_t        output_weights[1 * 32] AL64;
    int32_t         hidden1_biases[32] AL64;
    int32_t         hidden2_biases[32] AL64;
    int32_t         output_biases[1];
#endif
}   NNUE_PARAM;


/**
* nnue data structure
*/

typedef struct s_dirty_piece {
    int         count;
    int         piece[3];
    int         from[3];
    int         to[3];
}   NNUE_DIRTY;

typedef struct s_accumulator {
    int16_t     accumulation[2][256];
    int         computed;
}   NNUE_ACCUM;

typedef struct s_nnue_data {
    NNUE_ACCUM  accumulator;
    NNUE_DIRTY  dirty_piece;
}   NNUE_DATA;

typedef struct s_index_list {
    size_t      size;
    unsigned    values[30];
}   NNUE_INDEXES;

/**
* position data structure passed to core subroutines
*/
typedef struct s_nnue_position {
    int         player;
    int*        pieces;
    int*        squares;
    NNUE_DATA*  nnue_data[3];
}   NNUE_POSITION;

#define clamp(a, b, c) ((a) < (b) ? (b) : (a) > (c) ? (c) : (a))

//  nnue global vars
EXTERN NNUE_PARAM   nnue_param;
#define TUCANO_EVAL_FILE "tucano_nn01.bin"

//  nnue global functions
int nnue_init(const char* eval_file_name, NNUE_PARAM *p_nnue_param);
int nnue_calculate(NNUE_POSITION *pos);
int8_t nnue_piece(int color, int piece);
int8_t nnue_square(int square);

void nnue_test(void);

// END
