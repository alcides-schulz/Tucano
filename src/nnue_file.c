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

#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/mman.h>
#endif

/*
#if !defined(USE_AVX512)
static weight_t hidden1_weights alignas(64)[32 * 512];
static weight_t hidden2_weights alignas(64)[32 * 32];
#else
static weight_t hidden1_weights alignas(64)[64 * 512];
static weight_t hidden2_weights alignas(64)[64 * 32];
#endif
static weight_t output_weights alignas(64)[1 * 32];
static int32_t hidden1_biases alignas(64)[32];
static int32_t hidden2_biases alignas(64)[32];
static int32_t output_biases[1];
*/

//static int16_t ft_biases[kHalfDimensions];
//static int16_t ft_weights[kHalfDimensions * FtInDims];
//static weight_t hidden1_weights [32 * 512];
//static weight_t hidden2_weights [32 * 32];
//static weight_t output_weights [1 * 32];
//static int32_t hidden1_biases [32];
//static int32_t hidden2_biases [32];
//static int32_t output_biases[1];

FD nnue_open_file(const char *name)
{
#ifndef _WIN32
    return open(name, O_RDONLY);
#else
    return CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
}

void nnue_close_file(FD fd)
{
#ifndef _WIN32
    close(fd);
#else
    CloseHandle(fd);
#endif
}

size_t nnue_file_size(FD fd)
{
#ifndef _WIN32
    struct stat statbuf;
    fstat(fd, &statbuf);
    return statbuf.st_size;
#else
    DWORD size_low, size_high;
    size_low = GetFileSize(fd, &size_high);
    return ((uint64_t)size_high << 32) | size_low;
#endif
}

const void *nnue_map_file(FD fd, map_t *map)
{
#ifndef _WIN32
    *map = nnue_file_size(fd);
    void *data = mmap(NULL, *map, PROT_READ, MAP_SHARED, fd, 0);
#ifdef MADV_RANDOM
    madvise(data, *map, MADV_RANDOM);
#endif
    return data == MAP_FAILED ? NULL : data;
#else
    DWORD size_low, size_high;
    size_low = GetFileSize(fd, &size_high);
    *map = CreateFileMapping(fd, NULL, PAGE_READONLY, size_high, size_low, NULL);
    if (*map == NULL) {
        return NULL;
    }
    return MapViewOfFile(*map, FILE_MAP_READ, 0, 0, 0);
#endif
}

void nnue_unmap_file(const void *data, map_t map)
{
    if (!data) return;
#ifndef _WIN32
    munmap((void *)data, map);
#else
    UnmapViewOfFile(data);
    CloseHandle(map);
#endif
}

uint32_t nnue_read_u32(const void *p)
{
    const uint8_t *q = (const uint8_t*)p;
    return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

uint16_t nnue_read_u16(const void *p)
{
    const uint8_t *q = (const uint8_t*)p;
    return q[0] | (q[1] << 8);
}

int nnue_verify_net(const void *file_data, size_t size)
{
    if (size != 21022697) return FALSE;
    const char *d = (const char*)file_data;
    if (nnue_read_u32(d) != NNUE_VERSION) return FALSE;
    if (nnue_read_u32(d + 4) != 0x3e5aa6eeU) return FALSE;
    if (nnue_read_u32(d + 8) != 177) return FALSE;
    if (nnue_read_u32(d + TRANSFORMER_START) != 0x5d69d7b8) return FALSE;
    if (nnue_read_u32(d + NETWORK_START) != 0x63337156) return FALSE;
    return TRUE;
}

unsigned nnue_weight_index(unsigned r, unsigned c)
{
    return c * 32 + r;
}

const char *nnue_read_hidden_weights(weight_t *w, unsigned dims, const char *d)
{
    for (unsigned r = 0; r < 32; r++) {
        for (unsigned c = 0; c < dims; c++) {
            w[nnue_weight_index(r, c)] = *d++;
        }
    }
    return d;
}

void nnue_read_output_weights(weight_t *w, const char *d)
{
    for (unsigned i = 0; i < 32; i++) {
        unsigned c = i;
        w[c] = *d++;
    }
}

void nnue_init_weights(const void *file_data, NNUE_PARAM *p_nnue_param)
{
    const char *d = (const char *)file_data + TRANSFORMER_START + 4;
    // Read transformer
    for (unsigned i = 0; i < KHALF_DIMENSIONS; i++, d += 2) {
        p_nnue_param->ft_biases[i] = nnue_read_u16(d);
    }
    for (unsigned i = 0; i < KHALF_DIMENSIONS * FT_IN_DIMS; i++, d += 2) {
        p_nnue_param->ft_weights[i] = nnue_read_u16(d);
    }
    // Read network
    d += 4;
    for (unsigned i = 0; i < 32; i++, d += 4) {
        p_nnue_param->hidden1_biases[i] = nnue_read_u32(d);
    }
    d = nnue_read_hidden_weights(p_nnue_param->hidden1_weights, 512, d);
    for (unsigned i = 0; i < 32; i++, d += 4) {
        p_nnue_param->hidden2_biases[i] = nnue_read_u32(d);
    }
    d = nnue_read_hidden_weights(p_nnue_param->hidden2_weights, 32, d);
    for (unsigned i = 0; i < 1; i++, d += 4) {
        p_nnue_param->output_biases[i] = nnue_read_u32(d);
    }
    nnue_read_output_weights(p_nnue_param->output_weights, d);
}

int nnue_load_eval_file(const char *eval_file, NNUE_PARAM *p_nnue_param)
{
    FD fd = nnue_open_file(eval_file);
    if (fd == FD_ERR) {
        return FALSE;
    }
    map_t mapping;
    const void *file_data = nnue_map_file(fd, &mapping);
    size_t size = nnue_file_size(fd);
    nnue_close_file(fd);
    int success = nnue_verify_net(file_data, size);
    if (success) {
        nnue_init_weights(file_data, p_nnue_param);
    }
    if (mapping) {
        nnue_unmap_file(file_data, mapping);
    }
    return success;
}

int nnue_init(const char* eval_file_name, NNUE_PARAM *p_nnue_param)
{
    if (nnue_load_eval_file(eval_file_name, p_nnue_param)) {
        printf("\nEval file '%s' loaded !\n", eval_file_name);
        fflush(stdout);
        return TRUE;
    }
    printf("\nEval file '%s' not found!\n", eval_file_name);
    fflush(stdout);
    return FALSE;
}

//EOF