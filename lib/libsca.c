// This implements some of the main publicly-facing functions for the library.

// Imports
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "symbols.h"
#include "utils.h"
#include "mem.h"
#include "config.h"
#include "stats.h"


// ============================= Library Setup ============================== //
int PF(init)()
{
    // attempt to retrieve the L1 CPU data cache size
    long l1d_size = sysconf(_SC_LEVEL1_DCACHE_SIZE);
    if (l1d_size < 0)
    { return errno; }

    // attempt to retrieve the L1 CPU data cache associativity
    long l1d_assoc = sysconf(_SC_LEVEL1_DCACHE_ASSOC);
    if (l1d_assoc < 0)
    { return errno; }
    
    // attempt to retrieve the L1 CPU data cache line size
    long l1d_lsize = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (l1d_lsize < 0)
    { return errno; }

    // with these fields, update the initial library config state
    PS(config_t)* conf = PF(config_get)();
    conf->cache_size = l1d_size;
    conf->cache_associativity = l1d_assoc;
    conf->cache_line_size = l1d_lsize;

    return 0;
}


// ======================= Cache Timing Measurements ======================== //
unsigned long PF(flush)(void* addr)
{
    return LF(mem_flush)(addr);
}

unsigned long PF(flush_write)(void* addr, char new_value)
{
    return LF(mem_flush_overwrite)(addr, new_value);
}


// ======================= Cache Timing Measurements ======================== //
unsigned long PF(cycles)()
{ return LF(mem_cycles)(); }

unsigned long PF(load)(void* src, char* byte)
{ return LF(mem_load_cycles)(src, byte); }

unsigned long PF(store)(void* dst, char byte)
{ return LF(mem_store_cycles)(dst, byte); }

PE(result_e) PF(collect_timing)(unsigned int trials,
                                PS(dataset_t)* hits,
                                PS(dataset_t)* misses,
                                void (*callback)(unsigned long, unsigned long))
{
    // don't accept 0 as an input for number of trials
    if (trials == 0)
    { return LIBSCA_INVALID_INPUT; }

    // set up a memory region to play with during this measurement
    PS(config_t)* conf = PF(config_get)();
    size_t mem_size_lines = 256;
    void* mem = LF(mem_alloc_lines)(mem_size_lines);
    if (!mem)
    { return LIBSCA_ALLOC_FAILURE; }
    
    // set up datasets for recording data (this allocates a lot of memory at
    // once...)
    PF(dataset_init)(hits, mem_size_lines * trials);
    PF(dataset_init)(misses, mem_size_lines * trials);

    // perform the same trial several times
    for (unsigned int t = 0; t < trials; t++)
    {
        // first, flush all cache lines within the playground region
        for (size_t i = 0; i < mem_size_lines; i++)
        {
            void* addr = ((char*) mem) + (i * conf->cache_line_size);
            LF(mem_flush_overwrite(addr, 0x00));
        }

        // next, load all lines twice - once to measure thie miss time, another
        // to measure the hit time
        for (size_t i = 0; i < mem_size_lines; i++)
        {
            void* addr = ((char*) mem) + (i * conf->cache_line_size);

            // measure the miss access time
            unsigned long miss_cycles = PF(load)(addr, NULL);
            PF(dataset_add)(misses, miss_cycles);

            // measure the hit access time
            unsigned long hit_cycles = PF(load)(addr, NULL);
            PF(dataset_add)(hits, hit_cycles);

            // if a callback function was given, invoke that now
            if (callback)
            { callback(hit_cycles, miss_cycles); }
            
            // yield the processor to some other process (delay the next trial)
            PF(yield)();
        }

        // in between trials, yield the processor to add some delay
        PF(yield)();
    }
    
    // free the memory playground region and return
    free(mem);
    return LIBSCA_SUCCESS;
}

unsigned long PF(calculate_threshold)(PS(dataset_t)* hits,
                                      PS(dataset_t)* misses)
{
    if (hits->size == 0 || misses->size == 0)
    { return 0; }

    // compute medians for hits and misses, then free memory
    unsigned long hit_med = PF(dataset_median)(hits);
    unsigned long miss_med = PF(dataset_median)(misses);
    
    // if the miss median and hit median are very close together, we'll examine
    // the miss average instead
    unsigned long hit_cmp = hit_med;
    unsigned long miss_cmp = miss_med;
    if (miss_med - hit_med <= 8)
    { miss_cmp = PF(dataset_average)(misses); }

    // if the miss comparision value is LOWER than the hit comparision value,
    // we'll consider this a fluke and default to the hit median
    if (miss_cmp <= hit_cmp)
    { return hit_cmp; }

    // otherwise, compute a threshold that's closer in value to the hit
    // comparator
    unsigned long diff = (miss_cmp - hit_cmp) / 4;
    return hit_med + diff;
}

