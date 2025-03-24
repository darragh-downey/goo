/**
 * Goo Memory Management Demo - Zig Implementation
 * 
 * This example demonstrates the Zig implementation of the memory allocator.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

// Import our Zig-implemented allocator functions
extern int goo_memory_init(void);
extern void goo_memory_cleanup(void);
extern void* goo_alloc(size_t size);
extern void* goo_alloc_zero(size_t size);
extern void* goo_realloc(void* ptr, size_t old_size, size_t new_size);
extern void goo_free(void* ptr, size_t size);
extern void* goo_alloc_aligned(size_t size, size_t alignment);
extern void goo_free_aligned(void* ptr, size_t size, size_t alignment);

// Stats structure
typedef struct GooAllocStats {
    size_t bytes_allocated;     // Total bytes currently allocated
    size_t bytes_reserved;      // Total bytes reserved (may be higher than allocated)
    size_t max_bytes_allocated; // Peak allocation
    size_t allocation_count;    // Number of active allocations
    size_t total_allocations;   // Total number of allocations
    size_t total_frees;         // Total number of frees
    size_t failed_allocations;  // Number of failed allocations
} GooAllocStats;

extern struct GooAllocStats goo_get_alloc_stats(void* allocator);
extern void* goo_get_default_allocator(void);

// Scope-based allocation helper (auto-cleanup)
extern void goo_scope_cleanup(void** ptr);
#define GOO_SCOPE_ALLOC(name, size) \
    void* name __attribute__((cleanup(goo_scope_cleanup))) = goo_alloc(size)

// Thread function for concurrency demo
void* allocation_thread(void* arg) {
    const int thread_id = *(int*)arg;
    const int num_allocs = 1000;
    
    printf("Thread %d: Starting allocation sequence\n", thread_id);
    
    for (int i = 0; i < num_allocs; i++) {
        // Allocate with a size specific to this thread
        size_t alloc_size = 100 + (thread_id * 10) + (i % 50);
        void* ptr = goo_alloc(alloc_size);
        
        if (ptr) {
            // Do something with the memory
            memset(ptr, thread_id & 0xFF, alloc_size);
            
            // Free the memory
            goo_free(ptr, alloc_size);
        }
    }
    
    printf("Thread %d: Completed allocation sequence\n", thread_id);
    return NULL;
}

// Demo for basic allocations
void basic_allocation_demo(void) {
    printf("\n===== BASIC ALLOCATION DEMO =====\n");
    
    // Standard allocation
    printf("Allocating and using standard memory...\n");
    int* numbers = (int*)goo_alloc(10 * sizeof(int));
    
    for (int i = 0; i < 10; i++) {
        numbers[i] = i * 100;
    }
    
    printf("Allocated values: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    goo_free(numbers, 10 * sizeof(int));
    
    // Zero-initialized memory
    printf("\nAllocating zero-initialized memory...\n");
    char* buffer = (char*)goo_alloc_zero(100);
    
    printf("First 10 bytes after zero allocation: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");
    
    strcpy(buffer, "Memory safety is important!");
    printf("After writing: %s\n", buffer);
    
    goo_free(buffer, 100);
}

// Demo for aligned allocations
void aligned_allocation_demo(void) {
    printf("\n===== ALIGNED ALLOCATION DEMO =====\n");
    
    // Show different alignments
    const size_t alignments[] = {8, 16, 32, 64, 128};
    
    for (size_t i = 0; i < sizeof(alignments)/sizeof(alignments[0]); i++) {
        size_t alignment = alignments[i];
        void* ptr = goo_alloc_aligned(256, alignment);
        
        printf("Allocated with %zu-byte alignment: %p\n", alignment, ptr);
        printf("  Address modulo alignment: %zu\n", ((uintptr_t)ptr % alignment));
        
        goo_free_aligned(ptr, 256, alignment);
    }
}

// Demo for reallocation
void reallocation_demo(void) {
    printf("\n===== REALLOCATION DEMO =====\n");
    
    // Initial allocation
    printf("Initial allocation of 5 integers...\n");
    int* numbers = (int*)goo_alloc(5 * sizeof(int));
    
    for (int i = 0; i < 5; i++) {
        numbers[i] = i * 10;
    }
    
    printf("Initial values: ");
    for (int i = 0; i < 5; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Grow the allocation
    printf("Growing to 10 integers...\n");
    numbers = (int*)goo_realloc(numbers, 5 * sizeof(int), 10 * sizeof(int));
    
    // Add more values
    for (int i = 5; i < 10; i++) {
        numbers[i] = i * 10;
    }
    
    printf("Values after growing: ");
    for (int i = 0; i < 10; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    // Shrink the allocation
    printf("Shrinking to 3 integers...\n");
    numbers = (int*)goo_realloc(numbers, 10 * sizeof(int), 3 * sizeof(int));
    
    printf("Values after shrinking: ");
    for (int i = 0; i < 3; i++) {
        printf("%d ", numbers[i]);
    }
    printf("\n");
    
    goo_free(numbers, 3 * sizeof(int));
}

// Demo for allocation statistics
void allocation_stats_demo(void) {
    printf("\n===== ALLOCATION STATISTICS DEMO =====\n");
    
    // Get initial stats
    void* allocator = goo_get_default_allocator();
    GooAllocStats initial_stats = goo_get_alloc_stats(allocator);
    
    printf("Initial stats:\n");
    printf("  Bytes allocated: %zu\n", initial_stats.bytes_allocated);
    printf("  Allocation count: %zu\n", initial_stats.allocation_count);
    printf("  Total allocations: %zu\n", initial_stats.total_allocations);
    printf("  Total frees: %zu\n", initial_stats.total_frees);
    
    // Make some allocations
    printf("\nMaking 100 allocations...\n");
    void* ptrs[100];
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = goo_alloc(i * 10 + 100);
    }
    
    // Get mid-point stats
    GooAllocStats mid_stats = goo_get_alloc_stats(allocator);
    
    printf("Stats after allocations:\n");
    printf("  Bytes allocated: %zu\n", mid_stats.bytes_allocated);
    printf("  Allocation count: %zu\n", mid_stats.allocation_count);
    printf("  Total allocations: %zu\n", mid_stats.total_allocations);
    printf("  Total frees: %zu\n", mid_stats.total_frees);
    
    // Free half of the allocations
    printf("\nFreeing 50 allocations...\n");
    for (int i = 0; i < 50; i++) {
        goo_free(ptrs[i], i * 10 + 100);
    }
    
    // Get after-free stats
    GooAllocStats after_free_stats = goo_get_alloc_stats(allocator);
    
    printf("Stats after partial free:\n");
    printf("  Bytes allocated: %zu\n", after_free_stats.bytes_allocated);
    printf("  Allocation count: %zu\n", after_free_stats.allocation_count);
    printf("  Total allocations: %zu\n", after_free_stats.total_allocations);
    printf("  Total frees: %zu\n", after_free_stats.total_frees);
    
    // Free the rest
    printf("\nFreeing remaining allocations...\n");
    for (int i = 50; i < 100; i++) {
        goo_free(ptrs[i], i * 10 + 100);
    }
    
    // Get final stats
    GooAllocStats final_stats = goo_get_alloc_stats(allocator);
    
    printf("Final stats:\n");
    printf("  Bytes allocated: %zu\n", final_stats.bytes_allocated);
    printf("  Allocation count: %zu\n", final_stats.allocation_count);
    printf("  Total allocations: %zu\n", final_stats.total_allocations);
    printf("  Total frees: %zu\n", final_stats.total_frees);
    printf("  Peak memory usage: %zu bytes\n", final_stats.max_bytes_allocated);
}

// Demo for concurrent allocations
void concurrent_allocation_demo(void) {
    printf("\n===== CONCURRENT ALLOCATION DEMO =====\n");
    
    const int num_threads = 4;
    pthread_t threads[num_threads];
    int thread_ids[num_threads];
    
    // Get initial stats
    void* allocator = goo_get_default_allocator();
    GooAllocStats initial_stats = goo_get_alloc_stats(allocator);
    
    printf("Spawning %d threads for concurrent allocations...\n", num_threads);
    
    // Create threads
    for (int i = 0; i < num_threads; i++) {
        thread_ids[i] = i + 1;
        pthread_create(&threads[i], NULL, allocation_thread, &thread_ids[i]);
    }
    
    // Wait for threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Get final stats
    GooAllocStats final_stats = goo_get_alloc_stats(allocator);
    
    printf("\nConcurrent allocation results:\n");
    printf("  Total allocations: %zu\n", final_stats.total_allocations - initial_stats.total_allocations);
    printf("  Total frees: %zu\n", final_stats.total_frees - initial_stats.total_frees);
    printf("  Final allocation count: %zu\n", final_stats.allocation_count);
    printf("  Peak memory usage: %zu bytes\n", final_stats.max_bytes_allocated);
}

// Demo for scope-based memory management
void scope_allocation_demo(void) {
    printf("\n===== SCOPE-BASED ALLOCATION DEMO =====\n");
    
    printf("Outer scope starts\n");
    {
        // This buffer will be automatically freed when it goes out of scope
        GOO_SCOPE_ALLOC(outer_buffer, 1024);
        printf("  Allocated outer_buffer of 1024 bytes at %p\n", outer_buffer);
        
        memset(outer_buffer, 0xAA, 1024);
        
        printf("  Inner scope starts\n");
        {
            // Another auto-freed buffer
            GOO_SCOPE_ALLOC(inner_buffer, 512);
            printf("    Allocated inner_buffer of 512 bytes at %p\n", inner_buffer);
            
            memset(inner_buffer, 0xBB, 512);
            
            printf("    Using both buffers\n");
            // Both buffers are valid here
        }
        printf("  Inner scope ends - inner_buffer automatically freed\n");
        
        // Only outer_buffer is valid here
        printf("  Still using outer_buffer\n");
    }
    printf("Outer scope ends - outer_buffer automatically freed\n");
}

int main(void) {
    printf("***** GOO MEMORY ALLOCATOR DEMONSTRATION (ZIG IMPLEMENTATION) *****\n");
    
    // Set random seed
    srand(time(NULL));
    
    // Initialize the memory system
    if (!goo_memory_init()) {
        printf("FATAL: Failed to initialize memory system!\n");
        return EXIT_FAILURE;
    }
    
    // Run the demos
    basic_allocation_demo();
    aligned_allocation_demo();
    reallocation_demo();
    allocation_stats_demo();
    concurrent_allocation_demo();
    scope_allocation_demo();
    
    // Final stats
    void* allocator = goo_get_default_allocator();
    GooAllocStats final_stats = goo_get_alloc_stats(allocator);
    
    printf("\n***** FINAL ALLOCATOR STATISTICS *****\n");
    printf("  Bytes still allocated: %zu\n", final_stats.bytes_allocated);
    printf("  Active allocations: %zu\n", final_stats.allocation_count);
    printf("  Total allocations performed: %zu\n", final_stats.total_allocations);
    printf("  Total frees performed: %zu\n", final_stats.total_frees);
    printf("  Peak memory usage: %zu bytes\n", final_stats.max_bytes_allocated);
    printf("  Allocation failures: %zu\n", final_stats.failed_allocations);
    
    // Cleanup
    goo_memory_cleanup();
    
    printf("\nMemory demo completed successfully!\n");
    return EXIT_SUCCESS;
} 