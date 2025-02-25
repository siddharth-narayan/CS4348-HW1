#pragma once

#include <stdbool.h>

void generate_random_array(int *ptr, int len);
void seq_sum(int *src, int *dst, int len); // Runs non parallel prefix sum for sanity check

inline int max(int a, int b); // Returns max
uint32_t log_2(int x); // Returns the log base 2 of x, if x == 0, returns 0
uint32_t exp_2(int exp); // Returns 2^x, if x == 0, returns 0

void assert(bool b, char *msg); // If not true, prints the error message, and exits

#define INT_CACHE_ALIGN(x) (((x) + 15) & ~15)
#define PTR_CACHE_ALIGN(x) (void*)(((uintptr_t)x + 63) & ~63)

typedef struct {
    uint32_t mem_size;
    uint32_t proc_count;
    uint32_t num_count; // How many numbers there are total

    volatile uint64_t barrier; // O(1) barrier!

    int *volatile array;
    int *volatile array_swap;
} block_header;

uint64_t millisec_time();
__uint128_t microsec_time();