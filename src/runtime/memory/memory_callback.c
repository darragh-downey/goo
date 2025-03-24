/**
 * memory_callback.c
 * 
 * Implementation of callback handlers for Goo memory management.
 * This file provides the callback functions used for automatic memory management.
 */

#include <stdlib.h>
#include <string.h>
#include "memory/memory.h"
#include "runtime.h"

// Structure to hold cleanup data passed to the callback
typedef struct {
    void* ptr;        // Pointer to be freed
    size_t size;      // Size of the allocation
    size_t alignment; // Alignment of the allocation (0 for default)
} GooMemoryCleanupData;

/**
 * Memory cleanup callback for regular allocations.
 * This function is called when a scope exits and registered memory needs to be freed.
 * 
 * @param data Pointer to the cleanup data
 */
void goo_memory_cleanup_callback(void* data) {
    if (!data) return;
    
    GooMemoryCleanupData* cleanup_data = (GooMemoryCleanupData*)data;
    
    if (cleanup_data->ptr) {
        if (cleanup_data->alignment > 0) {
            goo_free_aligned(cleanup_data->ptr, cleanup_data->size, cleanup_data->alignment);
        } else {
            goo_free(cleanup_data->ptr, cleanup_data->size);
        }
    }
    
    // Free the cleanup data itself
    free(cleanup_data); // Using regular free since this was allocated by the scope system
}

/**
 * Register memory for automatic cleanup when the current scope exits.
 * 
 * @param ptr Pointer to the memory to free
 * @param size Size of the memory allocation
 * @param alignment Alignment of the memory (0 for default)
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_memory_cleanup(void* ptr, size_t size, size_t alignment) {
    if (!ptr) return false;
    
    // Allocate cleanup data
    GooMemoryCleanupData* cleanup_data = (GooMemoryCleanupData*)malloc(sizeof(GooMemoryCleanupData));
    if (!cleanup_data) return false;
    
    // Initialize cleanup data
    cleanup_data->ptr = ptr;
    cleanup_data->size = size;
    cleanup_data->alignment = alignment;
    
    // Register with the runtime scope system
    bool result = goo_scope_register_cleanup(cleanup_data, goo_memory_cleanup_callback);
    if (!result) {
        free(cleanup_data);
        return false;
    }
    
    return true;
}

/**
 * Register a resource with custom cleanup function for automatic cleanup when scope exits.
 * 
 * @param resource Pointer to the resource to clean up
 * @param cleanup_fn Function to call to clean up the resource
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_resource_cleanup(void* resource, void (*cleanup_fn)(void*)) {
    if (!resource || !cleanup_fn) return false;
    
    // Register directly with the runtime scope system
    return goo_scope_register_cleanup(resource, cleanup_fn);
} 