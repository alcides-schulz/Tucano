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
//  Transposition (hash) table functions.
//-------------------------------------------------------------------------------------------------

#define TT_BUCKETS  4

typedef struct strans_record
{
    U32     key;
    MOVE    best_move;
    S32     search_score;
    S16     age;
    S8      depth;
    S8      flag;
}   TT_REC;

typedef struct trans_entry 
{
    TT_REC  record[TT_BUCKETS];
}   TT_ENTRY;

TT_ENTRY    *trans_table = 0;
int         trans_size;
int         trans_entries;
S16         trans_age;

//-------------------------------------------------------------------------------------------------
//  Initialization.
//-------------------------------------------------------------------------------------------------
void tt_init(int size_mb)
{
    assert(sizeof(TT_REC) == 16);

    if (trans_table)
        free(trans_table);
    trans_size = 2;
    while (trans_size * 2 <= size_mb)
        trans_size *= 2;
    trans_size = trans_size * 1024 * 1024;
    trans_table = (TT_ENTRY *)malloc(trans_size);
    if (!trans_table)  {
        printf("no memory for transposition table!");
        exit(-1);
    }
    trans_entries = trans_size / sizeof(TT_ENTRY);
    
    assert(trans_entries * (int)sizeof(TT_ENTRY) == trans_size);

    tt_clear();
}

//-------------------------------------------------------------------------------------------------
//  Clear.
//-------------------------------------------------------------------------------------------------
void tt_clear(void)
{
    memset(trans_table, 0, trans_size);
    trans_age = 0;
}

//-------------------------------------------------------------------------------------------------
//  Update table age counter.
//-------------------------------------------------------------------------------------------------
void tt_age(void)
{
    trans_age++;
}

//-------------------------------------------------------------------------------------------------
//  Save information for current position.
//-------------------------------------------------------------------------------------------------
void tt_save(BOARD *board, int depth, int search_score, S8 flag, MOVE best_move)
{
    int     idx = board_key(board) % trans_entries;
    int     rec;
    TT_REC  *record1 = NULL;
    TT_REC  *record2 = NULL;
    int     depth_replace_age = MAX_DEPTH + 1;
    int     depth_replace = MAX_DEPTH + 1;

    assert(idx >= 0 && idx < trans_entries);
    assert(depth >= -1 && depth <= MAX_DEPTH);
    assert(flag == TT_LOWER || flag == TT_UPPER || flag == TT_EXACT);
    assert(search_score >= -MAX_SCORE && search_score <= MAX_SCORE);
    
    // Locate record to store information.
    for (rec = 0; rec < TT_BUCKETS; rec++)  {
        if (trans_table[idx].record[rec].key == LOW32(board_key(board))) {
            record1 = &trans_table[idx].record[rec];
            if (best_move == MOVE_NONE)
                best_move = record1->best_move;
            break;
        }
        if (trans_table[idx].record[rec].age != trans_age && 
            trans_table[idx].record[rec].depth < depth_replace_age) 
        {
            record1 = &trans_table[idx].record[rec];
            depth_replace_age = trans_table[idx].record[rec].depth;
        }
        if (trans_table[idx].record[rec].depth < depth_replace) {
            record2 = &trans_table[idx].record[rec];
            depth_replace = trans_table[idx].record[rec].depth;
        }
    }

    if (record1 == NULL) {
        record1 = record2;
        if (record1 == NULL)
            record1 = &trans_table[idx].record[0];
    }

    // Adjust mate score
    if (search_score > MAX_EVAL)
        search_score += get_ply(board);
    else
        if (search_score < -MAX_EVAL)
            search_score -= get_ply(board);

    // Store entry
    record1->key = LOW32(board_key(board));
    record1->depth = (S8)depth;
    record1->age = trans_age;
    record1->flag = flag;
    record1->search_score = search_score;
    record1->best_move = best_move;
}

//-------------------------------------------------------------------------------------------------
//  Probe current position.
//-------------------------------------------------------------------------------------------------
int tt_probe(BOARD *board, int depth, int alpha, int beta, int *search_score, MOVE *best_move)
{
    int     idx = board_key(board) % trans_entries;
    int     rec;
    int     tt_depth;
    S8      tt_flag;

    assert(depth >= -1 && depth <= MAX_DEPTH);
    assert(alpha < beta);
    assert(idx >= 0 && idx < trans_entries);

    *search_score = 0;
    *best_move = MOVE_NONE;
    
    for (rec = 0; rec < TT_BUCKETS; rec++) {
        if (trans_table[idx].record[rec].key != LOW32(board_key(board)))
            continue;

        tt_depth = trans_table[idx].record[rec].depth;
        tt_flag = trans_table[idx].record[rec].flag;

        assert(tt_depth >= -1 && tt_depth <= MAX_DEPTH);
        assert(tt_flag == TT_EXACT || tt_flag == TT_LOWER || tt_flag == TT_UPPER);

        *best_move = trans_table[idx].record[rec].best_move;
        *search_score = trans_table[idx].record[rec].search_score;

        if (tt_depth >= depth) {
            if (*search_score > MAX_EVAL)
                *search_score -= get_ply(board);
            else
                if (*search_score < -MAX_EVAL)
                    *search_score += get_ply(board);
            if ((tt_flag == TT_UPPER && *search_score <= alpha) ||
                (tt_flag == TT_LOWER && *search_score >= beta) ||
                (tt_flag == TT_EXACT))
            {
                trans_table[idx].record[rec].age = trans_age;
                return TRUE;
            }
        }
        return FALSE;
    }

    return FALSE;
}

//-------------------------------------------------------------------------------------------------
//  Return best move for current position.
//-------------------------------------------------------------------------------------------------
MOVE tt_move(BOARD *board)
{
    int        idx = board_key(board) % trans_entries;
    int        rec;

    assert(idx >= 0 && idx < trans_size);

    for (rec = 0; rec < TT_BUCKETS; rec++) {
        if (trans_table[idx].record[rec].key == LOW32(board_key(board)))
            return trans_table[idx].record[rec].best_move;
    }
    return MOVE_NONE;
}

//-------------------------------------------------------------------------------------------------
//  Return score and if is suitable for singular extension.
//-------------------------------------------------------------------------------------------------
int tt_score(BOARD *board, int min_depth, int *tt_score) {
    int        idx = board_key(board) % trans_entries;
    int        rec;
    int     tt_depth;
    S8      tt_flag;

    assert(idx >= 0 && idx < trans_size);

    for (rec = 0; rec < TT_BUCKETS; rec++) {
        if (trans_table[idx].record[rec].key == LOW32(board_key(board)))  {
            tt_depth  = trans_table[idx].record[rec].depth;
            tt_flag   = trans_table[idx].record[rec].flag;
            *tt_score = trans_table[idx].record[rec].search_score;
            if (tt_flag == TT_LOWER || tt_flag == TT_EXACT) {
                if (tt_depth >= min_depth && *tt_score > -MAX_EVAL && *tt_score < MAX_EVAL)  {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

//END

