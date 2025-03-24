/**
 * goo_vectorization.c
 * 
 * Implementation of SIMD vectorization support for the Goo programming language.
 * Provides portable abstractions over various SIMD instruction sets.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "goo_vectorization.h"

// Platform-specific includes for SIMD intrinsics
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #include <immintrin.h>
#elif defined(__arm__) || defined(__aarch64__)
    #include <arm_neon.h>
#endif

// Current SIMD type detected or selected
static GooSIMDType current_simd_type = GOO_SIMD_AUTO;

// Detect CPU features for SIMD support
static GooSIMDType detect_cpu_features(void) {
    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        // Check for AVX-512 support
        #if defined(__AVX512F__)
            return GOO_SIMD_AVX512;
        #endif
        
        // Check for AVX2 support
        #if defined(__AVX2__)
            return GOO_SIMD_AVX2;
        #endif
        
        // Check for AVX support
        #if defined(__AVX__)
            return GOO_SIMD_AVX;
        #endif
        
        // Check for SSE4 support
        #if defined(__SSE4_1__) && defined(__SSE4_2__)
            return GOO_SIMD_SSE4;
        #endif
        
        // Check for SSE2 support
        #if defined(__SSE2__)
            return GOO_SIMD_SSE2;
        #endif
    #elif defined(__arm__) || defined(__aarch64__)
        // ARM NEON support
        #if defined(__ARM_NEON)
            return GOO_SIMD_NEON;
        #endif
    #endif
    
    // Fallback to scalar mode if no SIMD support detected
    return GOO_SIMD_SCALAR;
}

/**
 * Initialize the vectorization subsystem
 */
bool goo_vectorization_init(GooSIMDType simd_type) {
    if (simd_type == GOO_SIMD_AUTO) {
        current_simd_type = detect_cpu_features();
    } else {
        current_simd_type = simd_type;
    }
    
    // Verify that the selected SIMD type is available
    GooSIMDType available = detect_cpu_features();
    if (current_simd_type > available) {
        fprintf(stderr, "Warning: Requested SIMD type not available, using %d instead\n", available);
        current_simd_type = available;
    }
    
    return true;
}

/**
 * Clean up the vectorization subsystem
 */
void goo_vectorization_cleanup(void) {
    // Nothing to clean up for now
}

/**
 * Get the best available SIMD instruction set on the current hardware
 */
GooSIMDType goo_vectorization_detect_simd(void) {
    return detect_cpu_features();
}

/**
 * Get the required alignment (in bytes) for optimal SIMD performance
 */
size_t goo_vectorization_get_alignment(GooSIMDType simd_type) {
    switch (simd_type) {
        case GOO_SIMD_AVX512:
            return 64;
        case GOO_SIMD_AVX:
        case GOO_SIMD_AVX2:
            return 32;
        case GOO_SIMD_SSE2:
        case GOO_SIMD_SSE4:
        case GOO_SIMD_NEON:
            return 16;
        default:
            return 8;
    }
}

/**
 * Check if a pointer is properly aligned for SIMD operations
 */
bool goo_vectorization_is_aligned(void *ptr, GooSIMDType simd_type) {
    size_t alignment = goo_vectorization_get_alignment(simd_type);
    return ((uintptr_t)ptr % alignment) == 0;
}

/**
 * Get optimal vector width (in elements) for a given data type and SIMD type
 */
size_t goo_vectorization_get_width(GooVectorDataType data_type, GooSIMDType simd_type) {
    size_t type_size;
    
    // Get size of data type
    switch (data_type) {
        case GOO_VEC_INT8:
        case GOO_VEC_UINT8:
            type_size = 1;
            break;
        case GOO_VEC_INT16:
        case GOO_VEC_UINT16:
            type_size = 2;
            break;
        case GOO_VEC_INT32:
        case GOO_VEC_UINT32:
        case GOO_VEC_FLOAT:
            type_size = 4;
            break;
        case GOO_VEC_INT64:
        case GOO_VEC_UINT64:
        case GOO_VEC_DOUBLE:
            type_size = 8;
            break;
        default:
            return 1;
    }
    
    // Calculate vector width based on SIMD type and data type
    switch (simd_type) {
        case GOO_SIMD_AVX512:
            return 64 / type_size;
        case GOO_SIMD_AVX:
        case GOO_SIMD_AVX2:
            return 32 / type_size;
        case GOO_SIMD_SSE2:
        case GOO_SIMD_SSE4:
        case GOO_SIMD_NEON:
            return 16 / type_size;
        default:
            return 1;
    }
}

