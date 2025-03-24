/**
 * @file safe_simd_demo.c
 * @brief Demo of safe SIMD operations with type safety and thread safety
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

#include "../include/goo_safety.h"
#include "../src/parallel/goo_vectorization.h"

#define VECTOR_SIZE 1024

/**
 * @brief Allocate aligned memory for vector operations
 * 
 * @param size Size in elements
 * @param alignment Required alignment
 * @return void* Pointer to aligned memory or NULL on failure
 */
void* allocate_aligned_memory(size_t size, size_t alignment) {
    if (size == 0 || alignment == 0) {
        return NULL;
    }
    
    void* ptr = NULL;
    int ret = posix_memalign(&ptr, alignment, size);
    if (ret != 0) {
        fprintf(stderr, "Failed to allocate aligned memory: %d\n", ret);
        return NULL;
    }
    
    /* Initialize memory to zero */
    memset(ptr, 0, size);
    
    return ptr;
}

/**
 * @brief Initialize a vector with sequential values
 * 
 * @param vec Pointer to the vector
 * @param multiplier Multiplier for sequential values
 */
void initialize_vector(float* vec, size_t size, float multiplier) {
    if (!vec) return;
    
    for (size_t i = 0; i < size; i++) {
        vec[i] = i * multiplier;
    }
}

/**
 * @brief Calculate sum of elements in a vector
 * 
 * @param vec Pointer to the vector
 * @param size Number of elements
 * @return float Sum of elements
 */
float vector_sum(float* vec, size_t size) {
    if (!vec) return 0.0f;
    
    float sum = 0.0f;
    for (size_t i = 0; i < size; i++) {
        sum += vec[i];
    }
    
    return sum;
}

/**
 * @brief Display vector contents (truncated for large vectors)
 * 
 * @param name Name of the vector
 * @param vec Pointer to the vector
 * @param size Number of elements
 */
void display_vector(const char* name, float* vec, size_t size) {
    if (!vec) return;
    
    printf("%s: [", name);
    
    // Display first 10 elements
    size_t display_count = (size > 10) ? 10 : size;
    for (size_t i = 0; i < display_count; i++) {
        printf("%.2f", vec[i]);
        if (i < display_count - 1) {
            printf(", ");
        }
    }
    
    // Indicate truncation
    if (size > 10) {
        printf(", ... (%zu more elements)", size - 10);
    }
    
    printf("]\n");
}

/**
 * @brief Perform vector operation using direct SIMD function
 * 
 * @param op Vector operation code
 * @param src1 First source vector
 * @param src2 Second source vector
 * @param dst Destination vector
 * @param size Number of elements
 * @return bool True if successful, false otherwise
 */
bool perform_vector_operation(int op, float* src1, float* src2, float* dst, size_t size) {
    if (!src1 || !src2 || !dst || size == 0) {
        return false;
    }
    
    GooVector vec_op = {
        .src1 = src1,
        .src2 = src2,
        .dst = dst,
        .elem_size = sizeof(float),
        .length = size,
        .op = op,
        .custom_op = NULL
    };
    
    return goo_vector_execute(&vec_op);
}

