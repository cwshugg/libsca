// Implements the function prototyped in mem.h.
//
//      Connor Shugg

// Standard imports
#include <stdlib.h>

// Local imports
#include "isa.h"
#include "mem.h"
#include "config.h"

// ISA-specific imports
#if (ISA == ISA_X86)
#include <x86intrin.h>
#endif


// =========================== Memory Allocation ============================ //
void* LF(mem_alloc_lines)(size_t size_lines)
{
    PS(config_t)* config = PF(config_get)();
    return malloc(size_lines * config->cache_line_size);
}

void* LF(mem_alloc_bytes)(size_t size_bytes)
{
    return malloc(size_bytes);
}


// =========================== Cache Maintenance ============================ //
unsigned long LF(mem_flush)(void* addr)
{
    unsigned long cycles1 = LF(mem_cycles)();

    #if (ISA == ISA_X86)
    _mm_clflush(addr);
    #else
    #error "Unsupported ISA"
    #endif
    
    unsigned long cycles2 = LF(mem_cycles)();
    return cycles2 - cycles1;
}

unsigned long LF(mem_flush_overwrite)(void* addr, char new_value)
{
    // write to the address to prevent issues with copy-on-write
    *((char*) addr) = new_value;
    return LF(mem_flush)(addr);
}


// ============================= Timed Accesses ============================= //
unsigned long LF(mem_cycles)()
{
    register uint64_t cycles = 0;
    
    // perform ISA-specific cycle retrieval
    #if (ISA == ISA_X86)
    unsigned int tmp = 0;
    cycles = __rdtscp(&tmp);
    #else
    #error "Unsupported ISA"
    #endif

    return (unsigned long) cycles;
}

// Timed load.
unsigned long LF(mem_load_cycles)(void* src, char* byte)
{
    // define a few variables to use for sampling (we specify 'register' to ask
    // the processor to keep the variable in a CPU register, if possible)
    register uint64_t cycles1 = 0;
    register uint64_t cycles2 = 0;

    // take clock-cycle sample 1
    cycles1 = LF(mem_cycles)();

    // perform the memory load
    char val = 0;
    val = *((char*) src);

    // take clock-cycle sample 2
    cycles2 = LF(mem_cycles)();
    
    // write the loaded byte out to the result, if a valid pointer was given
    if (byte)
    { *byte = val; }

    // compute the difference and return it
    return (unsigned long) (cycles2 - cycles1);
}

// Timed store.
unsigned long LF(mem_store_cycles)(void* dst, char byte)
{
    // same idea as 'loadc()' - we'll measure clock cycles before and
    // after a memory story
    register uint64_t cycles1 = 0;
    register uint64_t cycles2 = 0;

    // take clock-cycle sample 1
    cycles1 = LF(mem_cycles)();

    // perform the store
    *((char*) dst) = byte;

    // take clock-cycle sample 2
    cycles2 = LF(mem_cycles)();

    // compute the difference and return it
    return (unsigned long) (cycles2 - cycles1);
}