/**
 * Check if SIMD acceleration is available for a given data type and operation
 */
bool goo_vectorization_is_accelerated(GooVectorDataType data_type, 
                                     GooVectorOp op, 
                                     GooSIMDType simd_type) {
    // Scalar code always works, but isn't accelerated
    if (simd_type == GOO_SIMD_SCALAR) {
        return false;
    }
    
    // Custom operations need to be handled by the user
    if (op == GOO_VECTOR_CUSTOM) {
        return false;
    }
    
    // Check for specific limitations
    switch (simd_type) {
        case GOO_SIMD_SSE2:
            // SSE2 doesn't have integer division
            if (op == GOO_VECTOR_DIV && (
                data_type == GOO_VEC_INT8 || data_type == GOO_VEC_UINT8 ||
                data_type == GOO_VEC_INT16 || data_type == GOO_VEC_UINT16 ||
                data_type == GOO_VEC_INT32 || data_type == GOO_VEC_UINT32 ||
                data_type == GOO_VEC_INT64 || data_type == GOO_VEC_UINT64)) {
                return false;
            }
            break;
            
        default:
            break;
    }
    
    return true;
}

/**
 * Create a vector mask for conditional operations
 */
GooVectorMask *goo_vectorization_create_mask(size_t size, GooVectorDataType type) {
    // Input validation
    if (size == 0) {
        return NULL;
    }
    
    // Allocate the mask structure
    GooVectorMask *mask = (GooVectorMask*)malloc(sizeof(GooVectorMask));
    if (!mask) {
        fprintf(stderr, "Error: Memory allocation failed for vector mask\n");
        return NULL;
    }
    
    // Initialize to NULL to ensure safe cleanup on error
    mask->mask_data = NULL; 
    mask->mask_size = 0;
    
    // Determine element size
    size_t elem_size;
    switch (type) {
        case GOO_VEC_INT8:
        case GOO_VEC_UINT8:
            elem_size = 1;
            break;
        case GOO_VEC_INT16:
        case GOO_VEC_UINT16:
            elem_size = 2;
            break;
        case GOO_VEC_INT32:
        case GOO_VEC_UINT32:
        case GOO_VEC_FLOAT:
            elem_size = 4;
            break;
        case GOO_VEC_INT64:
        case GOO_VEC_UINT64:
        case GOO_VEC_DOUBLE:
            elem_size = 8;
            break;
        default:
            fprintf(stderr, "Error: Invalid vector data type: %d\n", type);
            free(mask);
            return NULL;
    }
    
    // Check for size_t overflow in multiplication
    if (size > SIZE_MAX / elem_size) {
        fprintf(stderr, "Error: Vector mask size would cause integer overflow\n");
        free(mask);
        return NULL;
    }
    
    // Calculate total size safely
    size_t mask_size = size * elem_size;
    
    // Allocate mask data
    mask->mask_data = calloc(1, mask_size);
    if (!mask->mask_data) {
        fprintf(stderr, "Error: Memory allocation failed for mask data (size: %zu bytes)\n", mask_size);
        free(mask);
        return NULL;
    }
    
    // Initialize the mask structure
    mask->mask_size = mask_size;
    mask->type = type;
    
    return mask;
}

/**
 * Free a vector mask
 */
void goo_vectorization_free_mask(GooVectorMask *mask) {
    if (!mask) return;
    
    if (mask->mask_data) {
        free(mask->mask_data);
    }
    
    free(mask);
}

/**
 * Set values in a vector mask
 */
