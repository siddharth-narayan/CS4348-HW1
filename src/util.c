#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "print.h"
#include "util.h"

void generate_random_array(int *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ptr[i] = rand() % 256 - 127;
    }

    debug(DEBUG, "Generated array:\n");
    if (DEBUG_MODE >= DEBUG) {
        print_array(ptr, len, "\e[1;34m");
    }
}

uint32_t log_2(int x) {
    uint32_t res = 0;
    while (x > 1) {
        x = x >> 1;
        res++;
    }

    return res;
}

uint32_t exp_2(int exp) {
    if (exp < 0) {
        return 0; // Actually comes in useful when calculating which indices to
                  // copy
    }

    uint32_t res = 1;
    for (int i = 0; i < exp; i++) {
        res *= 2;
    }

    return res;
}

void assert(bool b, char *msg) {
    if (!b) {
        color_bold();
        color_set_16(31);
        printf("%s\n", msg);
        exit(-1);
    }
}

void seq_sum(int *src, int *dst, int len) {
    memcpy(dst, src, sizeof(int) * len);

    for (int i = 1; i < len; i++) {
        // printf("dst[%i] = %i, dst[%i] = %i\n", i, dst[i], i - 1, dst[i - 1]);
        dst[i] = dst[i] + dst[i - 1];
    }

    return;
}

uint64_t millisec_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000 + t.tv_usec / 1000;
}

__uint128_t microsec_time() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec * 1000000 + t.tv_usec;
}