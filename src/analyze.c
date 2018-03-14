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
//  Analyze feature
//-------------------------------------------------------------------------------------------------

void analyze_mode(GAME *game)
{
    is_analysis = TRUE;

    SETTINGS    settings;
    settings.max_depth = MAX_DEPTH;
    settings.moves_level = 0;
    settings.post_flag = POST_XBOARD;
    settings.single_move_time = MAX_TIME;
    settings.total_move_time = MAX_TIME;
    settings.use_book = FALSE;

    while (is_analysis) {
        
        search_run(game, &settings);

        if (!strcmp(analysis_command, "exit")) {
            is_analysis = FALSE;
        }
        else
        if (!strcmp(analysis_command, "undo")) {
            undo_move(&game->board);
        }
        else
        if (!strcmp(analysis_command, "new")) {
            set_fen(&game->board, FEN_NEW_GAME) ;
        }
        else
        if (!strncmp(analysis_command, "setboard", 8)) {
            if (strlen(analysis_command) > 9) {
                set_fen(&game->board, &analysis_command[9]);
            }
        }    
        else {
            MOVE input_move = util_parse_move(game, analysis_command);
            if (input_move != MOVE_NONE && is_valid(&game->board, input_move))
                make_move(&game->board, input_move);
        }
    }
}

// EOF//
