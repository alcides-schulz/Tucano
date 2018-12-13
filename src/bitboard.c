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
//  Bitboard functions.
//
//  Bitboard is used from left to right. Leftmost bit is 0, and rightmost bit is 63.
//  We use a divide and conquer method to locate the index of a bit, using a union data BBIX
//  
//-------------------------------------------------------------------------------------------------

//  Note: For portability reasons, it only uses the BBIX structure when int type 
//        is 4 bytes and short type is 2 bytes. Otherwise we use a different approach.
//        UINT_MAX and USHRT_MAX are defined in limits.h.
#if (UINT_MAX == 0xffffffff && USHRT_MAX == 0xffff)
#define USE_BBIX_STRUCT
#else
#undef USE_BBIX_STRUCT
#endif

// Bitboards
U64     bb_square[64];           // bitboard for each board square
U64     bb_clear[64];            // bitboard used to clear bits

// Index tables for bit location and count within a bitboard.
S8      first_index_table[4][65536];
S8      last_index_table[4][65536];
U8      bit_count_table[65536];

// Local functions
void    init_square_bb(void);
void    init_first_index_table(void);
void    init_last_index_table(void);
void    init_count_table(void);

U8      get_bit_count(U64 bb);
S8      get_first_index(U16 value);
S8      get_last_index(U16 value);

//-------------------------------------------------------------------------------------------------
//  Initializations
//-------------------------------------------------------------------------------------------------
void bb_init(void)
{
    init_square_bb();
    init_first_index_table();
    init_last_index_table();
    init_count_table();
}

//-------------------------------------------------------------------------------------------------
//  Initialize bitboard square. One for each square.
//  Square A8 = 0 and square H1 = 63.
//-------------------------------------------------------------------------------------------------
void init_square_bb(void)
{
    int        index;

    for (index = 0; index < 64; index++) {
        bb_square[index] = (U64)1 << (63 - index);
        bb_clear[index] = ~bb_square[index];
    }
}

