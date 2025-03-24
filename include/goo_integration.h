#ifndef GOO_INTEGRATION_H
#define GOO_INTEGRATION_H

/**
 * Goo Runtime Integration
 * 
 * This header defines functions for integrating various components
 * of the Goo compiler and runtime system.
 */

#include <stdbool.h>
#include <stddef.h>
#include "goo_core.h"
#include "goo_memory.h"
#include "goo_vectorization.h"

#ifdef __cplusplus
extern "C" {
#endif

// Runtime integration status
bool goo_runtime_integration_init(void);
void goo_runtime_integration_cleanup(void);

// Zig runtime integration
bool goo_initialize_zig_runtime(void);
void goo_cleanup_zig_runtime(void);

// SIMD vectorization using Zig implementation
bool goo_perform_vector_operation(GooVectorOp op, 
                                void* src1, 
                                void* src2, 
                                void* dst, 
                                size_t elem_size, 
                                size_t length, 
                                GooVectorDataType data_type);

// Aligned memory management for SIMD operations
void* goo_allocate_simd_buffer(size_t size);
void goo_free_simd_buffer(void* ptr, size_t size);

// Testing functions
bool goo_test_zig_integration(void);
int goo_run_zig_tests(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_INTEGRATION_H 