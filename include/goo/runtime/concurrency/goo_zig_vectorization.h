/**
 * @file goo_zig_vectorization.h
 * @brief C bindings for Zig-based SIMD vectorization in the Goo language
 */

#ifndef GOO_ZIG_VECTORIZATION_H
#define GOO_ZIG_VECTORIZATION_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration of Zig types
typedef struct GooZigVectorMask GooZigVectorMask;
typedef struct GooZigVectorOperation GooZigVectorOperation;

/**
 * SIMD instruction set types
 */
typedef enum {
    GOO_ZIG_SIMD_AUTO = 0,     // Automatically detect best available
    GOO_ZIG_SIMD_SCALAR = 1,   // Fallback scalar implementation (no SIMD)
    GOO_ZIG_SIMD_SSE2 = 2,     // Intel SSE2
    GOO_ZIG_SIMD_SSE4 = 3,     // Intel SSE4
    GOO_ZIG_SIMD_AVX = 4,      // Intel AVX
    GOO_ZIG_SIMD_AVX2 = 5,     // Intel AVX2
    GOO_ZIG_SIMD_AVX512 = 6,   // Intel AVX-512
    GOO_ZIG_SIMD_NEON = 7      // ARM NEON
} GooZigSIMDType;

/**
 * Vector data types
 */
typedef enum {
    GOO_ZIG_VEC_INT8 = 0,      // 8-bit signed integer
    GOO_ZIG_VEC_UINT8 = 1,     // 8-bit unsigned integer
    GOO_ZIG_VEC_INT16 = 2,     // 16-bit signed integer
    GOO_ZIG_VEC_UINT16 = 3,    // 16-bit unsigned integer
    GOO_ZIG_VEC_INT32 = 4,     // 32-bit signed integer
    GOO_ZIG_VEC_UINT32 = 5,    // 32-bit unsigned integer
    GOO_ZIG_VEC_INT64 = 6,     // 64-bit signed integer
    GOO_ZIG_VEC_UINT64 = 7,    // 64-bit unsigned integer
    GOO_ZIG_VEC_FLOAT = 8,     // 32-bit floating point
    GOO_ZIG_VEC_DOUBLE = 9     // 64-bit floating point
} GooZigVectorDataType;

/**
 * Vector operations
 */
typedef enum {
    GOO_ZIG_VECTOR_ADD = 0,    // Element-wise addition
    GOO_ZIG_VECTOR_SUB = 1,    // Element-wise subtraction
    GOO_ZIG_VECTOR_MUL = 2,    // Element-wise multiplication
    GOO_ZIG_VECTOR_DIV = 3,    // Element-wise division
    GOO_ZIG_VECTOR_FMA = 4,    // Fused multiply-add
    GOO_ZIG_VECTOR_ABS = 5,    // Absolute value
    GOO_ZIG_VECTOR_SQRT = 6,   // Square root
    GOO_ZIG_VECTOR_CUSTOM = 7  // Custom operation function
} GooZigVectorOp;

/**
 * Initialize the Zig vectorization subsystem
 * @param simd_type The SIMD instruction set to use (GOO_ZIG_SIMD_AUTO for auto-detection)
 * @return true if initialization successful, false otherwise
 */
bool goo_zig_vectorization_init(GooZigSIMDType simd_type);

/**
 * Clean up the Zig vectorization subsystem
 */
void goo_zig_vectorization_cleanup(void);

/**
 * Get the best available SIMD instruction set on the current hardware
 * @return The detected SIMD type
 */
GooZigSIMDType goo_zig_vectorization_detect_simd(void);

/**
 * Execute a vector operation using SIMD instructions
 * @param vec_op The vector operation configuration
 * @return true if operation successful, false otherwise
 */
bool goo_zig_vectorization_execute(GooZigVectorOperation *vec_op);

/**
 * Create a vector mask for conditional operations
 * @param size Number of elements in the mask
 * @param type Data type of the mask elements
 * @return A new vector mask, or NULL on error
 */
GooZigVectorMask* goo_zig_vectorization_create_mask(size_t size, GooZigVectorDataType type);

/**
 * Free a vector mask
 * @param mask The mask to free
 */
void goo_zig_vectorization_free_mask(GooZigVectorMask *mask);

/**
 * Set values in a vector mask
 * @param mask The mask to modify
 * @param indices Array of indices to set to true
 * @param count Number of indices
 * @return true if successful, false otherwise
 */
bool goo_zig_vectorization_set_mask(GooZigVectorMask *mask, size_t *indices, size_t count);

/**
 * Check if SIMD acceleration is available for a given data type and operation
 * @param data_type The vector data type
 * @param op The vector operation
 * @param simd_type The SIMD instruction set to check
 * @return true if SIMD acceleration is available, false otherwise
 */
bool goo_zig_vectorization_is_accelerated(
    GooZigVectorDataType data_type,
    GooZigVectorOp op,
    GooZigSIMDType simd_type
);

/**
 * Get optimal vector width (in elements) for a given data type and SIMD type
 * @param data_type The vector data type
 * @param simd_type The SIMD instruction set
 * @return The optimal vector width in elements, or 1 for scalar
 */
size_t goo_zig_vectorization_get_width(GooZigVectorDataType data_type, GooZigSIMDType simd_type);

/**
 * Get the required alignment (in bytes) for optimal SIMD performance
 * @param simd_type The SIMD instruction set
 * @return The required alignment in bytes
 */
size_t goo_zig_vectorization_get_alignment(GooZigSIMDType simd_type);

/**
 * Check if a pointer is properly aligned for SIMD operations
 * @param ptr The pointer to check
 * @param simd_type The SIMD instruction set
 * @return true if the pointer is properly aligned, false otherwise
 */
bool goo_zig_vectorization_is_aligned(void *ptr, GooZigSIMDType simd_type);

/**
 * Safely allocate memory with the proper alignment for SIMD operations
 * 
 * @param size Number of bytes to allocate
 * @param simd_type The SIMD type to align for (or GOO_ZIG_SIMD_AUTO for current default)
 * @return Pointer to aligned memory or NULL on error
 */
void* goo_zig_vectorization_alloc_aligned(size_t size, GooZigSIMDType simd_type);

/**
 * Safely free memory allocated with goo_zig_vectorization_alloc_aligned
 * 
 * @param ptr Pointer to memory allocated with goo_zig_vectorization_alloc_aligned
 */
void goo_zig_vectorization_free_aligned(void* ptr);

#ifdef __cplusplus
}
#endif

#endif /* GOO_ZIG_VECTORIZATION_H */ 