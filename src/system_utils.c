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
//    System specific utilities: time, threads
//-------------------------------------------------------------------------------------------------

#define USE_WINDOWS_THREADS
#if defined(USE_POSIX_THREADS)
#undef USE_WINDOWS_THREADS
#endif

#if defined(_WIN32) || defined(_WIN64)

#define IS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN

#else

#undef IS_WINDOWS

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#endif

//-------------------------------------------------------------------------------------------------
//  Current time
//-------------------------------------------------------------------------------------------------
UINT util_get_time(void)
{
#if defined(IS_WINDOWS)
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    return (unsigned int) ((((U64)ft.dwHighDateTime << 32) | ft.dwLowDateTime) / 10000);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
}

//-------------------------------------------------------------------------------------------------
//  Input available: Used by analysis and ponder.
//-------------------------------------------------------------------------------------------------
int util_input_available(void) 
{
#ifdef IS_WINDOWS
    
    static int      init = FALSE;
    static int      is_pipe = FALSE;
    static HANDLE   stdin_h;
    
    DWORD    val;

#ifndef VS2017
    // FIXME: create a thread to handle input
    if (stdin->_cnt > 0) return TRUE;
#endif

    if (!init) {
        init = TRUE;
        stdin_h = GetStdHandle(STD_INPUT_HANDLE);
        is_pipe = !GetConsoleMode(stdin_h, &val);
        if (!is_pipe) {
            SetConsoleMode(stdin_h, val&~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
            FlushConsoleInputBuffer(stdin_h);
        }
    }

    if (is_pipe) {
        if (!PeekNamedPipe(stdin_h, NULL, 0, NULL, &val, NULL))
            return TRUE;
        return val > 0 ? TRUE : FALSE;
    }
    else {
        GetNumberOfConsoleInputEvents(stdin_h, &val);
        return val > 1 ? TRUE : FALSE;
    }
    
#else // NON_WINDOWS
   
    int val;
    fd_set set[1];
    struct timeval time_val[1];

    FD_ZERO(set);
    FD_SET(STDIN_FILENO,set);

    time_val->tv_sec = 0;
    time_val->tv_usec = 0;

    val = select(STDIN_FILENO+1,set,NULL,NULL,time_val);
    if (val == -1 && val != EINTR) {
        fprintf(stderr, "input_available(): select(): %s\n", strerror(errno));
    }
    return val > 0 ? TRUE : FALSE;

#endif
}

#ifdef IS_WINDOWS
//-------------------------------------------------------------------------------------------------
// Use ASCII extended codes to draw board.
//-------------------------------------------------------------------------------------------------
void util_draw_board(BOARD *board)
{
    char    b[64];

    util_get_board_chars(board, b);
    
    printf("       ");
    printf("%c", 218);
    for (int i = 0; i < 7; i++)
        printf("%c%c%c%c", 196, 196, 196, 194);
    printf("%c%c%c%c\n", 196, 196, 196, 191);
    for (int r = 0; r < 8; r++) {
        printf("%d (%2d) ", 8 - r, r * 8);
        for (int f = 0; f < 8; f++)
            printf("%c %c ", 179, b[r * 8 + f]);
        printf("%c\n", 179);
        printf("       ");
        if (r < 7) {
            printf("%c", 195);
            for (int i = 0; i < 7; i++)
                printf("%c%c%c%c", 196, 196, 196, 197);
            printf("%c%c%c%c\n", 196, 196, 196, 180);
        }
        else {
            printf("%c", 192);
            for (int i = 0; i < 7; i++)
                printf("%c%c%c%c", 196, 196, 196, 193);
            printf("%c%c%c%c\n", 196, 196, 196, 217);
        }
    }
    printf("         a   b   c   d   e   f   g   h  \n");
}
#else // non windows
//-------------------------------------------------------------------------------------------------
//  Draw board
//-------------------------------------------------------------------------------------------------
void util_draw_board(BOARD *board)
{
    char    b[64];

    util_get_board_chars(board, b);

    printf("       +---+---+---+---+---+---+---+---+\n");
    for (int r = 0; r < 8; r++) {
        printf("%d (%2d) ", 8 - r, r * 8);
        for (int f = 0; f < 8; f++) {
            printf("| %c ", b[r * 8 + f]);
        }
        printf("|\n       +---+---+---+---+---+---+---+---+\n");
    }
    printf("         a   b   c   d   e   f   g   h  \n");
}
#endif

//end
