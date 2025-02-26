#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "args.h"

int arg_parse_flag(args a, char *flag) {
    for (int i = 0; i < a.argc; i++) {
        char *arg = a.argv[i];

        int arg_len = strnlen(arg, ARG_MAX_LEN);
        int flag_len = strnlen(flag, ARG_MAX_LEN);

        if (arg_len != flag_len) {
            continue;
        }

        if (strncmp(arg, flag, arg_len) == 0) {
            return i;
        };
    }

    return -1;
}

int arg_parse_str(args a, char *flag) {
    int flag_index = arg_parse_flag(a, flag);
    if (flag_index < 0 || flag_index >= a.argc - 2) {
        return -1;
    }

    return flag_index + 1;
}

bool arg_parse_uint(args a, char *flag, uint32_t *res) {
    int flag_index = arg_parse_flag(a, flag);
    if (flag_index < 0) {
        return false;
    }

    if (flag_index < a.argc - 1) {
        *res = strtoul(a.argv[flag_index + 1], NULL, 10);
        return true;
    }

    return false;
}