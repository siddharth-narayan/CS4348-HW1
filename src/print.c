#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "print.h"

void color_clear() {
    printf("\e[0m");
}

void color_bold() {
    printf("\e[1m");
}

void color_set_16(int color_code) {
    if (color_code > 99 || color_code < 0) {
        return;
    }
    printf("\e[%im", color_code);
}

void color_set_256(int color_code) {
    if (color_code > 256 || color_code < 0) {
        return;
    }
    printf("\e[38;5;%im", color_code);
}

void print_array(int *ptr, int len) {
    // Wrap every 80 ints
    int i = 0;
    while (i < len / PRINT_WIDTH) {
        for (int j = 0; j < PRINT_WIDTH; j++) {
            printf("%-5i ", ptr[i * PRINT_WIDTH + j]);
        }

        printf("\n");

        i++;
    }

    // Print the remaining ints
    for (int j = 0; j < len % PRINT_WIDTH; j++) {
        printf("%-5i ", ptr[i * PRINT_WIDTH + j]);
    }

    printf("\n");
}

void print_bytes(char *buf, int len) {
    // Wrap every 80 bytes
    int i = 0;
    while (i < len / PRINT_WIDTH) {
        for (int j = 0; j < PRINT_WIDTH; j++) {
            printf("%02hhx ", buf[i * PRINT_WIDTH + j]);
        }

        printf("\n");

        i++;
    }

    // Print the remaining bytes
    for (int j = 0; j < len % PRINT_WIDTH; j++) {
        printf("%02hhx ", buf[i * PRINT_WIDTH + j]);
    }

    printf("\n");
}

/* Format makes this impossible to read so here's what it prints (in color)

Usage:
  By default uses 16 numbers and  2 processes
  parallel_prefix_sum  [OPTIONS]

Options:
  -s <num>             Sets the number of integers in the generated array that will be used for the prefix sum algorithm
  -j <num>             Sets the number of processes that will be created to run the prefix sum algorithm in parallel
  -h or --help         Print this help dialogue
*/
void print_help() {
    printf("\e[1;36mUsage:\n"
           "  \e[38;5;24mBy default uses 16 numbers and  2 processes\n"
           "  \e[1;36mparallel_prefix_sum \e[39m [OPTIONS]\n\n"
           "\e[36mOptions:\n"
           "  \e[36m-s \e[38;5;214m<num>             \e[39mSets the number "
           "of integers in the generated array that will be used for the prefix "
           "sum algorithm\n"
           "  \e[36m-j \e[38;5;214m<num>             \e[39mSets the number "
           "of processes that will be created to run the prefix sum algorithm in "
           "parallel\n"
           "  \e[36m-h \e[39mor \e[36m--help         \e[39mPrint this "
           "help "
           "dialogue\n"
           "\e[0m");
}

uint32_t DEBUG_MODE = 0;
const char *DEBUG_STR_ARRAY[] = {"NONE", "INFO", "DEBUG", "TRACE", "RIDICULOUS"};
void debug(uint32_t mode, char *format, ...) {
    if (mode > DEBUG_MODE) {
        return;
    }

    color_bold();

    switch (mode) {
    case INFO:
        color_set_16(36);
        break;

    case DEBUG:
        color_set_16(34);
        break;
    case TRACE:
        color_set_16(35);
        break;
    case RIDICULOUS: // What's wrong with you??
        color_set_16(31);
        break;
    }

    printf("[%s]: ", DEBUG_STR_ARRAY[mode]); // Print level

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    color_clear();
}
