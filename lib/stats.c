// Implements stats.h functions.
//
//      Connor Shugg

// Imports
#include <stdlib.h>
#include <string.h>

// Local imports
#include "stats.h"
#include "utils.h"


// ================================ Datasets ================================ //
int PF(dataset_init)(PS(dataset_t)* ds, size_t initial_size)
{
    ds->data = malloc(initial_size * sizeof(long));
    if (!ds->data)
    { return LIBSCA_ALLOC_FAILURE; }

    ds->size = 0;
    ds->capacity = initial_size;
    return LIBSCA_SUCCESS;
}

void PF(dataset_reset)(PS(dataset_t)* ds)
{ ds->size = 0; }

void PF(dataset_free)(PS(dataset_t)* ds)
{
    free(ds->data);
    ds->size = 0;
    ds->capacity = 0;
}

int PF(dataset_add)(PS(dataset_t)* ds, long value)
{
    // if there's room, add and return
    if (ds->capacity > 0 && ds->size < ds->capacity)
    {
        ds->data[ds->size++] = value;
        return LIBSCA_SUCCESS;
    }

    // otherwise, reallocate the buffer, roughly doubling in size
    size_t new_cap = (ds->capacity + 1) * 2;
    ds->data = realloc(ds->data, new_cap * sizeof(long));
    if (!ds->data)
    { return LIBSCA_ALLOC_FAILURE; }

    // update capacity and add
    ds->capacity = new_cap;
    ds->data[ds->size++] = value;
    return LIBSCA_SUCCESS;
}

ssize_t PF(dataset_find)(PS(dataset_t)* ds, long value)
{
    for (ssize_t i = 0; i < ds->size; i++)
    {
        if (ds->data[i] == value)
        { return i; }
    }
    return -1;
}

// Helper function used in qsort() to compare two dataset values.
static int LF(dataset_sort_cmp)(const void* a, const void* b)
{
    long ia = *((long*) a);
    long ib = *((long*) b);

    // we'll manually compare cases so we don't have issues with long-to-int
    // conversions
    if (ia == ib) { return 0; }
    return ia < ib ? -1 : 1;
}

void PF(dataset_sort)(PS(dataset_t)* ds)
{
    qsort(ds->data, ds->size, sizeof(long), LF(dataset_sort_cmp));
}

long PF(dataset_min)(PS(dataset_t)* ds)
{
    if (ds->size == 0)
    { return 0; }

    long result = ds->data[0];
    for (size_t i = 0; i < ds->size; i++)
    {
        if (ds->data[i] < result)
        { result = ds->data[i]; }
    }
    return result;
}

long PF(dataset_max)(PS(dataset_t)* ds)
{
    if (ds->size == 0)
    { return 0; }

    long result = ds->data[0];
    for (size_t i = 0; i < ds->size; i++)
    {
        if (ds->data[i] > result)
        { result = ds->data[i]; }
    }
    return result;
}

long PF(dataset_average)(PS(dataset_t)* ds)
{
    if (ds->size == 0)
    { return 0; }

    long result = 0;
    for (size_t i = 0; i < ds->size; i++)
    { result += ds->data[i]; }
    return result / ds->size;
}

long PF(dataset_median)(PS(dataset_t)* ds)
{
    if (ds->size == 0)
    { return 0; }

    // create a copy of the dataset's array and sort it
    long* copy = malloc(ds->size * sizeof(long));
    memcpy(copy, ds->data, ds->size * sizeof(long));
    qsort(copy, ds->size, sizeof(long), LF(dataset_sort_cmp));
    
    // compute the middle index and return it
    size_t mid = ds->size / 2;
    long result = copy[mid];
    free(copy);
    return result;
}


// ============================== Counter Sets ============================== //
int PF(countset_init)(PS(countset_t)* cs, size_t initial_size)
{
    cs->data = malloc(initial_size * sizeof(PS(countset_elem_t)));
    if (!cs->data)
    { return LIBSCA_ALLOC_FAILURE; }

    cs->size = 0;
    cs->capacity = initial_size;
    return LIBSCA_SUCCESS;
}

void PF(countset_reset)(PS(countset_t)* cs)
{ cs->size = 0; }

void PF(countset_free)(PS(countset_t)* cs)
{
    free(cs->data);
    cs->size = 0;
    cs->capacity = 0;
}

PE(result_e) PF(countset_add)(PS(countset_t)* cs, long value)
{
    // get the existing value, if it exists (otherwise, choose 1)
    unsigned long count = 1;
    PS(countset_elem_t)* e = PF(countset_find)(cs, value);
    count = e ? e->count + 1 : count;
    
    // set the value
    return PF(countset_set)(cs, value, count);
}

PE(result_e) PF(countset_set)(PS(countset_t)* cs, long value, unsigned long count)
{
    // if the value already exists, set it and return
    PS(countset_elem_t)* e = PF(countset_find)(cs, value);
    if (e)
    {
        e->count = count;
        return LIBSCA_SUCCESS;
    }

    // if there's room, add, set, and return
    if (cs->capacity > 0 && cs->size < cs->capacity)
    {
        e = &cs->data[cs->size++];
        e->value = value;
        e->count = count;
        return LIBSCA_SUCCESS;
    }

    // otherwise, reallocate the buffer, roughly doubling in size
    size_t new_cap = (cs->capacity + 1) * 2;
    cs->data = realloc(cs->data, new_cap * sizeof(PS(countset_elem_t)));
    if (!cs->data)
    { return LIBSCA_ALLOC_FAILURE; }

    // update capacity and add
    cs->capacity = new_cap;
    e = &cs->data[cs->size++];
    e->value = value;
    e->count = count;
    return LIBSCA_SUCCESS;
}

PS(countset_elem_t)* PF(countset_find)(PS(countset_t)* cs, long value)
{
    for (size_t i = 0; i < cs->size; i++)
    {
        if (cs->data[i].value == value)
        { return &cs->data[i]; }
    }
    return NULL;
}

PS(countset_elem_t)* PF(countset_highest)(PS(countset_t)* cs)
{
    PS(countset_elem_t)* max = NULL;
    for (size_t i = 0; i < cs->size; i++)
    {
        // update 'max' if it's NULL or the current element has a higher count
        PS(countset_elem_t)* e = &cs->data[i];
        if (!max || e->count > max->count)
        { max = e; }
    }
    return max;
}

PS(countset_elem_t)* PF(countset_lowest)(PS(countset_t)* cs)
{
    PS(countset_elem_t)* min = NULL;
    for (size_t i = 0; i < cs->size; i++)
    {
        // update 'min' if it's NULL or the current element has a lower count
        PS(countset_elem_t)* e = &cs->data[i];
        if (!min || e->count < min->count)
        { min = e; }
    }
    return min;
}

