# Goo Memory Management System

This directory contains the implementation of Goo's memory management system, providing a flexible and extensible memory allocation interface.

## Architecture

The memory system is organized as follows:

1. **Legacy API** - Simple allocation functions for backward compatibility
   - Available in `include/goo_memory.h`
   - Implementation in `src/runtime/memory.c`

2. **Modern API** - Advanced allocation with custom allocators and options
   - Available in `include/goo/runtime/memory.h`
   - Core implementation in `src/runtime/memory/memory_interface.c`

3. **Core Types** - Type definitions that both APIs depend on
   - Available in `include/goo/core/types.h`

## API Layers

### Legacy API (Simple)

```c
// Basic allocation
void* goo_runtime_alloc(size_t size);
void* goo_runtime_realloc(void* ptr, size_t new_size);
void goo_runtime_free(void* ptr);

// Legacy aliases
void* goo_alloc(size_t size);
void* goo_realloc(void* ptr, size_t size);
void goo_free(void* ptr);
```

### Modern API (Advanced)

```c
// Basic allocation
void* goo_memory_alloc(size_t size);
void* goo_memory_realloc(void* ptr, size_t old_size, size_t new_size);
void goo_memory_free(void* ptr, size_t size);

// Zero-initialized allocation
void* goo_memory_alloc_zeroed(size_t size);

// Custom allocator usage
void* goo_memory_alloc_with(GooAllocator* allocator, size_t size);
void* goo_memory_realloc_with(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size);
void goo_memory_free_with(GooAllocator* allocator, void* ptr, size_t size);

// Allocator management
GooAllocator* goo_system_allocator_create(void);
void goo_set_default_allocator(GooAllocator* allocator);
GooAllocator* goo_get_default_allocator(void);

// Utility functions
void goo_memory_copy(void* dest, const void* src, size_t size);
void goo_memory_set(void* dest, uint8_t value, size_t size);
```

## Custom Allocator Support

The memory system allows defining custom allocators with different strategies:

```c
struct GooAllocator {
    // Function pointers
    GooAllocFn alloc;
    GooReallocFn realloc;
    GooFreeFn free;
    GooDestroyFn destroy;
    
    // Properties
    GooAllocStrategy strategy;
    GooOutOfMemFn out_of_mem_fn;
    void* context;
    bool track_stats;
    
    // Statistics
    GooAllocStats stats;
};
```

## Specialized Allocator Implementations

The memory system includes several specialized allocator types:

1. **Arena Allocator** - Fast allocation with bulk free (in `goo_arena_allocator.zig`)
2. **Pool Allocator** - For fixed-sized objects (in `goo_pool_allocator.c` and `.zig`)
3. **Region Allocator** - For memory regions (in `goo_region_allocator.c` and `.zig`)
4. **Scoped Allocator** - For automatic cleanup (in `scoped_alloc.zig`)

## Integration Points

1. **Memory Bridge** - Facilitates interop between C and Zig (in `memory_bridge.c`)
2. **Memory Stats** - Tracks memory usage (in `goo_memory_stats.c`)
3. **Allocation Callbacks** - For monitoring or intercepting allocations (in `memory_callback.c`)

## Usage Examples

Basic usage:

```c
// Initialize memory system
goo_memory_init();

// Allocate memory
void* mem = goo_memory_alloc(1024);

// Use memory...

// Free memory
goo_memory_free(mem, 1024);

// Clean up memory system
goo_memory_cleanup();
```

Advanced usage with custom allocator:

```c
// Create a pool allocator
GooAllocator* pool = goo_pool_allocator_create(64, 100);

// Allocate from pool
void* obj = goo_memory_alloc_with(pool, 64);

// Free back to pool
goo_memory_free_with(pool, obj, 64);

// Destroy pool when done
pool->destroy(pool);
``` 