int PF(addr_collision_trial)(void* addr1, void* addr2,
                             unsigned long threshold,
                             unsigned int trials)
{
    // if trials is zero, return immediately
    if (trials == 0)
    { return 0; }

    unsigned int hits = 0;

    // flush both addresses from the cache before starting trials
    LF(mem_flush)(addr1);
    LF(mem_flush)(addr2);

    // perform a number of trials
    for (unsigned int t = 0; t < trials; t++)
    {
        // choose the first address randonly
        void* addrs[2] = {addr1, addr2};
        int idx = PF(rand_int)(0, 2);

        // perform both loads (the first will populate the cache, and the second
        // will either populate a DIFFERENT cache line or reference the same
        // cache line populated during the first load)
        LF(mem_load_cycles)(addrs[idx], NULL);
        unsigned long cycles2 = LF(mem_load_cycles)(addrs[(idx + 1) % 2], NULL);
        if (cycles2 <= threshold)
        { hits++; }

        // flush the two addresses and yield the processor before the next trial
        LF(mem_flush)(addr1);
        LF(mem_flush)(addr2);
        PF(yield)();
    }

    // return 1 if we got enough hits to be confident in a collision
    double rate = (double) hits / (double) trials;
    PS(config_t)* conf = PF(config_get)();
    return rate >= conf->addr_collision_trial_score ? 1 : 0;
}


// ============================ Cache Arithmetic ============================ //
size_t PF(addr_line_size)()
{
    // the "line offset" of an address is used to point to a specific byte
    // within a CPU cache line. It's calculated simply by finding the log base 2
    // of the cache line size (since we need that many bits to represent every
    // possible byte in the line)
    PS(config_t)* conf = PF(config_get)();
    return (size_t) LF(log2)(conf->cache_line_size - 1);
}

size_t PF(addr_set_size)()
{
    // the "set index" dictates what cache set a given address is placed into in
    // the CPU cache. It requires 2^S bits of a memory address, where S
    // represents the number of sets in the cache. So, to find the number of
    // bits required, we first must compute the number of cache sets:
    PS(config_t)* conf = PF(config_get)();
    size_t lines = conf->cache_size / conf->cache_line_size;
    size_t sets = lines / conf->cache_associativity;

    // with this value, we can simply take the log base 2 to determine how many
    // bits it would take to represent the number of available sets
    return (size_t) LF(log2)(sets - 1);
}

size_t PF(addr_tag_size)()
{
    // the "tag" of an address is comprised all all the remaining bits that
    // aren't a part of the "set index" or "line offset". It's used to determine
    // if a cache line in the correct set is valid for a cache lookup. We can
    // calculate this simply by subtracting the other two metrics' sizes from
    // the size of this system's memory addresses
    size_t addrsize = sizeof(void*) * 8;
    size_t set_index_size = PF(addr_set_size)();
    size_t line_offset_size = PF(addr_line_size)();
    return addrsize - (set_index_size + line_offset_size);
}

long PF(addr_line_bits)(void* addr)
{
    // build a bitmask that places 1s on the bits we want to extract (the low
    // bits of the address)
    size_t s = PF(addr_line_size)();
    size_t mask = LF(bitmask)(0, s - 1);

    // AND with the mask and return
    return (int) ((size_t) addr & mask);
}

long PF(addr_set_bits)(void* addr)
{
    // build a bitmask that places 1s on the bits we want to extract (the middle
    // bits between the line offset and tag)
    size_t size = PF(addr_set_size)();
    size_t los = PF(addr_line_size)();
    size_t mask = LF(bitmask)(los, los + (size - 1));

    // AND with the mask, shift down, and return
    return (int) (((size_t) addr & mask) >> los);
}

long PF(addr_tag_bits)(void* addr)
{
    // build a bitmask that places 1s on the bits we want to extract (the high
    // bits after the set index)
    size_t size = PF(addr_tag_size)();
    size_t los = PF(addr_line_size)();
    size_t sis = PF(addr_set_size)();
    size_t shift = los + sis;
    size_t mask = LF(bitmask)(shift, shift + size - 1);

    // AND with the mask, shift down, and return
    return (long) (((size_t) addr & mask) >> shift);
}

int PF(addr_collision_check)(void* addr1, void* addr2)
{
    // two addresses will collide in the cache if they have the same set index
    // and the same tag
    long tag1 = PF(addr_tag_bits)(addr1);
    long tag2 = PF(addr_tag_bits)(addr2);
    if (tag1 != tag2)
    { return 0; }

    long set1 = PF(addr_set_bits)(addr1);
    long set2 = PF(addr_set_bits)(addr2);
    if (set1 != set2)
    { return 0; }

    return 1;
}


