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
//    Move definition and pack/unpack routines
//-------------------------------------------------------------------------------------------------
// Move is a pack of bits with the following structure (from right to left):
// tosq: 6 bits - destination square. the "to square"
// frsq: 6 bits - originating square, the "from square"
// type: 4 bits - move type (quiet, capture, etc)
// mvpc: 3 bits - moving piece
// cppc: 3 bits - captured piece (Capture, Ep Capture, Capture/Promotion)
// prom: 3 bits - promotion piece (Promotion, Capture/Promotion)
// epsq or pwsq: 6 bits - ep square or pawn square (ep square when is pawn 2 square move, pawn square when is EP Capture)
// quiet: 1 bit - flag to indicate quiet moves.

#define QUIET_BIT    0x80000000

//-------------------------------------------------------------------------------------------------
//    Quiet move.
//-------------------------------------------------------------------------------------------------
MOVE pack_quiet(int moving_piece, int from_square, int to_square)
{
    MOVE    move;

    assert(moving_piece >= PAWN && moving_piece <= KING);
    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);

    move = 0;

    move |= QUIET_BIT;
    move |= moving_piece << 16;
    move |= MT_QUIET << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Pawn 2 square move.
//-------------------------------------------------------------------------------------------------
MOVE pack_pawn_2square(int from_square, int to_square, int ep_square)
{
    MOVE    move;

    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    assert(ep_square >= 0 && ep_square < 64);
    assert(get_file(from_square) == get_file(to_square));
    assert(get_file(from_square) == get_file(ep_square));
    
    move = 0;

    move |= QUIET_BIT;
    move |= ep_square << 25;
    move |= PAWN << 16;
    move |= MT_PAWN2 << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Castle moves.
//-------------------------------------------------------------------------------------------------
MOVE pack_castle(int from_square, int to_square, int castle)
{
    MOVE    move;

    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    assert(castle == MT_CSWKS || castle == MT_CSWQS || castle == MT_CSBKS || castle == MT_CSBQS);
    assert(castle == MT_CSWKS || castle == MT_CSWQS || castle == MT_CSBKS || castle == MT_CSBQS);

    move = 0;

    move |= QUIET_BIT;
    move |= KING << 16;
    move |= castle << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Capture move.
//-------------------------------------------------------------------------------------------------
MOVE pack_capture(int moving_piece, int captured_piece, int from_square, int to_square)
{
    MOVE    move;
    
    assert(moving_piece >= PAWN && moving_piece <= KING);
    assert(captured_piece >= PAWN && captured_piece <= QUEEN);
    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);

    move = 0;

    move |= captured_piece << 19;
    move |= moving_piece << 16;
    move |= MT_CAPPC << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}


//-------------------------------------------------------------------------------------------------
//    En passant capture move.
//-------------------------------------------------------------------------------------------------
MOVE pack_en_passant_capture(int from_square, int to_square, int pawn_square)
{
    MOVE    move;

    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    assert(pawn_square >= 0 && pawn_square < 64);

    move = 0;

    move |= pawn_square << 25;
    move |= PAWN << 19;
    move |= PAWN << 16;
    move |= MT_EPCAP << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Promotion move.
//-------------------------------------------------------------------------------------------------
MOVE pack_promotion(int from_square, int to_square, int prom_piece)
{
    MOVE    move;

    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    assert(prom_piece == QUEEN || prom_piece == ROOK || prom_piece == BISHOP || prom_piece == KNIGHT);

    move = 0;

    move |= prom_piece << 22;
    move |= PAWN << 16;
    move |= MT_PROMO << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Capture and promotion move.
//-------------------------------------------------------------------------------------------------
MOVE pack_capture_promotion(int captured_piece, int from_square, int to_square, int prom_piece)
{
    MOVE    move;

    assert(captured_piece >= PAWN && captured_piece <= QUEEN);
    assert(from_square >= 0 && from_square < 64);
    assert(to_square >= 0 && to_square < 64);
    assert(prom_piece == QUEEN || prom_piece == ROOK || prom_piece == BISHOP || prom_piece == KNIGHT);

    move = 0;

    move |= prom_piece << 22;
    move |= captured_piece << 19;
    move |= PAWN << 16;
    move |= MT_CPPRM << 12;
    move |= from_square << 6;
    move |= to_square;

    return move;
}

//-------------------------------------------------------------------------------------------------
//    Special null move
//-------------------------------------------------------------------------------------------------
MOVE pack_null_move(void)
{
    return (MOVE)(MT_NULL << 12);
}

//-------------------------------------------------------------------------------------------------
//    Return the move "from square"
//-------------------------------------------------------------------------------------------------
int unpack_from(MOVE move)
{
    return (move >> 6) & 0x3F;
}

//-------------------------------------------------------------------------------------------------
//    Return the move "to square"
//-------------------------------------------------------------------------------------------------
int unpack_to(MOVE move)
{
    return move & 0x3F;
}

//-------------------------------------------------------------------------------------------------
//    Return the move type
//-------------------------------------------------------------------------------------------------
int unpack_type(MOVE move)
{
    return (move >> 12) & 0x0F;
}

//-------------------------------------------------------------------------------------------------
//    Return the moving piece
//-------------------------------------------------------------------------------------------------
int unpack_piece(MOVE move)
{
    return (move >> 16) & 0x07;
}

//-------------------------------------------------------------------------------------------------
//    Return capture piece
//-------------------------------------------------------------------------------------------------
int unpack_capture(MOVE move)
{
    return (move >> 19) & 0x07;
}

//-------------------------------------------------------------------------------------------------
//    Return promotion piece
//-------------------------------------------------------------------------------------------------
int unpack_prom_piece(MOVE move)
{
    return (move >> 22) & 0x07;
}

//-------------------------------------------------------------------------------------------------
//    Return en passant square
//-------------------------------------------------------------------------------------------------
int unpack_ep_square(MOVE move)
{
    return (move >> 25) & 0x3F;
}

//-------------------------------------------------------------------------------------------------
//    Return en passant pawn square
//-------------------------------------------------------------------------------------------------
int unpack_ep_pawn_square(MOVE move)
{
    return (move >> 25) & 0x3F;
}

//-------------------------------------------------------------------------------------------------
//    Indicates if it is quiet move.
//-------------------------------------------------------------------------------------------------
int move_is_quiet(MOVE move)
{
    return move & QUIET_BIT;
}

//-------------------------------------------------------------------------------------------------
//    Indicates if it is castle move.
//-------------------------------------------------------------------------------------------------
int move_is_castle(MOVE move)
{
    int        type = unpack_type(move);

    return type == MT_CSWKS || type == MT_CSWQS || type == MT_CSBKS || type == MT_CSBQS;
}

//-------------------------------------------------------------------------------------------------
//    Indicates a promotion move (pawn move or pawn capture)
//-------------------------------------------------------------------------------------------------
int move_is_promotion(MOVE move)
{
    int        type = unpack_type(move);

    return type == MT_PROMO || type == MT_CPPRM;
}

//-------------------------------------------------------------------------------------------------
//    Indicates en passant capture move
//-------------------------------------------------------------------------------------------------
int move_is_en_passant(MOVE move)
{
    int        type = unpack_type(move);

    return type == MT_EPCAP;
}

//-------------------------------------------------------------------------------------------------
//    Indicates capture moves
//-------------------------------------------------------------------------------------------------
int move_is_capture(MOVE move)
{
    int        type = unpack_type(move);

    return type == MT_CAPPC || type == MT_CPPRM || type == MT_EPCAP;
}

//END
