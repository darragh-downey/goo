/**
 * @file goo_zig_allocator.h
 * @brief C bindings for Zig-based memory allocators in the Goo language
 */

#ifndef GOO_ZIG_ALLOCATOR_H
#define GOO_ZIG_ALLOCATOR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque types for Zig allocators
 */
typedef struct GooZigAllocator GooZigAllocator;
typedef struct GooZigPoolAllocator GooZigPoolAllocator;
typedef struct GooZigArenaAllocator GooZigArenaAllocator;
typedef struct GooZigRegionAllocator GooZigRegionAllocator;

/**
 * Core allocator functions
 */
GooZigAllocator* goo_zig_allocator_create(void);
void goo_zig_allocator_destroy(GooZigAllocator* allocator);
void* goo_zig_allocator_alloc(GooZigAllocator* allocator, size_t size);
void goo_zig_allocator_free(GooZigAllocator* allocator, void* ptr);
void* goo_zig_allocator_realloc(GooZigAllocator* allocator, void* ptr, size_t old_size, size_t new_size);

/**
 * Pool allocator functions
 */
GooZigPoolAllocator* goo_zig_pool_allocator_create(size_t block_size, size_t blocks_per_chunk);
void goo_zig_pool_allocator_destroy(GooZigPoolAllocator* pool_alloc);
void* goo_zig_pool_allocator_alloc(GooZigPoolAllocator* pool_alloc);
void goo_zig_pool_allocator_free(GooZigPoolAllocator* pool_alloc, void* ptr);
void goo_zig_pool_allocator_reset(GooZigPoolAllocator* pool_alloc);

/**
 * Arena allocator functions
 */
GooZigArenaAllocator* goo_zig_arena_allocator_create(size_t initial_size);
void goo_zig_arena_allocator_destroy(GooZigArenaAllocator* arena_alloc);
void* goo_zig_arena_allocator_alloc(GooZigArenaAllocator* arena_alloc, size_t size, size_t alignment);
void goo_zig_arena_allocator_reset(GooZigArenaAllocator* arena_alloc);

/**
 * Region allocator functions
 */
GooZigRegionAllocator* goo_zig_region_allocator_create(size_t region_size, size_t max_regions);
void goo_zig_region_allocator_destroy(GooZigRegionAllocator* region_alloc);
void* goo_zig_region_allocator_alloc(GooZigRegionAllocator* region_alloc, size_t size, size_t alignment);
bool goo_zig_region_allocator_free_region(GooZigRegionAllocator* region_alloc, size_t region_id);
void goo_zig_region_allocator_reset(GooZigRegionAllocator* region_alloc);

#ifdef __cplusplus
}
#endif

#endif /* GOO_ZIG_ALLOCATOR_H */ 