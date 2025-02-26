#pragma once

#include <stdbool.h>
bool read_array(char *filename, int *array, int len); // Reads array from file path
void generate_array(int *ptr, int len); // Does what it says :)

void seq_sum(int *src, int *dst, int len); // Runs non parallel prefix sum for sanity check

inline int max(int a, int b); // Returns max
uint32_t log_2(int x); // Returns the log base 2 of x, if x == 0, returns 0
uint32_t exp_2(int exp); // Returns 2^x, if x == 0, returns 0

void assert(bool b, char *msg); // If not true, prints the error message, and exits

#define INT_CACHE_ALIGN(x) (((x) + 15) & ~15) // Rounds up to nearest multiple of 16
#define PTR_CACHE_ALIGN(x) (void*)(((uintptr_t)x + 63) & ~63) // Rounds up to nearest multiple of 64

// The header for the shared memory block
typedef struct {
    uint32_t mem_size;
    uint32_t proc_count;
    uint32_t num_count; // How many numbers there are total

    volatile uint64_t barrier; // O(1) barrier!

    int *volatile array;
    int *volatile array_swap;
} block_header;

uint64_t millisec_time(); // Time in milliseconds
__uint128_t microsec_time(); // Time in microseconds