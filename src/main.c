#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#define ARG_MAX_LEN 16

typedef struct {
    int argc;
    char **argv;
} args;

typedef struct {
    int shmid;
    uint32_t proc_id;
} proc_arg;

typedef struct {
    uint32_t proc_count;
    uint32_t count;        // How many numbers there are total
    uint8_t barrrier_ctrl; // Whether processes are allowed to move on,
                           // manipulated by the main process
} block_header;

void assert(bool b, char *msg) {
    if (!b) {
        printf("%s\n", msg);
        exit(-1);
    }
}

// Creates a process just to run the function that func_ptr points to
// Exits the child process after that function is run.
void create_process(void *func_ptr, void *argument) {
    int res = fork();

    if (res < 0) {
        perror("Failed to create a new process");
        return;
    }

    if (res == 0) {
        ((void (*)(void *))func_ptr)(argument);
        exit(0);
    }

    return;
}

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

// The round number is the one that we are waiting to start, not the current one
void barrier_wait(int round_num, void *mem_block) {
    // printf("Waiting for round %i to finish\n", round_num - 1);
    block_header *header = (block_header *)mem_block;
    uint8_t *barrier = mem_block + sizeof(block_header);
    uint8_t barrier_sum = 0;

    // printf("With %i processes, barrier_sum (%i) should greater than %i to continue\n", header->proc_count, barrier_sum, (header->proc_count * round_num));
    // printf("%i > %i = %d\n", (header->proc_count * round_num), barrier_sum, (header->proc_count * round_num) > barrier_sum);
    while ((header->proc_count * round_num) >= barrier_sum) {
        barrier_sum = 0;
        for (int i = 0; i < header->proc_count; i++) {
            barrier_sum += barrier[i];
            // printf("Barrier sum %i\n", barrier_sum);
        }
        usleep(1000);
    }
}

void round_n(int *array_src, int *array_dst, uint32_t len, uint32_t proc_id,
             uint32_t proc_count, int round_num) {
    int unsolved = exp_2(round_num); // Index of first unsolved number

    int local_len =
        (len - unsolved + proc_count - 1) /
        proc_count; // The number of indices the process are responsible for
    int begin = unsolved + local_len * proc_id;
    int end = begin + local_len; // NON INCLUSIVE!

    if (begin > len) {
        return; // This process is unecessary
    }

    if (end > len) {
        end = len;
    }

    // printf("For round %u, process %u is responsible for indices %i-%i with "
    //        "local_len = %i\n",
    //     //    round_num, proc_id, begin, end - 1, local_len);

    int offset = exp_2(round_num);
    for (int i = begin; i < end; i++) {
        array_dst[i] = array_src[i] + array_src[i - offset];
    }
}

void adder(proc_arg *arg) {
    printf("Created Process %u\n", arg->proc_id);

    block_header *header;
    char *barrier;
    int *array;
    int *array_swap;

    void *mem_block = shmat(arg->shmid, NULL, 0);
    if (mem_block == -1) {
        perror("Process %u: Failed to access shared memory");
        exit(-1);
    }

    header = (block_header *)mem_block;
    barrier = mem_block + sizeof(block_header);
    array = (int *)(barrier + header->proc_count);
    array_swap = array + header->count;

    for (int i = 0; i < log_2(header->count); i++) {
        printf("Process %i beginning round %i\n", arg->proc_id, i);
        if (i % 2 == 0) {
            round_n(array, array_swap, header->count, arg->proc_id,
                    header->proc_count, i);
        } else {
            round_n(array_swap, array, header->count, arg->proc_id,
                header->proc_count, i);
        }

        barrier[arg->proc_id]++;
        printf("Beginning to wait\n");
        barrier_wait(i + 1, mem_block);
        while (i + 1 > header->barrrier_ctrl) {
            // Wait
        }
    }
}

void generate_random(int *ptr, int len) {
    for (int i = 0; i < len; i++) {
        ptr[i] = rand() % 256 - 127;
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
        printf("dst[%i] = %i, dst[%i] = %i\n", i, dst[i], i - 1, dst[i - 1]);
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

int main(int argc, char **argv) {
    srand(0);
    args arguments = {argc, argv};

    uint32_t size = 16;
    uint32_t proc_count = 2;
    arg_parse_uint(arguments, "-s", &size);
    arg_parse_uint(arguments, "-j", &proc_count);

    assert(size > 0, "Size -- Please enter a valid number greater than 0");
    assert(proc_count > 0,
           "Process Count -- Please enter a valid number greater than 0");

    int mem_size =
        sizeof(block_header) +
        proc_count  // Each process stores its barrier value in a single byte
        + (4 * size) * 2; // Size for the numbers themselves (and a swap array)

    int id =
        shmget(IPC_PRIVATE, mem_size,
               IPC_CREAT |
                   0600); // Allocates 4 * size bytes, plus one for storing info
    // shmctl(id, IPC_RMID, 0); // (Supposedly?) Remove after all processes exit
    if (id < 0) {
        perror("Failed to create shared memory");
        exit(-1);
    }

    void *mem_block = shmat(id, NULL, 0);
    if (mem_block == -1) {
        perror("Failed to access shared memory");
        exit(-1);
    }

    memset(mem_block, 0, mem_size); // Set the block to 0s

    block_header *header = (block_header *)mem_block;
    char *barrier = mem_block + sizeof(block_header);
    int *array = (int *)(barrier + proc_count);

    header->proc_count = proc_count;
    header->count = size;

    generate_random(array, size); // Initialize the array

    // print_bytes(mem_block, mem_size);
    print_array(array, size);

    int seq_array[size];
    seq_sum(array, seq_array, size);
    printf("Sequential:\n");
    print_array(seq_array, size);

    for (int i = 0; i < proc_count; i++) {

        proc_arg arg = {id, i};
        create_process(&adder, (void *)&arg);
        usleep(1000 * 500);
    }

    while (1) {
    };
}