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
//  Transposition Table
//-------------------------------------------------------------------------------------------------

typedef struct s_trans_entry {
    U64         key;
    TT_RECORD   record;
}   TT_ENTRY;

struct s_hash_structure {
    TT_ENTRY    *address;
    size_t      size;
    U64         count;
    U64         mask;
    U8          age;
}   hash_table;

//-------------------------------------------------------------------------------------------------
//  Initialization.
//-------------------------------------------------------------------------------------------------
void tt_init(size_t size_mb)
{
    assert(sizeof(TT_ENTRY) == 16);

    if (hash_table.address != NULL) free(hash_table.address);

    hash_table.size = 2;
    while (hash_table.size * 2 <= size_mb) {
        hash_table.size *= 2;
    }
    hash_table.size = hash_table.size * 1024 * 1024;

    hash_table.address = (TT_ENTRY *)malloc(hash_table.size);
    if (!hash_table.address) {
        printf("no memory for transposition table!");
        exit(-1);
    }

    hash_table.count = hash_table.size / sizeof(TT_ENTRY);
    hash_table.mask = hash_table.count - 1;

    tt_clear();
}

//-------------------------------------------------------------------------------------------------
//  Increment table age field.
//-------------------------------------------------------------------------------------------------
void tt_age(void)
{
    hash_table.age++;
}

//-------------------------------------------------------------------------------------------------
//  Clear.
//-------------------------------------------------------------------------------------------------
void tt_clear(void)
{
    memset(hash_table.address, 0, hash_table.size);
    hash_table.age = 0;
}

//-------------------------------------------------------------------------------------------------
//  Read entry for current boad key
//-------------------------------------------------------------------------------------------------
void tt_read(U64 key, TT_RECORD *record)
{
    TT_ENTRY *hash_entry = hash_table.address + (key & hash_table.mask);
    record->data = (hash_entry->key == key) ? hash_entry->record.data : 0;
}

//-------------------------------------------------------------------------------------------------
//  Save entry for current boad key
//-------------------------------------------------------------------------------------------------
void tt_save(U64 key, TT_RECORD *record)
{
    TT_ENTRY *hash_entry = hash_table.address + (key & hash_table.mask);

    if (hash_entry->key == key 
    || hash_entry->record.info.depth <= record->info.depth
    || hash_entry->record.info.age != hash_table.age) {
        TT_ENTRY new_entry;
        new_entry.key = key;
        new_entry.record.data = record->data;
        new_entry.record.info.age = hash_table.age;
        if (hash_entry->key == key && new_entry.record.info.move == MOVE_NONE) {
            new_entry.record.info.move = hash_entry->record.info.move;
        }
        *hash_entry = new_entry;
    }
}

//END
