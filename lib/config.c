// This implements function prototypes in config.h and defines the internal
// global config instance.

// Imports
#include "config.h"
#include "utils.h"

// Global config struct
PS(config_t) LG(config) = {
    .cache_size = 49152,
    .cache_associativity = 12,
    .cache_line_size = 64,
    .addr_collision_trial_score = 0.95
};

PS(config_t)* PF(config_get)()
{
    return &LG(config);
}

