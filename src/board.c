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
//  Board functionality, make/undo move, state information.
//  Squares are represented top down: A8 = 0 ... H1 = 63
//  Ranks go from 0 to 7: A8-H8 = rank 0 ... A1-H1 = rank 7
//  Files go from 0 to 7; A1-A8 = file 0 ... H1-H8 = file 7
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Setup a new game based on a FEN position.
//-------------------------------------------------------------------------------------------------
void set_fen(BOARD *board, char *fen) 
{
    int i, r, f, inc;

    memset(board, 0, sizeof(BOARD));
	for (i = 0; i < 64; i++) {
		board->state[WHITE].square[i] = NO_PIECE;
		board->state[BLACK].square[i] = NO_PIECE;
	}

    // Pieces
    i = r = f = inc = 0;
    while (fen[i] && fen[i] != ' ')  {
        if (fen[i] == '/')  {
            if (f > 0)  {
                f = 0;
                r++;
            }
            i++;
            continue;
        }
        inc = 1;
        switch (fen[i])  {
            case 'r': set_piece(board, BLACK, ROOK, r * 8 + f);   break;
            case 'n': set_piece(board, BLACK, KNIGHT, r * 8 + f); break;
            case 'b': set_piece(board, BLACK, BISHOP, r * 8 + f); break;
            case 'q': set_piece(board, BLACK, QUEEN, r * 8 + f);  break;
            case 'k': set_piece(board, BLACK, KING, r * 8 + f);   break;
            case 'p': set_piece(board, BLACK, PAWN, r * 8 + f);   break;
            case 'R': set_piece(board, WHITE, ROOK, r * 8 + f);   break;
            case 'N': set_piece(board, WHITE, KNIGHT, r * 8 + f); break;
            case 'B': set_piece(board, WHITE, BISHOP, r * 8 + f); break;
            case 'Q': set_piece(board, WHITE, QUEEN, r * 8 + f);  break;
            case 'K': set_piece(board, WHITE, KING, r * 8 + f);   break;
            case 'P': set_piece(board, WHITE, PAWN, r * 8 + f);   break;
            default: 
                if (fen[i] >= '1' && fen[i] <= '8') inc = fen[i] - '0'; 
                break;
        }
        f += inc;
        if (f > 7)  {
            r++;
            f = f - 8;
        }
        i++;
    }

    // White or black to play
    while (fen[i] && fen[++i] == ' ');
    if (fen[i] == 'w') 
        board->side_on_move = WHITE;
    if (fen[i] == 'b')  {
        board->side_on_move = BLACK;
        board->key ^= zk_color();
        board->pawn_key ^= zk_color();
    }

    // Castle status
    while (fen[i] && fen[++i] == ' ');
    while (fen[i] && fen[i] != ' ')  {
        switch (fen[i])  {
            case 'K': board->state[WHITE].can_castle_ks = 1; break;
            case 'Q': board->state[WHITE].can_castle_qs = 1; break;
            case 'k': board->state[BLACK].can_castle_ks = 1; break;
            case 'q': board->state[BLACK].can_castle_qs = 1; break;
        }
        i++;
    }
    board->key ^= zk_ks(WHITE, board->state[WHITE].can_castle_ks);
    board->key ^= zk_qs(WHITE, board->state[WHITE].can_castle_qs);
    board->key ^= zk_ks(BLACK, board->state[BLACK].can_castle_ks);
    board->key ^= zk_qs(BLACK, board->state[BLACK].can_castle_qs);

    // En-passant square
    board->ep_square = 0;
    if (fen[i] && fen[++i] == '-')
        i++;
    else  {
        if (fen[i] >= 'a' && fen[i] <= 'h' && fen[i+1] >= '1' && fen[i+1] <= '8')
            board->ep_square = (('8' - fen[i+1]) * 8) + (fen[i] - 'a');
        i += 2;
    }
    if (board->ep_square != 0)
        board->key ^= zk_ep(board->ep_square);

    // 50 move count
    while (fen[i] && fen[i] == ' ')
        i++;

    board->fifty_move_rule = 0;
    while (fen[i] && fen[i] != ' ')  {
        if (fen[i] >= '0' && fen[i] <= '9')
            board->fifty_move_rule = board->fifty_move_rule * 10 + (fen[i] - '0');
        i++;
    }

    if (board->fifty_move_rule <= 0 || board->fifty_move_rule > 100)
        board->fifty_move_rule = 0;

    // Setup additional state data.
    board->ply = 0;
    board->histply = 0;
    for (i = 0; i < NUM_PIECES; i++)  {
        board->state[WHITE].all_pieces |= board->state[WHITE].piece[i].u64;
        board->state[BLACK].all_pieces |= board->state[BLACK].piece[i].u64;
    }
    board->state[WHITE].king_square = (U8)bb_first(board->state[WHITE].piece[KING]);
    board->state[BLACK].king_square = (U8)bb_first(board->state[BLACK].piece[KING]);

    assert(board_state_is_ok(board));
}

