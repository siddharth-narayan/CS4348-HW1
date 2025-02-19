#pragma once

#include <stdint.h>

void color_clear(); // Clear color formatting
void color_bold(); // Set bold on
void color_set_16(int color_code); // One of 16 colors defined by terminal
void color_set_256(int color_code); // One of 256 additional colors defined by terminal

#define PRINT_WIDTH 24 // Divisible by 1, 2, 3, 4
void print_array(int *ptr, int len, char *ansi_color_format); // Print an integer array
void print_bytes(char *buf, int len, char *ansi_color_format); // Print an array of bytes
void print_help(); // Print the help command for this program

extern uint32_t DEBUG_MODE;
typedef enum {
    NONE = 0,
    INFO,
    DEBUG,
    TRACE,
    RIDICULOUS, // Use if insane
} debug_level;

extern const char* DEBUG_STR_ARRAY[]; // Contains enum to string mapping
void debug(uint32_t mode, char *format, ...); // Prints at the appropriate debug level