// This program implements a simple flush+reload cache attack entirely within
// the same user process.
//
//      Connor Shugg

// Imports
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <libsca.h>

// Globals
static int cache_threshold = 80;     // cache access time
static int seed = 0;            // random seed

// Test memory region
#define MEM_BLOCK_SIZE 4096
#define MEM_BLOCK_COUNT 256
static uint8_t mem[MEM_BLOCK_COUNT * MEM_BLOCK_SIZE];

// Victim/attacker cache line recording
sca_dataset_t victim_secrets;
sca_dataset_t attacker_discoveries;


// ============================== Victim Code =============================== //
// Represents the victim's behavior. Oblivious to the cache flushing/reloading
// done by the attacker, the victim makes a memory access, which leaves a
// footprint in the CPU cache.
static void victim_access()
{
    printf("%-12s Accessing %lu cache lines:\n",
           "VICTIM:", victim_secrets.size);

    // iterate across the chosen secret cache lines the victim will access
    for (size_t i = 0; i < victim_secrets.size; i++)
    {
        // compute an address and perform the access
        int64_t idx = victim_secrets.data[i];
        void* addr = mem + (idx * MEM_BLOCK_SIZE);
        uint64_t cycles = sca_load(addr, NULL);
        int was_cached = cycles <= cache_threshold;

        // log the access
        printf("%-12s Cache line %ld: %lu cycles (%s)\n",
               "", idx, cycles,
               was_cached ? "Already cached" : "Not cached");
    }
}


// ============================= Attacker Code ============================== //
// Flushes all cache lines from 'mem', the shared memory region between the
// attacker and victim.
static void attacker_flush()
{
    for (int i = 0; i < MEM_BLOCK_COUNT; i++)
    { sca_flush_write(mem + (i * MEM_BLOCK_SIZE), 0xff); }

    // log information about the flush
    printf("%-12s Flushed all %d cache lines.\n",
           "ATTACKER:", MEM_BLOCK_COUNT);
}

// Reloads all cache lines from 'mem' (the shared memory region between) and
// determines which ones were present in the CPU cache based on access time.
static void attacker_reload()
{
    printf("%-12s Reloading all %d cache lines:\n",
           "ATTACKER:", MEM_BLOCK_COUNT);
    for (int i = 0; i < MEM_BLOCK_COUNT; i++)
    {
        // compute the correct address and time the load
        void* addr = mem + (i * MEM_BLOCK_SIZE);
        uint64_t cycles = sca_load(addr, NULL);

        // if the cache line was already cached, this must have been accessed
        // by the victim
        int was_cached = cycles <= cache_threshold;
        if (was_cached)
        {
            sca_dataset_add(&attacker_discoveries, (int64_t) i);
            printf("%-12s Cache line %d is in the cache. "
                   "(Accessed in %lu cycles)\n",
                   "", i, cycles);
        }
    }
}

// Function that compares the victim's accesses to the attacker's discoveries
// and highlights any differences.
static void compare(sca_dataset_t* victim, sca_dataset_t* attacker)
{
    sca_dataset_sort(victim);
    sca_dataset_sort(attacker);
    printf("ANALYSIS:\n");

    // iterate through the victim's accesses and look for them in the attacker's
    // discovery set
    size_t matches = 0;
    for (size_t i = 0; i < victim->size; i++)
    {
        ssize_t idx = sca_dataset_find(attacker, victim->data[i]);
        matches += idx >= 0;

        // if the attacker didn't discover this cache line, print it out
        if (idx < 0)
        {
            printf("%-12s The attacker failed to discover cache line %ld.\n",
                   "", victim->data[i]);
        }
    }

    // now iterate through the attacker's discoveries and look for any extra
    // cache lines that WEREN'T accessed by the victim
    for (size_t i = 0; i < attacker->size; i++)
    {
        ssize_t idx = sca_dataset_find(victim, attacker->data[i]);
        if (idx < 0)
        {
            printf("%-12s The attacker found an extra cache line: %ld.\n",
                   "", attacker->data[i]);
        }
    }

    // if all cache lines were discovered by the attacker, print it out
    if (matches == victim->size)
    {
        printf("%-12s The attacker discovered all victim cache lines.\n", "");
    }
    else
    {
        printf("%-12s The attacker discovered %lu/%lu victim cache lines.\n",
               "", matches, victim->size);
    }

}


// ========================== Command-Line Options ========================== //
// Parses command-line arguments and updates globals accordingly.
static void args_parse(int argc, char** argv)
{
    // set up command-line options
    static struct option opts[] = {
        {"help",        no_argument,        NULL,   0},
        {"threshold",   required_argument,  NULL,   0},
        {"seed",        required_argument,  NULL,   0},
        {NULL, 0, NULL, 0}
    };
    int optidx = 0;

    // loop forever until all options are parsed
    while (1)
    {
        // parse the next option and quit on error
        int result = getopt_long_only(argc, argv, "", opts, &optidx);
        if (result == -1)
        { break; }
        if (result != 0)
        { goto args_parse_usage; }

        struct option* opt = &opts[optidx];
        if (!strcmp(opt->name, "help"))
        { goto args_parse_usage; }
        else if (!strcmp(opt->name, "threshold"))
        {
            int result = LF(str_to_int)(optarg, &cache_threshold);
            if (result || cache_threshold <= 0)
            {
                fprintf(stderr, "You must specify a positive, non-zero integer for --threshold.");
                exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp(opt->name, "seed"))
        {
            int result = LF(str_to_int)(optarg, &seed);
            if (result)
            {
                fprintf(stderr, "You must specify an integer for --seed.");
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
    
    // prints out a usage menu and exits the program
    args_parse_usage:
    printf("Flush+Reload Attack Test\n");
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Use this to verify that a cache flush+reload attack is possible on your CPU.\n"
           "This tool performs memory reads within a known region and looks for their CPU cache footprints.\n\n");

    printf("Options:\n");
    struct option* o = &opts[0];
    while (o->name)
    {
        printf("  --%s (-%c)\n", o->name, o->name[0]);
        o++;
    }
    exit(0);
}

// ================================== Main ================================== //
// Main function.
int main(int argc, char** argv)
{
    // parse command-line arguments
    seed = time(NULL);
    args_parse(argc, argv);
    sca_rand_seed(seed);

    // determine a random set of cache lines to have the victim access
    size_t victim_accesses = (size_t) sca_rand_int(1, 9);
    sca_dataset_init(&victim_secrets, victim_accesses);
    for (int i = 0; i < victim_accesses; i++)
    {
        // generate a random cache line index and only insert it into the set if
        // we haven't generated it before
        int64_t idx = (int64_t) sca_rand_int(0, MEM_BLOCK_COUNT);
        if (sca_dataset_find(&victim_secrets, idx) < 0)
        { sca_dataset_add(&victim_secrets, idx); }
    }
    sca_dataset_sort(&victim_secrets);
    sca_dataset_init(&attacker_discoveries, victim_accesses);
    
    // perform the attack
    attacker_flush();       // 1. attacker flushes all relevant cache lines
    victim_access();        // 2. victim accesses memory (one cache line)
    attacker_reload();      // 3. attacker reloads to learn which line

    // compare results and print an analysis
    compare(&victim_secrets, &attacker_discoveries);

    // free memory
    sca_dataset_free(&attacker_discoveries);
    sca_dataset_free(&victim_secrets);
}

