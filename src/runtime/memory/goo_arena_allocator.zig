const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("string.h");
});

const allocator_core = @import("goo_allocator_core.zig");
const Allocator = allocator_core.Allocator;
const AllocOptions = allocator_core.AllocOptions;
const DEFAULT_ALIGNMENT = allocator_core.DEFAULT_ALIGNMENT;

// Constants
const DEFAULT_ARENA_SIZE = 64 * 1024; // 64 KB
const DEFAULT_BLOCK_SIZE = 8 * 1024; // 8 KB
const MIN_BLOCK_SIZE = 1024; // Minimum block size of 1 KB

// Arena memory block
const ArenaBlock = struct {
    next: ?*ArenaBlock,
    size: usize,
    capacity: usize,
    data: [*]u8,

    fn create(size: usize) ?*ArenaBlock {
        // Allocate block with header plus data area
        const block_size = @sizeOf(ArenaBlock);
        const total_size = block_size + size;

        const ptr = c.malloc(total_size);
        if (ptr == null) return null;

        // Initialize the block
        const block = @as(*ArenaBlock, @ptrCast(@alignCast(ptr)));
        block.next = null;
        block.size = 0;
        block.capacity = size;
        block.data = @as([*]u8, @ptrCast(@alignCast(ptr))) + block_size;

        return block;
    }

    fn destroy(block: *ArenaBlock) void {
        c.free(block);
    }

    // Allocate memory from this block
    fn alloc(self: *ArenaBlock, size: usize, alignment: usize) ?*anyopaque {
        // Ensure alignment of current position
        const position = @intFromPtr(self.data) + self.size;
        const aligned_position = (position + alignment - 1) & ~(alignment - 1);
        const align_padding = aligned_position - position;

        // Check if we have enough space
        const new_size = self.size + align_padding + size;
        if (new_size > self.capacity) {
            return null;
        }

        // Update block and return aligned pointer
        const result = @as(*anyopaque, @ptrFromInt(aligned_position));
        self.size = new_size;
        return result;
    }
};

