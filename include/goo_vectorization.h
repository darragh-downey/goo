#ifndef GOO_VECTORIZATION_H
#define GOO_VECTORIZATION_H

/**
 * SIMD Vectorization Interface for Goo
 * This header is now a backwards compatibility layer that includes
 * the new header structure.
 */

// Include the new vectorization interface header
#include "goo/runtime/vectorization.h"

// Define a macro to indicate this is a compatibility layer
#define GOO_VECTORIZATION_LEGACY_COMPAT 1

#endif // GOO_VECTORIZATION_H 