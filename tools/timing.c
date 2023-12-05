// This program implements a utility to measure memory access times (in cycles)
// with and without valid cache entries. This is a good first step into taking
// advantage of cache side-channels: learning how fast cached vs. non-cached
// accesses are so we can detect when a memory address is in the CPU cache or
// not.
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
static int trials = 100;
static int show_summary = 1;
static int show_table = 0;
static int show_csv = 0;

// Test memory regions
#define MEM_BLOCK_SIZE 4096
#define MEM_BLOCK_COUNT 256
static uint8_t mem[MEM_BLOCK_COUNT * MEM_BLOCK_SIZE];


// ================================= Timing ================================= //
// Flushes all cache lines from the memory region we're using.
static void flush_all()
{
    for (int i = 0; i < MEM_BLOCK_COUNT; i++)
    { sca_flush_write(mem + (i * MEM_BLOCK_SIZE), 0xff); }
}

// Performs a number of memory accesses and computes a threshold from which we
// can determine if a memory access referenced a CPU cache or not. The computed
// threshold value (which is a number of CPU cycles) is returned.
static void measure(int trials)
{
    sca_dataset_t cache_misses;
    sca_dataset_t cache_hits;
    sca_dataset_t overall_miss_medians;
    sca_dataset_t overall_hit_medians;
    sca_dataset_init(&cache_misses, MEM_BLOCK_COUNT);
    sca_dataset_init(&cache_hits, MEM_BLOCK_COUNT);
    sca_dataset_init(&overall_miss_medians, trials);
    sca_dataset_init(&overall_hit_medians, trials);
    
    // print table/CSV header
    if (show_table || show_csv)
    {
        char* format = show_table ?
                       "%14s %14s %14s %14s %14s %14s %14s %14s %14s\n" :
                       "%s,%s,%s,%s,%s,%s,%s,%s,%s\n";
        printf(format, "Trial",
               "Miss Min", "Miss Max", "Miss Avg", "Miss Median",
               "Hit Min", "Hit Max", "Hit Avg", "Hit Median");
    }

    // repeat for the number of trials specified
    for (int t = 0; t < trials; t++)
    {
        // flush *all* cache lines in the memory region before beginning our
        // measurements
        flush_all();
        
        // add unpredictability by sleeping for a number of microseconds
        sca_rand_usleep(1000, 100000);
        
        // iterate through all cache lines
        for (int i = 0; i < MEM_BLOCK_COUNT; i++)
        {
            void* addr = mem + (i * MEM_BLOCK_SIZE);
            
            // CACHE MISS: load once (to fill up the cache) and record the time
            uint64_t cycles = sca_load(addr, NULL);
            sca_dataset_add(&cache_misses, (int64_t) cycles);

            // CACHE HIT: load again (with cache already full) and record
            cycles = sca_load(addr, NULL);
            sca_dataset_add(&cache_hits, (int64_t) cycles);
 
            // add more unpredictability by sleeping a short time
            sca_rand_usleep(10, 100);
        }

        // compute statistics on the collected data
        int64_t miss_min = sca_dataset_min(&cache_misses);
        int64_t miss_max = sca_dataset_max(&cache_misses);
        int64_t miss_avg = sca_dataset_average(&cache_misses);
        int64_t miss_med = sca_dataset_median(&cache_misses);
        int64_t hit_min = sca_dataset_min(&cache_hits);
        int64_t hit_max = sca_dataset_max(&cache_hits);
        int64_t hit_avg = sca_dataset_average(&cache_hits);
        int64_t hit_med = sca_dataset_median(&cache_hits);
        
        if (show_table || show_csv)
        {
            char* format = show_table ?
                           "%14d %14d %14d %14d %14d %14d %14d %14d %14d\n" :
                           "%d,%d,%d,%d,%d,%d,%d,%d,%d\n";

            // print statistics in a table-like format
            printf(format, (t + 1),
                   miss_min, miss_max, miss_avg, miss_med,
                   hit_min, hit_max, hit_avg, hit_med);
        }
        else if (show_summary)
        {
            // print a status line
            printf("\rTrial %06d/%06d "
                   "[Miss Avg: %05ld] [Miss Median: %05ld] "
                   "[Hit Avg: %05ld] [Hit Median: %05ld]%s",
                   (t + 1), trials,
                   miss_avg, miss_med,
                   hit_avg, hit_med,
                   t == trials - 1 ? "\n" : "");
        }
        fflush(stdout);

        // reset datasets for the next trial
        sca_dataset_reset(&cache_misses);
        sca_dataset_reset(&cache_hits);

        // add to overall datasets
        sca_dataset_add(&overall_miss_medians, miss_med);
        sca_dataset_add(&overall_hit_medians, hit_med);
    }

    // print overall statistics
    if (show_summary)
    {
        int64_t overall_miss_median_avg = sca_dataset_average(&overall_miss_medians);
        int64_t overall_hit_median_avg = sca_dataset_average(&overall_hit_medians);
        printf("%-32s %ld cycles\n", "Cache Miss Time:", overall_miss_median_avg);
        printf("%-32s %ld cycles\n", "Cache Hit Time:", overall_hit_median_avg);
    }

    // free dataset memory
    sca_dataset_free(&cache_misses);
    sca_dataset_free(&cache_hits);
    sca_dataset_free(&overall_miss_medians);
    sca_dataset_free(&overall_hit_medians);
}


// ========================== Command-Line Options ========================== //
// Parses command-line arguments and updates globals accordingly.
static void args_parse(int argc, char** argv)
{
    // set up command-line options
    static struct option opts[] = {
        {"help",        no_argument,        NULL,   0},
        {"trials",      required_argument,  NULL,   0},
        {"show-table",  no_argument,        NULL,   0},
        {"show-csv",    no_argument,        NULL,   0},
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
        else if (!strcmp(opt->name, "trials"))
        {
            int result = LF(str_to_int)(optarg, &trials);
            if (result || trials <= 0)
            {
                fprintf(stderr, "You must specify a positive, non-zero integer for --trials.");
                exit(EXIT_FAILURE);
            }
        }
        else if (!strcmp(opt->name, "show-table"))
        {
            show_table = 1;
            show_summary = 0;
        }
        else if (!strcmp(opt->name, "show-csv"))
        {
            show_csv = 1;
            show_summary = 0;
        }
    }
    return;
    
    // prints out a usage menu and exits the program
    args_parse_usage:
    printf("Cache Timing Measurement Utility\n");
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Use this to measure the number of CPU cycles memory accesses take on your machine.\n"
           "This tool measures for both cache hits and misses.\n\n");

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
    int result = sca_init();
    if (result)
    {
        printf("Library failed to initialize: %d\n", result);
        return result;
    }

    // parse command-line arguments
    args_parse(argc, argv);
    
    // seed the random generator and initialize data collection
    sca_rand_seed(time(NULL));

    // perform the actual measurement and dump results
    measure(trials);
}