//-------------------------------------------------------------------------------------------------
//  Side on move
//-------------------------------------------------------------------------------------------------
U8 side_on_move(BOARD *board)
{
    return board->side_on_move;
}

//-------------------------------------------------------------------------------------------------
//  Get last move made on the board
//-------------------------------------------------------------------------------------------------
MOVE get_last_move_made(BOARD *board)
{
    if (board->histply == 0) return MOVE_NONE;
    return board->history[board->histply - 1].move;
}

//-------------------------------------------------------------------------------------------------
//  Make a move, update state and save history data.
//-------------------------------------------------------------------------------------------------
void make_move(BOARD *board, MOVE move)
{
    int type = unpack_type(move);
    int frsq = unpack_from(move);
    int tosq = unpack_to(move);
    int mvpc = unpack_piece(move);

	assert(board->histply >= 0 && board->histply < MAX_HIST);

    // Save history data to be used during move undo.
    board->history[board->histply].move             = move;
    board->history[board->histply].can_castle_ks    = board->state[side_on_move(board)].can_castle_ks;
    board->history[board->histply].can_castle_qs    = board->state[side_on_move(board)].can_castle_qs;
    board->history[board->histply].ep_square        = board->ep_square;
    board->history[board->histply].board_key        = board->key;
    board->history[board->histply].pawn_key         = board->pawn_key;
    board->history[board->histply].fifty_move_rule  = board->fifty_move_rule;

    // Key
    board->key ^= zk_color();
    board->pawn_key ^= zk_color();
    board->key ^= zk_ks(side_on_move(board), board->state[side_on_move(board)].can_castle_ks);
    board->key ^= zk_qs(side_on_move(board), board->state[side_on_move(board)].can_castle_qs);

    // Clear ep square. The 2sq pawn move sets it.
    if (board->ep_square != 0)  {
        board->key ^= zk_ep(board->ep_square);
        board->ep_square = 0;
    }

    // Make move according its type
    switch (type)  {
        case MT_QUIET:
            move_piece(board, side_on_move(board), mvpc, frsq, tosq);
            break;
        case MT_CAPPC:
            remove_piece(board, flip_color(side_on_move(board)), unpack_capture(move), tosq);
            move_piece(board, side_on_move(board), mvpc, frsq, tosq);
            break;
        case MT_EPCAP:
            remove_piece(board, flip_color(side_on_move(board)), PAWN, unpack_ep_pawn_square(move));
            move_piece(board, side_on_move(board), PAWN, frsq, tosq);
            break;
        case MT_PAWN2:
            move_piece(board, side_on_move(board), PAWN, frsq, tosq);
            board->ep_square = (U8)unpack_ep_square(move);
            board->key ^= zk_ep(board->ep_square);
            break;
        case MT_PROMO:
            remove_piece(board, side_on_move(board), PAWN, frsq);
            set_piece(board, side_on_move(board), unpack_prom_piece(move), tosq);
            break;
        case MT_CPPRM:
            remove_piece(board, side_on_move(board), PAWN, frsq);
            remove_piece(board, flip_color(side_on_move(board)), unpack_capture(move), tosq);
            set_piece(board, side_on_move(board), unpack_prom_piece(move), tosq);
            break;
        case MT_CSWKS:
            move_piece(board, WHITE, KING, E1, G1);
            move_piece(board, WHITE, ROOK, H1, F1);
            break;
        case MT_CSWQS:
            move_piece(board, WHITE, KING, E1, C1);
            move_piece(board, WHITE, ROOK, A1, D1);
            break;
        case MT_CSBKS:
            move_piece(board, BLACK, KING, E8, G8);
            move_piece(board, BLACK, ROOK, H8, F8);
            break;
        case MT_CSBQS:
            move_piece(board, BLACK, KING, E8, C8);
            move_piece(board, BLACK, ROOK, A8, D8);
            break;
        case MT_NULL:
            break;
    }

    // Castle status and king square update.
    if (mvpc == KING)  {
        board->state[side_on_move(board)].can_castle_ks = 0;
        board->state[side_on_move(board)].can_castle_qs = 0;
        board->state[side_on_move(board)].king_square = (U8)tosq;
    }
    else {
        if (mvpc == ROOK)  {
            if ((side_on_move(board) == WHITE && frsq == A1) || (side_on_move(board) == BLACK && frsq == A8))
                board->state[side_on_move(board)].can_castle_qs = 0;
            if ((side_on_move(board) == WHITE && frsq == H1) || (side_on_move(board) == BLACK && frsq == H8))
                board->state[side_on_move(board)].can_castle_ks = 0;
        }
    }

    // fifty move rule
    if (type == MT_CAPPC || mvpc == PAWN)
        board->fifty_move_rule = 0;
    else
        board->fifty_move_rule++;

    board->ply++;
    board->histply++;

    board->key ^= zk_ks(board->side_on_move, board->state[board->side_on_move].can_castle_ks);
    board->key ^= zk_qs(board->side_on_move, board->state[board->side_on_move].can_castle_qs);

    board->side_on_move = flip_color(board->side_on_move);

    //assert(zk_board_key(board) == board->key);
    assert(board_state_is_ok(board));
    assert(board->ply <= MAX_PLY);
    assert(board->histply <= MAX_HIST);
}

