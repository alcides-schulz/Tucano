# Tucano makefile (linux, ARMV8)

CC = gcc
CFLAGS = -O3 -DEGTB_SYZYGY -DPOSIX_MADV_RANDOM -Wall -Wextra -march=native -flto 
LFLAGS = -lpthread -lm
EXE = tucano

old:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_old $(LFLAGS)

avx2:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_avx2 $(LFLAGS) -DUSE_AVX2 -mavx2 -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
	
sse4:
	$(CC) $(CFLAGS) *.c fathom/tbprobe.c -o $(EXE)_sse4 $(LFLAGS) -DUSE_SSE41 -msse4.1 -DUSE_SSSE3 -mssse3 -DUSE_SSE2 -msse2 -DUSE_SSE -msse
