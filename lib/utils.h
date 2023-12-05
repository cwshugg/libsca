// This module defines a number of general-purpose helper functions.

#ifndef LIBSCA_UTILS_H
#define LIBSCA_UTILS_H

// Imports
#include "symbols.h"
#include "error.h"


// ========================= Comparisons/Arithmetic ========================= //
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Compute log base 2 of a given integer.
int LF(log2)(int x);

// Creates and returns a bitmask from the 'low' bit to the 'high' bit.
long LF(bitmask)(long low, long high);


// =========================== Random Generation ============================ //
// Seeds the random number generator.
void PF(rand_seed)(unsigned int seed);

// Generates a random integer in the given range. The 'lower' is inclusive, and
// the 'upper' is exclusive.
int PF(rand_int)(int lower, int upper);

// Sleep for a random duration of microseconds between 'low' and 'high'. Useful
// for adding a little delay to operations to shake things up.
void PF(rand_usleep)(int low, int high);


// ============================= String Helpers ============================= //
// Attempts to parse an integer from the given string and write the result into
// *result. Returns a result enum.
PE(result_e) LF(str_to_int)(char* text, int* result);


// ============================= Other Helpers ============================== //
// Yields the processor, temporarily removing the calling processor from the
// running state. Useful for giving other programs on the system time to run.
void PF(yield)(void);

// Takes in a memory address and a size (in bits) and creates a heap-allocated
// string representing the big-endian-ordered binary stored at the address.
// The caller must free the given string.
char* PF(binary_string)(void* addr, size_t size_bits);

#endif

