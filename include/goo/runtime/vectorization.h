#ifndef GOO_RUNTIME_VECTORIZATION_H
#define GOO_RUNTIME_VECTORIZATION_H

/**
 * SIMD Vectorization Interface for Goo
 * This provides a consistent API for SIMD operations
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the vectorization subsystem
 * 
 * @param simd_type The SIMD instruction set to use, or GOO_SIMD_AUTO to detect
 * @return true if initialization was successful
 */
bool goo_vectorization_init(GooSIMDType simd_type);

/**
 * Clean up the vectorization subsystem
 */
void goo_vectorization_cleanup(void);

/**
 * Detect available SIMD instruction sets
 * 
 * @return The best available SIMD type
 */
GooSIMDType goo_vectorization_detect_simd(void);

/**
 * Get the current active SIMD type
 * 
 * @return The currently active SIMD type
 */
GooSIMDType goo_vectorization_get_active_simd(void);

/**
 * Get the required alignment for SIMD operations
 * 
 * @param simd_type The SIMD type to get alignment for
 * @return The required alignment in bytes
 */
size_t goo_vectorization_get_alignment(GooSIMDType simd_type);

/**
 * Check if a pointer is properly aligned for SIMD operations
 * 
 * @param ptr The pointer to check
 * @param simd_type The SIMD type to check alignment for
 * @return true if the pointer is properly aligned
 */
bool goo_vectorization_is_aligned(void* ptr, GooSIMDType simd_type);

/**
 * Get the optimal vector width for a data type with the given SIMD type
 * 
 * @param data_type The vector data type
 * @param simd_type The SIMD type
 * @return The number of elements that can be processed in one operation
 */
size_t goo_vectorization_get_width(GooVectorDataType data_type, GooSIMDType simd_type);

/**
 * Check if an operation can be accelerated with SIMD
 * 
 * @param data_type The vector data type
 * @param op The vector operation
 * @param simd_type The SIMD type
 * @return true if the operation can be accelerated
 */
bool goo_vectorization_is_accelerated(GooVectorDataType data_type, GooVectorOpExtended op, GooSIMDType simd_type);

/**
 * Execute a vector operation using SIMD
 * 
 * @param op The vector operation to execute
 * @param src1 First source operand
 * @param src2 Second source operand (can be NULL for unary operations)
 * @param dst Destination buffer
 * @param elem_size Size of each element in bytes
 * @param length Number of elements to process
 * @param data_type The data type of the elements
 * @param simd_type The SIMD type to use
 * @param mask Optional mask for masked operations (can be NULL)
 * @return true if the operation was successful
 */
bool goo_vectorization_execute(GooVectorOpExtended op, void* src1, void* src2, void* dst, 
                               size_t elem_size, size_t length, GooVectorDataType data_type, 
                               GooSIMDType simd_type, void* mask);

/**
 * Create a vector mask for masked operations
 * 
 * @param size The number of elements in the mask
 * @param data_type The data type for the mask
 * @return Pointer to the created mask or NULL on failure
 */
void* goo_vectorization_create_mask(size_t size, GooVectorDataType data_type);

/**
 * Free a vector mask
 * 
 * @param mask The mask to free
 */
void goo_vectorization_free_mask(void* mask);

/**
 * Set mask values in a vector mask
 * 
 * @param mask The mask to modify
 * @param indices Array of indices where mask should be true
 * @param count Number of indices
 * @return true if successful
 */
bool goo_vectorization_set_mask(void* mask, size_t* indices, size_t count);

/**
 * Parallel-for operation with SIMD
 * 
 * @param start Start index
 * @param end End index (exclusive)
 * @param step Step size
 * @param context User context to pass to function
 * @param func Function to call for each iteration
 * @param simd_type SIMD type to use
 * @return true if successful
 */
bool goo_vectorization_parallel_for(size_t start, size_t end, size_t step, 
                                   void* context, 
                                   void (*func)(size_t index, void* context), 
                                   GooSIMDType simd_type);

#ifdef __cplusplus
}
#endif

#endif // GOO_RUNTIME_VECTORIZATION_H 