//-------------------------------------------------------------------------------------------------
//  Undo last move based and restore state based on history.
//-------------------------------------------------------------------------------------------------
void undo_move(BOARD *board)
{
    if (board->histply == 0) return; // no move to undo

    board->ply--;
    board->histply--;

    // Get last move made
    MOVE move = board->history[board->histply].move;

    // Restore side to move 
    int opp = board->side_on_move;
    board->side_on_move = flip_color(board->side_on_move);
    int turn = board->side_on_move;

    // Restore board state.
    board->state[turn].can_castle_ks = board->history[board->histply].can_castle_ks;
    board->state[turn].can_castle_qs = board->history[board->histply].can_castle_qs;
    board->ep_square                 = board->history[board->histply].ep_square;
    board->key                       = board->history[board->histply].board_key;
    board->pawn_key                  = board->history[board->histply].pawn_key;
    board->fifty_move_rule           = board->history[board->histply].fifty_move_rule;

    //  basic move information
    int mvpc = unpack_piece(move);
    int frsq = unpack_from(move);
    int tosq = unpack_to(move);

    if (mvpc == KING) board->state[side_on_move(board)].king_square = (U8)frsq;

    switch (unpack_type(move)) {
        case MT_QUIET:
            //move_piece_undo(board, side_on_move(board), board->state[side_on_move(board)].square[tosq], tosq, frsq);
            move_piece_undo(board, turn, mvpc, tosq, frsq);
            break;
        case MT_CAPPC:
            //move_piece_undo(board, side_on_move(board), board->state[side_on_move(board)].square[tosq], tosq, frsq);
            move_piece_undo(board, turn, mvpc, tosq, frsq);
            set_piece_undo(board, opp, unpack_capture(move), tosq);
            break;
        case MT_EPCAP:
            move_piece_undo(board, turn, PAWN, tosq, frsq);
            set_piece_undo(board, opp, PAWN, unpack_ep_pawn_square(move));
            break;
        case MT_PAWN2:
            move_piece_undo(board, turn, PAWN, tosq, frsq);
            break;
        case MT_PROMO:
            remove_piece_undo(board, turn, unpack_prom_piece(move), tosq);
            set_piece_undo(board, turn, PAWN, frsq);
            break;
        case MT_CPPRM:
            remove_piece_undo(board, turn, unpack_prom_piece(move), tosq);
            set_piece_undo(board, turn, PAWN, frsq);
            set_piece_undo(board, opp, unpack_capture(move), tosq);
            break;
        case MT_CSWKS:
            move_piece_undo(board, WHITE, KING, G1, E1);
            move_piece_undo(board, WHITE, ROOK, F1, H1);
            break;
        case MT_CSWQS:
            move_piece_undo(board, WHITE, KING, C1, E1);
            move_piece_undo(board, WHITE, ROOK, D1, A1);
            break;
        case MT_CSBKS:
            move_piece_undo(board, BLACK, KING, G8, E8);
            move_piece_undo(board, BLACK, ROOK, F8, H8);
            break;
        case MT_CSBQS:
            move_piece_undo(board, BLACK, KING, C8, E8);
            move_piece_undo(board, BLACK, ROOK, D8, A8);
            break;
        case MT_NULL:
            break;
    }

    assert(board_state_is_ok(board));
}

