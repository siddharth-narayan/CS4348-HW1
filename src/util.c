#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "print.h"
#include "util.h"

void generate_random_array(int *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ptr[i] = rand() % 256 - 127;
    }

    if (DEBUG_MODE >= DEBUG) {
        color_bold();
        color_set_16(36);
        printf("Generated array:\n");
        print_array(ptr, len);
        color_clear();
    }
}

int log_2(int x) {
    int res = 0;
    while (x > 1) {
        x = x >> 1;
        res++;
    }

    return res;
}

int exp_2(int exp) {
    if (exp < 0) {
        return 0; // Actually comes in useful when calculating which indices to
                  // copy
    }

    int res = 1;
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