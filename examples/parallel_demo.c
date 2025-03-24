/**
 * Goo Parallel Execution Demo
 * 
 * This example demonstrates the key features of Goo's parallel execution system,
 * including various scheduling strategies and synchronization mechanisms.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include "../include/goo_runtime.h"

#define ARRAY_SIZE 100000000  // 100 million elements
#define NUM_ITERATIONS 5      // Number of iterations for benchmarks

// Structure for context data
typedef struct {
    double* data;
    size_t size;
    double result;
} ArrayContext;

// Initialize array with random values
void init_array(size_t index, void* ctx_ptr) {
    ArrayContext* ctx = (ArrayContext*)ctx_ptr;
    
    // Simple random value
    ctx->data[index] = (double)rand() / RAND_MAX;
}

// Compute square root of each element
void compute_sqrt(size_t index, void* ctx_ptr) {
    ArrayContext* ctx = (ArrayContext*)ctx_ptr;
    ctx->data[index] = sqrt(ctx->data[index]);
}

// Compute sum reduction (not thread-safe, for demonstration only)
void compute_sum_unsafe(size_t index, void* ctx_ptr) {
    ArrayContext* ctx = (ArrayContext*)ctx_ptr;
    ctx->result += ctx->data[index];
}

// Compute sum reduction with proper synchronization
void compute_sum_safe(size_t index, void* ctx_ptr) {
    ArrayContext* ctx = (ArrayContext*)ctx_ptr;
    
    // Use atomic operation for thread safety
    goo_atomic_add_double(&ctx->result, ctx->data[index]);
}

// Function to measure execution time
double measure_execution_time(void (*func)(void)) {
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    func();
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// Demo for different scheduling strategies
void demo_scheduling_strategies() {
    printf("\n=== Scheduling Strategies Demo ===\n");
    
    // Allocate array
    ArrayContext ctx;
    ctx.size = ARRAY_SIZE;
    ctx.data = (double*)malloc(ARRAY_SIZE * sizeof(double));
    ctx.result = 0.0;
    
    // Lambda for static scheduling test
    void test_static() {
        // Reset data
        ctx.result = 0.0;
        goo_parallel_for(0, ARRAY_SIZE, 1, init_array, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
        
        // Process with static scheduling
        goo_parallel_for(0, ARRAY_SIZE, 1, compute_sqrt, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
    }
    
    // Lambda for dynamic scheduling test
    void test_dynamic() {
        // Reset data
        ctx.result = 0.0;
        goo_parallel_for(0, ARRAY_SIZE, 1, init_array, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
        
        // Process with dynamic scheduling (chunk size 10000)
        goo_parallel_for(0, ARRAY_SIZE, 1, compute_sqrt, &ctx, 
                         GOO_SCHEDULE_DYNAMIC, 10000, 0);
    }
    
    // Lambda for guided scheduling test
    void test_guided() {
        // Reset data
        ctx.result = 0.0;
        goo_parallel_for(0, ARRAY_SIZE, 1, init_array, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
        
        // Process with guided scheduling
        goo_parallel_for(0, ARRAY_SIZE, 1, compute_sqrt, &ctx, 
                         GOO_SCHEDULE_GUIDED, 1000, 0);
    }
    
    // Measure and display performance of each scheduling strategy
    double static_time = 0, dynamic_time = 0, guided_time = 0;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        static_time += measure_execution_time(test_static);
        dynamic_time += measure_execution_time(test_dynamic);
        guided_time += measure_execution_time(test_guided);
    }
    
    static_time /= NUM_ITERATIONS;
    dynamic_time /= NUM_ITERATIONS;
    guided_time /= NUM_ITERATIONS;
    
    printf("Static scheduling:  %.6f seconds\n", static_time);
    printf("Dynamic scheduling: %.6f seconds\n", dynamic_time);
    printf("Guided scheduling:  %.6f seconds\n", guided_time);
    
    // Free array
    free(ctx.data);
}

// Demo for parallel reduction (sum)
void demo_parallel_reduction() {
    printf("\n=== Parallel Reduction Demo ===\n");
    
    // Allocate array
    ArrayContext ctx;
    ctx.size = ARRAY_SIZE;
    ctx.data = (double*)malloc(ARRAY_SIZE * sizeof(double));
    
    // Initialize array with values 1.0
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        ctx.data[i] = 1.0;
    }
    
    // Lambda for sequential sum
    void test_sequential_sum() {
        ctx.result = 0.0;
        for (size_t i = 0; i < ARRAY_SIZE; i++) {
            ctx.result += ctx.data[i];
        }
    }
    
    // Lambda for unsafe parallel sum (incorrect result due to race conditions)
    void test_unsafe_parallel_sum() {
        ctx.result = 0.0;
        goo_parallel_for(0, ARRAY_SIZE, 1, compute_sum_unsafe, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
    }
    
    // Lambda for safe parallel sum
    void test_safe_parallel_sum() {
        ctx.result = 0.0;
        goo_parallel_for(0, ARRAY_SIZE, 1, compute_sum_safe, &ctx, 
                         GOO_SCHEDULE_STATIC, 0, 0);
    }
    
    // Measure and display performance of each approach
    double seq_time = measure_execution_time(test_sequential_sum);
    double seq_result = ctx.result;
    printf("Sequential sum: %.1f (%.6f seconds)\n", seq_result, seq_time);
    
    double unsafe_time = measure_execution_time(test_unsafe_parallel_sum);
    double unsafe_result = ctx.result;
    printf("Unsafe parallel sum: %.1f (%.6f seconds) - INCORRECT due to race conditions\n", 
           unsafe_result, unsafe_time);
    
    double safe_time = measure_execution_time(test_safe_parallel_sum);
    double safe_result = ctx.result;
    printf("Safe parallel sum: %.1f (%.6f seconds)\n", safe_result, safe_time);
    
    // Calculate speedup
    printf("Speedup with safe parallel sum: %.2fx\n", seq_time / safe_time);
    
    // Free array
    free(ctx.data);
}

// Demo for parallel for with different loop parameters
void demo_parallel_for_variants() {
    printf("\n=== Parallel For Variants Demo ===\n");
    
    // Allocate array
    ArrayContext ctx;
    ctx.size = ARRAY_SIZE;
    ctx.data = (double*)malloc(ARRAY_SIZE * sizeof(double));
    
    // Forward iteration
    ctx.result = 0.0;
    goo_parallel_for(0, 1000, 1, init_array, &ctx, GOO_SCHEDULE_STATIC, 0, 0);
    printf("Forward iteration (0 to 999, step 1) completed\n");
    
    // Backward iteration
    ctx.result = 0.0;
    goo_parallel_for(999, -1, -1, init_array, &ctx, GOO_SCHEDULE_STATIC, 0, 0);
    printf("Backward iteration (999 to 0, step -1) completed\n");
    
    // Step iteration
    ctx.result = 0.0;
    goo_parallel_for(0, 1000, 5, init_array, &ctx, GOO_SCHEDULE_STATIC, 0, 0);
    printf("Step iteration (0 to <1000, step 5) completed\n");
    
    // Free array
    free(ctx.data);
}

// Demo for barriers and synchronization
void demo_barriers_and_sync() {
    printf("\n=== Barriers and Synchronization Demo ===\n");
    
    // Create a barrier for 4 threads
    GooBarrier* barrier = goo_barrier_create(4);
    
    // Worker function with barrier synchronization
    void barrier_worker(void* arg) {
        int id = *(int*)arg;
        
        printf("Worker %d: Phase 1 starting\n", id);
        usleep(100000 * (id % 3 + 1));  // Simulate work
        printf("Worker %d: Phase 1 complete\n", id);
        
        // Synchronize all threads before phase 2
        goo_barrier_wait(barrier);
        
        printf("Worker %d: Phase 2 starting\n", id);
        usleep(100000 * (id % 3 + 1));  // Simulate work
        printf("Worker %d: Phase 2 complete\n", id);
        
        // Synchronize all threads before phase 3
        goo_barrier_wait(barrier);
        
        printf("Worker %d: Phase 3 starting\n", id);
        usleep(100000 * (id % 3 + 1));  // Simulate work
        printf("Worker %d: Phase 3 complete\n", id);
    }
    
    // Spawn 4 worker goroutines
    int worker_ids[4] = {1, 2, 3, 4};
    for (int i = 0; i < 4; i++) {
        GooTask* task = goo_task_create(barrier_worker, &worker_ids[i], sizeof(int));
        goo_spawn(task);
    }
    
    // Wait for all goroutines to complete
    usleep(1000000);
    
    // Clean up
    goo_barrier_destroy(barrier);
}

int main() {
    // Initialize Goo runtime
    goo_runtime_init();
    
    // Set random seed
    srand(time(NULL));
    
    printf("=== Goo Parallel Execution Demo ===\n");
    printf("This demo showcases the parallel execution features of Goo.\n");
    
    // Run the different demos
    demo_scheduling_strategies();
    demo_parallel_reduction();
    demo_parallel_for_variants();
    demo_barriers_and_sync();
    
    printf("\n=== Demo Complete ===\n");
    
    // Cleanup Goo runtime
    goo_runtime_cleanup();
    
    return 0;
} 