//-------------------------------------------------------------------------------------------------
//  Return the full material value for side of color.
//-------------------------------------------------------------------------------------------------
int material_value(BOARD *board, int color)
{
    return (board->state[color].count[QUEEN]  * VALUE_QUEEN) +
           (board->state[color].count[ROOK]   * VALUE_ROOK) +
           (board->state[color].count[KNIGHT] * VALUE_KNIGHT) +
           (board->state[color].count[BISHOP] * VALUE_BISHOP) +
           (board->state[color].count[PAWN]   * VALUE_PAWN);
}

//-------------------------------------------------------------------------------------------------
//  Return the number of pieces for side of color.
//-------------------------------------------------------------------------------------------------
int pieces_count(BOARD *board, int color)
{
    return board->state[color].count[QUEEN]  +
           board->state[color].count[ROOK]   +
           board->state[color].count[KNIGHT] +
           board->state[color].count[BISHOP];
}

//-------------------------------------------------------------------------------------------------
//  Check if current board position is draw.
//-------------------------------------------------------------------------------------------------
int is_draw(BOARD *board)
{
    if (is_threefold_repetition(board)) return TRUE;
    if (reached_fifty_move_rule(board)) return TRUE;
    if (insufficient_material(board)) return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Check if current position already happened before.
//-------------------------------------------------------------------------------------------------
int is_threefold_repetition(BOARD *board)
{
    int repetitions = 0;

    for (int i = board->histply - board->fifty_move_rule; i < board->histply; i++)  {
        if (board->history[i].board_key == board->key) repetitions++;
    }

    // Uses repetition count as 2 instead of 3. Forces avoiding 3-fold at all.
    if (repetitions >= 1)
        return TRUE;
    else
        return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Check if there's enough pieces for mate. Bare kings.
//-------------------------------------------------------------------------------------------------
int insufficient_material(BOARD *board)
{
    if (board->state[WHITE].all_pieces != board->state[WHITE].piece[KING].u64)
        return FALSE;
    if (board->state[BLACK].all_pieces != board->state[BLACK].piece[KING].u64)
        return FALSE;
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Check if reached 50 move rule
//-------------------------------------------------------------------------------------------------
int reached_fifty_move_rule(BOARD *board)
{
    if (board->fifty_move_rule >= 100)
        return TRUE;
    else
        return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Move a piece and update color state.
//-------------------------------------------------------------------------------------------------
void move_piece(BOARD *board, int color, int type, int frsq, int tosq)
{
    U64     bb_from_to = square_bb(frsq) ^ square_bb(tosq);

    board->state[color].square[frsq] = NO_PIECE;
    board->state[color].piece[type].u64 ^= bb_from_to;
    board->state[color].all_pieces ^= bb_from_to;
    board->key ^= zk_square(color, type, frsq);
    board->state[color].square[tosq] = (U8)type; 
    board->key ^= zk_square(color, type, tosq);
    if (type == PAWN) {
        board->pawn_key ^= zk_square(color, PAWN, frsq);
        board->pawn_key ^= zk_square(color, PAWN, tosq);
    }
}

//-------------------------------------------------------------------------------------------------
//  Place a piece on square and update color state.
//-------------------------------------------------------------------------------------------------
void set_piece(BOARD *board, int color, int type, int tosq)
{
    U64     bb_set = square_bb(tosq);

    board->state[color].square[tosq] = (U8)type; 
    board->state[color].piece[type].u64 ^= bb_set;
    board->state[color].all_pieces ^= bb_set;
    board->key ^= zk_square(color, type, tosq);
    if (type == PAWN) board->pawn_key ^= zk_square(color, PAWN, tosq);
    board->state[color].count[type]++;
}

//-------------------------------------------------------------------------------------------------
//  Remove a piece from square and update color state.
//-------------------------------------------------------------------------------------------------
void remove_piece(BOARD *board, int color, int type, int frsq)
{
    U64     bb_remove = square_bb(frsq);

    board->state[color].square[frsq] = NO_PIECE;
    board->state[color].piece[type].u64 ^= bb_remove;
    board->state[color].all_pieces ^= bb_remove;
    board->key ^= zk_square(color, type, frsq);
    if (type == PAWN) board->pawn_key ^= zk_square(color, PAWN, frsq);
    board->state[color].count[type]--;
}

//-------------------------------------------------------------------------------------------------
//  Move a piece during undo method. Does not need to update some data.
//-------------------------------------------------------------------------------------------------
void move_piece_undo(BOARD *board, int color, int type, int frsq, int tosq)
{
    U64     bb_from_to = square_bb(frsq) ^ square_bb(tosq);

    board->state[color].square[frsq] = NO_PIECE;
    board->state[color].piece[type].u64 ^= bb_from_to;
    board->state[color].all_pieces ^= bb_from_to;
    board->state[color].square[tosq] = (U8)type; 
}

//-------------------------------------------------------------------------------------------------
//  Place a piece on square during undo method. Does not need to update some data.
//-------------------------------------------------------------------------------------------------
void set_piece_undo(BOARD *board, int color, int type, int index)
{
    U64     bb_set = square_bb(index);

    board->state[color].square[index] = (U8)type;
    board->state[color].piece[type].u64 ^= bb_set;
    board->state[color].all_pieces ^= bb_set;
    board->state[color].count[type]++;
}

//-------------------------------------------------------------------------------------------------
//  Remove a piece from square during undo method. Does not need to update some data.
//-------------------------------------------------------------------------------------------------
void remove_piece_undo(BOARD *board, int color, int type, int index)
{
    U64     bb_remove = square_bb(index);

    board->state[color].square[index] = NO_PIECE;
    board->state[color].piece[type].u64 ^= bb_remove;
    board->state[color].all_pieces ^= bb_remove;
    board->state[color].count[type]--;
}

//-------------------------------------------------------------------------------------------------
//  Calculate the distance between 2 squares
//-------------------------------------------------------------------------------------------------
int square_distance(int square1, int square2)
{
    int        file_distance = get_file(square1) - get_file(square2);
    int        rank_distance = get_rank(square1) - get_rank(square2);

    file_distance = ABS(file_distance);
    rank_distance = ABS(rank_distance);

    return MAX(file_distance, rank_distance);
}

//-------------------------------------------------------------------------------------------------
//  King BB
//-------------------------------------------------------------------------------------------------
U64 king_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[KING].u64;
}

//-------------------------------------------------------------------------------------------------
//  King count
//-------------------------------------------------------------------------------------------------
int king_count(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].count[KING];
}

//-------------------------------------------------------------------------------------------------
//  Queen BB
//-------------------------------------------------------------------------------------------------
U64 queen_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[QUEEN].u64;
}

//-------------------------------------------------------------------------------------------------
//  Queen count
//-------------------------------------------------------------------------------------------------
int queen_count(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].count[QUEEN];
}