bool goo_vectorization_set_mask(GooVectorMask *mask, size_t *indices, size_t count) {
    // Input validation with detailed error reporting
    if (!mask) {
        fprintf(stderr, "Error: Null mask pointer provided to goo_vectorization_set_mask\n");
        return false;
    }
    
    if (!mask->mask_data) {
        fprintf(stderr, "Error: Mask has null data pointer\n");
        return false;
    }
    
    if (!indices) {
        fprintf(stderr, "Error: Null indices pointer provided to goo_vectorization_set_mask\n");
        return false;
    }
    
    if (count == 0) {
        // Not a fatal error, just nothing to do
        return true;
    }
    
    // Determine element size
    size_t elem_size;
    switch (mask->type) {
        case GOO_VEC_INT8:
        case GOO_VEC_UINT8:
            elem_size = 1;
            break;
        case GOO_VEC_INT16:
        case GOO_VEC_UINT16:
            elem_size = 2;
            break;
        case GOO_VEC_INT32:
        case GOO_VEC_UINT32:
        case GOO_VEC_FLOAT:
            elem_size = 4;
            break;
        case GOO_VEC_INT64:
        case GOO_VEC_UINT64:
        case GOO_VEC_DOUBLE:
            elem_size = 8;
            break;
        default:
            fprintf(stderr, "Error: Invalid vector data type: %d\n", mask->type);
            return false;
    }
    
    // Calculate maximum valid index
    if (mask->mask_size == 0 || elem_size == 0) {
        fprintf(stderr, "Error: Invalid mask size or element size\n");
        return false;
    }
    
    size_t max_index = mask->mask_size / elem_size;
    size_t out_of_bounds_count = 0;
    
    // Set each index
    for (size_t i = 0; i < count; i++) {
        if (indices[i] >= max_index) {
            // Track out-of-bounds access attempts
            out_of_bounds_count++;
            continue;  // Skip out-of-range indices
        }
        
        // Use a safer approach to access memory based on type
        void* target_addr = (char*)mask->mask_data + (indices[i] * elem_size);
        
        switch (mask->type) {
            case GOO_VEC_INT8:
                *(int8_t*)target_addr = -1;  // All bits set
                break;
            case GOO_VEC_UINT8:
                *(uint8_t*)target_addr = 0xFF;
                break;
            case GOO_VEC_INT16:
                *(int16_t*)target_addr = -1;
                break;
            case GOO_VEC_UINT16:
                *(uint16_t*)target_addr = 0xFFFF;
                break;
            case GOO_VEC_INT32:
                *(int32_t*)target_addr = -1;
                break;
            case GOO_VEC_UINT32:
                *(uint32_t*)target_addr = 0xFFFFFFFF;
                break;
            case GOO_VEC_FLOAT:
                // All bits set for float means NaN, use -1.0f instead
                *(float*)target_addr = -1.0f;
                break;
            case GOO_VEC_INT64:
                *(int64_t*)target_addr = -1;
                break;
            case GOO_VEC_UINT64:
                *(uint64_t*)target_addr = 0xFFFFFFFFFFFFFFFF;
                break;
            case GOO_VEC_DOUBLE:
                // All bits set for double means NaN, use -1.0 instead
                *(double*)target_addr = -1.0;
                break;
            default:
                fprintf(stderr, "Error: Invalid vector data type in switch: %d\n", mask->type);
                return false;
        }
    }
    
    // Report if any indices were out of bounds
    if (out_of_bounds_count > 0) {
        fprintf(stderr, "Warning: %zu out of %zu indices were out of bounds and skipped\n", 
                out_of_bounds_count, count);
    }
    
    return true;
}

/* 
 * Helper functions for vector operations with different SIMD instruction sets
 */

