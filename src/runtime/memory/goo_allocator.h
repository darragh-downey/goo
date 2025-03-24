#ifndef GOO_ALLOCATOR_H
#define GOO_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct GooAllocator GooAllocator;
typedef struct GooMemoryArena GooMemoryArena;
typedef struct GooAllocStats GooAllocStats;
typedef struct GooRegionAllocator GooRegionAllocator;
typedef struct GooPoolAllocator GooPoolAllocator;

// Memory allocation error handling function type
typedef void (*GooOutOfMemFn)(void);

// Allocation options
typedef enum {
    GOO_ALLOC_DEFAULT     = 0,       // Standard allocation
    GOO_ALLOC_ZERO        = 1 << 0,  // Zero the memory
    GOO_ALLOC_ALIGNED     = 1 << 1,  // Use aligned allocation
    GOO_ALLOC_EXACT       = 1 << 2,  // Exact size allocation (no extra padding)
    GOO_ALLOC_PERSISTENT  = 1 << 3,  // Memory persists across arena resets
    GOO_ALLOC_PAGE        = 1 << 4,  // Use page-aligned allocation
    GOO_ALLOC_NO_FAIL     = 1 << 5,  // Abort on allocation failure
} GooAllocOptions;

// Error handling strategy
typedef enum {
    GOO_ALLOC_STRATEGY_PANIC,   // Abort process on failure
    GOO_ALLOC_STRATEGY_NULL,    // Return NULL on failure
    GOO_ALLOC_STRATEGY_RETRY,   // Retry allocation after calling out-of-memory handler
    GOO_ALLOC_STRATEGY_GC,      // Trigger garbage collection (future)
} GooAllocStrategy;

// Allocation statistics
typedef struct GooAllocStats {
    size_t bytes_allocated;     // Total bytes currently allocated
    size_t bytes_reserved;      // Total bytes reserved (may be higher than allocated)
    size_t max_bytes_allocated; // Peak allocation
    size_t allocation_count;    // Number of active allocations
    size_t total_allocations;   // Total number of allocations
    size_t total_frees;         // Total number of frees
    size_t failed_allocations;  // Number of failed allocations
} GooAllocStats;

// Allocator interface - similar to Zig's Allocator
typedef struct GooAllocator {
    // Core allocation functions
    void* (*alloc)(struct GooAllocator* self, size_t size, size_t alignment, GooAllocOptions options);
    void* (*realloc)(struct GooAllocator* self, void* ptr, size_t old_size, size_t new_size, 
                    size_t alignment, GooAllocOptions options);
    void (*free)(struct GooAllocator* self, void* ptr, size_t size, size_t alignment);
    void (*destroy)(struct GooAllocator* self);
    
    // Error handling strategy and callback
    GooAllocStrategy strategy;
    GooOutOfMemFn out_of_mem_fn;
    
    // Allocator-specific data
    void* context;
    
    // Optional statistics tracking
    bool track_stats;
    GooAllocStats stats;
} GooAllocator;

// Memory arena allocator - fast allocations from pre-allocated blocks
typedef struct GooMemoryArena {
    GooAllocator allocator;     // Base allocator interface
    GooAllocator* parent;       // Parent allocator used to get memory
    void* blocks;               // Linked list of memory blocks
    size_t block_size;          // Size of each block
    size_t current_offset;      // Current allocation position
    void* current_block;        // Current block being allocated from
    bool allow_resize;          // Whether blocks can be resized
} GooMemoryArena;

// Region allocator - scoped memory management
typedef struct GooRegionAllocator {
    GooAllocator allocator;     // Base allocator interface
    GooAllocator* parent;       // Parent allocator used to get memory
    void* regions;              // List of active regions
    unsigned int current_depth; // Current region nesting depth
} GooRegionAllocator;

// Pool allocator - efficient fixed-size object allocation
typedef struct GooPoolAllocator {
    GooAllocator allocator;     // Base allocator interface
    GooAllocator* parent;       // Parent allocator used to get memory
    void* free_list;            // Linked list of free blocks
    void* blocks;               // Linked list of allocated memory blocks
    size_t block_size;          // Size of each memory block
    size_t chunk_size;          // Size of each allocation chunk
    size_t chunks_per_block;    // Number of chunks per block
    size_t alignment;           // Alignment of allocations
    size_t free_chunks;         // Number of available chunks
    size_t total_chunks;        // Total number of chunks
} GooPoolAllocator;

// Default/global allocators
extern GooAllocator* goo_default_allocator;  // Default system allocator (thread-safe)
extern __thread GooAllocator* goo_thread_allocator;  // Thread-local allocator (not thread-safe)

// Initialize the memory subsystem
bool goo_memory_init(void);

// Clean up the memory subsystem
void goo_memory_cleanup(void);

// Set the default allocator
void goo_set_default_allocator(GooAllocator* allocator);

// Get the default allocator
GooAllocator* goo_get_default_allocator(void);

// Get the thread-local allocator
GooAllocator* goo_get_thread_allocator(void);

// Set the thread-local allocator
void goo_set_thread_allocator(GooAllocator* allocator);

// Create a system allocator (malloc/free based)
GooAllocator* goo_system_allocator_create(void);

// Create a memory arena allocator
GooMemoryArena* goo_arena_allocator_create(GooAllocator* parent, size_t block_size);

// Reset a memory arena (free all memory except persistent allocations)
void goo_arena_reset(GooMemoryArena* arena);

// Create a region allocator
GooRegionAllocator* goo_region_allocator_create(GooAllocator* parent);

// Begin a new memory region
void goo_region_begin(GooRegionAllocator* region);

// End the current memory region (free all memory allocated in this region)
void goo_region_end(GooRegionAllocator* region);

// Utility functions for common allocations (using default allocator)
void* goo_alloc(size_t size);
void* goo_alloc_zero(size_t size);
void* goo_realloc(void* ptr, size_t old_size, size_t new_size);
void goo_free(void* ptr, size_t size);

// Allocations with custom alignment
void* goo_alloc_aligned(size_t size, size_t alignment);
void goo_free_aligned(void* ptr, size_t size, size_t alignment);

// Scope-based allocations (auto-freed at end of scope)
#define GOO_SCOPE_ALLOC(name, size) \
    __attribute__((cleanup(goo_scope_cleanup))) void* name = goo_alloc(size)

// Temporary allocations (use arena allocator, auto-reset)
void* goo_temp_alloc(size_t size);
void goo_temp_reset(void);

// Set out-of-memory handler
void goo_set_out_of_mem_handler(GooOutOfMemFn handler);

// Get allocation statistics
GooAllocStats goo_get_alloc_stats(GooAllocator* allocator);

// Helper for scope-based allocations
void goo_scope_cleanup(void** ptr);

// Create a memory pool allocator
GooPoolAllocator* goo_pool_allocator_create(GooAllocator* parent, size_t chunk_size, 
                                           size_t alignment, size_t initial_capacity);

// Create a sized memory pool allocator with specific block size
GooPoolAllocator* goo_pool_allocator_create_sized(GooAllocator* parent, size_t chunk_size, 
                                                 size_t alignment, size_t chunks_per_block);

// Reset a memory pool (return all allocations to the pool)
void goo_pool_reset(GooPoolAllocator* pool);

// Get pool statistics
void goo_pool_get_stats(GooPoolAllocator* pool, size_t* free_chunks, size_t* total_chunks);

#endif // GOO_ALLOCATOR_H 