//-------------------------------------------------------------------------------------------------
//  Table used to give first index on a bitboard
//-------------------------------------------------------------------------------------------------
void init_first_index_table(void)
{
    U32        i;

    first_index_table[0][0] = -1;
    first_index_table[1][0] = -1;
    first_index_table[2][0] = -1;
    first_index_table[3][0] = -1;

    for (i = 1; i < 65536; i++) {
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
    U32        i;

    last_index_table[0][0]  = -1;
    last_index_table[1][0]  = -1;
    last_index_table[2][0]  = -1;
    last_index_table[3][0]  = -1;

    for (i = 1; i < 65536; i++) {
        last_index_table[0][i]  = get_last_index((U16)i);
        last_index_table[1][i]  = get_last_index((U16)i) + 16;
        last_index_table[2][i]  = get_last_index((U16)i) + 32;
        last_index_table[3][i]  = get_last_index((U16)i) + 48;
    }
}


//-------------------------------------------------------------------------------------------------
//  Table used to calculate numbers of bits for a bitboard.
//-------------------------------------------------------------------------------------------------
void init_count_table(void)
{
    U64        j;

    for (j = 0; j < 65536; j++) {
        bit_count_table[j] = get_bit_count(j);
    }
}

//-------------------------------------------------------------------------------------------------
//  Return a bitboard for square index
//  Square A8 = 0 and square H1 = 63.
//-------------------------------------------------------------------------------------------------
U64 square_bb(int index)
{
    assert(index >= 0 && index < 64);
    return bb_square[index];
}

//-------------------------------------------------------------------------------------------------
//  Turn bit to 1 using index (0-63)
//-------------------------------------------------------------------------------------------------
void bb_set_bit(U64 *bb, int index)
{
    assert(index >= 0 && index < 64);
    assert(bb != NULL);
    *bb |= square_bb(index);
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
int bb_is_one(U64 bb, int index)
{
    assert(index >= 0 && index < 64);
    return bb & square_bb(index) ? TRUE : FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Turn bit to 0
//-------------------------------------------------------------------------------------------------
void bb_clear_bit(U64 *bb, int index)
{
    assert(index >= 0 && index < 64);
    assert(bb != NULL);
    *bb &= bb_clear[index];
}

//-------------------------------------------------------------------------------------------------
//  Return the color bitboard for square
//-------------------------------------------------------------------------------------------------
U64 square_color_bb(int index)
{
    assert(index >= 0 && index < 64);
    return square_bb(index) & BB_LIGHT_SQ ? BB_LIGHT_SQ : BB_DARK_SQ;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the first "1" bit of BBIX: 0 to 63, or -1 when bitboard is 0
//-------------------------------------------------------------------------------------------------
int bb_first(BBIX bbix)
{
#if defined(USE_BBIX_STRUCT)
    if (bbix.u32[1]) {
        if (bbix.u16[3])
            return (int)first_index_table[0][bbix.u16[3]];
        else
            return (int)first_index_table[1][bbix.u16[2]];
    }
    else {
        if (bbix.u16[1])
            return (int)first_index_table[2][bbix.u16[1]];
        else
            return (int)first_index_table[3][bbix.u16[0]];
    }
#else
    if (bbix.u64 & (U64)0xFFFF000000000000)
        return first_index_table[0][(bbix.u64 & (U64)0xFFFF000000000000) >> 48];
    if (bbix.u64 & (U64)0x0000FFFF00000000)
        return first_index_table[1][(bbix.u64 & (U64)0x0000FFFF00000000) >> 32];
    if (bbix.u64 & (U64)0x00000000FFFF0000)
        return first_index_table[2][(bbix.u64 & (U64)0x00000000FFFF0000) >> 16];
    return first_index_table[3][(bbix.u64 & (U64)0x000000000000FFFF)];
#endif
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit of BBIX: 0 to 63, or -1 when bitboard is 0
//-------------------------------------------------------------------------------------------------
int bb_last(BBIX bbix)
{
#ifdef USE_BBIX_STRUCT
    if (bbix.u32[0]) {
        if (bbix.u16[0])
            return (int)last_index_table[3][bbix.u16[0]];
        else
            return (int)last_index_table[2][bbix.u16[1]];
    }
    else {
        if (bbix.u16[2])
            return (int)last_index_table[1][bbix.u16[2]];
        else
            return (int)last_index_table[0][bbix.u16[3]];
    }
#else
    if (bbix.u64 & (U64)0x000000000000FFFF)
        return last_index_table[3][bbix.u64 & (U64)0x000000000000FFFF];
    if (bbix.u64 & (U64)0x00000000FFFF0000)
        return last_index_table[2][(bbix.u64 & (U64)0x00000000FFFF0000) >> 16];
    if (bbix.u64 & (U64)0x0000FFFF00000000)
        return last_index_table[1][(bbix.u64 & (U64)0x0000FFFF00000000) >> 32];
    return last_index_table[0][(bbix.u64 & (U64)0xFFFF000000000000) >> 48];
#endif
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the BBIX
//-------------------------------------------------------------------------------------------------
int bb_count(BBIX bbix)
{
#ifdef USE_BBIX_STRUCT
    return (int)(bit_count_table[bbix.u16[0]] + bit_count_table[bbix.u16[1]] +
                 bit_count_table[bbix.u16[2]] + bit_count_table[bbix.u16[3]]);
#else
    return (int)(bit_count_table[(bbix.u64 & (U64)0xFFFF000000000000) >> 48] + 
                 bit_count_table[(bbix.u64 & (U64)0x0000FFFF00000000) >> 32] +
                 bit_count_table[(bbix.u64 & (U64)0x00000000FFFF0000) >> 16] + 
                 bit_count_table[(bbix.u64 & (U64)0x000000000000FFFF)]);
#endif
}

//-------------------------------------------------------------------------------------------------
//  Number of "1" bits in the bitboard
//-------------------------------------------------------------------------------------------------
int bb_count_u64(U64 bb)
{
    return bit_count_table[(bb & (U64)0xFFFF000000000000) >> 48] +
           bit_count_table[(bb & (U64)0x0000FFFF00000000) >> 32] +
           bit_count_table[(bb & (U64)0x00000000FFFF0000) >> 16] +
           bit_count_table[(bb & (U64)0x000000000000FFFF)];
}

//-------------------------------------------------------------------------------------------------
//  Get first index of U64 bitboard
//-------------------------------------------------------------------------------------------------
int first_index(U64 bb)
{
    BBIX temp;
    temp.u64 = bb;
    return bb_first(temp);
}

//-------------------------------------------------------------------------------------------------
//  Get last index of U64 bitboard
//-------------------------------------------------------------------------------------------------
int last_index(U64 bb)
{
    BBIX temp;
    temp.u64 = bb;
    return bb_last(temp);
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
    for (S8 i = 0; i < 16; i++)
        if (value & (1 << (15 - i))) return i;
    return -1;
}

//-------------------------------------------------------------------------------------------------
//  Return index of the last "1" bit for a short integer, or -1 when 0.
//  Last bit is found from left to right.
//    Used for tables initialization only.
//-------------------------------------------------------------------------------------------------
S8 get_last_index(U16 value)
{
    for (S8 i = 0; i < 16; i++)
        if (value & (1 << i)) return (S8)(15 - i);
    return -1;
}

//End

