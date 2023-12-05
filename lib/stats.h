// This header file defines structures and prototypes for simple data collection
// and processing.

#ifndef LIBSCA_STATS_H
#define LIBSCA_STATS_H

// Imports
#include "symbols.h"
#include "error.h"


// ================================ Datasets ================================ //
// Essentially a wrapper for an array that comes with a function interface for
// allocating, freeing, adding, and doing mathematical operations.
typedef struct LS(dataset)
{
    long* data;         // dynamically-allocated array
    size_t size;        // current used size
    size_t capacity;    // current capacity
} PS(dataset_t);

// Allocates memory for the dataset given the initial size (in 64-bit integers,
// not in bytes).
int PF(dataset_init)(PS(dataset_t)* ds, size_t initial_size);

// Resets a dataset to allow for memory reuse.
void PF(dataset_reset)(PS(dataset_t)* ds);

// Frees the dataset's memory.
void PF(dataset_free)(PS(dataset_t)* ds);

// Adds an entry to the dataset, increasing capacity if necessary.
int PF(dataset_add)(PS(dataset_t)* ds, long value);

// Searches the dataset for the first occurrence of 'value' and returns it.
// Returns -1 if the value couldn't be found, and the index if it was found.
ssize_t PF(dataset_find)(PS(dataset_t)* ds, long value);

// Sorts the dataset's entries.
void PF(dataset_sort)(PS(dataset_t)* ds);

// Computes the min of the dataset and returns it.
long PF(dataset_min)(PS(dataset_t)* ds);

// Computes the max of the dataset and returns it.
long PF(dataset_max)(PS(dataset_t)* ds);

// Computes the average of the dataset and returns it.
long PF(dataset_average)(PS(dataset_t)* ds);

// Computes the median of the dataset and returns it.
long PF(dataset_median)(PS(dataset_t)* ds);


// ============================== Counter Sets ============================== //
// A data structure used to count the occurrences of certain numbers.

// Individual element.
typedef struct LS(countset_elem)
{
    long value;             // element value
    unsigned long count;    // number of occurrences
} PS(countset_elem_t);

// One countset.
typedef struct LS(countset)
{
    PS(countset_elem_t)* data;  // dynamically-allocated array
    size_t size;            // current used size
    size_t capacity;        // current capacity
} PS(countset_t);

// Initializes the countset to a given initial capacity.
int PF(countset_init)(PS(countset_t)* cs, size_t initial_size);

// Resets a dataset to allow for memory reuse.
void PF(countset_reset)(PS(countset_t)* cs);

// Frees the countset's memory.
void PF(countset_free)(PS(countset_t)* cs);

// Adds the value to the countset if it doesn't already exist (with an initial
// count of 1). Increments the value if it already exists.
// Returns a result enum.
PE(result_e) PF(countset_add)(PS(countset_t)* cs, long value);

// Sets the given value's count to the given count value. Adds it to the set
// if it doesn't already exist.
PE(result_e) PF(countset_set)(PS(countset_t)* cs, long value, unsigned long count);

// Searches the set for the given value and returns a pointer to it if found.
// Returns NULL if not found.
PS(countset_elem_t)* PF(countset_find)(PS(countset_t)* cs, long value);

// Returns the element with the highest counter.
PS(countset_elem_t)* PF(countset_highest)(PS(countset_t)* cs);

// Returns the element with the lowest counter.
PS(countset_elem_t)* PF(countset_lowest)(PS(countset_t)* cs);

#endif

