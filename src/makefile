# Tucano makefile (linux, ARMV8)

CC = gcc
CFLAGS = -O3 -DEGTB_SYZYGY -DPOSIX_MADV_RANDOM -Wall -Wextra -march=native -flto 
LFLAGS = -lpthread -lm
EXE = tucano

old:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_old $(LFLAGS)

avx2:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_avx2 $(LFLAGS) -DTNNAVX2 -mavx2 -msse4.1 -mssse3 -msse2 -msse
	
sse4:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_sse4 $(LFLAGS) -DTNNSSE4 -msse4.1 -mssse3 -msse2 -msse