//-------------------------------------------------------------------------------------------------
//  Rook BB
//-------------------------------------------------------------------------------------------------
U64 rook_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[ROOK].u64;
}

//-------------------------------------------------------------------------------------------------
//  Rook count
//-------------------------------------------------------------------------------------------------
int rook_count(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].count[ROOK];
}

//-------------------------------------------------------------------------------------------------
//  Bishop BB
//-------------------------------------------------------------------------------------------------
U64 bishop_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[BISHOP].u64;
}

//-------------------------------------------------------------------------------------------------
//  Bishop count
//-------------------------------------------------------------------------------------------------
int bishop_count(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].count[BISHOP];
}

//-------------------------------------------------------------------------------------------------
//  Knight BB
//-------------------------------------------------------------------------------------------------
U64 knight_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[KNIGHT].u64;
}

//-------------------------------------------------------------------------------------------------
//  Knight count
//-------------------------------------------------------------------------------------------------
int knight_count(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].count[KNIGHT];
}

//-------------------------------------------------------------------------------------------------
//  Pawn BB
//-------------------------------------------------------------------------------------------------
U64 pawn_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[PAWN].u64;
}

//-------------------------------------------------------------------------------------------------
//  Knight count
//-------------------------------------------------------------------------------------------------
int pawn_count(BOARD *board, int color)
{
    assert(valid_color(color));
    //return bb_count(board->state[color].piece[PAWN]);
    return board->state[color].count[PAWN];
}

