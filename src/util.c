#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

void generate_random_array(int *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ptr[i] = rand() % 256 - 127;
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
        printf("%s\n", msg);
        exit(-1);
    }
}

const int WIDTH = 20;
void print_array(int *ptr, int len) {
    // Wrap every 80 bytes
    int i = 0;
    while (i < len / WIDTH) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%i ", ptr[i * WIDTH + j]);
        }

        printf("\n");

        i++;
    }

    // Print the remaining bytes
    for (int j = 0; j < len % WIDTH; j++) {
        printf("%i ", ptr[i * WIDTH + j]);
    }

    printf("\n");
}

void seq_sum(int *src, int *dst, int len) {
    memcpy(dst, src, sizeof(int) * len);

    for (int i = 1; i < len; i++) {
        // printf("dst[%i] = %i, dst[%i] = %i\n", i, dst[i], i - 1, dst[i - 1]);
        dst[i] = dst[i] + dst[i - 1];
    }

    return;
}

void print_bytes(char *buf, int len) {
    // Wrap every 80 bytes
    int i = 0;
    while (i < len / WIDTH) {
        for (int j = 0; j < WIDTH; j++) {
            printf("%02hhx ", buf[i * WIDTH + j]);
        }

        printf("\n");

        i++;
    }

    // Print the remaining bytes
    for (int j = 0; j < len % WIDTH; j++) {
        printf("%02hhx ", buf[i * WIDTH + j]);
    }

    printf("\n");
}