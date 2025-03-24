/**
 * vectorization.c
 * 
 * Example demonstrating the SIMD vectorization capabilities of the Goo language.
 * This example performs various vector operations using both scalar and SIMD implementations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../src/parallel/goo_vectorization.h"
#include "../src/parallel/goo_parallel.h"

// Benchmark parameters
#define ARRAY_SIZE 1000000
#define NUM_ITERATIONS 10
#define ALIGNMENT 32

// Helper function to allocate aligned memory
void* aligned_alloc_wrapper(size_t alignment, size_t size) {
    void* ptr = NULL;
    
    #if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
        // Use posix_memalign for POSIX systems
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return NULL;
        }
        return ptr;
    #elif defined(_MSC_VER) || defined(__MINGW32__)
        // Use _aligned_malloc for Windows
        return _aligned_malloc(size, alignment);
    #else
        // Fallback: Allocate extra space to ensure alignment
        void* raw = malloc(size + alignment);
        if (!raw) return NULL;
        
        // Calculate aligned address
        uintptr_t addr = (uintptr_t)raw + alignment;
        addr &= ~(uintptr_t)(alignment - 1);
        
        // Store original pointer before aligned address
        *((void**)(addr - sizeof(void*))) = raw;
        return (void*)addr;
    #endif
}

// Helper function to free aligned memory
void aligned_free_wrapper(void* ptr) {
    #if defined(_MSC_VER) || defined(__MINGW32__)
        // Use _aligned_free for Windows
        _aligned_free(ptr);
    #elif defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L
        // For POSIX systems, we can use regular free
        free(ptr);
    #else
        // Fallback: Get original pointer and free it
        if (ptr) {
            void* raw = *((void**)((uintptr_t)ptr - sizeof(void*)));
            free(raw);
        }
    #endif
}

// Benchmark a vector operation
double benchmark_vector_op(GooVector* vec_op, GooVectorDataType data_type, GooSIMDType simd_type, const char* op_name) {
    clock_t start, end;
    double elapsed_total = 0.0;

    GooVectorOperation op = {
        .base = *vec_op,
        .simd_type = simd_type,
        .data_type = data_type,
        .mask = NULL,
        .aligned = true
    };
    
    // Perform multiple iterations to get accurate timing
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        start = clock();
        
        if (!goo_vectorization_execute(&op)) {
            printf("Error executing vector operation\n");
            return -1.0;
        }
        
        end = clock();
        elapsed_total += ((double)(end - start)) / CLOCKS_PER_SEC;
    }
    
    double avg_time = elapsed_total / NUM_ITERATIONS;
    printf("%s operation (%s): %.6f seconds (%.2f million elements/sec)\n", 
            op_name,
            simd_type == GOO_SIMD_SCALAR ? "Scalar" : "SIMD",
            avg_time,
            (ARRAY_SIZE / 1000000.0) / avg_time);
            
    return avg_time;
}

// Compare results of scalar and SIMD operations
bool compare_results(float* result1, float* result2, size_t length) {
    for (size_t i = 0; i < length; i++) {
        // Allow small differences due to floating point precision
        if (fabsf(result1[i] - result2[i]) > 0.000001f) {
            printf("Results differ at index %zu: %f vs %f\n", i, result1[i], result2[i]);
            return false;
        }
    }
    return true;
}

int main() {
    // Initialize the vectorization subsystem
    if (!goo_vectorization_init(GOO_SIMD_AUTO)) {
        printf("Failed to initialize vectorization\n");
        return 1;
    }
    
    printf("Vectorization example\n");
    printf("--------------------\n");
    
    // Detect available SIMD instruction set
    GooSIMDType simd_type = goo_vectorization_detect_simd();
    printf("Detected SIMD type: %d\n", simd_type);
    
    // Allocate aligned memory for vector operations
    size_t alignment = goo_vectorization_get_alignment(simd_type);
    printf("Required alignment: %zu bytes\n", alignment);
    
    float* src1 = (float*)aligned_alloc_wrapper(alignment, ARRAY_SIZE * sizeof(float));
    float* src2 = (float*)aligned_alloc_wrapper(alignment, ARRAY_SIZE * sizeof(float));
    float* dst_simd = (float*)aligned_alloc_wrapper(alignment, ARRAY_SIZE * sizeof(float));
    float* dst_scalar = (float*)aligned_alloc_wrapper(alignment, ARRAY_SIZE * sizeof(float));
    
    if (!src1 || !src2 || !dst_simd || !dst_scalar) {
        printf("Failed to allocate memory\n");
        return 1;
    }
    
    // Initialize data
    for (int i = 0; i < ARRAY_SIZE; i++) {
        src1[i] = (float)i / 10.0f;
        src2[i] = (float)(ARRAY_SIZE - i) / 20.0f;
    }
    
    printf("\nBenchmarking vector operations with %d elements (%d iterations):\n", 
           ARRAY_SIZE, NUM_ITERATIONS);
    printf("--------------------\n");
    
    // Set up the vector operation structure
    GooVector vec_op = {
        .src1 = src1,
        .src2 = src2,
        .dst = dst_simd,
        .elem_size = sizeof(float),
        .length = ARRAY_SIZE,
        .op = GOO_VECTOR_ADD,
        .custom_op = NULL
    };
    
    // Benchmark vector addition
    vec_op.op = GOO_VECTOR_ADD;
    vec_op.dst = dst_simd;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, simd_type, "Addition");
    
    vec_op.dst = dst_scalar;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, GOO_SIMD_SCALAR, "Addition");
    
    if (compare_results(dst_simd, dst_scalar, ARRAY_SIZE)) {
        printf("Results match for addition\n");
    } else {
        printf("Results differ for addition\n");
    }
    
    // Benchmark vector multiplication
    vec_op.op = GOO_VECTOR_MUL;
    vec_op.dst = dst_simd;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, simd_type, "Multiplication");
    
    vec_op.dst = dst_scalar;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, GOO_SIMD_SCALAR, "Multiplication");
    
    if (compare_results(dst_simd, dst_scalar, ARRAY_SIZE)) {
        printf("Results match for multiplication\n");
    } else {
        printf("Results differ for multiplication\n");
    }
    
    // Benchmark vector division
    vec_op.op = GOO_VECTOR_DIV;
    vec_op.dst = dst_simd;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, simd_type, "Division");
    
    vec_op.dst = dst_scalar;
    benchmark_vector_op(&vec_op, GOO_VEC_FLOAT, GOO_SIMD_SCALAR, "Division");
    
    if (compare_results(dst_simd, dst_scalar, ARRAY_SIZE)) {
        printf("Results match for division\n");
    } else {
        printf("Results differ for division\n");
    }
    
    // Demo parallel for loop with vectorization
    printf("\nDemonstrating parallel execution with vectorization:\n");
    printf("--------------------\n");
    
    // Initialize parallel subsystem
    if (!goo_parallel_init(0)) {
        printf("Failed to initialize parallel subsystem\n");
        return 1;
    }
    
    // Define a function that will be vectorized
    typedef struct {
        float* src1;
        float* src2;
        float* dst;
    } VectorContext;
    
    void vector_add_func(uint64_t i, void* context) {
        VectorContext* ctx = (VectorContext*)context;
        ctx->dst[i] = ctx->src1[i] + ctx->src2[i];
    }
    
    VectorContext ctx = {
        .src1 = src1,
        .src2 = src2,
        .dst = dst_simd
    };
    
    // Create a parallel loop with vectorization enabled
    GooParallelLoop loop = {
        .mode = GOO_PARALLEL_FOR,
        .schedule = GOO_SCHEDULE_STATIC,
        .chunk_size = 1000,
        .vectorize = true,
        .num_threads = 0,
        .start = 0,
        .end = ARRAY_SIZE,
        .step = 1,
        .context = &ctx,
        .body = vector_add_func,
        .priority = 0
    };
    
    // Apply vectorization to the loop
    if (goo_vectorization_apply_to_loop(&loop, GOO_VEC_FLOAT, simd_type)) {
        printf("Successfully applied vectorization to loop\n");
    } else {
        printf("Failed to apply vectorization to loop\n");
    }
    
    // Execute the parallel loop with vectorization
    clock_t start_t = clock();
    
    if (goo_parallel_for(loop.start, loop.end, loop.step, loop.body, loop.context,
                         loop.schedule, loop.chunk_size, loop.num_threads)) {
        printf("Successfully executed parallel loop\n");
    } else {
        printf("Failed to execute parallel loop\n");
    }
    
    clock_t end_t = clock();
    double parallel_time = ((double)(end_t - start_t)) / CLOCKS_PER_SEC;
    
    // Execute the same operation sequentially for comparison
    start_t = clock();
    
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        dst_scalar[i] = src1[i] + src2[i];
    }
    
    end_t = clock();
    double sequential_time = ((double)(end_t - start_t)) / CLOCKS_PER_SEC;
    
    printf("Parallel + SIMD execution time: %.6f seconds\n", parallel_time);
    printf("Sequential execution time: %.6f seconds\n", sequential_time);
    printf("Speedup: %.2fx\n", sequential_time / parallel_time);
    
    // Clean up
    aligned_free_wrapper(src1);
    aligned_free_wrapper(src2);
    aligned_free_wrapper(dst_simd);
    aligned_free_wrapper(dst_scalar);
    
    goo_vectorization_cleanup();
    goo_parallel_cleanup();
    
    return 0;
} 