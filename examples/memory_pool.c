/**
 * memory_pool.c
 * 
 * Example demonstrating the usage of Goo's memory pool allocator.
 * This example shows how to create and use memory pools for efficient
 * fixed-size object allocation, with benchmarking comparisons to system malloc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/memory/goo_allocator.h"

// Define a simple object type for demonstration
typedef struct TestObject {
    int id;
    double value;
    char name[32];
    struct TestObject* next;
} TestObject;

// Function to measure time in nanoseconds
uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Benchmark allocation using system malloc
void benchmark_malloc(int iterations, size_t obj_size) {
    printf("Benchmarking system malloc with %d allocations of size %zu bytes:\n", 
           iterations, obj_size);
    
    void** ptrs = malloc(iterations * sizeof(void*));
    
    // Allocation benchmark
    uint64_t start_time = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = malloc(obj_size);
        if (!ptrs[i]) {
            printf("Failed to allocate memory at iteration %d\n", i);
            exit(1);
        }
        
        // Write something to ensure the memory is committed
        memset(ptrs[i], 1, obj_size);
    }
    
    uint64_t end_time = get_time_ns();
    double alloc_time_ms = (end_time - start_time) / 1000000.0;
    printf("  Allocation: %.2f ms (%.2f ns per allocation)\n", 
           alloc_time_ms, (end_time - start_time) / (double)iterations);
    
    // Deallocation benchmark
    start_time = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        free(ptrs[i]);
    }
    
    end_time = get_time_ns();
    double dealloc_time_ms = (end_time - start_time) / 1000000.0;
    printf("  Deallocation: %.2f ms (%.2f ns per deallocation)\n", 
           dealloc_time_ms, (end_time - start_time) / (double)iterations);
    
    printf("  Total: %.2f ms\n\n", alloc_time_ms + dealloc_time_ms);
    
    free(ptrs);
}

// Benchmark allocation using pool allocator
void benchmark_pool(int iterations, size_t obj_size) {
    printf("Benchmarking pool allocator with %d allocations of size %zu bytes:\n", 
           iterations, obj_size);
    
    void** ptrs = malloc(iterations * sizeof(void*));
    
    // Create a system allocator to serve as parent
    GooAllocator* system_alloc = goo_system_allocator_create();
    if (!system_alloc) {
        printf("Failed to create system allocator\n");
        exit(1);
    }
    
    // Create pool allocator with 16-byte alignment
    GooPoolAllocator* pool = goo_pool_allocator_create(system_alloc, obj_size, 16, 1024);
    if (!pool) {
        printf("Failed to create pool allocator\n");
        exit(1);
    }
    
    GooAllocator* pool_alloc = &pool->allocator;
    
    // Allocation benchmark
    uint64_t start_time = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = pool_alloc->alloc(pool_alloc, obj_size, 16, GOO_ALLOC_ZERO);
        if (!ptrs[i]) {
            printf("Failed to allocate memory at iteration %d\n", i);
            exit(1);
        }
        
        // Write something to the memory
        memset(ptrs[i], 1, obj_size);
    }
    
    uint64_t end_time = get_time_ns();
    double alloc_time_ms = (end_time - start_time) / 1000000.0;
    printf("  Allocation: %.2f ms (%.2f ns per allocation)\n", 
           alloc_time_ms, (end_time - start_time) / (double)iterations);
    
    // Deallocation benchmark
    start_time = get_time_ns();
    
    for (int i = 0; i < iterations; i++) {
        pool_alloc->free(pool_alloc, ptrs[i], obj_size, 16);
    }
    
    end_time = get_time_ns();
    double dealloc_time_ms = (end_time - start_time) / 1000000.0;
    printf("  Deallocation: %.2f ms (%.2f ns per deallocation)\n", 
           dealloc_time_ms, (end_time - start_time) / (double)iterations);
    
    printf("  Total: %.2f ms\n\n", alloc_time_ms + dealloc_time_ms);
    
    // Print pool statistics
    size_t free_chunks, total_chunks;
    goo_pool_get_stats(pool, &free_chunks, &total_chunks);
    printf("  Pool statistics after benchmark:\n");
    printf("    Free chunks: %zu\n", free_chunks);
    printf("    Total chunks: %zu\n", total_chunks);
    printf("    Utilized: %.1f%%\n\n", 100.0 * (total_chunks - free_chunks) / total_chunks);
    
    // Test reset functionality
    printf("  Testing pool reset... ");
    goo_pool_reset(pool);
    goo_pool_get_stats(pool, &free_chunks, &total_chunks);
    if (free_chunks == total_chunks) {
        printf("Success! All chunks returned to pool.\n\n");
    } else {
        printf("Failed! Expected %zu free chunks, but got %zu\n\n", total_chunks, free_chunks);
    }
    
    // Clean up allocators
    pool_alloc->destroy(pool_alloc);
    system_alloc->destroy(system_alloc);
    
    free(ptrs);
}

// Demonstrate practical usage with linked list
void demonstrate_linked_list() {
    printf("Demonstrating practical usage with linked list of objects:\n");
    
    // Create system allocator
    GooAllocator* system_alloc = goo_system_allocator_create();
    if (!system_alloc) {
        printf("Failed to create system allocator\n");
        exit(1);
    }
    
    // Create pool allocator for TestObject structures
    GooPoolAllocator* pool = goo_pool_allocator_create(system_alloc, sizeof(TestObject), 16, 100);
    if (!pool) {
        printf("Failed to create pool allocator\n");
        exit(1);
    }
    
    GooAllocator* pool_alloc = &pool->allocator;
    
    // Create a linked list of 1000 objects
    const int num_objects = 1000;
    TestObject* head = NULL;
    TestObject* current = NULL;
    
    printf("  Creating linked list with %d objects...\n", num_objects);
    
    for (int i = 0; i < num_objects; i++) {
        // Allocate new object
        TestObject* obj = (TestObject*)pool_alloc->alloc(pool_alloc, 
                                                        sizeof(TestObject), 
                                                        16, 
                                                        GOO_ALLOC_ZERO);
        
        // Initialize object
        obj->id = i + 1;
        obj->value = (double)i * 1.5;
        snprintf(obj->name, sizeof(obj->name), "Object %d", i + 1);
        obj->next = NULL;
        
        // Add to list
        if (!head) {
            head = obj;
            current = obj;
        } else {
            current->next = obj;
            current = obj;
        }
    }
    
    // Traverse and print some elements
    printf("  Linked list created successfully.\n");
    printf("  Sampling some elements:\n");
    
    current = head;
    for (int i = 0; i < 5 && current; i++) {
        printf("    Node %d: id=%d, value=%.1f, name='%s'\n", 
               i, current->id, current->value, current->name);
        current = current->next;
    }
    
    printf("    ... (%d more objects) ...\n", num_objects - 10);
    
    // Skip to last few objects
    current = head;
    for (int i = 0; i < num_objects - 5 && current; i++) {
        current = current->next;
    }
    
    for (int i = num_objects - 5; i < num_objects && current; i++) {
        printf("    Node %d: id=%d, value=%.1f, name='%s'\n", 
               i, current->id, current->value, current->name);
        current = current->next;
    }
    
    // Free the list (one by one)
    printf("\n  Freeing linked list...\n");
    while (head) {
        TestObject* next = head->next;
        pool_alloc->free(pool_alloc, head, sizeof(TestObject), 16);
        head = next;
    }
    
    // Show pool stats after freeing
    size_t free_chunks, total_chunks;
    goo_pool_get_stats(pool, &free_chunks, &total_chunks);
    printf("  Pool statistics after freeing list:\n");
    printf("    Free chunks: %zu\n", free_chunks);
    printf("    Total chunks: %zu\n", total_chunks);
    printf("    Utilized: %.1f%%\n\n", 100.0 * (total_chunks - free_chunks) / total_chunks);
    
    // Clean up allocators
    pool_alloc->destroy(pool_alloc);
    system_alloc->destroy(system_alloc);
}

int main() {
    printf("===== Goo Memory Pool Allocator Example =====\n\n");
    
    // Run benchmarks with different object sizes
    const int num_iterations = 100000;
    
    // Small object benchmark
    benchmark_malloc(num_iterations, 32);
    benchmark_pool(num_iterations, 32);
    
    // Medium object benchmark
    benchmark_malloc(num_iterations, 128);
    benchmark_pool(num_iterations, 128);
    
    // Larger object benchmark
    benchmark_malloc(num_iterations, sizeof(TestObject));
    benchmark_pool(num_iterations, sizeof(TestObject));
    
    // Demonstrate practical usage
    demonstrate_linked_list();
    
    printf("===== Memory Pool Example Completed =====\n");
    return 0;
} 