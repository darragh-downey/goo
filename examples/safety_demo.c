/**
 * @file safety_demo.c
 * @brief Demonstration of the Goo Safety System features
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/goo_safety.h"

#define VECTOR_SIZE 1000

/**
 * Demonstrate type safety features
 */
void demonstrate_type_safety(void) {
    printf("\n=== Type Safety Demonstration ===\n");
    
    // Create type signatures
    GooTypeSignature int_sig = goo_type_signature("int", sizeof(int));
    GooTypeSignature float_sig = goo_type_signature("float", sizeof(float));
    GooTypeSignature double_sig = goo_type_signature("double", sizeof(double));
    
    printf("Type signatures created:\n");
    printf("  int: ID=%llu, size=%zu\n", (unsigned long long)int_sig.type_id, int_sig.type_size);
    printf("  float: ID=%llu, size=%zu\n", (unsigned long long)float_sig.type_id, float_sig.type_size);
    printf("  double: ID=%llu, size=%zu\n", (unsigned long long)double_sig.type_id, double_sig.type_size);
    
    // Allocate memory with type tracking
    printf("\nAllocating typed memory...\n");
    int* int_array = GOO_SAFETY_MALLOC(10, int);
    float* float_array = GOO_SAFETY_MALLOC(10, float);
    
    if (!int_array || !float_array) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    // Initialize arrays
    for (int i = 0; i < 10; i++) {
        int_array[i] = i;
        float_array[i] = i * 1.5f;
    }
    
    // Check type safety
    printf("\nPerforming type checks...\n");
    printf("int_array has int type: %s\n", goo_check_type(int_array, int_sig) ? "YES" : "NO");
    printf("int_array has float type: %s\n", goo_check_type(int_array, float_sig) ? "YES" : "NO");
    printf("float_array has float type: %s\n", goo_check_type(float_array, float_sig) ? "YES" : "NO");
    printf("float_array has int type: %s\n", goo_check_type(float_array, int_sig) ? "YES" : "NO");
    
    // Demonstrate bounds checking
    printf("\nDemonstrating bounds checking...\n");
    for (int i = 8; i < 12; i++) {
        int value = GOO_ARRAY_GET_SAFE(int_array, i, 10, -1);
        printf("int_array[%d] = %d%s\n", i, value, (i >= 10) ? " (out of bounds)" : "");
    }
    
    // Free memory
    printf("\nFreeing typed memory...\n");
    GOO_SAFETY_FREE(int_array);
    GOO_SAFETY_FREE(float_array);
    
    printf("Type safety demonstration complete.\n");
}

/**
 * Demonstrate memory safety features
 */
void demonstrate_memory_safety(void) {
    printf("\n=== Memory Safety Demonstration ===\n");
    
    // Allocate a large array
    size_t large_size = 10 * 1024 * 1024; // 10 MB
    printf("Allocating a large array (%zu bytes)...\n", large_size);
    
    char* large_array = GOO_SAFETY_MALLOC(large_size, char);
    if (!large_array) {
        GooErrorInfo* error = goo_get_error_info();
        printf("Allocation failed: %s (code %d)\n", error->message, error->error_code);
        return;
    }
    
    printf("Large array allocated successfully at %p\n", (void*)large_array);
    
    // Test integer overflow protection
    printf("\nTesting integer overflow protection...\n");
    size_t huge_count = SIZE_MAX / 8 + 1;
    double* overflow_test = GOO_SAFETY_MALLOC(huge_count, double);
    
    if (!overflow_test) {
        GooErrorInfo* error = goo_get_error_info();
        printf("Correctly detected integer overflow: %s (code %d)\n", error->message, error->error_code);
    } else {
        printf("WARNING: Integer overflow not detected!\n");
        GOO_SAFETY_FREE(overflow_test);
    }
    
    // Free large array
    printf("\nFreeing large array...\n");
    GOO_SAFETY_FREE(large_array);
    
    printf("Memory safety demonstration complete.\n");
}

/**
 * Define a custom vector structure for the demo
 */
typedef struct {
    float* src1;
    float* src2;
    float* result;
    size_t size;
    int operation;
} DemoVector;

/**
 * Demonstrate concurrency safety features
 */
void demonstrate_concurrency_safety(void) {
    printf("\n=== Concurrency Safety Demonstration ===\n");
    
    // Allocate vector data
    printf("Allocating and initializing vector data...\n");
    float* src1 = GOO_SAFETY_MALLOC(VECTOR_SIZE, float);
    float* src2 = GOO_SAFETY_MALLOC(VECTOR_SIZE, float);
    float* result = GOO_SAFETY_MALLOC(VECTOR_SIZE, float);
    
    if (!src1 || !src2 || !result) {
        printf("Memory allocation failed!\n");
        return;
    }
    
    // Initialize vectors
    for (size_t i = 0; i < VECTOR_SIZE; i++) {
        src1[i] = (float)i;
        src2[i] = (float)(VECTOR_SIZE - i);
        result[i] = 0.0f;
    }
    
    // Create vector operation
    DemoVector vec_op = {
        .src1 = src1,
        .src2 = src2,
        .result = result,
        .size = VECTOR_SIZE,
        .operation = 0  // Addition
    };
    
    // Create type signature for vector operation
    GooTypeSignature vec_sig = goo_type_signature("DemoVector", sizeof(DemoVector));
    
    // Execute vector operation with timeout
    printf("Executing vector operation with timeout...\n");
    unsigned int timeout_ms = 1000;  // 1 second timeout
    
    bool success = goo_safety_vector_execute(&vec_op, vec_sig, timeout_ms);
    
    if (success) {
        printf("Vector operation completed successfully within %u ms\n", timeout_ms);
        // In a real implementation, we would check the result here
    } else {
        GooErrorInfo* error = goo_get_error_info();
        printf("Vector operation failed: %s (code %d)\n", error->message, error->error_code);
    }
    
    // Demonstrate incorrect type
    printf("\nDemonstrating type check failure...\n");
    GooTypeSignature wrong_sig = goo_type_signature("WrongType", sizeof(int));
    
    success = goo_safety_vector_execute(&vec_op, wrong_sig, timeout_ms);
    
    if (!success) {
        GooErrorInfo* error = goo_get_error_info();
        printf("Correctly detected type mismatch: %s (code %d)\n", error->message, error->error_code);
    } else {
        printf("WARNING: Type mismatch not detected!\n");
    }
    
    // Free vector data
    printf("\nFreeing vector data...\n");
    GOO_SAFETY_FREE(src1);
    GOO_SAFETY_FREE(src2);
    GOO_SAFETY_FREE(result);
    
    printf("Concurrency safety demonstration complete.\n");
}

int main(void) {
    printf("=== Goo Safety System Demonstration ===\n");
    
    // Initialize safety system
    int ret = goo_safety_init();
    if (ret != 0) {
        printf("Failed to initialize safety system: %d\n", ret);
        return 1;
    }
    
    printf("Safety system initialized successfully.\n");
    
    // Run demonstrations
    demonstrate_type_safety();
    demonstrate_memory_safety();
    demonstrate_concurrency_safety();
    
    printf("\n=== Demonstration Complete ===\n");
    return 0;
} 