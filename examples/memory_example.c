#include "../src/memory/goo_allocator.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// Utility function to print memory statistics
void print_stats(const char* label, GooAllocator* allocator) {
    GooAllocStats stats = goo_get_alloc_stats(allocator);
    printf("%s:\n", label);
    printf("  Bytes allocated: %zu\n", stats.bytes_allocated);
    printf("  Bytes reserved: %zu\n", stats.bytes_reserved);
    printf("  Peak allocation: %zu\n", stats.max_bytes_allocated);
    printf("  Active allocations: %zu\n", stats.allocation_count);
    printf("  Total allocations: %zu\n", stats.total_allocations);
    printf("  Total frees: %zu\n", stats.total_frees);
    printf("  Failed allocations: %zu\n", stats.failed_allocations);
    printf("\n");
}

// Example of a custom out-of-memory handler
void custom_out_of_mem_handler(void) {
    printf("Custom out-of-memory handler called!\n");
    printf("This would be a good place to release cached resources.\n");
}

// Calculate execution time
double get_execution_time(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

int main() {
    // Initialize memory system
    if (!goo_memory_init()) {
        printf("Failed to initialize memory system\n");
        return 1;
    }
    
    printf("=== Goo Memory Allocator Example ===\n\n");
    
    // Get the default allocator
    GooAllocator* default_allocator = goo_get_default_allocator();
    print_stats("Initial memory state", default_allocator);
    
    // Example 1: Basic allocation and freeing
    printf("Example 1: Basic allocation\n");
    char* string = (char*)goo_alloc(100);
    strcpy(string, "Hello, Goo Memory System!");
    printf("Allocated string: %s\n", string);
    print_stats("After allocation", default_allocator);
    
    goo_free(string, 100);
    print_stats("After free", default_allocator);
    
    // Example 2: Zero-initialized memory
    printf("Example 2: Zero-initialized memory\n");
    int* numbers = (int*)goo_alloc_zero(10 * sizeof(int));
    printf("First few zeroed numbers: %d, %d, %d\n", numbers[0], numbers[1], numbers[2]);
    
    // Fill with data
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * 10;
    }
    printf("After filling: %d, %d, %d\n", numbers[0], numbers[1], numbers[2]);
    
    goo_free(numbers, 10 * sizeof(int));
    
    // Example 3: Scope-based allocation
    printf("\nExample 3: Scope-based allocation\n");
    {
        printf("Entering scope\n");
        GOO_SCOPE_ALLOC(scoped_data, 200);
        
        strcpy((char*)scoped_data, "This memory will be automatically freed");
        printf("Scoped data: %s\n", (char*)scoped_data);
        
        print_stats("Inside scope", default_allocator);
        printf("Leaving scope\n");
    }
    // scoped_data is automatically freed here
    print_stats("After scope", default_allocator);
    
    // Example 4: Aligned allocation
    printf("\nExample 4: Aligned allocation\n");
    void* aligned_data = goo_alloc_aligned(1024, 64);
    printf("Aligned allocation address: %p\n", aligned_data);
    printf("Address modulo 64: %zu (should be 0)\n", ((uintptr_t)aligned_data % 64));
    
    goo_free_aligned(aligned_data, 1024, 64);
    
    // Example 5: Reallocation
    printf("\nExample 5: Reallocation\n");
    char* buffer = (char*)goo_alloc(50);
    strcpy(buffer, "Original buffer");
    printf("Original buffer (%zu bytes): %s\n", (size_t)50, buffer);
    
    buffer = (char*)goo_realloc(buffer, 50, 200);
    strcat(buffer, " - now expanded to fit more text in the reallocated memory");
    printf("Expanded buffer (%zu bytes): %s\n", (size_t)200, buffer);
    
    goo_free(buffer, 200);
    
    // Example 6: Performance comparison
    printf("\nExample 6: Performance comparison\n");
    
    // Number of allocations to perform
    const int num_allocs = 1000000;
    const size_t alloc_size = 8;
    
    // Standard malloc/free
    struct timespec start, end;
    void* ptrs[num_allocs];
    
    printf("Performing %d allocations of %zu bytes each...\n", num_allocs, alloc_size);
    
    // Measure standard malloc/free
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = malloc(alloc_size);
    }
    for (int i = 0; i < num_allocs; i++) {
        free(ptrs[i]);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Standard malloc/free time: %.6f seconds\n", get_execution_time(start, end));
    
    // Measure Goo allocator
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = goo_alloc(alloc_size);
    }
    for (int i = 0; i < num_allocs; i++) {
        goo_free(ptrs[i], alloc_size);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Goo allocator time: %.6f seconds\n", get_execution_time(start, end));
    
    // Example 7: Custom out-of-memory handler
    printf("\nExample 7: Custom out-of-memory handler\n");
    
    // Set custom handler
    goo_set_out_of_mem_handler(custom_out_of_mem_handler);
    
    printf("Attempting a massive allocation (will likely fail)...\n");
    
    // Try to allocate a massive amount of memory (will likely fail)
    // The default allocator uses GOO_ALLOC_STRATEGY_PANIC, so this will call
    // our handler but then abort. To see the effect without crashing,
    // we'd need to change the strategy to GOO_ALLOC_STRATEGY_NULL.
    default_allocator->strategy = GOO_ALLOC_STRATEGY_NULL;
    void* massive = goo_alloc(SIZE_MAX / 2);
    
    if (massive == NULL) {
        printf("Allocation failed as expected\n");
    } else {
        printf("Unexpectedly succeeded in allocating a massive block\n");
        goo_free(massive, SIZE_MAX / 2);
    }
    
    // Reset to panic strategy
    default_allocator->strategy = GOO_ALLOC_STRATEGY_PANIC;
    
    // Print final stats
    print_stats("Final memory state", default_allocator);
    
    // Clean up
    goo_memory_cleanup();
    
    return 0;
} 