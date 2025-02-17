#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "util.h"
#include "args.h"

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
    // printf("Waiting for round %i to finish\n", round_num - 1);
    block_header *header = (block_header *)mem_block;
    uint8_t *barrier = mem_block + sizeof(block_header);
    uint8_t barrier_sum = 0;

    // printf("With %i processes, barrier_sum (%i) should be >= than %i to "
    //        "continue\n",
    //        header->proc_count, barrier_sum, (header->proc_count *
    //        round_num));

    while ((header->proc_count * round_num) > barrier_sum) {
        // printf("%i > %i = %d\n", (header->proc_count * round_num), barrier_sum, (header->proc_count * round_num) > barrier_sum);
        barrier_sum = 0;
        for (int i = 0; i < header->proc_count; i++) {
            barrier_sum += barrier[i];
            // printf("Barrier sum %i\n", barrier_sum);
        }
        usleep(1000 * 250);
    }

    // printf("Barrier done\n");
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

    for (uint32_t i = 0; i < log_2(header->count); i++) {
        printf("Process %i beginning round %i\n", arg->proc_id, i);

        round_n(array, array_swap, header->count, arg->proc_id,
                header->proc_count, i);

        // Swap pointer
        void *tmp = array;
        array = array_swap;
        array_swap = tmp;

        barrier[arg->proc_id]++;
        printf("Process %i: Waiting for round %i to complete\n", arg->proc_id, i);
        barrier_wait(i + 1, mem_block);

        // printf("Process %i: Done waiting\n", arg->proc_id);
        while (i + 1 > header->barrrier_ctrl) {
            // Wait
            usleep(1000);
            // printf("Process %i: %u > %u = %d\n", arg->proc_id, i+1, header->barrrier_ctrl, i + 1 > header->barrrier_ctrl);
        }
    }
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
        proc_count // Each process stores its barrier value in a single byte
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
    int *array_swap = array + size;

    header->proc_count = proc_count;
    header->count = size;

    generate_random_array(array, size); // Initialize the array

    // print_bytes(mem_block, mem_size);
    print_array(array, size);

    // int seq_array[size];
    // seq_sum(array, seq_array, size);
    // printf("Sequential:\n");
    // print_array(seq_array, size);

    for (int i = 0; i < proc_count; i++) {

        proc_arg arg = {id, i};
        create_process(&adder, (void *)&arg);
        // usleep(1000 * 500);
    }

    for (int i = 0; i < log_2(size); i++) {
        barrier_wait(i + 1, mem_block);
        printf("Round %i complete\n", i);

        int begin = exp_2(i - 1);
        int end = exp_2(i);
        int len = end - begin;

        memcpy(array_swap + begin, array + begin, len * 4);
        void *tmp = array;
        array = array_swap;
        array_swap = tmp;

        header->barrrier_ctrl++; // Allow processes to continue
    }

    printf("Final array:\n");
    print_array(array, size);
}