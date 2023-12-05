// This implements the prototypes defined in utils.c.
//
//      Connor Shugg

// Imports
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sched.h>

// Local imports
#include "utils.h"
#include "config.h"
#include "mem.h"


// ========================= Comparisons/Arithmetic ========================= //
// Log base 2.
int LF(log2)(int x)
{
    int shift = 0;
    while (x >> shift)
    { shift++; }
    return shift;
}

long LF(bitmask)(long low, long high)
{
    long size = (high - low) + 1;

    // set the lowest N bits equal to 1s (where N = 'size')
    long mask = 0;
    for (long i = 0; i < size; i++)
    {
        mask = (mask << 1) | 0x1;
    }

    // depending on the starting low bit, shift everything left more
    mask <<= low;
    return mask;
}


// =========================== Random Generation ============================ //
// RNG seeder.
void PF(rand_seed)(unsigned int seed)
{ srand(seed); }

// Random range.
int PF(rand_int)(int lower, int upper)
{ return (rand() % (upper - lower)) + lower; }

// Random usleep.
void PF(rand_usleep)(int low, int high)
{
    int sleep_time = PF(rand_int)(low, high);
    usleep(sleep_time);
}


// ============================= String Helpers ============================= //
// Integer parser.
PE(result_e) LF(str_to_int)(char* text, int* result)
{
    // determine what base to use based on if the "0x" prefix is present
    int base = 10;
    if (!strncmp(text, "0x", 2) || !strncmp(text, "0X", 2))
    { base = 16; }

    // attempt to extract the number
    char* endptr;
    int num = strtol(text, &endptr, base);

    // if a number couldn't be parsed, return non-zero
    if (endptr == text && num == 0)
    { return LIBSCA_FAILURE; }

    // write into 'result' and return 0 to indicate success
    *result = num;
    return LIBSCA_SUCCESS;
}

// ============================= Other Helpers ============================== //
void PF(yield)(void)
{
    sched_yield();
}

char* PF(binary_string)(void* addr, size_t size_bits)
{
    // allocate an appropriate size
    char* str = LF(mem_alloc_bytes)(size_bits);
    if (!str)
    { return NULL; }
    
    size_t count = 0;
    while (count < size_bits)
    {
        uint8_t* byte = ((uint8_t*) addr) + (count / 8);
        uint8_t shift = count % 8;
        str[(size_bits - 1) - count++] = (*byte >> shift) & 0x1 ? '1' : '0';
    }
    return str;
}

