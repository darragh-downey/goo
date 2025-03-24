/**
 * goo_comptime_simd.h
 * 
 * Definitions for compile-time SIMD features in the Goo programming language.
 * Provides safe, hardware-agnostic SIMD operations through compile-time code generation.
 */

#ifndef GOO_COMPTIME_SIMD_H
#define GOO_COMPTIME_SIMD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "runtime/concurrency/goo_vectorization.h"

/**
 * Represents a SIMD vector type
 */
typedef struct GooVectorType {
    char name[64];              // Type name (e.g., "Float4")
    size_t width;               // Vector width (e.g., 4 for 4 elements)
    size_t alignment;           // Required memory alignment
    GooVectorDataType data_type; // Element data type
    bool is_safe;               // Whether safety checks are enabled
} GooVectorType;

/**
 * Represents a SIMD operation (e.g., add, mul)
 */
typedef struct GooVectorOperation {
    char name[64];              // Operation name (e.g., "Float4Add")
    GooVectorOp op_type;        // Operation type
    char vector_type[64];       // Vector type name this operation applies to
    bool masked;                // Whether this is a masked operation
    bool fused;                 // Whether this is a fused operation (e.g., FMA)
} GooVectorOperation;

/**
 * Represents a compile-time SIMD context
 */
typedef struct GooComptimeSIMD {
    // SIMD types
    GooVectorType *vector_types;
    size_t num_types;
    size_t types_capacity;
    
    // SIMD operations
    GooVectorOperation *vector_ops;
    size_t num_ops;
    size_t ops_capacity;
    
    // Configuration
    bool allow_unsafe;
    bool allow_fallback;
    bool auto_detect;
} GooComptimeSIMD;

/**
 * Create a new SIMD context
 */
GooComptimeSIMD *goo_comptime_simd_create(void);

/**
 * Free a SIMD context
 */
void goo_comptime_simd_free(GooComptimeSIMD *ctx);

/**
 * Add a SIMD type to the context
 */
bool goo_comptime_simd_add_type(
    GooComptimeSIMD *ctx,
    const char *name,
    size_t width,
    GooVectorDataType data_type, 
    size_t alignment,
    bool is_safe
);

/**
 * Add a SIMD operation to the context
 */
bool goo_comptime_simd_add_operation(
    GooComptimeSIMD *ctx,
    const char *name,
    GooVectorOp op_type,
    const char *vector_type,
    bool masked,
    bool fused
);

/**
 * Look up a SIMD type by name
 */
GooVectorType *goo_comptime_simd_get_type(
    GooComptimeSIMD *ctx,
    const char *name
);

/**
 * Look up a SIMD operation by name
 */
GooVectorOperation *goo_comptime_simd_get_operation(
    GooComptimeSIMD *ctx,
    const char *name
);

/**
 * Check if a vector operation is supported on the current hardware
 */
bool goo_comptime_simd_is_supported(
    GooComptimeSIMD *ctx,
    const GooVectorType *type,
    const GooVectorOperation *op
);

#endif // GOO_COMPTIME_SIMD_H 