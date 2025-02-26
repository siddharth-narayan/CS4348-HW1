// #include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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
        nanosleep(&ts, NULL); // Sleep to allow quicker execution for other processes
        backoff *= 2;
    }
}

// Runs the nth round of calculation
void round_n(block_header *header, uint32_t proc_id, uint32_t round_num) {
    uint32_t unsolved = exp_2(round_num); // Index of first unsolved number
    uint32_t aligned_unsolved = INT_CACHE_ALIGN(unsolved);

    uint32_t local_len = ((header->num_count - aligned_unsolved) + (header->proc_count - 1)) /
                         header->proc_count; // The number of indices the processes are responsible for

    local_len = INT_CACHE_ALIGN(max(local_len, 16)); // local_len should be 16 at minimum to avoid cache invalidation
                                                     // from other processes writing to close memory spots

    uint32_t begin = aligned_unsolved + local_len * proc_id;
    uint32_t end = begin + local_len; // NON INCLUSIVE!

    if (proc_id == 0) {
        begin = unsolved; // The first process must take on the unaligned part as well
    }

    if (begin > header->num_count) {
        return; // This process is unecessary
    }

    if (end > header->num_count) {
        end = header->num_count;
    }

    debug(DEBUG, "For round %u, process %u is responsible for indices %i-%i (len = %i)\n", round_num, proc_id, begin,
          end - 1, local_len);

    uint32_t offset = exp_2(round_num);
    for (uint32_t i = begin; i < end; i++) {
        header->array_swap[i] = header->array[i] + header->array[i - offset];
        debug(RIDICULOUS, "Process %i: array_dst[%i] = array_src[%i] + array_src[%i] => array_dst[%i] = %i + %i = %i\n",
              proc_id, i, i, i - offset, i, header->array[i], header->array[i - offset], header->array_swap[i]);
    }
}

// The entry point for child processes to begin calculation
void child_entry(proc_arg *arg) {
    debug(INFO, "Created Process %u\n", arg->proc_id);

    block_header *header;

    void *mem_block = shmat(arg->shmid, NULL, 0);
    if (mem_block == -1) {
        perror("Process %u: Failed to access shared memory");
        exit(-1);
    }

    header = (block_header *)mem_block;

    for (uint32_t round_num = 0; round_num < log_2(header->num_count * 2 - 1);
         round_num++) { // Run for ceil(log_2(num_count)) times
        debug(TRACE, "Process %i beginning round %i\n", arg->proc_id, round_num);

        round_n(header, arg->proc_id, round_num); // Run calculation for round n

        // atomic_fetch_add(&header->barrier, 1); // Mark this process as done with its work for this round

        debug(TRACE, "Process %i: Waiting for round %i to complete\n", arg->proc_id, round_num);

        // Wait for other processes
        uint32_t target_barrier_val = (header->proc_count + 1) * (round_num) + arg->proc_id;
        barrier_wait(target_barrier_val, &header->barrier);

        header->barrier++;

        target_barrier_val = (header->proc_count + 1) * (round_num + 1);
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
    assert(proc_count > 1, "-j -- Please enter a valid number greater than 1");

    int mem_size = sizeof(block_header) + (4 * size) * 2 +
                   127; // Size for the header, the numbers and a swap array, and padding for cache alignment

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

    debug(INFO, "Created shared memory with id %i\n", id);

    memset(mem_block, 0, mem_size); // Set the block to 0s

    block_header *header = (block_header *)mem_block;
    header->mem_size = mem_size;
    header->proc_count = proc_count;
    header->num_count = size;
    header->barrier = 0;

    header->array = PTR_CACHE_ALIGN(mem_block + sizeof(block_header));
    header->array_swap = PTR_CACHE_ALIGN(header->array + (4 * header->num_count)); // Overlaps here?

    int filename_index = arg_parse_str(args, "-f");
    if (filename_index < 0) {
        generate_array(header->array, header->num_count); // Initialize the array`
    } else {
        if (!read_array(args.argv[filename_index], header->array, header->num_count)) {
            generate_array(header->array, header->num_count);
        }
    }

    int *seq_array = malloc(size * sizeof(int));

    __uint128_t seq_timer = microsec_time();
    seq_sum(header->array, seq_array, header->num_count);
    printf("seq_sum took \e[1;32m%llu \e[0mmicroseconds\n", microsec_time() - seq_timer);

    debug(DEBUG, "Sequential prefix sum result:\n");
    if (DEBUG_MODE >= DEBUG) {
        print_array(seq_array, header->num_count, "\e[1;34m");
    }

    for (uint32_t i = 0; i < proc_count; i++) {
        proc_arg arg = {id, i};
        create_process(&child_entry, (void *)&arg);
    }

    uint64_t parallel_timer = millisec_time();

    for (uint32_t round_num = 0; round_num < log_2(header->num_count * 2 - 1);
         round_num++) { // Run for ceil(log_2(size)) times

        __uint128_t round_timer = microsec_time();
        uint64_t barrier_target = ((header->proc_count + 1) * (round_num + 1)) - 1;
        barrier_wait(barrier_target, &header->barrier);

        debug(DEBUG, "Round %i complete in \e[1;32m%llu \e[0;1;36mmicroseconds\n", round_num,
              microsec_time() - round_timer);

        int begin = exp_2(round_num - 1);
        int end = exp_2(round_num);
        int len = end - begin;

        memcpy(header->array_swap + begin, header->array + begin,
               len * 4);          // Copy the already solved indices to the destination array
        int *tmp = header->array; // Swap pointers so src -> dst and dst -> src
        header->array = header->array_swap;
        header->array_swap = tmp;

        debug(TRACE, "The array looks like this at the end of round %i:\n", round_num);
        if (DEBUG_MODE >= TRACE) {
            print_array(header->array, header->num_count, "\e[1;35m");
        }

        debug(INFO, "Round %u has ended\n", round_num);

        header->barrier++; // This is safe
    }

    debug(DEBUG, "Final array:\n");
    if (DEBUG_MODE >= DEBUG) {
        print_array(header->array, header->num_count, "\e[1;34m");
    }

    debug(TRACE, "Final bytes:\n");
    if (DEBUG_MODE >= TRACE) {
        print_bytes(mem_block, mem_size, "\e[1;35m");
    }

    printf("Parallel prefix sum took \e[1;32m%lu \e[0mmilliseconds\n", millisec_time() - parallel_timer);

    if (memcmp(header->array, seq_array, sizeof(int) * size) == 0) {
        printf("\e[1;32mParallel prefix sum matches sequential prefix sum\n");
    } else {
        printf("\e[1;31mParallel prefix sum does NOT match sequential prefix sum\n");
    }

    FILE *output = fopen("output", "w");
    for (int i = 0; i < header->num_count; i++) {
        fprintf(output, "%d\n", header->array[i]);
    }

    fclose(output);

    //// Cleanup ////

    color_clear();

    free(seq_array);

    // Remove shared memory?
    if (shmctl(id, IPC_RMID, 0) < 0) {
        perror("Failed to free shared memory");
    }

    return 0;
}