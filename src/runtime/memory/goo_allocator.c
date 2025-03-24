#include "goo_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdatomic.h>
#include <stdalign.h>

// Allocator constants
#define GOO_DEFAULT_ALIGNMENT 8
#define GOO_DEFAULT_ARENA_SIZE (64 * 1024)  // 64 KB

// Forward declarations
static void default_out_of_memory_handler(void);
static void* system_alloc(GooAllocator* self, size_t size, size_t alignment, GooAllocOptions options);
static void* system_realloc(GooAllocator* self, void* ptr, size_t old_size, size_t new_size, 
                          size_t alignment, GooAllocOptions options);
static void system_free(GooAllocator* self, void* ptr, size_t size, size_t alignment);
static void system_destroy(GooAllocator* self);

// System allocator implementation
typedef struct {
    GooAllocator allocator;
    atomic_bool initialized;
} GooSystemAllocator;

// Global/default allocators
static GooSystemAllocator g_system_allocator = {0};
GooAllocator* goo_default_allocator = NULL;
__thread GooAllocator* goo_thread_allocator = NULL;

// Initialize memory system
bool goo_memory_init(void) {
    // Skip if already initialized
    bool expected = false;
    if (!atomic_compare_exchange_strong(&g_system_allocator.initialized, &expected, true)) {
        return true; // Already initialized
    }

    // Initialize system allocator
    GooAllocator* system_allocator = &g_system_allocator.allocator;
    system_allocator->alloc = system_alloc;
    system_allocator->realloc = system_realloc;
    system_allocator->free = system_free;
    system_allocator->destroy = system_destroy;
    system_allocator->strategy = GOO_ALLOC_STRATEGY_PANIC;
    system_allocator->out_of_mem_fn = default_out_of_memory_handler;
    system_allocator->context = NULL;
    system_allocator->track_stats = true;
    memset(&system_allocator->stats, 0, sizeof(GooAllocStats));

    // Set as default allocator
    goo_default_allocator = system_allocator;

    return true;
}

void goo_memory_cleanup(void) {
    // Nothing to do for now
}

// Set default allocator
void goo_set_default_allocator(GooAllocator* allocator) {
    goo_default_allocator = allocator;
}

// Get default allocator
GooAllocator* goo_get_default_allocator(void) {
    if (goo_default_allocator == NULL) {
        goo_memory_init();
    }
    return goo_default_allocator;
}

// Get thread-local allocator
GooAllocator* goo_get_thread_allocator(void) {
    if (goo_thread_allocator == NULL) {
        return goo_get_default_allocator();
    }
    return goo_thread_allocator;
}

// Set thread-local allocator
void goo_set_thread_allocator(GooAllocator* allocator) {
    goo_thread_allocator = allocator;
}

// Calculate aligned address
static void* align_pointer(void* ptr, size_t alignment) {
    return (void*)(((uintptr_t)ptr + alignment - 1) & ~(alignment - 1));
}

// Calculate aligned size
static size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Handle allocation failure based on strategy
static void* handle_allocation_failure(GooAllocator* allocator, size_t size) {
    if (allocator->track_stats) {
        allocator->stats.failed_allocations++;
    }

    switch (allocator->strategy) {
        case GOO_ALLOC_STRATEGY_PANIC:
            if (allocator->out_of_mem_fn) {
                allocator->out_of_mem_fn();
            }
            fprintf(stderr, "Fatal error: Out of memory (requested %zu bytes)\n", size);
            abort();
            return NULL; // Never reached

        case GOO_ALLOC_STRATEGY_RETRY:
            if (allocator->out_of_mem_fn) {
                allocator->out_of_mem_fn();
                // Try again after handler runs
                return allocator->alloc(allocator, size, GOO_DEFAULT_ALIGNMENT, GOO_ALLOC_DEFAULT);
            }
            // Fall through to NULL if no handler

        case GOO_ALLOC_STRATEGY_NULL:
        case GOO_ALLOC_STRATEGY_GC: // GC not implemented yet
        default:
            return NULL;
    }
}

// System allocator implementation
static void* system_alloc(GooAllocator* self, size_t size, size_t alignment, GooAllocOptions options) {
    if (size == 0) {
        return NULL;
    }

    // Use standard alignment if not specified
    if (alignment == 0) {
        alignment = GOO_DEFAULT_ALIGNMENT;
    }

    // For simple cases, use malloc directly
    void* ptr;
    if (alignment <= GOO_DEFAULT_ALIGNMENT) {
        ptr = malloc(size);
    } else {
        // Use aligned allocation
#ifdef _WIN32
        ptr = _aligned_malloc(size, alignment);
#else
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = NULL;
        }
#endif
    }

    // Handle allocation failure
    if (ptr == NULL) {
        return handle_allocation_failure(self, size);
    }

    // Zero memory if requested
    if (options & GOO_ALLOC_ZERO) {
        memset(ptr, 0, size);
    }

    // Update statistics
    if (self->track_stats) {
        self->stats.bytes_allocated += size;
        self->stats.bytes_reserved += size;
        if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
            self->stats.max_bytes_allocated = self->stats.bytes_allocated;
        }
        self->stats.allocation_count++;
        self->stats.total_allocations++;
    }

    return ptr;
}

