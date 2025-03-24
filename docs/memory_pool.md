# Memory Pool Allocator

The Goo Memory Pool Allocator provides efficient memory management for fixed-size allocations, dramatically reducing allocation overhead and memory fragmentation compared to general-purpose allocators. This document covers the design, implementation, and usage of the pool allocator.

## Overview

Memory pools (also known as object pools or memory arenas) are a memory allocation pattern that pre-allocates a large block of memory and subdivides it into smaller, fixed-size chunks. When the application requests memory, the pool allocator returns one of these chunks without needing to interact with the system allocator for each request.

### Key Benefits

- **Performance**: Much faster allocation and deallocation compared to general-purpose allocators (often 10-100x faster)
- **Reduced Fragmentation**: By allocating fixed-size chunks, memory fragmentation is virtually eliminated
- **Memory Locality**: Allocations from the same pool are placed close together in memory, improving cache efficiency
- **Predictable Memory Usage**: Memory consumption is more predictable and controllable

### Use Cases

Memory pools are ideal for scenarios where:

- Many objects of the same size are frequently allocated and deallocated
- Performance is critical, such as in real-time systems or high-performance code
- Memory fragmentation needs to be minimized
- Allocation patterns are predictable

Common applications include:
- Object allocation for data structures (nodes in linked lists, trees, etc.)
- Managing large numbers of small objects
- Handling fixed-size messages or packets
- Game entity management
- Buffer pooling for I/O operations

## Implementation Details

The Goo Memory Pool Allocator consists of several key components:

### Data Structures

1. **GooPoolAllocator**: The main structure that implements the `GooAllocator` interface and holds pool-specific data.
2. **PoolBlock**: Represents a large memory block that gets subdivided into chunks.
3. **FreeChunk**: A simple header for tracking free chunks in a linked list.

### Memory Organization

The pool allocator manages memory in this hierarchical structure:

```
GooPoolAllocator
└── PoolBlock 1
    ├── Chunk 1
    ├── Chunk 2
    └── ... (more chunks)
└── PoolBlock 2
    ├── Chunk 1
    ├── Chunk 2
    └── ... (more chunks)
└── ... (more blocks as needed)
```

- Each PoolBlock contains multiple chunks of the same size
- Free chunks are tracked in a singly-linked free list
- When the free list is empty, new blocks are allocated automatically

### Allocation Strategy

1. When a request is made, the allocator first checks if the requested size fits in a chunk
2. If the free list is empty, a new block is allocated and all its chunks are added to the free list
3. The allocator removes the first chunk from the free list and returns it to the caller
4. When memory is freed, the chunk is returned to the free list for reuse

## API Reference

### Core Functions

#### Creation and Destruction

```c
// Create a memory pool allocator with default settings
GooPoolAllocator* goo_pool_allocator_create(
    GooAllocator* parent, 
    size_t chunk_size, 
    size_t alignment, 
    size_t initial_capacity
);

// Create a memory pool allocator with specific block size
GooPoolAllocator* goo_pool_allocator_create_sized(
    GooAllocator* parent, 
    size_t chunk_size, 
    size_t alignment, 
    size_t chunks_per_block
);
```

#### Pool Management

```c
// Reset a memory pool (return all allocations to the pool)
void goo_pool_reset(GooPoolAllocator* pool);

// Get pool statistics
void goo_pool_get_stats(
    GooPoolAllocator* pool, 
    size_t* free_chunks, 
    size_t* total_chunks
);
```

#### Allocator Interface Functions

The pool allocator implements the standard `GooAllocator` interface, which includes:

