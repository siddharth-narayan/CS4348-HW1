#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#include "args.h"
#include "print.h"
#include "util.h"

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

// The round number is the one that we are waiting to start, not the current one
void barrier_wait(int round_num, void *mem_block) {
    block_header *header = (block_header *)mem_block;
    uint8_t *barrier = mem_block + sizeof(block_header);
    uint8_t barrier_sum = 0;
    while ((header->proc_count * round_num) > barrier_sum) {
        barrier_sum = 0;
        for (int i = 0; i < header->proc_count; i++) {
            barrier_sum += barrier[i];
        }
        usleep(1000 * 250);
    }
}

void round_n(int *array_src, int *array_dst, uint32_t len, uint32_t proc_id, uint32_t proc_count, int round_num) {
    int unsolved = exp_2(round_num); // Index of first unsolved number

    int local_len =
        (len - unsolved + proc_count - 1) / proc_count; // The number of indices the process are responsible for
    int begin = unsolved + local_len * proc_id;
    int end = begin + local_len; // NON INCLUSIVE!

    if (begin > len) {
        return; // This process is unecessary
    }

    if (end > len) {
        end = len;
    }

    debug(DEBUG, "For round %u, process %u is responsible for indices %i-%i\n", round_num, proc_id, begin, end - 1,
          local_len);

    int offset = exp_2(round_num);
    for (int i = begin; i < end; i++) {
        array_dst[i] = array_src[i] + array_src[i - offset];
    }
}

void adder(proc_arg *arg) {
    debug(INFO, "Created Process %u\n", arg->proc_id);

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

    for (uint32_t i = 0; i < log_2(header->count); i++) {
        debug(INFO, "Process %i beginning round %i\n", arg->proc_id, i);

        round_n(array, array_swap, header->count, arg->proc_id, header->proc_count, i);

        // Swap pointer
        void *tmp = array;
        array = array_swap;
        array_swap = tmp;

        barrier[arg->proc_id]++;
        debug(INFO, "Process %i: Waiting for round %i to complete\n", arg->proc_id, i);
        barrier_wait(i + 1, mem_block);

        while (i + 1 > header->barrrier_ctrl) {
            // Wait
            usleep(1000);
        }
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
    args args = {argc, argv};

    uint32_t size = 16;
    uint32_t proc_count = 2;

    if (arg_parse_flag(args, "--help") >= 0 || arg_parse_flag(args, "-h") >= 0) {
        print_help();
        exit(0);
    }
    arg_parse_uint(args, "-s", &size);
    arg_parse_uint(args, "-j", &proc_count);
    if (arg_parse_flag(args, "-d") >= 0) {
        DEBUG_MODE = 1;
        arg_parse_uint(args, "-d", &DEBUG_MODE);
        if (!DEBUG_MODE) {
            DEBUG_MODE = 1;
        }
    }

    assert(size > 0, "Size -- Please enter a valid number greater than 0");
    assert(proc_count > 0, "Process Count -- Please enter a valid number greater than 0");

    int mem_size = sizeof(block_header) + proc_count // Each process stores its barrier value in a single byte
                   + (4 * size) * 2;                 // Size for the numbers themselves (and a swap array)

    int id = shmget(IPC_PRIVATE, mem_size,
                    IPC_CREAT | 0600);
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
    int *array_swap = array + size;

    header->proc_count = proc_count;
    header->count = size;

    generate_random_array(array, size); // Initialize the array

    int *seq_array = malloc(size * sizeof(int));
    seq_sum(array, seq_array, size);

    if (DEBUG_MODE >= DEBUG) {
        color_bold();
        color_set_256(214);

        printf("Sequential prefix sum result:\n");
        print_array(seq_array, size);
        
        color_clear();
    }

    for (int i = 0; i < proc_count; i++) {

        proc_arg arg = {id, i};
        create_process(&adder, (void *)&arg);
        // usleep(1000 * 500);
    }

    for (int i = 0; i < log_2(size); i++) {
        barrier_wait(i + 1, mem_block);
        debug(INFO, "Round %i complete\n", i);

        int begin = exp_2(i - 1);
        int end = exp_2(i);
        int len = end - begin;

        memcpy(array_swap + begin, array + begin, len * 4);
        void *tmp = array;
        array = array_swap;
        array_swap = tmp;

        header->barrrier_ctrl++; // Allow processes to continue
    }

    if (DEBUG_MODE >= DEBUG) {
        printf("\e[1;38;5;214m");
        printf("Final array:\n");
        print_array(array, size);
        printf("\e[0m");
    }

    if (memcmp(array, seq_array, sizeof(int) * size) == 0) {
        printf("\e[1;32mParallel prefix sum matches sequential prefix sum\n");
    } else {
        printf("\e[1;31mParallel prefix sum does NOT match sequential prefix sum\n");
    }
}