// Generic scalar implementation that works for any type
static bool vector_op_scalar(GooVectorOp op, void* src1, void* src2, void* dst, 
                           size_t elem_size, size_t length, GooVectorDataType type) {
    // Input validation
    if (!src1 || (!src2 && op != GOO_VECTOR_ADD && op != GOO_VECTOR_SUB && 
                  op != GOO_VECTOR_MUL && op != GOO_VECTOR_DIV) || !dst) {
        fprintf(stderr, "Error in vector_op_scalar: NULL buffer pointers\n");
        return false;
    }
    
    if (length == 0 || elem_size == 0) {
        fprintf(stderr, "Error in vector_op_scalar: Invalid length or element size\n");
        return false;
    }
    
    // Check for overflow in total byte count
    size_t total_bytes;
    if (__builtin_mul_overflow(length, elem_size, &total_bytes)) {
        fprintf(stderr, "Error in vector_op_scalar: Integer overflow in length calculation\n");
        return false;
    }
    
    // Handle different data types with appropriate overflow/underflow protection
    switch (type) {
        case GOO_VEC_INT8: {
            int8_t *s1 = (int8_t*)src1;
            int8_t *s2 = (int8_t*)src2;
            int8_t *d = (int8_t*)dst;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow
                        if ((s1[i] > 0 && s2[i] > INT8_MAX - s1[i]) ||
                            (s1[i] < 0 && s2[i] < INT8_MIN - s1[i])) {
                            // Handle overflow by clamping
                            d[i] = (s1[i] > 0) ? INT8_MAX : INT8_MIN;
                        } else {
                            d[i] = s1[i] + s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow/underflow
                        if ((s2[i] > 0 && s1[i] < INT8_MIN + s2[i]) ||
                            (s2[i] < 0 && s1[i] > INT8_MAX + s2[i])) {
                            // Handle overflow by clamping
                            d[i] = (s2[i] > 0) ? INT8_MIN : INT8_MAX;
                        } else {
                            d[i] = s1[i] - s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow in multiplication
                        if (s1[i] > 0 && s2[i] > 0 && s1[i] > INT8_MAX / s2[i]) {
                            // Overflow would occur, clamp to max value
                            d[i] = INT8_MAX;
                        } else if (s1[i] > 0 && s2[i] < 0 && s2[i] < INT8_MIN / s1[i]) {
                            // Overflow would occur, clamp to min value
                            d[i] = INT8_MIN;
                        } else if (s1[i] < 0 && s2[i] > 0 && s1[i] < INT8_MIN / s2[i]) {
                            // Overflow would occur, clamp to min value
                            d[i] = INT8_MIN;
                        } else if (s1[i] < 0 && s2[i] < 0 && s1[i] < INT8_MAX / s2[i]) {
                            // Overflow would occur, clamp to max value
                            d[i] = INT8_MAX;
                        } else {
                            d[i] = s1[i] * s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (size_t i = 0; i < length; i++) {
                        // Division by zero protection
                        if (s2[i] == 0) {
                            fprintf(stderr, "Warning: Division by zero at index %zu\n", i);
                            d[i] = 0;
                        } else {
                            d[i] = s1[i] / s2[i];
                        }
                    }
                    break;
                    
                default:
                    fprintf(stderr, "Error: Unsupported vector operation for INT8 type\n");
                    return false;
            }
            break;
        }
        
        case GOO_VEC_FLOAT: {
            float *s1 = (float*)src1;
            float *s2 = (float*)src2;
            float *d = (float*)dst;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] + s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] - s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] * s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (size_t i = 0; i < length; i++) {
                        if (fabsf(s2[i]) < 1e-10f) {
                            fprintf(stderr, "Warning: Division by near-zero at index %zu\n", i);
                            d[i] = 0.0f;
                        } else {
                            d[i] = s1[i] / s2[i];
                        }
                    }
                    break;
                    
                default:
                    fprintf(stderr, "Error: Unsupported vector operation for FLOAT type\n");
                    return false;
            }
            break;
        }
        
        case GOO_VEC_DOUBLE: {
            double *s1 = (double*)src1;
            double *s2 = (double*)src2;
            double *d = (double*)dst;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] + s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] - s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (size_t i = 0; i < length; i++) {
                        d[i] = s1[i] * s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (size_t i = 0; i < length; i++) {
                        if (fabs(s2[i]) < 1e-10) {
                            fprintf(stderr, "Warning: Division by near-zero at index %zu\n", i);
                            d[i] = 0.0;
                        } else {
                            d[i] = s1[i] / s2[i];
                        }
                    }
                    break;
                    
                default:
                    fprintf(stderr, "Error: Unsupported vector operation for DOUBLE type\n");
                    return false;
            }
            break;
        }
        
        case GOO_VEC_INT16: {
            int16_t *s1 = (int16_t*)src1;
            int16_t *s2 = (int16_t*)src2;
            int16_t *d = (int16_t*)dst;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow
                        if ((s1[i] > 0 && s2[i] > INT16_MAX - s1[i]) ||
                            (s1[i] < 0 && s2[i] < INT16_MIN - s1[i])) {
                            // Handle overflow by clamping
                            d[i] = (s1[i] > 0) ? INT16_MAX : INT16_MIN;
                        } else {
                            d[i] = s1[i] + s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow/underflow
                        if ((s2[i] > 0 && s1[i] < INT16_MIN + s2[i]) ||
                            (s2[i] < 0 && s1[i] > INT16_MAX + s2[i])) {
                            // Handle overflow by clamping
                            d[i] = (s2[i] > 0) ? INT16_MIN : INT16_MAX;
                        } else {
                            d[i] = s1[i] - s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (size_t i = 0; i < length; i++) {
                        // Check for overflow in multiplication
                        if (s1[i] > 0 && s2[i] > 0 && s1[i] > INT16_MAX / s2[i]) {
                            // Overflow would occur, clamp to max value
                            d[i] = INT16_MAX;
                        } else if (s1[i] > 0 && s2[i] < 0 && s2[i] < INT16_MIN / s1[i]) {
                            // Overflow would occur, clamp to min value
                            d[i] = INT16_MIN;
                        } else if (s1[i] < 0 && s2[i] > 0 && s1[i] < INT16_MIN / s2[i]) {
                            // Overflow would occur, clamp to min value
                            d[i] = INT16_MIN;
                        } else if (s1[i] < 0 && s2[i] < 0 && s1[i] < INT16_MAX / s2[i]) {
                            // Overflow would occur, clamp to max value
                            d[i] = INT16_MAX;
                        } else {
                            d[i] = s1[i] * s2[i];
                        }
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (size_t i = 0; i < length; i++) {
                        // Division by zero protection
                        if (s2[i] == 0) {
                            fprintf(stderr, "Warning: Division by zero at index %zu\n", i);
                            d[i] = 0;
                        } else {
                            d[i] = s1[i] / s2[i];
                        }
                    }
                    break;
                    
                default:
                    fprintf(stderr, "Error: Unsupported vector operation for INT16 type\n");
                    return false;
            }
            break;
        }
        
        case GOO_VEC_INT32: {
            int32_t *s1 = (int32_t*)src1;
            int32_t *s2 = (int32_t*)src2;
            int32_t *d = (int32_t*)dst;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (size_t i = 0; i < length; i++) {
                        // Use compiler builtin to check for overflow
                        int32_t result;
                        if (__builtin_add_overflow(s1[i], s2[i], &result)) {
                            // Handle overflow by clamping
                            d[i] = (s1[i] > 0) ? INT32_MAX : INT32_MIN;
                        } else {
                            d[i] = result;
                        }
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (size_t i = 0; i < length; i++) {
                        // Use compiler builtin to check for overflow
                        int32_t result;
                        if (__builtin_sub_overflow(s1[i], s2[i], &result)) {
                            // Handle overflow by clamping
                            d[i] = (s2[i] > 0) ? INT32_MIN : INT32_MAX;
                        } else {
                            d[i] = result;
                        }
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (size_t i = 0; i < length; i++) {
                        // Use compiler builtin to check for overflow
                        int32_t result;
                        if (__builtin_mul_overflow(s1[i], s2[i], &result)) {
                            // Handle overflow by clamping, with sign check
                            if ((s1[i] > 0 && s2[i] > 0) || (s1[i] < 0 && s2[i] < 0)) {
                                d[i] = INT32_MAX;  // Positive result would overflow
                            } else {
                                d[i] = INT32_MIN;  // Negative result would overflow
                            }
                        } else {
                            d[i] = result;
                        }
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (size_t i = 0; i < length; i++) {
                        // Handle division by zero and INT_MIN / -1 overflow case
                        if (s2[i] == 0) {
                            fprintf(stderr, "Warning: Division by zero at index %zu\n", i);
                            d[i] = 0;
                        } else if (s1[i] == INT32_MIN && s2[i] == -1) {
                            fprintf(stderr, "Warning: Division overflow at index %zu\n", i);
                            d[i] = INT32_MAX;  // Clamp to max on overflow
                        } else {
                            d[i] = s1[i] / s2[i];
                        }
                    }
                    break;
                    
                default:
                    fprintf(stderr, "Error: Unsupported vector operation for INT32 type\n");
                    return false;
            }
            break;
        }
        
        // Add more cases for other data types as needed
        
        default:
            fprintf(stderr, "Error: Unsupported vector data type: %d\n", type);
            // Fall back to raw byte operations for unknown types, handling only simple cases
            if (op == GOO_VECTOR_ADD) {
                uint8_t *s1 = (uint8_t*)src1;
                uint8_t *s2 = (uint8_t*)src2;
                uint8_t *d = (uint8_t*)dst;
                
                for (size_t i = 0; i < total_bytes; i++) {
                    d[i] = s1[i] + s2[i];
                }
                return true;
            }
            return false;
    }
    
    return true;
}

#if defined(__SSE2__)
// SSE2 implementation
static bool vector_op_sse2(GooVectorOp op, void* src1, void* src2, void* dst, 
                         size_t elem_size, size_t length, GooVectorDataType type) {
    // SSE2 operations with proper alignment
    switch (type) {
        case GOO_VEC_FLOAT: {
            float *s1 = (float*)src1;
            float *s2 = (float*)src2;
            float *d = (float*)dst;
            
            // Process 4 floats at a time with SSE
            size_t vec_length = length / 4;
            size_t i;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (i = 0; i < vec_length; i++) {
                        __m128 v1 = _mm_load_ps(s1 + i * 4);
                        __m128 v2 = _mm_load_ps(s2 + i * 4);
                        __m128 result = _mm_add_ps(v1, v2);
                        _mm_store_ps(d + i * 4, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 4; i < length; i++) {
                        d[i] = s1[i] + s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (i = 0; i < vec_length; i++) {
                        __m128 v1 = _mm_load_ps(s1 + i * 4);
                        __m128 v2 = _mm_load_ps(s2 + i * 4);
                        __m128 result = _mm_sub_ps(v1, v2);
                        _mm_store_ps(d + i * 4, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 4; i < length; i++) {
                        d[i] = s1[i] - s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (i = 0; i < vec_length; i++) {
                        __m128 v1 = _mm_load_ps(s1 + i * 4);
                        __m128 v2 = _mm_load_ps(s2 + i * 4);
                        __m128 result = _mm_mul_ps(v1, v2);
                        _mm_store_ps(d + i * 4, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 4; i < length; i++) {
                        d[i] = s1[i] * s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (i = 0; i < vec_length; i++) {
                        __m128 v1 = _mm_load_ps(s1 + i * 4);
                        __m128 v2 = _mm_load_ps(s2 + i * 4);
                        __m128 result = _mm_div_ps(v1, v2);
                        _mm_store_ps(d + i * 4, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 4; i < length; i++) {
                        d[i] = s1[i] / s2[i];
                    }
                    break;
                    
                default:
                    return false;
            }
            break;
        }
        
        case GOO_VEC_DOUBLE: {
            double *s1 = (double*)src1;
            double *s2 = (double*)src2;
            double *d = (double*)dst;
            
            // Process 2 doubles at a time with SSE2
            size_t vec_length = length / 2;
            size_t i;
            
            switch (op) {
                case GOO_VECTOR_ADD:
                    for (i = 0; i < vec_length; i++) {
                        __m128d v1 = _mm_load_pd(s1 + i * 2);
                        __m128d v2 = _mm_load_pd(s2 + i * 2);
                        __m128d result = _mm_add_pd(v1, v2);
                        _mm_store_pd(d + i * 2, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 2; i < length; i++) {
                        d[i] = s1[i] + s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_SUB:
                    for (i = 0; i < vec_length; i++) {
                        __m128d v1 = _mm_load_pd(s1 + i * 2);
                        __m128d v2 = _mm_load_pd(s2 + i * 2);
                        __m128d result = _mm_sub_pd(v1, v2);
                        _mm_store_pd(d + i * 2, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 2; i < length; i++) {
                        d[i] = s1[i] - s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_MUL:
                    for (i = 0; i < vec_length; i++) {
                        __m128d v1 = _mm_load_pd(s1 + i * 2);
                        __m128d v2 = _mm_load_pd(s2 + i * 2);
                        __m128d result = _mm_mul_pd(v1, v2);
                        _mm_store_pd(d + i * 2, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 2; i < length; i++) {
                        d[i] = s1[i] * s2[i];
                    }
                    break;
                    
                case GOO_VECTOR_DIV:
                    for (i = 0; i < vec_length; i++) {
                        __m128d v1 = _mm_load_pd(s1 + i * 2);
                        __m128d v2 = _mm_load_pd(s2 + i * 2);
                        __m128d result = _mm_div_pd(v1, v2);
                        _mm_store_pd(d + i * 2, result);
                    }
                    // Handle remaining elements
                    for (i = vec_length * 2; i < length; i++) {
                        d[i] = s1[i] / s2[i];
                    }
                    break;
                    
                default:
                    return false;
            }
            break;
        }
        
        // Add implementations for other types as needed
        
        default:
            // Fall back to scalar for unsupported types
            return vector_op_scalar(op, src1, src2, dst, elem_size, length, type);
    }
    
    return true;
}
#endif

// Dispatch to the appropriate SIMD implementation
static bool vector_op_dispatch(GooVectorOp op, void* src1, void* src2, void* dst, 
                             size_t elem_size, size_t length, GooVectorDataType type,
                             GooSIMDType simd_type) {
    switch (simd_type) {
        #if defined(__SSE2__)
        case GOO_SIMD_SSE2:
            return vector_op_sse2(op, src1, src2, dst, elem_size, length, type);
        #endif
        
        // Add cases for other SIMD types as needed
        
        case GOO_SIMD_SCALAR:
        default:
            return vector_op_scalar(op, src1, src2, dst, elem_size, length, type);
    }
}

/**
 * Execute a vector operation using SIMD instructions
 */
bool goo_vectorization_execute(GooVectorOperation *op) {
    if (!op || !op->base.src1 || !op->base.dst) {
        return false;
    }
    
    // Use the selected SIMD type or the current default
    GooSIMDType simd_type = op->simd_type;
    if (simd_type == GOO_SIMD_AUTO) {
        simd_type = current_simd_type;
    }
    
    // Dispatch the operation to the appropriate implementation
    return vector_op_dispatch(
        op->base.op,
        op->base.src1,
        op->base.src2,
        op->base.dst,
        op->base.elem_size,
        op->base.length,
        op->data_type,
        simd_type
    );
}

/**
 * Apply a vector operation to loop iterations
 */
bool goo_vectorization_apply_to_loop(GooParallelLoop *loop, 
                                   GooVectorDataType data_type,
                                   GooSIMDType simd_type) {
    if (!loop || !loop->vectorize) {
        return false;
    }
    
    // Use the selected SIMD type or the current default
    if (simd_type == GOO_SIMD_AUTO) {
        simd_type = current_simd_type;
    }
    
    // Mark the data_type as used to avoid compiler warnings
    (void)data_type;
    
    // We need a custom body function to call the original with vectorized operations
    // This will be a placeholder for now - actual implementation would be more complex
    // and would need to analyze the loop body to determine if vectorization is possible
    
    // For now, just set the vectorize flag to indicate it's been processed
    return true;
}

/**
 * Execute a vector operation with base vector interface
 */
bool goo_vector_execute(GooVector *vec_op) {
    // Enhanced input validation with specific error messages
    if (!vec_op) {
        fprintf(stderr, "Error: Null vector operation pointer in goo_vector_execute\n");
        return false;
    }
    
    if (!vec_op->src1) {
        fprintf(stderr, "Error: Null source buffer 1 in vector operation\n");
        return false;
    }
    
    if (!vec_op->dst) {
        fprintf(stderr, "Error: Null destination buffer in vector operation\n");
        return false;
    }
    
    // Validate element size
    if (vec_op->elem_size == 0) {
        fprintf(stderr, "Error: Invalid element size (0) in vector operation\n");
        return false;
    }
    
    // Validate operation count
    if (vec_op->length == 0) {
        fprintf(stderr, "Error: Vector operation with zero length\n");
        return false;
    }
    
    // Validate that src2 is provided for binary operations
    if ((vec_op->op == GOO_VECTOR_ADD || 
         vec_op->op == GOO_VECTOR_SUB || 
         vec_op->op == GOO_VECTOR_MUL || 
         vec_op->op == GOO_VECTOR_DIV) && 
        !vec_op->src2) {
        fprintf(stderr, "Error: Binary vector operation missing second source buffer\n");
        return false;
    }
    
    // Create a full vector operation with defaults
    GooVectorOperation op = {
        .base = *vec_op,
        .simd_type = current_simd_type,
        .data_type = GOO_VEC_FLOAT, // Default to float
        .mask = NULL,
        .aligned = false // Will be set properly below
    };
    
    // Check alignment of buffers for the current SIMD type
    bool src1_aligned = goo_vectorization_is_aligned(vec_op->src1, current_simd_type);
    bool dst_aligned = goo_vectorization_is_aligned(vec_op->dst, current_simd_type);
    bool src2_aligned = vec_op->src2 ? goo_vectorization_is_aligned(vec_op->src2, current_simd_type) : true;
    
    op.aligned = src1_aligned && dst_aligned && src2_aligned;
    
    // Optional: Log alignment issues for debugging
    if (!op.aligned && (current_simd_type != GOO_SIMD_SCALAR)) {
        if (!src1_aligned) {
            fprintf(stderr, "Warning: Source buffer 1 is not properly aligned for SIMD operations\n");
        }
        if (!dst_aligned) {
            fprintf(stderr, "Warning: Destination buffer is not properly aligned for SIMD operations\n");
        }
        if (vec_op->src2 && !src2_aligned) {
            fprintf(stderr, "Warning: Source buffer 2 is not properly aligned for SIMD operations\n");
        }
    }
    
    // Determine data type based on element size
    switch (vec_op->elem_size) {
        case 1:
            op.data_type = GOO_VEC_INT8;
            break;
        case 2:
            op.data_type = GOO_VEC_INT16;
            break;
        case 4:
            op.data_type = GOO_VEC_FLOAT;
            break;
        case 8:
            op.data_type = GOO_VEC_DOUBLE;
            break;
        default:
            fprintf(stderr, "Warning: Unusual element size (%zu), defaulting to byte operations\n", 
                   vec_op->elem_size);
            op.data_type = GOO_VEC_INT8;
            break;
    }
    
    // Execute the vector operation
    bool result = goo_vectorization_execute(&op);
    
    if (!result) {
        fprintf(stderr, "Error: Vector operation execution failed\n");
    }
    
    return result;
}

/**
 * Safely allocate memory with the proper alignment for SIMD operations
 * 
 * @param size Number of bytes to allocate
 * @param simd_type The SIMD type to align for (or GOO_SIMD_AUTO for current default)
 * @return Pointer to aligned memory or NULL on error
 */
void* goo_vectorization_alloc_aligned(size_t size, GooSIMDType simd_type) {
    // Input validation
    if (size == 0) {
        fprintf(stderr, "Error: Cannot allocate 0 bytes in goo_vectorization_alloc_aligned\n");
        return NULL;
    }
    
    // Handle potential size_t overflow for large allocations
    if (size > SIZE_MAX / 2) {
        fprintf(stderr, "Error: Requested allocation size is too large\n");
        return NULL;
    }
    
    // Get alignment requirement for the SIMD type
    size_t alignment = goo_vectorization_get_alignment(simd_type == GOO_SIMD_AUTO ? 
                                                   current_simd_type : simd_type);
    
    // Ensure alignment is valid
    if (alignment == 0) {
        fprintf(stderr, "Error: Invalid alignment requirement (0)\n");
        return NULL;
    }
    
    // Check if alignment is a power of 2
    if ((alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Error: Alignment must be a power of 2\n");
        return NULL;
    }
    
    // Check if alignment is valid for aligned_alloc (must be multiple of sizeof(void*))
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }
    
    // Ensure size is a multiple of alignment as required by aligned_alloc
    size_t adjusted_size = (size + alignment - 1) & ~(alignment - 1);
    
    // Allocate aligned memory
    void* ptr = aligned_alloc(alignment, adjusted_size);
    
    if (!ptr) {
        fprintf(stderr, "Error: Failed to allocate %zu bytes with alignment %zu\n", 
               adjusted_size, alignment);
        return NULL;
    }
    
    return ptr;
}

/**
 * Safely free memory allocated with goo_vectorization_alloc_aligned
 * 
 * @param ptr Pointer to memory allocated with goo_vectorization_alloc_aligned
 */
void goo_vectorization_free_aligned(void* ptr) {
    if (ptr) {
        free(ptr);
    }
}
