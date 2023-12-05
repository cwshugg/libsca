// This module defines an enum for tracking internal error codes and operation
// results.

#ifndef LIBSCA_ERROR_H
#define LIBSCA_ERROR_H

// Imports
#include "symbols.h"


// Enum representing any internally-defined error codes.
typedef enum LE(result)
{
    LIBSCA_SUCCESS,         // successful operation
    LIBSCA_FAILURE,         // generic failure
    LIBSCA_INVALID_INPUT,   // invalid input parameter
    LIBSCA_ALLOC_FAILURE,   // failed to allocate memory
    LIBSCA_RESULT_COUNT,    // -------------------------------------------------
    LIBSCA_UNKNOWN,         // unknown result
} PE(result_e);

#endif