//-------------------------------------------------------------------------------------------------
//  Occupied squares
//-------------------------------------------------------------------------------------------------
U64 occupied_bb(BOARD *board)
{
    return board->state[WHITE].all_pieces | board->state[BLACK].all_pieces;
}

//-------------------------------------------------------------------------------------------------
//  Empty squares
//-------------------------------------------------------------------------------------------------
U64 empty_bb(BOARD *board)
{
    return ~(board->state[WHITE].all_pieces | board->state[BLACK].all_pieces);
}

//-------------------------------------------------------------------------------------------------
//  Queen and Bishop pieces
//-------------------------------------------------------------------------------------------------
U64 queen_bishop_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[QUEEN].u64 | board->state[color].piece[BISHOP].u64;
}

//-------------------------------------------------------------------------------------------------
//  Queen and Rook pieces
//-------------------------------------------------------------------------------------------------
U64 queen_rook_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].piece[QUEEN].u64 | board->state[color].piece[ROOK].u64;
}

//-------------------------------------------------------------------------------------------------
//  King square
//-------------------------------------------------------------------------------------------------
int king_square(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].king_square;
}

//-------------------------------------------------------------------------------------------------
//  Board key
//-------------------------------------------------------------------------------------------------
U64 board_key(BOARD *board)
{
    return board->key;
}

//-------------------------------------------------------------------------------------------------
//  Board pawn key
//-------------------------------------------------------------------------------------------------
U64 board_pawn_key(BOARD *board)
{
    return board->pawn_key;
}

//-------------------------------------------------------------------------------------------------
//  Piece on square
//-------------------------------------------------------------------------------------------------
int piece_on_square(BOARD *board, int color, int square)
{
    assert(color == WHITE || color == BLACK);
    assert(square >= 0 && square < 64);

    //for (int type = 0; type < NUM_PIECES; type++)
    //    if (board->state[color].piece[type].u64 & square_bb(square))
    //        return type;

    //return NO_PIECE;
    return board->state[color].square[square];
}

//-------------------------------------------------------------------------------------------------
//  All Pieces
//-------------------------------------------------------------------------------------------------
U64 all_pieces_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return board->state[color].all_pieces;
}

//-------------------------------------------------------------------------------------------------
//  QRNB Pieces
//-------------------------------------------------------------------------------------------------
U64 qrnb_bb(BOARD *board, int color)
{
    assert(valid_color(color));
    return queen_bb(board, color) | rook_bb(board, color) | knight_bb(board, color) | bishop_bb(board, color);
}

//-------------------------------------------------------------------------------------------------
//  Piece bitboard
//-------------------------------------------------------------------------------------------------
U64 piece_bb(BOARD *board, int color, int piece)
{
    assert(valid_color(color));
    assert(piece >= PAWN && piece <= KING);
    return board->state[color].piece[piece].u64;
}

