/**
 * memory_bridge.c
 * 
 * Bridge implementation connecting the C runtime with the Zig memory allocator.
 * This file provides C wrappers around Zig memory functions to be used by the Goo runtime.
 */

#include <stddef.h>
#include <stdbool.h>
#include "memory/memory.h"
#include "runtime.h"

// External declarations of Zig functions (implemented in memory.zig)
extern bool memoryInit(void);
extern void memoryCleanup(void);
extern void* allocAligned(size_t size, size_t alignment);
extern void freeAligned(void* ptr, size_t size, size_t alignment);
extern void* reallocAligned(void* ptr, size_t old_size, size_t new_size, size_t alignment);

// Constants
#define DEFAULT_ALIGNMENT 16

// C API functions for memory management

// Initialize the memory subsystem
bool goo_memory_init(void) {
    return memoryInit();
}

// Clean up the memory subsystem
void goo_memory_cleanup(void) {
    memoryCleanup();
}

// Allocate memory using the Zig allocator
void* goo_alloc(size_t size) {
    if (size == 0) return NULL;
    return allocAligned(size, DEFAULT_ALIGNMENT);
}

// Free memory allocated with goo_alloc
void goo_free(void* ptr, size_t size) {
    if (ptr == NULL) return;
    freeAligned(ptr, size, DEFAULT_ALIGNMENT);
}

// Reallocate memory using the Zig allocator
void* goo_realloc(void* ptr, size_t old_size, size_t new_size) {
    return reallocAligned(ptr, old_size, new_size, DEFAULT_ALIGNMENT);
}

// Allocate aligned memory
void* goo_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0) return NULL;
    if (alignment == 0) alignment = DEFAULT_ALIGNMENT;
    return allocAligned(size, alignment);
}

// Free aligned memory
void goo_free_aligned(void* ptr, size_t size, size_t alignment) {
    if (ptr == NULL) return;
    if (alignment == 0) alignment = DEFAULT_ALIGNMENT;
    freeAligned(ptr, size, alignment);
}

// Reallocate aligned memory
void* goo_realloc_aligned(void* ptr, size_t old_size, size_t new_size, size_t alignment) {
    if (alignment == 0) alignment = DEFAULT_ALIGNMENT;
    return reallocAligned(ptr, old_size, new_size, alignment);
}

// Memory utility functions for the Goo runtime

// Allocate and zero memory
void* goo_calloc(size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = goo_alloc(total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Create a duplicate of the given string using the Zig allocator
char* goo_strdup(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup = (char*)goo_alloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

// Allocate memory with an error handler
void* goo_alloc_or_panic(size_t size) {
    void* ptr = goo_alloc(size);
    if (!ptr && size > 0) {
        goo_runtime_panic("Out of memory");
    }
    return ptr;
} 