// Arena allocator implementation
pub const ArenaAllocator = struct {
    // Base allocator interface
    allocator: Allocator,

    // Arena-specific state
    head: ?*ArenaBlock,
    current: ?*ArenaBlock,
    total_capacity: usize,
    min_block_size: usize,

    // Initialize an arena allocator
    pub fn init(initial_size: usize) ?*ArenaAllocator {
        // Allocate the arena object
        const self = @as(*ArenaAllocator, @ptrCast(@alignCast(c.malloc(@sizeOf(ArenaAllocator)) orelse return null)));

        // Initialize fields
        self.* = .{
            .allocator = .{
                .vtable = &arenaVTable,
                .strategy = .null,
                .out_of_mem_fn = null,
                .context = self,
                .track_stats = true,
                .stats = .{},
            },
            .head = null,
            .current = null,
            .total_capacity = 0,
            .min_block_size = if (initial_size > MIN_BLOCK_SIZE) initial_size else DEFAULT_BLOCK_SIZE,
        };

        // Allocate initial block
        if (!self.addBlock(initial_size)) {
            c.free(self);
            return null;
        }

        return self;
    }

    // Add a new block to the arena
    fn addBlock(self: *ArenaAllocator, size: usize) bool {
        // Use at least min_block_size
        const block_size = if (size > self.min_block_size) size else self.min_block_size;

        // Create the block
        const block = ArenaBlock.create(block_size) orelse return false;

        // Link it into the list
        if (self.head == null) {
            self.head = block;
        } else if (self.current) |current| {
            current.next = block;
        }

        self.current = block;
        self.total_capacity += block_size;

        // Update stats
        if (self.allocator.track_stats) {
            self.allocator.stats.bytes_reserved += block_size;
        }

        return true;
    }

    // Allocate memory from the arena
    fn alloc(allocator: *Allocator, size: usize, alignment: usize, ret_addr: c_int) ?*anyopaque {
        if (size == 0) return null;

        const self = @as(*ArenaAllocator, @ptrCast(@alignCast(allocator.context.?)));
        const actual_alignment = if (alignment == 0) DEFAULT_ALIGNMENT else alignment;

        // Try to allocate from current block
        if (self.current) |current| {
            if (current.alloc(size, actual_alignment)) |ptr| {
                // Update stats
                if (allocator.track_stats) {
                    allocator.stats.bytes_allocated += size;
                    allocator.stats.allocation_count += 1;
                    allocator.stats.total_allocations += 1;

                    // Update max bytes
                    if (allocator.stats.bytes_allocated > allocator.stats.max_bytes_allocated) {
                        allocator.stats.max_bytes_allocated = allocator.stats.bytes_allocated;
                    }
                }

                // Zero memory if requested
                const alloc_options = AllocOptions.fromC(ret_addr);
                if (alloc_options.zero) {
                    @memset(@as([*]u8, @ptrCast(ptr))[0..size], 0);
                }

                return ptr;
            }
        }

        // Current block full, allocate a new one
        if (!self.addBlock(size)) {
            // Failed to allocate a new block
            if (allocator.track_stats) {
                allocator.stats.failed_allocations += 1;
            }
            return null;
        }

        // Try again with the new block
        return ArenaAllocator.alloc(allocator, size, alignment, ret_addr);
    }

    // Realloc is not efficient in an arena, but we provide it for compatibility
    fn realloc(allocator: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, options: c_int) ?*anyopaque {
        // Special cases
        if (ptr == null) {
            return ArenaAllocator.alloc(allocator, new_size, alignment, options);
        }

        if (new_size == 0) {
            // In an arena, we don't actually free individual allocations
            // But we update stats
            if (allocator.track_stats) {
                allocator.stats.bytes_allocated -|= old_size;
                allocator.stats.allocation_count -|= 1;
                allocator.stats.total_frees += 1;
            }
            return null;
        }

        // For arenas, growing in place is usually not possible, so we always allocate new memory
        const new_ptr = ArenaAllocator.alloc(allocator, new_size, alignment, options);
        if (new_ptr == null) {
            return null;
        }

        // Copy old data (up to min of old and new size)
        const copy_size = @min(old_size, new_size);
        @memcpy(@as([*]u8, @ptrCast(new_ptr))[0..copy_size], @as([*]u8, @ptrCast(ptr))[0..copy_size]);

        // Update stats to reflect the deallocation of the old memory
        if (allocator.track_stats) {
            allocator.stats.bytes_allocated -|= old_size;
            allocator.stats.allocation_count -|= 1;
            allocator.stats.total_frees += 1;
        }

        return new_ptr;
    }

    // Individual frees are no-ops in an arena, but we track stats
    fn free(allocator: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
        _ = alignment;
        if (ptr == null) return;

        // No actual freeing, but update stats
        if (allocator.track_stats) {
            allocator.stats.bytes_allocated -|= size;
            allocator.stats.allocation_count -|= 1;
            allocator.stats.total_frees += 1;
        }
    }

    // Reset the arena, keeping blocks for reuse
    pub fn reset(self: *ArenaAllocator) void {
        var block = self.head;
        while (block) |b| {
            b.size = 0;
            block = b.next;
        }

        // Reset stats
        if (self.allocator.track_stats) {
            self.allocator.stats.bytes_allocated = 0;
            self.allocator.stats.allocation_count = 0;
        }
    }

    // Destroy the arena and all its blocks
    fn destroy(allocator: *Allocator) void {
        const self = @as(*ArenaAllocator, @ptrCast(@alignCast(allocator.context.?)));

        // Free all blocks
        var block = self.head;
        while (block != null) {
            const next = block.?.next;
            ArenaBlock.destroy(block.?);
            block = next;
        }

        // Free the arena itself
        c.free(self);
    }
};

// Allocator vtable for arena allocator
const arenaVTable = allocator_core.AllocatorVTable{
    .allocFn = ArenaAllocator.alloc,
    .reallocFn = ArenaAllocator.realloc,
    .freeFn = ArenaAllocator.free,
    .destroyFn = ArenaAllocator.destroy,
};

// Exported C API functions
export fn goo_arena_allocator_create(initial_size: usize) ?*Allocator {
    // Use default size if zero
    const size = if (initial_size == 0) DEFAULT_ARENA_SIZE else initial_size;

    // Create the arena
    if (ArenaAllocator.init(size)) |arena| {
        return &arena.allocator;
    }

    return null;
}

export fn goo_arena_allocator_reset(allocator: ?*Allocator) void {
    if (allocator == null) return;

    const self = @as(*ArenaAllocator, @ptrCast(@alignCast(allocator.?.context.?)));
    self.reset();
}

export fn goo_arena_allocator_destroy(allocator: ?*Allocator) void {
    if (allocator == null) return;

    allocator.?.destroy();
}
