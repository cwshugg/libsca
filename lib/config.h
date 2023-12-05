// This defines a set of configuration object(s)/function(s) to configure the
// library's internal behavior.

#ifndef LIBSCA_CONFIG_H
#define LIBSCA_CONFIG_H

// Imports
#include <stddef.h>
#include "symbols.h"

// This struct represents the global config for the library. If this library is
// ever used by multiple threads at once, this is NOT thread-safe. The config
// fields should be initialized at the start of the program, then left alone as
// a read-only data structure once threads are spawned.
typedef struct LS(config)
{
    size_t cache_size;                  // total number of bytes in the cache
    size_t cache_associativity;         // number of cache lines per set
    size_t cache_line_size;             // size of each cache line (in bytes)
    double addr_collision_trial_score;  // [0.0, 1.0] hit rate required to consider two addresses colliding

} PS(config_t);

// A few notes on CPU caching:
// The CPU cache size is calculated as such:
//
//      SIZE = (S * E * B) bytes
//       - S = the number of sets in the cache
//       - E = associativity: the number of lines in a set
//       - B = the number of bytes in a cache line
//
// So, given the above config fields, we can compute various cache metrics:
//
//      NUM_LINES = (cache_size / cache_line_size)
//      NUM_SETS = (NUM_LINES / cache_associativity)
//
// In Linux systems, one way to find the size of the cache is by running
// `getconf -a` and searching for fields containing the word "CACHE". Example:
//
//      LEVEL1_ICACHE_SIZE                 32768
//      LEVEL1_ICACHE_ASSOC                8
//      LEVEL1_ICACHE_LINESIZE             64
//      LEVEL1_DCACHE_SIZE                 49152
//      LEVEL1_DCACHE_ASSOC                12
//      LEVEL1_DCACHE_LINESIZE             64
//      LEVEL2_CACHE_SIZE                  1310720
//      LEVEL2_CACHE_ASSOC                 20
//      LEVEL2_CACHE_LINESIZE              64
//      LEVEL3_CACHE_SIZE                  12582912
//      LEVEL3_CACHE_ASSOC                 12
//      LEVEL3_CACHE_LINESIZE              64
//      LEVEL4_CACHE_SIZE                  0
//      LEVEL4_CACHE_ASSOC                 0
//      LEVEL4_CACHE_LINESIZE              0

// Returns a pointer to the current library config struct.
PS(config_t)* PF(config_get)();

#endif

