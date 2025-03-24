#include <stdlib.h>
#include <string.h>
#include "goo/runtime/memory.h"
#include "goo/core/types.h"

// Default allocator implementation
static void* default_alloc(void* ctx, size_t size, size_t alignment, GooAllocOptions options) {
    (void)ctx; // Unused
    (void)alignment; // Currently unused in default implementation
    
    void* ptr = malloc(size);
    
    // Handle zero-initialization if requested
    if (ptr && (options & GOO_ALLOC_ZERO)) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

static void* default_realloc(void* ctx, void* ptr, size_t old_size, size_t new_size, 
                            size_t alignment, GooAllocOptions options) {
    (void)ctx; // Unused
    (void)old_size; // Currently unused in default implementation
    (void)alignment; // Currently unused in default implementation
    (void)options; // Currently unused in default implementation
    
    return realloc(ptr, new_size);
}

static void default_free(void* ctx, void* ptr, size_t size, size_t alignment) {
    (void)ctx; // Unused
    (void)size; // Currently unused in default implementation
    (void)alignment; // Currently unused in default implementation
    
    free(ptr);
}

static void default_destroy(void* self) {
    (void)self; // Nothing to do for default allocator
}

// Global allocator state
static GooAllocator g_default_allocator = {
    .alloc = default_alloc,
    .realloc = default_realloc,
    .free = default_free,
    .destroy = default_destroy,
    .strategy = GOO_ALLOC_STRATEGY_NULL,
    .out_of_mem_fn = NULL,
    .context = NULL,
    .track_stats = false,
    .stats = {0}
};

static GooAllocator* g_current_allocator = NULL;

// Initialize the memory subsystem
bool goo_memory_init(void) {
    g_current_allocator = &g_default_allocator;
    return true;
}

// Clean up the memory subsystem
void goo_memory_cleanup(void) {
    g_current_allocator = NULL;
}

// Create a system allocator
GooAllocator* goo_system_allocator_create(void) {
    // We're just returning a reference to our static default allocator
    // In a real implementation, this would create a new instance
    return &g_default_allocator;
}

// Set the default allocator
void goo_set_default_allocator(GooAllocator* allocator) {
    g_current_allocator = allocator ? allocator : &g_default_allocator;
}

// Get the default allocator
GooAllocator* goo_get_default_allocator(void) {
    return g_current_allocator ? g_current_allocator : &g_default_allocator;
}

// Get the thread-local allocator (simplified implementation)
GooAllocator* goo_get_thread_allocator(void) {
    // In a real implementation, this would check thread-local storage
    return goo_get_default_allocator();
}

// Set the thread-local allocator (simplified implementation)
void goo_set_thread_allocator(GooAllocator* allocator) {
    // In a real implementation, this would use thread-local storage
    // Here we just set the default allocator
    goo_set_default_allocator(allocator);
}

// Set the out-of-memory handler
void goo_set_out_of_mem_handler(GooOutOfMemFn handler) {
    g_default_allocator.out_of_mem_fn = handler;
}

// Get allocation statistics
GooAllocStats goo_get_alloc_stats(GooAllocator* allocator) {
    allocator = allocator ? allocator : goo_get_default_allocator();
    return allocator->stats;
}

// Allocate memory
void* goo_memory_alloc(size_t size) {
    return goo_memory_alloc_with(goo_get_default_allocator(), size);
}

// Allocate memory with custom allocator
void* goo_memory_alloc_with(GooAllocator* allocator, size_t size) {
    if (size == 0) return NULL;
    
    allocator = allocator ? allocator : goo_get_default_allocator();
    return allocator->alloc(allocator->context, size, 0, GOO_ALLOC_DEFAULT);
}

// Allocate and zero-initialize memory
void* goo_memory_alloc_zeroed(size_t size) {
    if (size == 0) return NULL;
    
    GooAllocator* allocator = goo_get_default_allocator();
    return allocator->alloc(allocator->context, size, 0, GOO_ALLOC_ZERO);
}

// Reallocate memory
void* goo_memory_realloc(void* ptr, size_t old_size, size_t new_size) {
    return goo_memory_realloc_with(goo_get_default_allocator(), ptr, old_size, new_size);
}

// Reallocate memory with custom allocator
void* goo_memory_realloc_with(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size) {
    if (new_size == 0) {
        goo_memory_free(ptr, old_size);
        return NULL;
    }
    
    if (ptr == NULL) {
        return goo_memory_alloc_with(allocator, new_size);
    }
    
    allocator = allocator ? allocator : goo_get_default_allocator();
    return allocator->realloc(allocator->context, ptr, old_size, new_size, 0, GOO_ALLOC_DEFAULT);
}

// Free memory
void goo_memory_free(void* ptr, size_t size) {
    goo_memory_free_with(goo_get_default_allocator(), ptr, size);
}

// Free memory with custom allocator
void goo_memory_free_with(GooAllocator* allocator, void* ptr, size_t size) {
    if (ptr == NULL) return;
    
    allocator = allocator ? allocator : goo_get_default_allocator();
    allocator->free(allocator->context, ptr, size, 0);
}

// The legacy interface functions have been moved to memory.c
// to avoid duplicate definition errors
#ifdef INCLUDE_LEGACY_MEMORY_FUNCTIONS
// Legacy interface functions for compatibility
void* goo_alloc(size_t size) {
    return goo_memory_alloc(size);
}

void* goo_realloc(void* ptr, size_t size) {
    // For the legacy API, we don't have the old_size
    // In a real implementation, this might use a size tracking mechanism
    return goo_memory_realloc(ptr, 0, size);
}

void goo_free(void* ptr) {
    // For the legacy API, we don't have the size
    // In a real implementation, this might use a size tracking mechanism
    goo_memory_free(ptr, 0);
}
#endif 