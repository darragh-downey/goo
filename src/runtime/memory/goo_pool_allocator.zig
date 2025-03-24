const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("string.h");
});

const allocator_core = @import("goo_allocator_core.zig");
const Allocator = allocator_core.Allocator;
const AllocOptions = allocator_core.AllocOptions;
const AllocStats = allocator_core.AllocStats;
const DEFAULT_ALIGNMENT = allocator_core.DEFAULT_ALIGNMENT;

// Constants for the pool allocator
const MIN_CHUNK_COUNT = 16; // Minimum number of chunks per block
const DEFAULT_CHUNK_COUNT = 64; // Default number of chunks
const MAX_POOL_CHUNK_SIZE = 1024; // Maximum chunk size for pools

// A block in the pool, containing multiple chunks
const PoolBlock = struct {
    next: ?*PoolBlock, // Linked list of blocks
    chunk_size: usize, // Size of each chunk
    chunk_count: usize, // Number of chunks in this block
    free_chunks: usize, // Number of free chunks
    data: [*]u8, // Raw storage for chunks
    free_list: ?*FreeChunk, // Linked list of free chunks

    // Create a new pool block
    fn create(chunk_size: usize, chunk_count: usize) ?*PoolBlock {
        // Calculate the size of each chunk including header
        const chunk_with_header_size = chunk_size + @sizeOf(ChunkHeader);
        const aligned_chunk_size = std.mem.alignForward(usize, chunk_with_header_size, @alignOf(ChunkHeader));

        // Calculate total size needed
        const block_size = @sizeOf(PoolBlock);
        const data_size = aligned_chunk_size * chunk_count;
        const total_size = block_size + data_size;

        // Allocate memory for block and chunks
        const ptr = c.malloc(total_size);
        if (ptr == null) return null;

        // Initialize the block
        const block = @as(*PoolBlock, @ptrCast(@alignCast(ptr)));
        block.* = .{
            .next = null,
            .chunk_size = chunk_size,
            .chunk_count = chunk_count,
            .free_chunks = chunk_count,
            .data = @as([*]u8, @ptrCast(@alignCast(ptr))) + block_size,
            .free_list = null,
        };

        // Initialize free list
        block.initFreeList(aligned_chunk_size);

        return block;
    }

    // Initialize the free list for this block
    fn initFreeList(self: *PoolBlock, aligned_chunk_size: usize) void {
        // Build the free list in reverse
        var prev: ?*FreeChunk = null;

        // For each chunk in the block
        var i: usize = self.chunk_count;
        while (i > 0) {
            i -= 1;

            // Get chunk pointer and initialize it
            const offset = i * aligned_chunk_size;
            const chunk_ptr = &self.data[offset];
            const chunk = @as(*FreeChunk, @ptrCast(@alignCast(chunk_ptr)));

            // Link to previous chunk
            chunk.next = prev;
            prev = chunk;
        }

        // Set the head of the free list
        self.free_list = prev;
    }

    // Destroy a block
    fn destroy(block: *PoolBlock) void {
        c.free(block);
    }

    // Allocate a chunk from this block
    fn allocChunk(self: *PoolBlock) ?*anyopaque {
        // Check if we have free chunks
        if (self.free_chunks == 0 or self.free_list == null) {
            return null;
        }

        // Take the first free chunk
        const chunk = self.free_list.?;
        self.free_list = chunk.next;
        self.free_chunks -= 1;

        // Initialize chunk header
        const header = @as(*ChunkHeader, @ptrCast(chunk));
        header.block = self;

        // Return data portion of the chunk
        return @as(*anyopaque, @ptrFromInt(@intFromPtr(chunk) + @sizeOf(ChunkHeader)));
    }

    // Free a chunk back to this block
    fn freeChunk(self: *PoolBlock, ptr: *anyopaque) void {
        // Get the chunk header
        const header_ptr = @intFromPtr(ptr) - @sizeOf(ChunkHeader);
        const header = @as(*ChunkHeader, @ptrFromInt(header_ptr));

        // Ensure this chunk belongs to this block
        if (header.block != self) {
            return; // Error: chunk doesn't belong to this block
        }

        // Cast to free chunk and add to free list
        const chunk = @as(*FreeChunk, @ptrCast(@alignCast(header)));
        chunk.next = self.free_list;
        self.free_list = chunk;
        self.free_chunks += 1;
    }
};

