// This header file defines the publicly-facing functions available in this
// side-channel attack library.
//
//      Connor Shugg

#ifndef LIBSCA_H
#define LIBSCA_H

// Imports
#include <unistd.h>
#include "symbols.h"
#include "config.h"
#include "stats.h"
#include "utils.h"


// ============================= Library Setup ============================== //
// Initializes the library. Returns 0 on success or an error number on failure.
int PF(init)();


// ====================== Cache Maintenance Operations ====================== //
// Flushes a given address from the CPU caches Returns the number of CPU clock
// cycles it took to perform the flush.
unsigned long PF(flush)(void* addr);

// Flushes a given address from the CPU caches while also writing a given byte
// into the address' location prior to flushing. Returns the number of CPU clock
// cycles it took to perform the flush.
unsigned long PF(flush_write)(void* addr, char new_value);


// ======================= Cache Timing Measurements ======================== //
// Retrieves the current processor cycle count and returns it.
unsigned long PF(cycles)();

// Loads a single byte of memory from 'src' into '*byte'. The number of CPU
// clock cycles the load takes is recorded and returned.
// If 'byte' is NULL, it won't be touched.
unsigned long PF(load)(void* src, char* byte);

// Stores a single byte ('byte') into the '*dst'. The number of CPU clock cycles
// the store takes is recorded and returned.
unsigned long PF(store)(void* dst, char byte);

// Performs a large number of memory accesses and cache flushes to measure and
// return statistics on cache hit/miss timing.
// The 'trials' parameter indicates the number of trials to perform. The more
// trials, the more accurrate the results. If zero is specified, this function
// returns immediately.
// The 'hits' and 'misses' parameters are used to store the resulting data
// collection. Each dataset will contain a number of measurements (in CPU
// cycles) for cache hits and cache misses. The dataset API can be used to
// compute statistics on this data.
// The caller is responsible for invoking dataset_free() on each of the returned
// datasets.
// The 'callback' function, if non-NULL, will be invoked each time a measurement
// is made. The hit time and miss time will be passed into this function. This
// may be useful for those that want a real-time update of each measurement
// being made.
PE(result_e) PF(collect_timing)(unsigned int trials,
                                PS(dataset_t)* hits,
                                PS(dataset_t)* misses,
                                void (*callback)(unsigned long, unsigned long));

// Takes in datasets of cache hit and cache miss times (such as the ones
// returned from collect_timing()) and estimates a threshold to use when determining
// if a timed memory load was a cache hit or not.
// The estimated value is returned. If one or both of the datasets are empty, 0
// is returned.
unsigned long PF(calculate_threshold)(PS(dataset_t)* hits,
                                      PS(dataset_t)* misses);

// Examines two addresses and performs a number of trials to determine if the
// two addresses collide in the CPU cache.
// Returns 1 if they are believed to collide, and 0 if not.
int PF(addr_collision_trial)(void* addr1, void* addr2,
                             unsigned long threshold,
                             unsigned int trials);


// ============================ Cache Arithmetic ============================ //
// Computes and returns the number of bits required to represent the cache line
// offset for a memory address, based on the library's config fields.
size_t PF(addr_line_size)();

// Computes and returns the number of bits required to represent the cache set
// index for a memory address, based on the library's config fields.
size_t PF(addr_set_size)();

// Computes and returns the number of bits required to represent the cache tag
// for a memory address, based on the library's config fields.
size_t PF(addr_tag_size)();

// Extracts and returns the bits from the given address that represent the CPU
// cache line offset.
long PF(addr_line_bits)(void* addr);

// Extracts and returns the bits from the given address that represent the CPU
// cache set index.
long PF(addr_set_bits)(void* addr);

// Extracts and returns the bits from the given address that represent the CPU
// cache tag.
long PF(addr_tag_bits)(void* addr);

// Examines two addresses and uses arithmetic to determine if the two addresses
// will collide in the CPU cache.
// Returns 1 if they collide, and 0 if not.
int PF(addr_collision_check)(void* addr1, void* addr2);

#endif

