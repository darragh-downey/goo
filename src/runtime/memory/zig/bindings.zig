const std = @import("std");
const core = @import("../goo_allocator_core.zig");
const pool = @import("../goo_pool_allocator.zig");
const arena = @import("../goo_arena_allocator.zig");
const region = @import("../goo_region_allocator.zig");

const c = @cImport({
    @cInclude("stdio.h");
    @cInclude("stdlib.h");
    @cInclude("stdint.h");
    @cInclude("stdbool.h");
    @cInclude("string.h");
});

// Core allocator C bindings
export fn goo_zig_allocator_create() ?*core.GooAllocator {
    return core.GooAllocator.create(std.heap.page_allocator) catch null;
}

export fn goo_zig_allocator_destroy(allocator: ?*core.GooAllocator) void {
    if (allocator) |alloc| {
        alloc.destroy();
    }
}

export fn goo_zig_allocator_alloc(allocator: ?*core.GooAllocator, size: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.alloc(size) catch null;
}

export fn goo_zig_allocator_free(allocator: ?*core.GooAllocator, ptr: ?*anyopaque) void {
    if (allocator == null or ptr == null) return;
    allocator.?.free(ptr.?);
}

export fn goo_zig_allocator_realloc(allocator: ?*core.GooAllocator, ptr: ?*anyopaque, old_size: usize, new_size: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.realloc(ptr, old_size, new_size) catch null;
}

// Pool allocator C bindings
export fn goo_zig_pool_allocator_create(block_size: usize, blocks_per_chunk: usize) ?*pool.GooPoolAllocator {
    return pool.GooPoolAllocator.create(std.heap.page_allocator, block_size, blocks_per_chunk) catch null;
}

export fn goo_zig_pool_allocator_destroy(pool_alloc: ?*pool.GooPoolAllocator) void {
    if (pool_alloc) |alloc| {
        alloc.destroy();
    }
}

export fn goo_zig_pool_allocator_alloc(pool_alloc: ?*pool.GooPoolAllocator) ?*anyopaque {
    if (pool_alloc == null) return null;
    return pool_alloc.?.alloc() catch null;
}

export fn goo_zig_pool_allocator_free(pool_alloc: ?*pool.GooPoolAllocator, ptr: ?*anyopaque) void {
    if (pool_alloc == null or ptr == null) return;
    pool_alloc.?.free(ptr.?);
}

export fn goo_zig_pool_allocator_reset(pool_alloc: ?*pool.GooPoolAllocator) void {
    if (pool_alloc) |alloc| {
        alloc.reset();
    }
}

// Arena allocator C bindings
export fn goo_zig_arena_allocator_create(initial_size: usize) ?*arena.GooArenaAllocator {
    return arena.GooArenaAllocator.create(std.heap.page_allocator, initial_size) catch null;
}

export fn goo_zig_arena_allocator_destroy(arena_alloc: ?*arena.GooArenaAllocator) void {
    if (arena_alloc) |alloc| {
        alloc.destroy();
    }
}

export fn goo_zig_arena_allocator_alloc(arena_alloc: ?*arena.GooArenaAllocator, size: usize, alignment: usize) ?*anyopaque {
    if (arena_alloc == null) return null;
    return arena_alloc.?.alloc(size, alignment) catch null;
}

export fn goo_zig_arena_allocator_reset(arena_alloc: ?*arena.GooArenaAllocator) void {
    if (arena_alloc) |alloc| {
        alloc.reset();
    }
}

// Region allocator C bindings
export fn goo_zig_region_allocator_create(region_size: usize, max_regions: usize) ?*region.GooRegionAllocator {
    return region.GooRegionAllocator.create(std.heap.page_allocator, region_size, max_regions) catch null;
}

export fn goo_zig_region_allocator_destroy(region_alloc: ?*region.GooRegionAllocator) void {
    if (region_alloc) |alloc| {
        alloc.destroy();
    }
}

export fn goo_zig_region_allocator_alloc(region_alloc: ?*region.GooRegionAllocator, size: usize, alignment: usize) ?*anyopaque {
    if (region_alloc == null) return null;
    return region_alloc.?.alloc(size, alignment) catch null;
}

export fn goo_zig_region_allocator_free_region(region_alloc: ?*region.GooRegionAllocator, region_id: usize) bool {
    if (region_alloc == null) return false;
    return region_alloc.?.freeRegion(region_id);
}

export fn goo_zig_region_allocator_reset(region_alloc: ?*region.GooRegionAllocator) void {
    if (region_alloc) |alloc| {
        alloc.reset();
    }
}
