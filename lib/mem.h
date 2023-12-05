// This header file defines functions used to make various memory accesses.
// In particular, this is for timing memory accesses using special instructions
// specific to the machine's instruction set.

#ifndef LIBSCA_MEM_H
#define LIBSCA_MEM_H

// Imports
#include <stdint.h>
#include <stddef.h>
#include "symbols.h"
#include "error.h"


// =========================== Memory Allocation ============================ //
// Allocates memory given the number of desired cache lines.
void* LF(mem_alloc_lines)(size_t size_lines);

// Allocates memory given the number of desired bytes.
void* LF(mem_alloc_bytes)(size_t size_bytes);


// =========================== Cache Maintenance ============================ //
// Takes in an address and uses ISA-specific instructions to flush the cache
// line corresponding to the address from the CPU caches.
// Returns the number of clock cycles the flush operation took.
unsigned long LF(mem_flush)(void* addr);

// "Flush W" = "Flush and Write first"
// Performs the same cache-flushing operation as 'mem_flush()', but additionally
// writes the given byte into the address' location before flushing.
// Returns the number of clock cycles the flush operation took.
unsigned long LF(mem_flush_overwrite)(void* addr, char new_value);


// ============================= Timed Accesses ============================= //
// Invokes the ISA-specific instruction(s) to retrieve the current number of
// clock cycles executed. Returns the value as an unsigned long.
unsigned long LF(mem_cycles)();

// Loads a single byte of memory from 'src' into the memory pointed at by
// 'byte'. Uses architecture-specific timing instructions to measure the
// number of clock cycles that occurred during the load and returns the number.
// (If 'byte' is NULL, the loaded value is discarded.)
unsigned long LF(mem_load_cycles)(void* src, char* byte);

// Stores a single byte of memory ('byte') into the memory pointed at by 'dst'.
// Uses architecture-specific timing instructions to measure the number of clock
// cycles that occurred during the store and returns the number.
unsigned long LF(mem_store_cycles)(void* dst, char byte);

#endif

