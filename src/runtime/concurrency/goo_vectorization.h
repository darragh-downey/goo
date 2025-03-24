/**
 * goo_vectorization.h
 * 
 * SIMD vectorization support for the Goo programming language.
 * Provides portable abstractions over various SIMD instruction sets.
 */

#ifndef GOO_VECTORIZATION_H
#define GOO_VECTORIZATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "goo_core.h"

// Basic vector operation structure
typedef struct {
    void *src1;          // First source buffer
    void *src2;          // Second source buffer (optional for some ops)
    void *dst;           // Destination buffer
    size_t elem_size;    // Size of each element in bytes
    size_t length;       // Number of elements to process
    GooVectorOp op;      // Operation to perform
    void (*custom_op)(void*, void*, void*, size_t); // Custom operation function
} GooVector;

// Loop parallelism types
typedef enum {
    GOO_SCHEDULE_AUTO,   // Automatically determine schedule
    GOO_SCHEDULE_STATIC, // Static work distribution
    GOO_SCHEDULE_DYNAMIC,// Dynamic work distribution
    GOO_SCHEDULE_GUIDED  // Guided work distribution
} GooScheduleType;

// Parallel loop structure
typedef struct {
    bool vectorize;      // Whether to try vectorization
    size_t start;        // Loop start index
    size_t end;          // Loop end index
    size_t step;         // Loop step size
    void *context;       // User context
    void (*body)(size_t, void*); // Loop body function
    GooScheduleType schedule; // Scheduling strategy
    int chunk_size;      // Chunk size for work distribution
    int num_threads;     // Number of threads
} GooParallelLoop;

typedef struct GooVectorMask {
    void *mask_data;           // Mask data
    size_t mask_size;          // Size of mask in bytes
    GooVectorDataType type;    // Mask data type
} GooVectorMask;

typedef struct GooVectorOperation {
    GooVector base;           // Base vector operation
    GooSIMDType simd_type;    // SIMD instruction set to use
    GooVectorDataType data_type; // Data type of vector elements
    GooVectorMask *mask;      // Optional mask for masked operations
    bool aligned;             // Whether data is aligned for SIMD
} GooVectorOperation;

/**
 * Initialize the vectorization subsystem.
 * 
 * @param simd_type SIMD instruction set to use
 * @return true if successful, false otherwise
 */
bool goo_vectorization_init(GooSIMDType simd_type);

/**
 * Cleanup the vectorization subsystem.
 */
void goo_vectorization_cleanup(void);

/**
 * Detect the best available SIMD instruction set.
 * 
 * @return The detected SIMD type
 */
GooSIMDType goo_vectorization_detect_simd(void);

/**
 * Execute a simple vector operation.
 * 
 * @param vec_op Vector operation to execute
 * @return true if successful, false otherwise
 */
bool goo_vector_execute(GooVector *vec_op);

/**
 * Execute a vector operation with extended features.
 * 
 * @param vec_op Vector operation to execute
 * @return true if successful, false otherwise
 */
bool goo_vectorization_execute(GooVectorOperation *vec_op);

/**
 * Apply vectorization to a parallel loop.
 * 
 * @param loop Parallel loop to vectorize
 * @param data_type Data type of loop elements
 * @param simd_type SIMD instruction set to use
 * @return true if successful, false otherwise
 */
bool goo_vectorization_apply_to_loop(GooParallelLoop *loop, 
                                    GooVectorDataType data_type,
                                    GooSIMDType simd_type);

/**
 * Create a mask for masked vector operations.
 */
GooVectorMask *goo_vectorization_create_mask(size_t size, GooVectorDataType type);

/**
 * Free a vector mask.
 */
void goo_vectorization_free_mask(GooVectorMask *mask);

/**
 * Set mask values using indices.
 * 
 * @param mask The mask to set
 * @param indices Array of indices to set in the mask
 * @param count Number of indices
 */
bool goo_vectorization_set_mask(GooVectorMask *mask, size_t *indices, size_t count);

/**
 * Check if a vector operation can be accelerated with SIMD.
 * 
 * @param data_type Data type of vector elements
 * @param op Operation to perform
 * @param simd_type SIMD instruction set to use
 * @return true if the operation can be accelerated, false otherwise
 */
bool goo_vectorization_is_accelerated(GooVectorDataType data_type, 
                                     GooVectorOp op,
                                     GooSIMDType simd_type);

/**
 * Get the SIMD vector width for a data type and SIMD type.
 */
size_t goo_vectorization_get_width(GooVectorDataType data_type, GooSIMDType simd_type);

/**
 * Get the required alignment for a SIMD type.
 */
size_t goo_vectorization_get_alignment(GooSIMDType simd_type);

/**
 * Check if a pointer is properly aligned for SIMD operations.
 * 
 * @param ptr Pointer to check
 * @param simd_type SIMD instruction set to use
 * @return true if aligned, false otherwise
 */
bool goo_vectorization_is_aligned(void *ptr, GooSIMDType simd_type);

/**
 * Allocate memory aligned for SIMD operations.
 */
void* goo_vectorization_alloc_aligned(size_t size, GooSIMDType simd_type);

/**
 * Free aligned memory.
 */
void goo_vectorization_free_aligned(void* ptr);

#endif // GOO_VECTORIZATION_H 