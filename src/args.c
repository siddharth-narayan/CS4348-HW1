#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "args.h"

bool arg_parse_uint(args a, char *flag, uint32_t *res) {
    for (int i = 0; i < a.argc; i++) {
        char *arg = a.argv[i];

        int arg_len = strnlen(arg, ARG_MAX_LEN);
        int flag_len = strnlen(flag, ARG_MAX_LEN);

        if (arg_len != flag_len) {
            continue;
        }

        if (strncmp(arg, flag, arg_len) == 0 && i < a.argc - 1) {
            *res = strtoul(a.argv[i + 1], NULL, 10);
            return true;
        };
    }

    return false;
}