#ifndef GOO_RUNTIME_MEMORY_H
#define GOO_RUNTIME_MEMORY_H

/**
 * Unified Memory Management Interface for Goo
 * This provides a consistent API for memory operations
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "../core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocation statistics tracking
 */
typedef struct {
    size_t bytes_allocated;     // Current bytes allocated
    size_t bytes_reserved;      // Current bytes reserved (may be > allocated)
    size_t max_bytes_allocated; // Peak bytes allocated
    size_t allocation_count;    // Number of active allocations
    size_t total_allocations;   // Total allocations made
    size_t total_frees;         // Total frees performed 
    size_t failed_allocations;  // Number of failed allocations
} GooAllocStats;

/**
 * Out-of-memory handler function type
 */
typedef void (*GooOutOfMemFn)(void);

/**
 * Allocator function signatures
 */
typedef void* (*GooAllocFn)(void* ctx, size_t size, size_t alignment, GooAllocOptions options);
typedef void* (*GooReallocFn)(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t alignment, GooAllocOptions options);
typedef void (*GooFreeFn)(void* ctx, void* ptr, size_t size, size_t alignment);
typedef void (*GooDestroyFn)(void* self);

/**
 * Allocator structure
 */
struct GooAllocator {
    // Allocator functions
    GooAllocFn alloc;
    GooReallocFn realloc;
    GooFreeFn free;
    GooDestroyFn destroy;
    
    // Allocator properties
    GooAllocStrategy strategy;
    GooOutOfMemFn out_of_mem_fn;
    void* context;
    bool track_stats;
    
    // Statistics
    GooAllocStats stats;
};

/**
 * Initialize the memory management subsystem
 * 
 * @return true if initialization was successful
 */
bool goo_memory_init(void);

/**
 * Clean up the memory management subsystem
 */
void goo_memory_cleanup(void);

/**
 * Create a system allocator
 * 
 * @return Pointer to the system allocator
 */
GooAllocator* goo_system_allocator_create(void);

/**
 * Set the default allocator
 * 
 * @param allocator The allocator to set as default
 */
void goo_set_default_allocator(GooAllocator* allocator);

/**
 * Get the default allocator
 * 
 * @return The default allocator
 */
GooAllocator* goo_get_default_allocator(void);

/**
 * Get the thread-local allocator
 * 
 * @return The thread-local allocator, or the default allocator if none is set
 */
GooAllocator* goo_get_thread_allocator(void);

/**
 * Set the thread-local allocator
 * 
 * @param allocator The allocator to set for the current thread
 */
void goo_set_thread_allocator(GooAllocator* allocator);

/**
 * Set the out-of-memory handler
 * 
 * @param handler The function to call when out of memory
 */
void goo_set_out_of_mem_handler(GooOutOfMemFn handler);

/**
 * Get allocation statistics
 * 
 * @param allocator The allocator to get statistics for, or NULL for the default
 * @return The allocation statistics
 */
GooAllocStats goo_get_alloc_stats(GooAllocator* allocator);

/**
 * Allocate memory
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* goo_memory_alloc(size_t size);

/**
 * Allocate memory with custom allocator
 * 
 * @param allocator The allocator to use
 * @param size Size in bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 */
void* goo_memory_alloc_with(GooAllocator* allocator, size_t size);

/**
 * Allocate and zero-initialize memory
 * 
 * @param size Size in bytes to allocate
 * @return Pointer to zero-initialized memory or NULL on failure
 */
void* goo_memory_alloc_zeroed(size_t size);

/**
 * Reallocate memory
 * 
 * @param ptr Pointer to previously allocated memory
 * @param old_size Old size of the allocation
 * @param new_size New size to allocate
 * @return Pointer to reallocated memory or NULL on failure
 */
void* goo_memory_realloc(void* ptr, size_t old_size, size_t new_size);

/**
 * Reallocate memory with custom allocator
 * 
 * @param allocator The allocator to use
 * @param ptr Pointer to previously allocated memory
 * @param old_size Old size of the allocation
 * @param new_size New size to allocate
 * @return Pointer to reallocated memory or NULL on failure
 */
void* goo_memory_realloc_with(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size);

/**
 * Free memory
 * 
 * @param ptr Pointer to allocated memory
 * @param size Size of the allocation
 */
void goo_memory_free(void* ptr, size_t size);

/**
 * Free memory with custom allocator
 * 
 * @param allocator The allocator to use
 * @param ptr Pointer to allocated memory
 * @param size Size of the allocation
 */
void goo_memory_free_with(GooAllocator* allocator, void* ptr, size_t size);

/**
 * Allocate aligned memory
 * 
 * @param size Size in bytes to allocate
 * @param alignment Alignment requirement (must be a power of 2)
 * @return Pointer to aligned memory or NULL on failure
 */
void* goo_memory_alloc_aligned(size_t size, size_t alignment);

/**
 * Free aligned memory
 * 
 * @param ptr Pointer to aligned memory
 * @param size Size of the allocation
 * @param alignment Alignment that was used for allocation
 */
void goo_memory_free_aligned(void* ptr, size_t size, size_t alignment);

/**
 * Copy memory
 * 
 * @param dest Destination pointer
 * @param src Source pointer
 * @param size Number of bytes to copy
 */
void goo_memory_copy(void* dest, const void* src, size_t size);

/**
 * Set memory to a specific value
 * 
 * @param dest Destination pointer
 * @param value Value to set (each byte will be set to this value)
 * @param size Number of bytes to set
 */
void goo_memory_set(void* dest, uint8_t value, size_t size);

/**
 * Helper for scope-based allocations using cleanup attributes
 * For use with __attribute__((cleanup(goo_memory_cleanup_ptr)))
 * 
 * @param ptr Pointer to the pointer to free
 */
void goo_memory_cleanup_ptr(void** ptr);

#ifdef __cplusplus
}
#endif

#endif // GOO_RUNTIME_MEMORY_H 