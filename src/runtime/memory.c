#include <stdlib.h>
#include <string.h>
#include "include/goo_memory.h"  // Legacy header
#include "include/goo/runtime/memory.h"  // Modern API header
#include "include/goo/core/types.h"

/**
 * Simple implementation of memory functions for the Goo runtime
 * This provides both legacy and modern API compatibility
 */

// Legacy API implementation
void* goo_runtime_alloc(size_t size) {
    return goo_memory_alloc(size);
}

void* goo_runtime_realloc(void* ptr, size_t new_size) {
    // For legacy compatibility, we don't have the old_size
    return goo_memory_realloc(ptr, 0, new_size);
}

void goo_runtime_free(void* ptr) {
    // For legacy compatibility, we don't have the size
    goo_memory_free(ptr, 0);
}

// The following are redirects to maintain compatibility with older code
void* goo_alloc(size_t size) {
    return goo_memory_alloc(size);
}

void* goo_realloc(void* ptr, size_t size) {
    return goo_memory_realloc(ptr, 0, size);
}

void goo_free(void* ptr) {
    goo_memory_free(ptr, 0);
}

// Basic memory operations not provided by memory_interface.c
void goo_memory_copy(void* dest, const void* src, size_t size) {
    if (dest && src && size > 0) {
        memcpy(dest, src, size);
    }
}

void goo_memory_set(void* dest, uint8_t value, size_t size) {
    if (dest && size > 0) {
        memset(dest, value, size);
    }
}

void goo_memory_cleanup_ptr(void** ptr) {
    if (ptr && *ptr) {
        goo_memory_free(*ptr, 0);
        *ptr = NULL;
    }
} 