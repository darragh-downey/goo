#include "../src/parallel/goo_parallel.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Sample array size
#define ARRAY_SIZE 10000000

// Context structure for parallel operation
typedef struct {
    int* array;
    int value;
} ArrayContext;

// Function to initialize array in parallel
void init_array(uint64_t index, void* context) {
    ArrayContext* ctx = (ArrayContext*)context;
    ctx->array[index] = ctx->value;
}

// Function to sum array in parallel with reduction
void sum_array(uint64_t index, void* context) {
    ArrayContext* ctx = (ArrayContext*)context;
    ctx->value += ctx->array[index];
}

// Function to find max value in parallel with reduction
void find_max(uint64_t index, void* context) {
    ArrayContext* ctx = (ArrayContext*)context;
    if (ctx->array[index] > ctx->value) {
        ctx->value = ctx->array[index];
    }
}

// Calculate execution time
double get_execution_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    // Initialize random number generator
    srand(time(NULL));
    
    // Create sample array
    int* array = (int*)malloc(ARRAY_SIZE * sizeof(int));
    if (!array) {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    // Initialize parallel runtime
    if (!goo_parallel_init(0)) {  // 0 means use all available cores
        printf("Failed to initialize parallel runtime\n");
        free(array);
        return 1;
    }
    
    printf("Parallel runtime initialized with %d threads\n", 
           goo_parallel_get_num_threads());
    
    // Create context
    ArrayContext context = {array, 1};
    
    // Time measurement
    struct timespec start, end;
    
    // Example 1: Initialize array in parallel (each element = 1)
    printf("\nExample 1: Initialize array in parallel\n");
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Use parallel_for with static scheduling
    if (!goo_parallel_for(0, ARRAY_SIZE, 1, init_array, &context,
                         GOO_SCHEDULE_STATIC, 0, 0)) {
        printf("Parallel for failed\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time to initialize %d elements: %.6f seconds\n", 
           ARRAY_SIZE, get_execution_time(start, end));
    
    // Verify a few elements
    printf("Verification: array[0]=%d, array[%d]=%d, array[%d]=%d\n",
           array[0], ARRAY_SIZE/2, array[ARRAY_SIZE/2], 
           ARRAY_SIZE-1, array[ARRAY_SIZE-1]);
    
    // Example 2: Fill with random numbers using dynamic scheduling
    printf("\nExample 2: Fill with random numbers (dynamic scheduling)\n");
    
    context.value = 0;  // Use this as a seed offset
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Worker function to fill with random numbers
    void fill_random(uint64_t index, void* ctx) {
        ArrayContext* context = (ArrayContext*)ctx;
        context->array[index] = rand() % 1000;  // Random number between 0-999
    }
    
    // Use parallel_for with dynamic scheduling
    if (!goo_parallel_for(0, ARRAY_SIZE, 1, fill_random, &context,
                         GOO_SCHEDULE_DYNAMIC, 10000, 0)) {
        printf("Parallel for failed\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Time to fill with random numbers: %.6f seconds\n", 
           get_execution_time(start, end));
    
    // Example 3: Find sum using guided scheduling
    printf("\nExample 3: Calculate sum (guided scheduling)\n");
    
    // Reset the context value to use for sum
    context.value = 0;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // For actual reduction, we'd need thread-local storage and proper reduction
    // This is a simplified version that may have race conditions
    if (!goo_parallel_for(0, ARRAY_SIZE, 1, sum_array, &context,
                         GOO_SCHEDULE_GUIDED, 0, 0)) {
        printf("Parallel for failed\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Parallel sum result: %d\n", context.value);
    printf("Time to calculate sum: %.6f seconds\n", 
           get_execution_time(start, end));
    
    // Serial verification (for comparison)
    int serial_sum = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        serial_sum += array[i];
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Serial sum result: %d\n", serial_sum);
    printf("Time for serial sum: %.6f seconds\n", 
           get_execution_time(start, end));
    
    // Example 4: Find maximum value
    printf("\nExample 4: Find maximum value\n");
    
    // Reset the context value to use for max
    context.value = 0;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Again, this simplified version may have race conditions
    if (!goo_parallel_for(0, ARRAY_SIZE, 1, find_max, &context,
                         GOO_SCHEDULE_STATIC, 0, 0)) {
        printf("Parallel for failed\n");
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Parallel max result: %d\n", context.value);
    printf("Time to find max: %.6f seconds\n", 
           get_execution_time(start, end));
    
    // Serial verification (for comparison)
    int serial_max = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < ARRAY_SIZE; i++) {
        if (array[i] > serial_max) {
            serial_max = array[i];
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Serial max result: %d\n", serial_max);
    printf("Time for serial max: %.6f seconds\n", 
           get_execution_time(start, end));
    
    // Clean up
    goo_parallel_cleanup();
    free(array);
    
    return 0;
} 