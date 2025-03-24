// Main allocator implementation file that includes all specialized allocators
const std = @import("std");
const core = @import("goo_allocator_core.zig");

// Import specialized allocator implementations
const arena = @import("goo_arena_allocator.zig");
const pool = @import("goo_pool_allocator.zig");
const region = @import("goo_region_allocator.zig");

// Import the transport module to ensure it's included in the build
const transport = @import("goo_transport.zig");

// Re-export core functionality
pub const Allocator = core.Allocator;
pub const AllocOptions = core.AllocOptions;
pub const AllocStrategy = core.AllocStrategy;
pub const AllocStats = core.AllocStats;
pub const DEFAULT_ALIGNMENT = core.DEFAULT_ALIGNMENT;

// Re-export core functions
pub const goo_get_default_allocator = core.goo_get_default_allocator;
pub const goo_memory_init = core.goo_memory_init;
pub const goo_memory_shutdown = core.goo_memory_shutdown;
pub const goo_allocator_alloc = core.goo_allocator_alloc;
pub const goo_allocator_realloc = core.goo_allocator_realloc;
pub const goo_allocator_free = core.goo_allocator_free;
pub const goo_allocator_aligned_alloc = core.goo_allocator_aligned_alloc;
pub const goo_allocator_calloc = core.goo_allocator_calloc;
pub const goo_allocator_strdup = core.goo_allocator_strdup;
pub const goo_allocator_get_stats = core.goo_allocator_get_stats;
pub const goo_allocator_reset_stats = core.goo_allocator_reset_stats;

// These comptime declarations force the imports to be included in the build
comptime {
    _ = arena;
    _ = pool;
    _ = region;
    _ = transport;
}
