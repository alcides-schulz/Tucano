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

//-------------------------------------------------------------------------------------------------
//  Bitboard functions.
//
//  Bitboard is used from left to right. Leftmost bit is 0, and rightmost bit is 63.
//  Main methods are bb_first_index, bb_last_index, bb_bit_count.
//  Takes advantage of GCC compiler builtins or MSC intrinsics (visual studio).
//  Otherwise will use software implementation.
//-------------------------------------------------------------------------------------------------

#if defined(__GNUC__)
//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
extern inline int bb_first_index(U64 bb)
{
    assert(bb != 0);
    return __builtin_clzll(bb);
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
extern inline int bb_last_index(U64 bb)
{
    assert(bb != 0);
    return __builtin_ctzll(bb) ^ 63;
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the bitboard
//-------------------------------------------------------------------------------------------------
extern inline int bb_bit_count(U64 bb)
{
    return __builtin_popcountll(bb);
}

// Not necessary when using builtins functions.
void init_first_index_table(void){}
void init_last_index_table(void){}
void init_count_table(void){}

#elif defined(_WIN64) && defined(_MSC_VER)

#include <intrin.h>
//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
extern inline int bb_first_index(U64 bb)
{
    assert(bb != 0);
    unsigned long idx;
    _BitScanReverse64(&idx, bb);
    return (int)idx ^ 63;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
extern inline int bb_last_index(U64 bb)
{
    assert(bb != 0);
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return (int)idx ^ 63;
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the bitboard
//-------------------------------------------------------------------------------------------------
extern inline int bb_bit_count(U64 bb)
{
    return (int)_mm_popcnt_u64(bb);
}

// Not necessary when using intrinsics functions.
void init_first_index_table(void){}
void init_last_index_table(void){}
void init_count_table(void){}

#else

// Software implementation of bitboard functions. Uses tables with pre-calculated values.

// Index tables for bit location and count within a bitboard.
S8      first_index_table[4][65536];
S8      last_index_table[4][65536];
U8      bit_count_table[65536];

U8      get_bit_count(U64 bb);
S8      get_first_index(U16 value);
S8      get_last_index(U16 value);

//-------------------------------------------------------------------------------------------------
//  Table used to give first index on a bitboard
//-------------------------------------------------------------------------------------------------
void init_first_index_table(void)
{
    first_index_table[0][0] = -1;
    first_index_table[1][0] = -1;
    first_index_table[2][0] = -1;
    first_index_table[3][0] = -1;

    for (U32 i = 1; i < 65536; i++) {
        first_index_table[0][i] = get_first_index((U16)i);
        first_index_table[1][i] = get_first_index((U16)i) + 16;
        first_index_table[2][i] = get_first_index((U16)i) + 32;
        first_index_table[3][i] = get_first_index((U16)i) + 48;
    }
}

//-------------------------------------------------------------------------------------------------
//  Table used to give last index on a bitboard.
//-------------------------------------------------------------------------------------------------
void init_last_index_table(void)
{
    last_index_table[0][0] = -1;
    last_index_table[1][0] = -1;
    last_index_table[2][0] = -1;
    last_index_table[3][0] = -1;

    for (U32 i = 1; i < 65536; i++) {
        last_index_table[0][i] = get_last_index((U16)i);
        last_index_table[1][i] = get_last_index((U16)i) + 16;
        last_index_table[2][i] = get_last_index((U16)i) + 32;
        last_index_table[3][i] = get_last_index((U16)i) + 48;
    }
}

//-------------------------------------------------------------------------------------------------
//  Table used to calculate numbers of bits for a bitboard.
//-------------------------------------------------------------------------------------------------
void init_count_table(void)
{
    for (U64 j = 0; j < 65536; j++) {
        bit_count_table[j] = get_bit_count(j);
    }
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the bitboard
//-------------------------------------------------------------------------------------------------
U8 get_bit_count(U64 bb)
{
    U8 count = 0;
    while (bb) {
        count++;
        bb &= bb - 1;
    }
    return count;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit for a short integer, or -1 when 0. 
//  First bit is found from right to left.
//    Used for tables initialization only.
//-------------------------------------------------------------------------------------------------
S8 get_first_index(U16 value)
{
    for (S8 i = 0; i < 16; i++) {
        if (value & (1 << (15 - i))) return i;
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit for a short integer, or -1 when 0.
//  Last bit is found from left to right.
//    Used for tables initialization only.
//-------------------------------------------------------------------------------------------------
S8 get_last_index(U16 value)
{
    for (S8 i = 0; i < 16; i++) {
        if (value & (1 << i)) return (S8)(15 - i);
    }
    return -1;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
int bb_first_index(U64 bb)
{
    assert(bb != 0);
    if (bb & (U64)0xFFFF000000000000) return first_index_table[0][(bb & (U64)0xFFFF000000000000) >> 48];
    if (bb & (U64)0x0000FFFF00000000) return first_index_table[1][(bb & (U64)0x0000FFFF00000000) >> 32];
    if (bb & (U64)0x00000000FFFF0000) return first_index_table[2][(bb & (U64)0x00000000FFFF0000) >> 16];
    return first_index_table[3][(bb & (U64)0x000000000000FFFF)];
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit of a bitboard: 0 to 63.
//-------------------------------------------------------------------------------------------------
int bb_last_index(U64 bb)
{
    assert(bb != 0);
    if (bb & (U64)0x000000000000FFFF) return last_index_table[3][bb & (U64)0x000000000000FFFF];
    if (bb & (U64)0x00000000FFFF0000) return last_index_table[2][(bb & (U64)0x00000000FFFF0000) >> 16];
    if (bb & (U64)0x0000FFFF00000000) return last_index_table[1][(bb & (U64)0x0000FFFF00000000) >> 32];
    return last_index_table[0][(bb & (U64)0xFFFF000000000000) >> 48];
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the bitboard
//-------------------------------------------------------------------------------------------------
int bb_bit_count(U64 bb)
{
    return bit_count_table[(bb & (U64)0xFFFF000000000000) >> 48] +
           bit_count_table[(bb & (U64)0x0000FFFF00000000) >> 32] +
           bit_count_table[(bb & (U64)0x00000000FFFF0000) >> 16] +
           bit_count_table[(bb & (U64)0x000000000000FFFF)];
}
#endif

//-------------------------------------------------------------------------------------------------
//  Initializations
//-------------------------------------------------------------------------------------------------
void bb_init(void)
{
    init_first_index_table();
    init_last_index_table();
    init_count_table();
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for square index
//  Square A8 = 0 and square H1 = 63.
//-------------------------------------------------------------------------------------------------
extern inline U64 square_bb(int index)
{
    assert(index >= 0 && index < 64);
    return (U64)0x8000000000000000 >> index;
}

//-------------------------------------------------------------------------------------------------
//  Turn bit to 1 using index (0-63)
//-------------------------------------------------------------------------------------------------
extern inline void bb_set_bit(U64 *bb, int index)
{
    assert(index >= 0 && index < 64);
    assert(bb != NULL);
    *bb |= (U64)0x8000000000000000 >> index;
}

//-------------------------------------------------------------------------------------------------
//  Turn bit to 1 using rank and file
//-------------------------------------------------------------------------------------------------
void bb_set_bit_rf(U64 *bb, int rank, int file)
{
    assert(rank >= 0 && rank < 8);
    assert(file >= 0 && file < 8);
    assert(bb != NULL);
    bb_set_bit(bb, rank * 8 + file);
}

//-------------------------------------------------------------------------------------------------
//  Return TRUE if bit is 1
//-------------------------------------------------------------------------------------------------
extern inline int bb_is_one(U64 bb, int index)
{
    assert(index >= 0 && index < 64);
    return bb & square_bb(index) ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Turn bit to 0
//-------------------------------------------------------------------------------------------------
extern inline void bb_clear_bit(U64 *bb, int index)
{
    assert(index >= 0 && index < 64);
    assert(bb != NULL);
    *bb &= ~((U64)0x8000000000000000 >> index);
}

//-------------------------------------------------------------------------------------------------
//  Return the color bitboard for square
//-------------------------------------------------------------------------------------------------
extern inline U64 square_color_bb(int index)
{
    assert(index >= 0 && index < 64);
    return square_bb(index) & BB_LIGHT_SQ ? BB_LIGHT_SQ : BB_DARK_SQ;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit of a bitboard: 0 to 63, and clear the bit.
//-------------------------------------------------------------------------------------------------
extern inline int bb_pop_first_index(U64 *bb)
{
    assert(bb != NULL && *bb != 0);
    int index = bb_first_index(*bb);
    *bb &= ~((U64)0x8000000000000000 >> index);
    return index;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit of a bitboard: 0 to 63, and clear the bit.
//-------------------------------------------------------------------------------------------------
extern inline int bb_pop_last_index(U64 *bb)
{
    assert(bb != NULL && *bb != 0);
    int index = bb_last_index(*bb);
    *bb &= ~((U64)0x8000000000000000 >> index);
    return index;
}

//End

