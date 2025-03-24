/**
 * goo_pool_allocator.c
 * 
 * Implementation of a memory pool allocator for the Goo programming language.
 * Provides efficient fixed-size object allocation with minimal fragmentation.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "../memory/goo_allocator.h"

// Memory block in the pool
typedef struct PoolBlock {
    struct PoolBlock* next;    // Next block in the chain
    size_t capacity;           // Number of chunks in this block
    size_t size;               // Size of this block in bytes
    char data[];               // Flexible array for the actual memory
} PoolBlock;

// Free chunk header (overlays with the allocated memory)
typedef struct FreeChunk {
    struct FreeChunk* next;    // Next free chunk in the list
} FreeChunk;

// Helper functions for alignment
static size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Calculate optimal block size for a given chunk size and count
static size_t calculate_block_size(size_t chunk_size, size_t count) {
    // Ensure minimum alignment of chunk_size
    const size_t min_chunk_size = 8;  // Minimum chunk size must fit a pointer
    chunk_size = chunk_size < min_chunk_size ? min_chunk_size : chunk_size;
    
    // Aim for blocks that are roughly page-sized by default
    const size_t target_block_size = 4096;  // Typical page size
    
    // Calculate total size needed
    size_t data_size = chunk_size * count;
    size_t total_size = sizeof(PoolBlock) + data_size;
    
    // If too small, increase to a reasonable size
    if (total_size < target_block_size / 4) {
        // Calculate how many chunks would fit in a reasonable block
        size_t new_count = (target_block_size - sizeof(PoolBlock)) / chunk_size;
        return sizeof(PoolBlock) + (new_count * chunk_size);
    }
    
    // If too large, try to align to page size
    if (total_size > target_block_size) {
        size_t pages = (total_size + target_block_size - 1) / target_block_size;
        return pages * target_block_size;
    }
    
    return total_size;
}

// Create a new memory block for the pool
static PoolBlock* create_pool_block(GooAllocator* parent, size_t block_size, 
                                    size_t chunk_size, FreeChunk** free_list) {
    PoolBlock* block = parent->alloc(parent, block_size, 16, GOO_ALLOC_ZERO);
    if (!block) {
        return NULL;
    }
    
    block->next = NULL;
    block->size = block_size;
    
    // Calculate number of chunks that fit in this block
    size_t usable_size = block_size - sizeof(PoolBlock);
    size_t chunks = usable_size / chunk_size;
    block->capacity = chunks;
    
    // Initialize free list with chunks from this block
    char* data = block->data;
    for (size_t i = 0; i < chunks; i++) {
        FreeChunk* chunk = (FreeChunk*)(data + (i * chunk_size));
        chunk->next = *free_list;
        *free_list = chunk;
    }
    
    return block;
}

// Pool allocator functions

// Forward declarations
static void pool_free(GooAllocator* self, void* ptr, size_t size, size_t alignment);

static void* pool_alloc(GooAllocator* self, size_t size, size_t alignment, GooAllocOptions options) {
    GooPoolAllocator* pool = (GooPoolAllocator*)self;
    
    // Update allocation stats if enabled
    if (self->track_stats) {
        self->stats.total_allocations++;
    }
    
    // Validate request size against pool chunk size
    if (size > pool->chunk_size) {
        // Request too large for this pool
        if (self->track_stats) {
            self->stats.failed_allocations++;
        }
        
        // Handle allocation failure based on strategy
        switch (self->strategy) {
            case GOO_ALLOC_STRATEGY_PANIC:
                fprintf(stderr, "FATAL: Allocation request too large for pool (requested %zu, max %zu)\n", 
                        size, pool->chunk_size);
                abort();
                break;
                
            case GOO_ALLOC_STRATEGY_RETRY:
                if (self->out_of_mem_fn) {
                    self->out_of_mem_fn();
                    // Try again (though likely to fail again)
                    return pool_alloc(self, size, alignment, options);
                }
                // Fall through to NULL if no handler
                
            case GOO_ALLOC_STRATEGY_NULL:
            default:
                return NULL;
        }
    }
    
    // Check if requested alignment is compatible
    if (alignment > pool->alignment && (pool->chunk_size % alignment) != 0) {
        // Incompatible alignment
        if (self->track_stats) {
            self->stats.failed_allocations++;
        }
        
        // Handle based on strategy (similar to above)
        if (self->strategy == GOO_ALLOC_STRATEGY_PANIC) {
            fprintf(stderr, "FATAL: Incompatible alignment for pool (requested %zu, pool %zu)\n",
                    alignment, pool->alignment);
            abort();
        }
        return NULL;
    }
    
    // Check if we have any free chunks
    if (!pool->free_list) {
        // Allocate a new block
        size_t new_block_size = pool->block_size;
        PoolBlock* new_block = create_pool_block(pool->parent, new_block_size, 
                                               pool->chunk_size, (FreeChunk**)&pool->free_list);
        if (!new_block) {
            // Failed to allocate new block
            if (self->track_stats) {
                self->stats.failed_allocations++;
            }
            
            // Handle allocation failure based on strategy
            if (self->strategy == GOO_ALLOC_STRATEGY_PANIC) {
                fprintf(stderr, "FATAL: Out of memory in pool allocator\n");
                abort();
            } else if (self->strategy == GOO_ALLOC_STRATEGY_RETRY && self->out_of_mem_fn) {
                self->out_of_mem_fn();
                // Try again
                return pool_alloc(self, size, alignment, options);
            }
            return NULL;
        }
        
        // Add new block to the block list
        new_block->next = (PoolBlock*)pool->blocks;
        pool->blocks = new_block;
        
        // Update statistics
        pool->free_chunks += new_block->capacity;
        pool->total_chunks += new_block->capacity;
        
        if (self->track_stats) {
            self->stats.bytes_reserved += new_block_size;
        }
    }
    
    // Get a chunk from the free list
    FreeChunk* chunk = (FreeChunk*)pool->free_list;
    pool->free_list = chunk->next;
    pool->free_chunks--;
    
    // Zero the memory if requested
    if (options & GOO_ALLOC_ZERO) {
        memset(chunk, 0, pool->chunk_size);
    }
    
    // Update allocation stats
    if (self->track_stats) {
        self->stats.bytes_allocated += pool->chunk_size;
        self->stats.allocation_count++;
        if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
            self->stats.max_bytes_allocated = self->stats.bytes_allocated;
        }
    }
    
    return chunk;
}

static void* pool_realloc(GooAllocator* self, void* ptr, size_t old_size, size_t new_size, 
                        size_t alignment, GooAllocOptions options) {
    GooPoolAllocator* pool = (GooPoolAllocator*)self;
    
    // If ptr is NULL, this is just an allocation
    if (!ptr) {
        return pool_alloc(self, new_size, alignment, options);
    }
    
    // If new_size is 0, this is just a free
    if (new_size == 0) {
        pool_free(self, ptr, old_size, alignment);
        return NULL;
    }
    
    // Check if the reallocation fits in the existing chunk
    if (new_size <= pool->chunk_size) {
        // It fits, no need to allocate a new chunk
        return ptr;
    }
    
    // Need to allocate a new, larger chunk
    void* new_ptr = pool_alloc(self, new_size, alignment, options & ~GOO_ALLOC_ZERO);
    if (!new_ptr) {
        return NULL;  // Allocation failed
    }
    
    // Copy data to new location
    size_t copy_size = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy_size);
    
    // Zero any additional memory if requested
    if (options & GOO_ALLOC_ZERO && new_size > old_size) {
        memset((char*)new_ptr + old_size, 0, new_size - old_size);
    }
    
    // Free the old chunk
    pool_free(self, ptr, old_size, alignment);
    
    return new_ptr;
}

static void pool_free(GooAllocator* self, void* ptr, size_t size, size_t alignment) {
    GooPoolAllocator* pool = (GooPoolAllocator*)self;
    
    if (!ptr) return;
    
    // Insert this chunk at the head of the free list
    FreeChunk* chunk = (FreeChunk*)ptr;
    chunk->next = (FreeChunk*)pool->free_list;
    pool->free_list = chunk;
    pool->free_chunks++;
    
    // Update statistics
    if (self->track_stats) {
        self->stats.bytes_allocated -= pool->chunk_size;
        self->stats.allocation_count--;
        self->stats.total_frees++;
    }
}

static void pool_destroy(GooAllocator* self) {
    GooPoolAllocator* pool = (GooPoolAllocator*)self;
    
    // Free all blocks
    PoolBlock* block = (PoolBlock*)pool->blocks;
    while (block) {
        PoolBlock* next = block->next;
        pool->parent->free(pool->parent, block, block->size, 16);
        block = next;
    }
    
    // Free the pool allocator itself
    pool->parent->free(pool->parent, pool, sizeof(GooPoolAllocator), 16);
}

// Create a memory pool allocator with default settings
GooPoolAllocator* goo_pool_allocator_create(GooAllocator* parent, size_t chunk_size, 
                                           size_t alignment, size_t initial_capacity) {
    if (!parent) return NULL;
    
    // Ensure minimum chunk size
    const size_t min_chunk_size = sizeof(FreeChunk);
    chunk_size = chunk_size < min_chunk_size ? min_chunk_size : chunk_size;
    
    // Adjust chunk size for alignment
    chunk_size = align_size(chunk_size, alignment);
    
    // Calculate optimal block size
    size_t block_size = calculate_block_size(chunk_size, initial_capacity);
    
    // Create the pool allocator
    GooPoolAllocator* pool = (GooPoolAllocator*)parent->alloc(parent, sizeof(GooPoolAllocator), 16, GOO_ALLOC_ZERO);
    if (!pool) return NULL;
    
    // Initialize allocator interface
    pool->allocator.alloc = pool_alloc;
    pool->allocator.realloc = pool_realloc;
    pool->allocator.free = pool_free;
    pool->allocator.destroy = pool_destroy;
    pool->allocator.strategy = GOO_ALLOC_STRATEGY_NULL;
    pool->allocator.out_of_mem_fn = NULL;
    pool->allocator.context = pool;
    pool->allocator.track_stats = true;
    memset(&pool->allocator.stats, 0, sizeof(GooAllocStats));
    
    // Initialize pool-specific data
    pool->parent = parent;
    pool->free_list = NULL;
    pool->blocks = NULL;
    pool->block_size = block_size;
    pool->chunk_size = chunk_size;
    pool->chunks_per_block = (block_size - sizeof(PoolBlock)) / chunk_size;
    pool->alignment = alignment;
    pool->free_chunks = 0;
    pool->total_chunks = 0;
    
    // Allocate initial block if requested
    if (initial_capacity > 0) {
        PoolBlock* initial_block = create_pool_block(parent, block_size, chunk_size, (FreeChunk**)&pool->free_list);
        if (!initial_block) {
            parent->free(parent, pool, sizeof(GooPoolAllocator), 16);
            return NULL;
        }
        
        pool->blocks = initial_block;
        pool->free_chunks = initial_block->capacity;
        pool->total_chunks = initial_block->capacity;
        
        if (pool->allocator.track_stats) {
            pool->allocator.stats.bytes_reserved += block_size;
        }
    }
    
    return pool;
}

// Create a sized memory pool allocator with specific block size
GooPoolAllocator* goo_pool_allocator_create_sized(GooAllocator* parent, size_t chunk_size, 
                                                 size_t alignment, size_t chunks_per_block) {
    if (!parent || chunks_per_block == 0) return NULL;
    
    // Ensure minimum chunk size
    const size_t min_chunk_size = sizeof(FreeChunk);
    chunk_size = chunk_size < min_chunk_size ? min_chunk_size : chunk_size;
    
    // Adjust chunk size for alignment
    chunk_size = align_size(chunk_size, alignment);
    
    // Calculate block size based on chunk size and count
    size_t block_size = sizeof(PoolBlock) + (chunk_size * chunks_per_block);
    
    // Create the pool allocator
    GooPoolAllocator* pool = (GooPoolAllocator*)parent->alloc(parent, sizeof(GooPoolAllocator), 16, GOO_ALLOC_ZERO);
    if (!pool) return NULL;
    
    // Initialize allocator interface
    pool->allocator.alloc = pool_alloc;
    pool->allocator.realloc = pool_realloc;
    pool->allocator.free = pool_free;
    pool->allocator.destroy = pool_destroy;
    pool->allocator.strategy = GOO_ALLOC_STRATEGY_NULL;
    pool->allocator.out_of_mem_fn = NULL;
    pool->allocator.context = pool;
    pool->allocator.track_stats = true;
    memset(&pool->allocator.stats, 0, sizeof(GooAllocStats));
    
    // Initialize pool-specific data
    pool->parent = parent;
    pool->free_list = NULL;
    pool->blocks = NULL;
    pool->block_size = block_size;
    pool->chunk_size = chunk_size;
    pool->chunks_per_block = chunks_per_block;
    pool->alignment = alignment;
    pool->free_chunks = 0;
    pool->total_chunks = 0;
    
    // Allocate initial block
    PoolBlock* initial_block = create_pool_block(parent, block_size, chunk_size, (FreeChunk**)&pool->free_list);
    if (!initial_block) {
        parent->free(parent, pool, sizeof(GooPoolAllocator), 16);
        return NULL;
    }
    
    pool->blocks = initial_block;
    pool->free_chunks = initial_block->capacity;
    pool->total_chunks = initial_block->capacity;
    
    if (pool->allocator.track_stats) {
        pool->allocator.stats.bytes_reserved += block_size;
    }
    
    return pool;
}

// Reset a memory pool (return all allocations to the pool)
void goo_pool_reset(GooPoolAllocator* pool) {
    if (!pool) return;
    
    GooAllocator* self = &pool->allocator;
    
    // Empty the free list
    pool->free_list = NULL;
    pool->free_chunks = 0;
    
    // Rebuild the free list from all blocks
    PoolBlock* block = (PoolBlock*)pool->blocks;
    while (block) {
        // Add all chunks from this block to the free list
        char* data = block->data;
        for (size_t i = 0; i < block->capacity; i++) {
            FreeChunk* chunk = (FreeChunk*)(data + (i * pool->chunk_size));
            chunk->next = (FreeChunk*)pool->free_list;
            pool->free_list = chunk;
            pool->free_chunks++;
        }
        
        block = block->next;
    }
    
    // Reset statistics
    if (self->track_stats) {
        self->stats.bytes_allocated = 0;
        self->stats.allocation_count = 0;
        self->stats.total_frees += self->stats.allocation_count;
    }
}

// Get pool statistics
void goo_pool_get_stats(GooPoolAllocator* pool, size_t* free_chunks, size_t* total_chunks) {
    if (!pool) return;
    
    if (free_chunks) {
        *free_chunks = pool->free_chunks;
    }
    
    if (total_chunks) {
        *total_chunks = pool->total_chunks;
    }
} 