//-------------------------------------------------------------------------------------------------
//  Has pieces (rook, queen, bishop, knight)
//-------------------------------------------------------------------------------------------------
int has_pieces(BOARD *board, int color)
{
    assert(valid_color(color));
    if (board->state[color].piece[QUEEN].u64 || board->state[color].piece[ROOK].u64)
        return TRUE;
    if (board->state[color].piece[BISHOP].u64 || board->state[color].piece[KNIGHT].u64)
        return TRUE;
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Ep square bitboard
//-------------------------------------------------------------------------------------------------
U64 ep_square_bb(BOARD *board)
{
    return board->ep_square != 0 ? square_bb(board->ep_square) : 0;
}

//-------------------------------------------------------------------------------------------------
//  Ep square
//-------------------------------------------------------------------------------------------------
int ep_square(BOARD *board)
{
    return board->ep_square;
}

//-------------------------------------------------------------------------------------------------
//  Return flag for king side castle move
//-------------------------------------------------------------------------------------------------
int can_castle_ks_flag(BOARD *board, int color)
{
    return board->state[color].can_castle_ks;
}

//-------------------------------------------------------------------------------------------------
//  Return flag for queen side castle move
//-------------------------------------------------------------------------------------------------
int can_castle_qs_flag(BOARD *board, int color)
{
    return board->state[color].can_castle_qs;
}

//-------------------------------------------------------------------------------------------------
//  Can generate king side castle move
//-------------------------------------------------------------------------------------------------
int can_generate_castle_ks(BOARD *board, int color)
{
    static const int ROOK_SQUARE[2] = {H1, H8};
    static const U64 KR_SQUARES[2] = {(U64)0x0000000000000006, (U64)0x0600000000000000};

    assert(valid_color(color));

    if (board->state[color].can_castle_ks && bb_is_one(board->state[color].piece[ROOK].u64, ROOK_SQUARE[color]))
        if (!(KR_SQUARES[color] & occupied_bb(board)))
            return TRUE;

    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Can generate queen side castle move
//-------------------------------------------------------------------------------------------------
int can_generate_castle_qs(BOARD *board, int color)
{
    static const int ROOK_SQUARE[2] = {A1, A8};
    static const U64 KR_SQUARES[2] = {(U64)0x0000000000000070, (U64)0x7000000000000000};

    assert(valid_color(color));

    if (board->state[color].can_castle_qs && bb_is_one(board->state[color].piece[ROOK].u64, ROOK_SQUARE[color]))
        if (!(KR_SQUARES[color] & occupied_bb(board)))
            return TRUE;

    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  board data
//-------------------------------------------------------------------------------------------------
void set_ply(BOARD *board, U16 value)
{
    board->ply = value;
}

U16 get_ply(BOARD *board)
{
    return board->ply;
}

U16 get_history_ply(BOARD *board)
{
    return board->histply;
}

//-------------------------------------------------------------------------------------------------
//  Get the number of moves played by color
//-------------------------------------------------------------------------------------------------
int get_played_moves_count(BOARD *board, int color)
{
    assert(board != NULL);
    assert(color == WHITE || color == BLACK);

    int played = board->histply / 2;
    if (color == WHITE && board->histply % 2 == 1) played++;
    assert(played >= 0 && played < MAX_HIST);
    return played;
}

//-------------------------------------------------------------------------------------------------
//  Get the current game list of moves
//-------------------------------------------------------------------------------------------------
int get_played_moves(BOARD *board, char *line, size_t max_chars)
{
    U32     i;
    char    move_string[10];

    line[0] = 0;
    for (i = 0; i < board->histply; i++) {
        if (i > 0)
            strcat(line, " ");
        util_get_move_string(board->history[i].move, move_string);
        if (strlen(line) + strlen(move_string) > max_chars)
            return FALSE;
        strcat(line, move_string);
    }

    return TRUE;
}

//-------------------------------------------------------------------------------------------------
//  Get the history moves
//-------------------------------------------------------------------------------------------------
int get_history_moves(BOARD *board, MOVE move[], int max_moves)
{
    int        i;

    for (i = 0; i < board->histply; i++) {
        if (i >= max_moves) return FALSE;
        move[i] = board->history[i].move;
    }

    return TRUE;
}

//end