int main(void) {
    // Initialize safety system
    if (goo_safety_init() != 0) {
        fprintf(stderr, "Failed to initialize safety system\n");
        return 1;
    }
    
    // Initialize vectorization
    if (goo_vectorization_init(GOO_SIMD_AUTO) != 0) {
        fprintf(stderr, "Failed to initialize vectorization\n");
        return 1;
    }
    
    // Detect SIMD type
    int simd_type = goo_vectorization_detect_simd();
    printf("Detected SIMD type: %d\n", simd_type);
    
    // Get required alignment
    size_t alignment = goo_vectorization_get_alignment(simd_type);
    printf("Required alignment: %zu bytes\n", alignment);
    
    // Allocate vectors with proper alignment
    float* src1 = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    float* src2 = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    float* add_result = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    float* mul_result = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    
    if (!src1 || !src2 || !add_result || !mul_result) {
        fprintf(stderr, "Failed to allocate vectors\n");
        return 1;
    }
    
    // Initialize vectors with test data
    initialize_vector(src1, VECTOR_SIZE, 1.0f);  // 0, 1, 2, ..., VECTOR_SIZE-1
    initialize_vector(src2, VECTOR_SIZE, 2.0f);  // 0, 2, 4, ..., 2*(VECTOR_SIZE-1)
    
    printf("\nExecuting vector operations through safety system...\n");
    
    // Using the new safer API with GooSafeVector
    
    // Addition operation with the macro
    printf("Performing vector addition with GOO_SAFETY_VECTOR_EXECUTE macro...\n");
    bool add_success = GOO_SAFETY_VECTOR_EXECUTE(
        src1, src2, add_result, VECTOR_SIZE, GOO_VECTOR_ADD, alignment, 1000
    );
    if (!add_success) {
        GooErrorInfo* error = goo_get_error_info();
        fprintf(stderr, "Safe vector addition failed: %s\n", error->message);
        return 1;
    }
    
    // Multiplication operation with the structured API
    printf("Performing vector multiplication with structured API...\n");
    GooSafeVector mul_vector = goo_create_safe_vector(
        src1, src2, mul_result, VECTOR_SIZE, GOO_VECTOR_MUL, alignment
    );
    
    bool mul_success = goo_safe_vector_execute(&mul_vector, 1000);
    if (!mul_success) {
        GooErrorInfo* error = goo_get_error_info();
        fprintf(stderr, "Safe vector multiplication failed: %s\n", error->message);
        return 1;
    }
    
    // Display vectors (limited to first few elements)
    printf("\nSource and result vectors:\n");
    display_vector("Source 1", src1, VECTOR_SIZE);
    display_vector("Source 2", src2, VECTOR_SIZE);
    display_vector("Addition Result", add_result, VECTOR_SIZE);
    display_vector("Multiplication Result", mul_result, VECTOR_SIZE);
    
    // Calculate and display sums for verification
    float sum_src1 = vector_sum(src1, VECTOR_SIZE);
    float sum_src2 = vector_sum(src2, VECTOR_SIZE);
    float sum_add = vector_sum(add_result, VECTOR_SIZE);
    float sum_mul = vector_sum(mul_result, VECTOR_SIZE);
    
    printf("\nVerification sums:\n");
    printf("Sum of Source 1: %.2f\n", sum_src1);
    printf("Sum of Source 2: %.2f\n", sum_src2);
    printf("Sum of Addition Result: %.2f\n", sum_add);
    printf("Sum of Multiplication Result: %.2f\n", sum_mul);
    
    // Verify results
    printf("\nVerification:\n");
    printf("Addition verification: %.2f == %.2f %s\n",
           sum_add, sum_src1 + sum_src2, 
           (fabsf(sum_add - (sum_src1 + sum_src2)) < 0.01f) ? "PASSED" : "FAILED");
    
    // Compare with direct operation (non-safety version)
    printf("\nComparing with direct SIMD operations...\n");
    
    // Allocate new result vectors for direct operations
    float* direct_add_result = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    float* direct_mul_result = (float*)allocate_aligned_memory(VECTOR_SIZE * sizeof(float), alignment);
    
    if (!direct_add_result || !direct_mul_result) {
        fprintf(stderr, "Failed to allocate vectors for direct operations\n");
        return 1;
    }
    
    // Perform direct operations
    bool direct_add_success = perform_vector_operation(GOO_VECTOR_ADD, src1, src2, direct_add_result, VECTOR_SIZE);
    bool direct_mul_success = perform_vector_operation(GOO_VECTOR_MUL, src1, src2, direct_mul_result, VECTOR_SIZE);
    
    if (!direct_add_success || !direct_mul_success) {
        fprintf(stderr, "Direct vector operations failed\n");
        return 1;
    }
    
    // Verify direct operation results match safety-enhanced results
    bool add_match = true;
    bool mul_match = true;
    
    for (size_t i = 0; i < VECTOR_SIZE; i++) {
        if (fabsf(add_result[i] - direct_add_result[i]) > 0.001f) {
            add_match = false;
            break;
        }
        
        if (fabsf(mul_result[i] - direct_mul_result[i]) > 0.001f) {
            mul_match = false;
            break;
        }
    }
    
    printf("Direct vs. Safe addition results match: %s\n", add_match ? "PASSED" : "FAILED");
    printf("Direct vs. Safe multiplication results match: %s\n", mul_match ? "PASSED" : "FAILED");
    
    // Clean up
    free(src1);
    free(src2);
    free(add_result);
    free(mul_result);
    free(direct_add_result);
    free(direct_mul_result);
    
    return 0;
} 