// Header for allocated chunks
const ChunkHeader = struct {
    block: *PoolBlock, // Pointer back to the owning block
};

// Structure for free chunks in the free list
const FreeChunk = struct {
    next: ?*FreeChunk, // Next free chunk
};

// Pool allocator implementation
pub const PoolAllocator = struct {
    // Base allocator interface
    allocator: Allocator,

    // Pool-specific state
    blocks: ?*PoolBlock,
    chunk_size: usize,
    alignment: usize,
    chunk_count_per_block: usize,
    free_list: ?*FreeChunk,

    // Initialize a pool allocator
    pub fn init(chunk_size: usize, alignment: usize, chunks_per_block: usize) ?*PoolAllocator {
        // Validate parameters
        if (chunk_size == 0 or chunk_size > MAX_POOL_CHUNK_SIZE) {
            return null;
        }

        // Ensure alignment is valid
        const actual_alignment = if (alignment == 0) DEFAULT_ALIGNMENT else alignment;
        if (!std.math.isPowerOfTwo(actual_alignment)) {
            return null;
        }

        // Use default or minimum chunk count if needed
        const actual_chunk_count = if (chunks_per_block < MIN_CHUNK_COUNT)
            DEFAULT_CHUNK_COUNT
        else
            chunks_per_block;

        // Allocate the allocator struct
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(c.malloc(@sizeOf(PoolAllocator)) orelse return null)));

        // Initialize fields
        self.* = .{
            .allocator = .{
                .vtable = &poolVTable,
                .strategy = .null,
                .out_of_mem_fn = null,
                .context = self,
                .track_stats = true,
                .stats = .{},
            },
            .blocks = null,
            .chunk_size = chunk_size,
            .alignment = actual_alignment,
            .chunk_count_per_block = actual_chunk_count,
            .free_list = null,
        };

        // Allocate initial block
        if (!self.addBlock()) {
            c.free(self);
            return null;
        }

        return self;
    }

    // Add a new block to the pool
    fn addBlock(self: *PoolAllocator) bool {
        // Create a new block
        const block = PoolBlock.create(self.chunk_size, self.chunk_count_per_block) orelse return false;

        // Add to the block list
        block.next = self.blocks;
        self.blocks = block;

        // Update the free list - merge block's free list with the pool's free list
        if (block.free_list) |free_head| {
            // Find the last chunk in the block's free list
            var last_chunk = free_head;
            while (last_chunk.next != null) {
                last_chunk = last_chunk.next.?;
            }

            // Link the block's free list to the pool's free list
            last_chunk.next = self.free_list;
            self.free_list = free_head;
        }

        // Update stats
        const block_capacity = block.chunk_size * block.chunk_count;
        if (self.allocator.track_stats) {
            self.allocator.stats.bytes_reserved += block_capacity;
        }

        return true;
    }

    // Allocate a chunk from the pool
    fn alloc(allocator: *Allocator, size: usize, alignment: usize, ret_addr: c_int) ?*anyopaque {
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(allocator.context.?)));

        // Check if the requested size fits in our chunk size
        if (size > self.chunk_size) return null;

        // Look for a free chunk in the pool's free list
        if (self.free_list) |free_chunk| {
            // Update the free list
            self.free_list = free_chunk.next;

            // Find which block this chunk belongs to by checking its address range
            var block = self.blocks;
            while (block) |b| {
                const chunk_addr = @intFromPtr(free_chunk);
                const block_data_addr = @intFromPtr(b.data);
                const block_size = b.chunk_count * @sizeOf(FreeChunk); // Use FreeChunk size as safe measure

                // Check if chunk is within the block's data area
                if (chunk_addr >= block_data_addr and chunk_addr < block_data_addr + block_size) {
                    // Found the block, update its free_chunks count
                    b.free_chunks -= 1;

                    // Initialize chunk header
                    const header = @as(*ChunkHeader, @ptrCast(free_chunk));
                    header.block = b;

                    // Update statistics
                    if (allocator.track_stats) {
                        allocator.stats.bytes_allocated += self.chunk_size;
                        allocator.stats.allocation_count += 1;
                        allocator.stats.total_allocations += 1;

                        if (allocator.stats.bytes_allocated > allocator.stats.max_bytes_allocated) {
                            allocator.stats.max_bytes_allocated = allocator.stats.bytes_allocated;
                        }
                    }

                    // Zero memory if requested
                    const alloc_options = AllocOptions.fromC(ret_addr);
                    if (alloc_options.zero) {
                        const data_offset = @sizeOf(ChunkHeader);
                        const data_ptr = @as([*]u8, @ptrCast(free_chunk)) + data_offset;
                        @memset(data_ptr[0..self.chunk_size], 0);
                    }

                    // Return the data portion of the chunk
                    return @as(*anyopaque, @ptrFromInt(@intFromPtr(free_chunk) + @sizeOf(ChunkHeader)));
                }

                block = b.next;
            }
        }

        // No free chunks, add a new block
        if (!self.addBlock()) {
            if (allocator.track_stats) {
                allocator.stats.failed_allocations += 1;
            }
            return null;
        }

        // Try again with the new block
        return PoolAllocator.alloc(allocator, size, alignment, ret_addr);
    }

    // Reallocate memory - in pool allocators, reallocations are tricky since chunks are fixed size
    fn realloc(allocator: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, ret_addr: c_int) ?*anyopaque {
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(allocator.context.?)));

        // Special cases
        if (ptr == null) {
            return PoolAllocator.alloc(allocator, new_size, alignment, ret_addr);
        }

        if (new_size == 0) {
            PoolAllocator.free(allocator, ptr, old_size, alignment);
            return null;
        }

        // If new size still fits in chunk, we can reuse the same chunk
        if (new_size <= self.chunk_size) {
            // Reuse the existing allocation

            // Update stats (no actual change in memory)

            // Zero any new memory if requested
            const alloc_options = AllocOptions.fromC(ret_addr);
            if (alloc_options.zero and new_size > old_size) {
                @memset(@as([*]u8, @ptrCast(ptr.?))[old_size..new_size], 0);
            }

            return ptr;
        }

        // If new size is too large, we need a new allocation
        if (allocator.track_stats) {
            allocator.stats.failed_allocations += 1;
        }
        return null;
    }

    // Free a chunk back to the pool
    fn free(allocator: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
        _ = size;
        _ = alignment;
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(allocator.context.?)));
        if (ptr == null) return;

        // Get the chunk header to find the block
        const header_ptr = @intFromPtr(ptr.?) - @sizeOf(ChunkHeader);
        const header = @as(*ChunkHeader, @ptrFromInt(header_ptr));

        // Get the chunk and add it to the pool's free list
        const chunk = @as(*FreeChunk, @ptrCast(@alignCast(header)));
        chunk.next = self.free_list;
        self.free_list = chunk;

        // Update the block's free_chunks count
        header.block.free_chunks += 1;

        // Update stats
        if (allocator.track_stats) {
            allocator.stats.bytes_allocated -|= self.chunk_size;
            allocator.stats.allocation_count -|= 1;
            allocator.stats.total_frees += 1;
        }
    }

    // Destroy the pool and all its blocks
    fn destroy(allocator: *Allocator) void {
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(allocator.context.?)));

        // Free all blocks
        var block = self.blocks;
        while (block != null) {
            const next = block.?.next;
            PoolBlock.destroy(block.?);
            block = next;
        }

        // Free the pool itself
        c.free(self);
    }

    // Get memory stats
    fn getStats(allocator: *Allocator) AllocStats {
        // Get the pool allocator context
        const self = @as(*PoolAllocator, @ptrCast(@alignCast(allocator.context.?)));
        _ = self; // Currently unused but will be used in a full implementation

        // Implementation of getStats function
        // This is a placeholder and should be implemented based on your actual AllocStats structure
        return .{};
    }
};

// Allocator vtable for pool allocator
const poolVTable = allocator_core.AllocatorVTable{
    .allocFn = PoolAllocator.alloc,
    .reallocFn = PoolAllocator.realloc,
    .freeFn = PoolAllocator.free,
    .destroyFn = PoolAllocator.destroy,
};

// Exported C API functions
export fn goo_pool_allocator_create(chunk_size: usize, alignment: usize, chunks_per_block: usize) ?*Allocator {
    if (PoolAllocator.init(chunk_size, alignment, chunks_per_block)) |pool| {
        return &pool.allocator;
    }

    return null;
}

export fn goo_pool_allocator_destroy(allocator: ?*Allocator) void {
    if (allocator == null) return;

    allocator.?.destroy();
}