static void* system_realloc(GooAllocator* self, void* ptr, size_t old_size, size_t new_size, 
                          size_t alignment, GooAllocOptions options) {
    // Handle special cases
    if (ptr == NULL) {
        return self->alloc(self, new_size, alignment, options);
    }
    
    if (new_size == 0) {
        self->free(self, ptr, old_size, alignment);
        return NULL;
    }

    // Update statistics for the old allocation
    if (self->track_stats) {
        self->stats.bytes_allocated -= old_size;
        self->stats.bytes_reserved -= old_size;
        self->stats.allocation_count--;
    }

    // Use aligned realloc for larger alignments
    void* new_ptr;
    if (alignment <= GOO_DEFAULT_ALIGNMENT) {
        new_ptr = realloc(ptr, new_size);
    } else {
        // For aligned memory, we need to allocate new memory and copy
#ifdef _WIN32
        new_ptr = _aligned_malloc(new_size, alignment);
#else
        if (posix_memalign(&new_ptr, alignment, new_size) != 0) {
            new_ptr = NULL;
        }
#endif
        if (new_ptr != NULL) {
            // Copy old data
            size_t copy_size = (old_size < new_size) ? old_size : new_size;
            memcpy(new_ptr, ptr, copy_size);
            // Free old memory
#ifdef _WIN32
            _aligned_free(ptr);
#else
            free(ptr);
#endif
        }
    }

    // Handle allocation failure
    if (new_ptr == NULL) {
        // Restore statistics for the old allocation since we still have it
        if (self->track_stats) {
            self->stats.bytes_allocated += old_size;
            self->stats.bytes_reserved += old_size;
            self->stats.allocation_count++;
        }
        return handle_allocation_failure(self, new_size);
    }

    // Update statistics for the new allocation
    if (self->track_stats) {
        self->stats.bytes_allocated += new_size;
        self->stats.bytes_reserved += new_size;
        if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
            self->stats.max_bytes_allocated = self->stats.bytes_allocated;
        }
        self->stats.allocation_count++;
        self->stats.total_allocations++;
    }

    return new_ptr;
}

static void system_free(GooAllocator* self, void* ptr, size_t size, size_t alignment) {
    if (ptr == NULL) {
        return;
    }

    // Update statistics
    if (self->track_stats) {
        self->stats.bytes_allocated -= size;
        self->stats.bytes_reserved -= size;
        self->stats.allocation_count--;
        self->stats.total_frees++;
    }

    // Use appropriate free function based on alignment
    if (alignment <= GOO_DEFAULT_ALIGNMENT) {
        free(ptr);
    } else {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
}

static void system_destroy(GooAllocator* self) {
    // System allocator is a singleton, so we don't actually destroy it
    memset(&self->stats, 0, sizeof(GooAllocStats));
}

// Default out-of-memory handler
static void default_out_of_memory_handler(void) {
    // In the future, this could attempt to free caches or trigger GC
    fprintf(stderr, "Error: Out of memory\n");
}

// Create a system allocator
GooAllocator* goo_system_allocator_create(void) {
    // We maintain a singleton system allocator
    if (!g_system_allocator.initialized) {
        goo_memory_init();
    }
    return &g_system_allocator.allocator;
}

// Utility functions

// Set out-of-memory handler
void goo_set_out_of_mem_handler(GooOutOfMemFn handler) {
    GooAllocator* allocator = goo_get_default_allocator();
    allocator->out_of_mem_fn = handler;
}

// Helper for scope-based allocations
void goo_scope_cleanup(void** ptr) {
    if (ptr && *ptr) {
        goo_free(*ptr, 0); // Size 0 is suboptimal but works for simple cases
        *ptr = NULL;
    }
}

// Simple allocation using default allocator
void* goo_alloc(size_t size) {
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    return allocator->alloc(allocator, size, GOO_DEFAULT_ALIGNMENT, GOO_ALLOC_DEFAULT);
}

// Zero-initialized allocation
void* goo_alloc_zero(size_t size) {
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    return allocator->alloc(allocator, size, GOO_DEFAULT_ALIGNMENT, GOO_ALLOC_ZERO);
}

// Reallocation
void* goo_realloc(void* ptr, size_t old_size, size_t new_size) {
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    return allocator->realloc(allocator, ptr, old_size, new_size, GOO_DEFAULT_ALIGNMENT, GOO_ALLOC_DEFAULT);
}

// Free memory
void goo_free(void* ptr, size_t size) {
    if (ptr == NULL) {
        return;
    }
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    allocator->free(allocator, ptr, size, GOO_DEFAULT_ALIGNMENT);
}

// Aligned allocation
void* goo_alloc_aligned(size_t size, size_t alignment) {
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    return allocator->alloc(allocator, size, alignment, GOO_ALLOC_DEFAULT);
}

// Free aligned memory
void goo_free_aligned(void* ptr, size_t size, size_t alignment) {
    if (ptr == NULL) {
        return;
    }
    GooAllocator* allocator = goo_get_thread_allocator();
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    allocator->free(allocator, ptr, size, alignment);
}

// Get allocation statistics
GooAllocStats goo_get_alloc_stats(GooAllocator* allocator) {
    if (allocator == NULL) {
        allocator = goo_get_default_allocator();
    }
    return allocator->stats;
}

// Memory API with prefix to avoid conflicts with goo.h
void* goo_memory_alloc(size_t size) {
    return goo_alloc(size);
}

void* goo_memory_realloc(void* ptr, size_t old_size, size_t new_size) {
    return goo_realloc(ptr, old_size, new_size);
}

void goo_memory_free(void* ptr, size_t size) {
    goo_free(ptr, size);
} 