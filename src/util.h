#pragma once

void generate_random_array(int *ptr, int len);
void seq_sum(int *src, int *dst, int len); // Runs non parallel prefix sum for sanity check

int log_2(int x); // Returns the log base 2 of x, if x == 0, returns 0
int exp_2(int exp); // Returns 2^x, if x == 0, returns 0

void assert(bool b, char *msg); // If not true, prints the error message, and exits


