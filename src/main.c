#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#include "args.h"
#include "print.h"
#include "util.h"

typedef struct {
    int shmid;
    uint32_t proc_id;
} proc_arg;

typedef struct {
    uint32_t proc_count;
    uint32_t count;            // How many numbers there are total
    volatile uint64_t barrier; // O(1) barrier!
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
void barrier_wait(uint64_t target, volatile uint64_t *barrier) {
    uint32_t backoff = 1; // Milliseconds
    while (target > *barrier) {
        struct timespec ts = {0, backoff * 1000};
        nanosleep(&ts, NULL);  // Sleep to allow quicker execution
        backoff *= 2;
        //debug(INFO, "Target: %lu barrier: %lu\n", target, barrier);
    }
}

// Runs the nth round of calculation
void round_n(int *array_src, int *array_dst, uint32_t len, uint32_t proc_id, uint32_t proc_count, uint32_t round_num) {
    uint32_t unsolved = exp_2(round_num); // Index of first unsolved number

    uint32_t local_len =
        (len - unsolved + proc_count - 1) / proc_count; // The number of indices the processes are responsible for
    uint32_t begin = unsolved + local_len * proc_id;
    uint32_t end = begin + local_len; // NON INCLUSIVE!

    if (begin > len) {
        return; // This process is unecessary
    }

    if (end > len) {
        end = len;
    }

    //debug(DEBUG, "For round %u, process %u is responsible for indices %i-%i (len = %i)\n", round_num, proc_id, begin,
        //   end - 1, local_len);

    uint32_t offset = exp_2(round_num);
    for (uint32_t i = begin; i < end; i++) {
        array_dst[i] = array_src[i] + array_src[i - offset];
        //debug(TRACE, "Process %i: array_dst[%i] = array_src[%i] + array_src[%i]\n=> array_dst[%i] = %i + %i = %i\n",
            //   proc_id, i, i, i - offset, i, array_src[i], array_src[i - offset], array_dst[i]);
    }
}

// The entry point for child processes to begin calculation
void child_entry(proc_arg *arg) {
    //debug(INFO, "Created Process %u\n", arg->proc_id);

    block_header *header;
    int *array;
    int *array_swap;

    void *mem_block = shmat(arg->shmid, NULL, 0);
    if (mem_block == -1) {
        perror("Process %u: Failed to access shared memory");
        exit(-1);
    }

    header = (block_header *)mem_block;
    array = mem_block + sizeof(block_header);
    array_swap = array + header->count;

    for (uint32_t round_num = 0; round_num < log_2(header->count * 2 - 1);
         round_num++) { // Run for ceil(log_2(count)) times
        //debug(INFO, "Process %i beginning round %i\n", arg->proc_id, round_num);

        round_n(array, array_swap, header->count, arg->proc_id, header->proc_count,
                round_num); // Run calculation for round n

        // Swap pointer
        void *tmp = array;
        array = array_swap;
        array_swap = tmp;

        atomic_fetch_add(&header->barrier, 1); // Mark this process as done with its work for this round

        //debug(INFO, "Process %i: Waiting for round %i to complete\n", arg->proc_id, round_num);

        // Wait for other processes
        uint32_t target_barrier_val = (header->proc_count + 1) * (round_num + 1);
        barrier_wait(target_barrier_val, &header->barrier);
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
    args args = {argc, argv};

    uint32_t size = 16;
    uint32_t proc_count = 2;

    // Parse arguments
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

    assert(size > 0, "-s -- Please enter a valid number greater than 0");
    assert(proc_count > 0, "-j -- Please enter a valid number greater than 0");

    int mem_size = sizeof(block_header) + (4 * size) * 2; // Size for the header, the numbers themselves, (and a swap array)

    int id = shmget(IPC_PRIVATE, mem_size, IPC_CREAT | 0600);
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
    int *array = (int *)(mem_block + sizeof(block_header));
    int *array_swap = array + size;

    header->proc_count = proc_count;
    header->count = size;
    header->barrier = 0;

    generate_random_array(array, size); // Initialize the array

    int *seq_array = malloc(size * sizeof(int));

    uint64_t seq_timer = millisec_time();
    seq_sum(array, seq_array, size);
    printf("seq_sum took \e[1;32m%lu \e[0mmilliseconds\n", millisec_time() - seq_timer);

    //debug(DEBUG, "Sequential prefix sum result:\n");
    if (DEBUG_MODE >= DEBUG) {
        print_array(seq_array, size, "\e[1;34m");
    }

    for (uint32_t i = 0; i < proc_count; i++) {

        proc_arg arg = {id, i};
        create_process(&child_entry, (void *)&arg);
    }

    uint64_t parallel_timer = millisec_time();

    for (uint32_t round_num = 0; round_num < log_2(size * 2 - 1); round_num++) { // Run for ceil(log_2(size)) times
        //debug(INFO, "Main process: waiting for round %u to complete\n", round_num);

        __uint128_t round_timer = microsec_time();
        uint64_t barrier_target = ((header->proc_count + 1) * (round_num + 1)) - 1;
        barrier_wait(barrier_target, &header->barrier);

        //debug(INFO, "Round %i complete in \e[1;32m%llu \e[0;1;36mmicroseconds\n", round_num, microsec_time() - round_timer);

        int begin = exp_2(round_num - 1);
        int end = exp_2(round_num);
        int len = end - begin;

        memcpy(array_swap + begin, array + begin, len * 4); // Copy the already solved indices to the destination array
        void *tmp = array;                                  // Swap pointers so src -> dst and dst -> src
        array = array_swap;
        array_swap = tmp;

        //debug(DEBUG, "The array looks like this at the end of round %i:\n", round_num);
        if (DEBUG_MODE >= DEBUG) {
            print_array(array, size, "\e[1;34m");
        }

        __atomic_fetch_add(&header->barrier, 1, __ATOMIC_SEQ_CST);
    }

    //debug(DEBUG, "Final array:\n");
    if (DEBUG_MODE >= DEBUG) {
        print_array(array, size, "\e[1;34m");
    }

    printf("Parallel prefix sum took \e[1;32m%lu \e[0mmilliseconds\n", millisec_time() - parallel_timer);

    if (memcmp(array, seq_array, sizeof(int) * size) == 0) {
        printf("\e[1;32mParallel prefix sum matches sequential prefix sum\n");
    } else {
        printf("\e[1;31mParallel prefix sum does NOT match sequential prefix sum\n");
    }

    sleep(1); // Wait for everything to exit

    //// Free everything ////

    free(seq_array);

    // Remove shared memory?
    if (shmctl(id, IPC_RMID, 0) < 0) {
        perror("Failed to free shared memory");
    }

    return 0;
}