#pragma once

#include <stdbool.h>

#define ARG_MAX_LEN 16

typedef struct {
    int argc;
    char **argv;
} args;

// Returns the index of the flag if found
// Otherwise returns -1
int arg_parse_flag(args a, char *flag);

// Returns the index of the string if found
// Otherwise returns -1
int arg_parse_str(args a, char *flag);

// Returns true if the flag was found, false if not
// If it returns true, it might not necessarily have
// Parsed a real number. For instance
// "-j Hello" will result in *res being set to 0
bool arg_parse_uint(args a, char *flag, uint32_t *res);