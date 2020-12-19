# Tucano makefile (linux, ARMV8)

CC = gcc
CFLAGS = -std=c99 -O3 -DEGTB_SYZYGY -Wall -Wextra -march=native -flto 
LFLAGS = -lpthread -lm
EXE = tucano

release:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE) $(LFLAGS)
