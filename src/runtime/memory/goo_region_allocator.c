/**
 * goo_region_allocator.c
 * 
 * Implementation of region-based memory allocator for the Goo programming language.
 * Region allocators allow for efficient scoped memory management.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "../memory/goo_allocator.h"

// Region allocation block
typedef struct RegionBlock {
    struct RegionBlock* next;     // Next block in region
    size_t size;                  // Size of this block
    size_t used;                  // Bytes used in this block
    unsigned int region_depth;    // Region depth this block belongs to
    char data[];                  // Flexible array member for actual data
} RegionBlock;

// Region tracking
typedef struct RegionInfo {
    struct RegionInfo* parent;    // Parent region
    RegionBlock* blocks;          // Blocks allocated in this region
    unsigned int depth;           // Nesting depth
} RegionInfo;

// Default block size for region allocations
#define REGION_DEFAULT_BLOCK_SIZE (64 * 1024)  // 64K

// Minimum block size
#define REGION_MIN_BLOCK_SIZE 1024  // 1K

// Helper to calculate required block size
static size_t region_calc_block_size(size_t requested_size) {
    // Ensure we have at least REGION_MIN_BLOCK_SIZE bytes
    size_t block_size = REGION_DEFAULT_BLOCK_SIZE;
    
    // If requested size is larger than default, allocate a custom-sized block
    if (requested_size > REGION_DEFAULT_BLOCK_SIZE - sizeof(RegionBlock)) {
        block_size = requested_size + sizeof(RegionBlock);
        // Round up to multiple of page size (assume 4K)
        block_size = (block_size + 4095) & ~4095;
    }
    
    return block_size;
}

// Create a new region block
static RegionBlock* region_block_create(GooAllocator* parent, size_t size, unsigned int depth) {
    size_t block_size = region_calc_block_size(size);
    RegionBlock* block = parent->alloc(parent, block_size, 16, GOO_ALLOC_ZERO);
    if (!block) return NULL;
    
    block->next = NULL;
    block->size = block_size - sizeof(RegionBlock);
    block->used = 0;
    block->region_depth = depth;
    
    return block;
}

// Allocate memory from a region block
static void* region_block_alloc(RegionBlock* block, size_t size, size_t alignment) {
    // Calculate alignment padding needed
    size_t offset = block->used;
    size_t padding = ((offset + alignment - 1) & ~(alignment - 1)) - offset;
    size_t total_size = size + padding;
    
    // Check if there's enough space
    if (block->used + total_size > block->size) {
        return NULL;
    }
    
    // Allocate from the block
    void* ptr = &block->data[offset + padding];
    block->used += total_size;
    
    return ptr;
}

// Region allocator functions
static void* region_alloc(GooAllocator* self, size_t size, size_t alignment, GooAllocOptions options) {
    GooRegionAllocator* region = (GooRegionAllocator*)self;
    if (!region || !region->parent) return NULL;
    
    // Update statistics if enabled
    if (self->track_stats) {
        self->stats.total_allocations++;
    }
    
    // Find the current region info
    RegionInfo* current_region = (RegionInfo*)region->regions;
    if (!current_region) {
        // No active region, use parent allocator directly
        void* ptr = region->parent->alloc(region->parent, size, alignment, options);
        
        if (self->track_stats && ptr) {
            self->stats.bytes_allocated += size;
            self->stats.allocation_count++;
            if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
                self->stats.max_bytes_allocated = self->stats.bytes_allocated;
            }
        }
        
        return ptr;
    }
    
    // Try to allocate from the current region's first block
    RegionBlock* block = current_region->blocks;
    if (block) {
        void* ptr = region_block_alloc(block, size, alignment);
        if (ptr) {
            if (self->track_stats) {
                self->stats.bytes_allocated += size;
                self->stats.allocation_count++;
                if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
                    self->stats.max_bytes_allocated = self->stats.bytes_allocated;
                }
            }
            
            // Zero memory if requested
            if (options & GOO_ALLOC_ZERO) {
                memset(ptr, 0, size);
            }
            
            return ptr;
        }
    }
    
    // Couldn't allocate from existing block, create a new one
    size_t block_size = region_calc_block_size(size);
    RegionBlock* new_block = region_block_create(region->parent, size, current_region->depth);
    if (!new_block) {
        if (self->track_stats) {
            self->stats.failed_allocations++;
        }
        
        // Handle allocation failure based on strategy
        switch (self->strategy) {
            case GOO_ALLOC_STRATEGY_PANIC:
                fprintf(stderr, "FATAL: Out of memory in region allocator\n");
                abort();
                break;
                
            case GOO_ALLOC_STRATEGY_RETRY:
                if (self->out_of_mem_fn) {
                    self->out_of_mem_fn();
                    // Try again
                    return region_alloc(self, size, alignment, options);
                }
                // Fall through to NULL if no handler
                
            case GOO_ALLOC_STRATEGY_NULL:
            default:
                return NULL;
        }
    }
    
    // Add new block to the region
    new_block->next = current_region->blocks;
    current_region->blocks = new_block;
    
    // Allocate from the new block
    void* ptr = region_block_alloc(new_block, size, alignment);
    if (!ptr) {
        // This should never happen since we just created a block big enough
        if (self->track_stats) {
            self->stats.failed_allocations++;
        }
        return NULL;
    }
    
    if (self->track_stats) {
        self->stats.bytes_allocated += size;
        self->stats.bytes_reserved += block_size;
        self->stats.allocation_count++;
        if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
            self->stats.max_bytes_allocated = self->stats.bytes_allocated;
        }
    }
    
    // Zero memory if requested
    if (options & GOO_ALLOC_ZERO) {
        memset(ptr, 0, size);
    }
    
    return ptr;
}

// Reallocation in a region allocator (note: only works for the most recent allocation)
static void* region_realloc(GooAllocator* self, void* ptr, size_t old_size, size_t new_size, 
                            size_t alignment, GooAllocOptions options) {
    GooRegionAllocator* region = (GooRegionAllocator*)self;
    if (!region || !region->parent) return NULL;
    
    // If ptr is NULL, this is just an allocation
    if (!ptr) {
        return region_alloc(self, new_size, alignment, options);
    }
    
    // If new_size is 0, this is just a free (which is a no-op in regions)
    if (new_size == 0) {
        return NULL;
    }
    
    // Find the current region info
    RegionInfo* current_region = (RegionInfo*)region->regions;
    if (!current_region) {
        // No active region, use parent allocator directly
        return region->parent->realloc(region->parent, ptr, old_size, new_size, alignment, options);
    }
    
    // Check if this is the most recent allocation in the current block
    RegionBlock* block = current_region->blocks;
    if (block) {
        char* data_end = &block->data[block->used];
        char* ptr_end = (char*)ptr + old_size;
        
        // If this pointer is at the end of the block's used memory, we can just adjust the block size
        if (ptr_end == data_end) {
            // Check if we can expand in place
            if (new_size <= old_size + (block->size - block->used)) {
                // Can expand in place
                block->used += (new_size - old_size);
                
                if (self->track_stats) {
                    self->stats.bytes_allocated += (new_size - old_size);
                    if (self->stats.bytes_allocated > self->stats.max_bytes_allocated) {
                        self->stats.max_bytes_allocated = self->stats.bytes_allocated;
                    }
                }
                
                // Zero any new memory if requested
                if (options & GOO_ALLOC_ZERO && new_size > old_size) {
                    memset((char*)ptr + old_size, 0, new_size - old_size);
                }
                
                return ptr;
            }
        }
    }
    
    // Can't realloc in place, allocate new memory and copy
    void* new_ptr = region_alloc(self, new_size, alignment, options & ~GOO_ALLOC_ZERO);
    if (!new_ptr) return NULL;
    
    // Copy old data
    memcpy(new_ptr, ptr, old_size < new_size ? old_size : new_size);
    
    // Zero any new memory if requested
    if (options & GOO_ALLOC_ZERO && new_size > old_size) {
        memset((char*)new_ptr + old_size, 0, new_size - old_size);
    }
    
    // Note: We don't free the old memory in a region allocator
    
    return new_ptr;
}

// Free in a region allocator (no-op, memory is freed when the region is ended)
static void region_free(GooAllocator* self, void* ptr, size_t size, size_t alignment) {
    // No individual free in region allocator, memory is freed when region is ended
    GooRegionAllocator* region = (GooRegionAllocator*)self;
    if (!region) return;
    
    if (self->track_stats) {
        self->stats.total_frees++;
        self->stats.bytes_allocated -= size;
        self->stats.allocation_count--;
    }
}

// Destroy the region allocator
static void region_destroy(GooAllocator* self) {
    GooRegionAllocator* region = (GooRegionAllocator*)self;
    if (!region || !region->parent) return;
    
    // Free all regions and their blocks
    RegionInfo* info = (RegionInfo*)region->regions;
    while (info) {
        RegionInfo* parent = info->parent;
        
        // Free all blocks in this region
        RegionBlock* block = info->blocks;
        while (block) {
            RegionBlock* next = block->next;
            region->parent->free(region->parent, block, block->size + sizeof(RegionBlock), 16);
            block = next;
        }
        
        // Free the region info
        region->parent->free(region->parent, info, sizeof(RegionInfo), 16);
        
        info = parent;
    }
    
    // Free the region allocator itself
    region->parent->free(region->parent, region, sizeof(GooRegionAllocator), 16);
}

// Create a region allocator
GooRegionAllocator* goo_region_allocator_create(GooAllocator* parent) {
    if (!parent) return NULL;
    
    GooRegionAllocator* region = parent->alloc(parent, sizeof(GooRegionAllocator), 16, GOO_ALLOC_ZERO);
    if (!region) return NULL;
    
    // Initialize allocator interface
    region->allocator.alloc = region_alloc;
    region->allocator.realloc = region_realloc;
    region->allocator.free = region_free;
    region->allocator.destroy = region_destroy;
    region->allocator.strategy = GOO_ALLOC_STRATEGY_NULL;
    region->allocator.out_of_mem_fn = NULL;
    region->allocator.context = region;
    region->allocator.track_stats = true;
    memset(&region->allocator.stats, 0, sizeof(GooAllocStats));
    
    // Initialize region-specific data
    region->parent = parent;
    region->regions = NULL;
    region->current_depth = 0;
    
    return region;
}

// Begin a new memory region
void goo_region_begin(GooRegionAllocator* region) {
    if (!region || !region->parent) return;
    
    // Create a new region info
    RegionInfo* info = region->parent->alloc(region->parent, sizeof(RegionInfo), 16, GOO_ALLOC_ZERO);
    if (!info) return;
    
    // Initialize the region info
    info->parent = (RegionInfo*)region->regions;
    info->blocks = NULL;
    info->depth = ++region->current_depth;
    
    // Set as current region
    region->regions = info;
}

// End the current memory region
void goo_region_end(GooRegionAllocator* region) {
    if (!region || !region->parent) return;
    
    RegionInfo* info = (RegionInfo*)region->regions;
    if (!info) return;
    
    // Update region depth
    region->current_depth--;
    
    // Update the current region to the parent
    region->regions = info->parent;
    
    // Free all blocks in this region
    RegionBlock* block = info->blocks;
    while (block) {
        RegionBlock* next = block->next;
        
        // Update stats
        if (region->allocator.track_stats) {
            region->allocator.stats.bytes_reserved -= (block->size + sizeof(RegionBlock));
        }
        
        region->parent->free(region->parent, block, block->size + sizeof(RegionBlock), 16);
        block = next;
    }
    
    // Free the region info
    region->parent->free(region->parent, info, sizeof(RegionInfo), 16);
} 