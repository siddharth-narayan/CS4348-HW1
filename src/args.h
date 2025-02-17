
#define ARG_MAX_LEN 16

typedef struct {
    int argc;
    char **argv;
} args;

bool arg_parse_uint(args a, char *flag, uint32_t *res);