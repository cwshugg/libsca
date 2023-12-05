// This program implements a utility for analyzing and describing the layout of
// the L1D CPU cache built into the processor on which this runs.
// It uses libsca to read system configurations to understand the cache
// architecture.
//
//      Connor Shugg

// Imports
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <getopt.h>
#include <libsca.h>

// Globals
int do_visual = 0;

// Prints an escape sequence to stdout that positions the cursor in the
// terminal.
static void position_cursor(int x, int y)
{
    printf("\033[%d;%dH", y, x);
}

// Creates a description of the CPU cache and prints it.
static void describe()
{
    // retrieve the library config and compute cache metrics
    sca_config_t* conf = sca_config_get();
    size_t cache_size = conf->cache_size;
    size_t cache_lsize = conf->cache_line_size;
    size_t cache_lcount = cache_size / cache_lsize;
    size_t cache_assoc = conf->cache_associativity;
    size_t cache_scount = cache_lcount / cache_assoc;

    // report the cache info the user
    printf("Your CPU's L1D cache is a ");
    if (cache_assoc > 1)
    { printf("%lu-way set-associative cache ", cache_assoc); }
    else
    { printf("direct-mapped cache "); }
    printf("with %lu total sets, %lu total %lu-byte lines, ",
           cache_scount, cache_lcount, cache_lsize);
    printf("and a total size of %lu bytes.\n", cache_size);
}

// Visualizes the CPU cache.
static void visualize()
{
    // TODO - this is a WIP

    // retrieve the library config and compute cache metrics
    sca_config_t* conf = sca_config_get();
    size_t cache_size = conf->cache_size;
    size_t cache_lsize = conf->cache_line_size;
    size_t cache_lcount = cache_size / cache_lsize;
    size_t cache_assoc = conf->cache_associativity;
    size_t cache_scount = cache_lcount / cache_assoc;
    
    // allocate a memory region as big as the cache
    void* mem = malloc(cache_size);
    if (!mem)
    {
        fprintf(stderr, "Failed to allocate memory: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    void* mem_end = (uint8_t*) mem + cache_size - 1;

    // walk down the memory region, one byte at a time, until we hit a byte
    // whose virtual address uses set index 0 and line offset 0. We'll start
    // from here while iterating
    void* start = NULL;
    for (uint8_t* byte = (uint8_t*) mem; byte < (uint8_t*) mem + cache_size; byte++)
    {
        long set = sca_addr_set_bits(byte);
        long loff = sca_addr_line_bits(byte);
        if (set == 0 && loff == 0)
        { start = byte; }
    }

    while (1)
    {
        // iterate through all cache lines, starting at the starting point
        void* cursor = start;
        for (size_t i = 0; i < cache_lcount; i++)
        {
            // get the addresses set index and line offset
            long tag = sca_addr_tag_bits(cursor);
            long set = sca_addr_set_bits(cursor);
            long loff = sca_addr_line_bits(cursor);
            printf("0x%-20lx 0x%-16lx 0x%-4lx 0x%-4lx\n", (long) cursor, tag, set, loff);

            // compute the next cache line (wrap around if necessary)
            void* next = (uint8_t*) cursor + cache_lsize;
            if (next > mem_end)
            { next = start; }
            cursor = next;
        }

        break;  // DEBUGGING
    }

    // free memory
    free(mem);
}


// ============================ Argument Parsing ============================ //
// Parses command-line arguments and updates globals accordingly.
static void args_parse(int argc, char** argv)
{
    // set up command-line options
    static struct option opts[] = {
        {"help",        no_argument,        NULL,   0},
        {"visual",      no_argument,        NULL,   0},
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
        else if (!strcmp(opt->name, "visual"))
        { do_visual = 1; }
    }
    return;
    
    // prints out a usage menu and exits the program
    args_parse_usage:
    printf("Cache Information Utility\n");
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Use this to learn about the architecture of your CPU cache.\n");
    printf("\n");

    printf("Options:\n");
    struct option* o = &opts[0];
    while (o->name)
    {
        printf("  --%s (-%c)\n", o->name, o->name[0]);
        o++;
    }
    exit(0);
}


// =========================== Main Functionality =========================== //
// Main function.
int main(int argc, char** argv)
{
    // initialize the library and parse arguments
    sca_init(); // initialize the library
    args_parse(argc, argv);

    // if the user asked for a visualization, do it
    if (do_visual)
    {
        visualize();
        return EXIT_SUCCESS;
    }

    // otherwise, describe the cache
    describe();
    return EXIT_SUCCESS;
}