```c
// Allocate memory from the pool
void* pool_alloc(
    GooAllocator* self, 
    size_t size, 
    size_t alignment, 
    GooAllocOptions options
);

// Reallocate memory (note: only works if new size fits in a chunk)
void* pool_realloc(
    GooAllocator* self, 
    void* ptr, 
    size_t old_size, 
    size_t new_size, 
    size_t alignment, 
    GooAllocOptions options
);

// Free memory back to the pool
void pool_free(
    GooAllocator* self, 
    void* ptr, 
    size_t size, 
    size_t alignment
);

// Destroy the pool and free all its memory
void pool_destroy(GooAllocator* self);
```

### Parameters

- **parent**: Parent allocator used to allocate blocks for the pool
- **chunk_size**: Size of each individual chunk in the pool
- **alignment**: Memory alignment requirement for chunks
- **initial_capacity**: Number of initial chunks to allocate
- **chunks_per_block**: Number of chunks per block (for `create_sized` variant)

## Usage Examples

### Basic Usage

```c
// Create a system allocator as the parent
GooAllocator* system_alloc = goo_system_allocator_create();

// Create a pool allocator for objects of size 128 bytes with 16-byte alignment
// and an initial capacity of 1024 chunks
GooPoolAllocator* pool = goo_pool_allocator_create(system_alloc, 128, 16, 1024);

// Get the allocator interface
GooAllocator* pool_alloc = &pool->allocator;

// Allocate memory from the pool
void* ptr = pool_alloc->alloc(pool_alloc, 100, 16, GOO_ALLOC_ZERO);

// Use the memory...

// Free memory back to the pool
pool_alloc->free(pool_alloc, ptr, 100, 16);

// Get pool statistics
size_t free_chunks, total_chunks;
goo_pool_get_stats(pool, &free_chunks, &total_chunks);
printf("Pool usage: %zu/%zu chunks\n", total_chunks - free_chunks, total_chunks);

// Reset the pool (return all allocations to free list)
goo_pool_reset(pool);

// Destroy the pool when done
pool_alloc->destroy(pool_alloc);
system_alloc->destroy(system_alloc);
```

### Object Allocation Example

```c
// Define a structure
typedef struct MyObject {
    int id;
    char name[32];
    double value;
} MyObject;

// Create a pool allocator for MyObject structures
GooPoolAllocator* obj_pool = goo_pool_allocator_create(
    system_alloc, sizeof(MyObject), 16, 100
);
GooAllocator* allocator = &obj_pool->allocator;

// Allocate an object
MyObject* obj = (MyObject*)allocator->alloc(
    allocator, sizeof(MyObject), 16, GOO_ALLOC_ZERO
);

// Initialize the object
obj->id = 42;
strcpy(obj->name, "Example");
obj->value = 3.14;

// Use the object...

// Free the object
allocator->free(allocator, obj, sizeof(MyObject), 16);

// Destroy the pool
allocator->destroy(allocator);
```

## Performance Considerations

### Optimal Usage

For best performance:

1. Choose a chunk size that matches your most common allocation size
2. Set alignment based on the data structure requirements
3. Choose initial capacity based on expected number of objects
4. Use `goo_pool_reset()` instead of individual frees when possible
5. Consider creating separate pools for different object sizes

### Limitations

- Not suitable for varying allocation sizes (wastes memory)
- Reallocation only works if the new size fits in the original chunk size
- Not thread-safe by default (use a mutex or thread-local pools for concurrent access)

## Integration with Goo Memory System

The memory pool allocator follows the standard `GooAllocator` interface, allowing it to:

- Work with all memory utilities in the Goo runtime
- Integrate with error handling mechanisms
- Use allocation tracking facilities
- Support allocation options like zero-initialization
- Be used with scoped allocations

This integration enables the pool allocator to be used anywhere the standard allocator is expected, making it easy to optimize specific parts of your code without changing the overall memory management approach.

## Future Enhancements

Potential improvements planned for the pool allocator:

- Thread-safe implementation variant
- Configurable allocation/free strategies
- Multiple chunk sizes within a single pool
- Adaptive block sizing based on allocation patterns
- Memory access pattern optimization for NUMA systems
- Integration with memory safety features 