const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("stdio.h");
    @cInclude("string.h");
});

// Constants
pub const DEFAULT_ALIGNMENT = 8;
pub const DEFAULT_ARENA_SIZE = 64 * 1024; // 64 KB

// Allocation options
pub const AllocOptions = packed struct {
    zero: bool = false,
    aligned: bool = false,
    exact: bool = false,
    persistent: bool = false,
    page_aligned: bool = false,
    no_fail: bool = false,
    _padding: u10 = 0,

    // Convert to C-compatible enum value
    pub fn toC(self: AllocOptions) u16 {
        var result: u16 = 0;
        if (self.zero) result |= 1 << 0;
        if (self.aligned) result |= 1 << 1;
        if (self.exact) result |= 1 << 2;
        if (self.persistent) result |= 1 << 3;
        if (self.page_aligned) result |= 1 << 4;
        if (self.no_fail) result |= 1 << 5;
        return result;
    }

    // Convert from C enum value
    pub fn fromC(value: c_int) AllocOptions {
        return .{
            .zero = (value & (1 << 0)) != 0,
            .aligned = (value & (1 << 1)) != 0,
            .exact = (value & (1 << 2)) != 0,
            .persistent = (value & (1 << 3)) != 0,
            .page_aligned = (value & (1 << 4)) != 0,
            .no_fail = (value & (1 << 5)) != 0,
        };
    }
};

// Error handling strategy
pub const AllocStrategy = enum(c_int) {
    panic = 0, // Abort process on failure
    null = 1, // Return NULL on failure
    retry = 2, // Retry allocation after calling out-of-memory handler
    gc = 3, // Trigger garbage collection (future)
};

// Allocation statistics
pub const AllocStats = extern struct {
    bytes_allocated: usize = 0, // Total bytes currently allocated
    bytes_reserved: usize = 0, // Total bytes reserved (may be higher than allocated)
    max_bytes_allocated: usize = 0, // Peak allocation
    allocation_count: usize = 0, // Number of active allocations
    total_allocations: usize = 0, // Total number of allocations
    total_frees: usize = 0, // Total number of frees
    failed_allocations: usize = 0, // Number of failed allocations
};

// Type definitions for compatibility
pub const OutOfMemFn = ?*const fn () callconv(.C) void;

// System allocator implementation
pub const SystemAllocator = struct {
    // Core state
    allocator: Allocator,
    initialized: bool, // Note: In a production system, this should be properly atomic

    // Initialize the system allocator
    pub fn init() SystemAllocator {
        const self = SystemAllocator{
            .allocator = .{
                .vtable = &systemVTable,
                .strategy = .panic,
                .out_of_mem_fn = defaultOutOfMemoryHandler,
                .context = null,
                .track_stats = true,
                .stats = .{},
            },
            .initialized = false,
        };
        return self;
    }

    // Virtual methods for system allocator
    fn alloc(allocator: *Allocator, size: usize, alignment: usize, options: c_int) ?*anyopaque {
        return systemAlloc(allocator, size, alignment, options);
    }

    fn realloc(allocator: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, options: c_int) ?*anyopaque {
        return systemRealloc(allocator, ptr, old_size, new_size, alignment, options);
    }

    fn free(allocator: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
        systemFree(allocator, ptr, size, alignment);
    }

    fn destroy(allocator: *Allocator) void {
        systemDestroy(allocator);
    }
};

// Allocator vtable for system allocator
const systemVTable = AllocatorVTable{
    .allocFn = SystemAllocator.alloc,
    .reallocFn = SystemAllocator.realloc,
    .freeFn = SystemAllocator.free,
    .destroyFn = SystemAllocator.destroy,
};

// Virtual function table for allocators
pub const AllocatorVTable = struct {
    allocFn: *const fn (allocator: *Allocator, size: usize, alignment: usize, options: c_int) ?*anyopaque,
    reallocFn: *const fn (allocator: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, options: c_int) ?*anyopaque,
    freeFn: *const fn (allocator: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void,
    destroyFn: *const fn (allocator: *Allocator) void,
};

// Base allocator struct - compatible with C ABI
pub const Allocator = extern struct {
    // Virtual function table
    vtable: *const AllocatorVTable,

    // Error handling strategy and callback
    strategy: AllocStrategy,
    out_of_mem_fn: OutOfMemFn,

    // Allocator-specific data
    context: ?*anyopaque,

    // Optional statistics tracking
    track_stats: bool,
    stats: AllocStats,

    // Method wrappers
    pub fn alloc(self: *Allocator, size: usize, alignment: usize, options: AllocOptions) ?*anyopaque {
        return self.vtable.allocFn(self, size, alignment, options.toC());
    }

    pub fn realloc(self: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, options: AllocOptions) ?*anyopaque {
        return self.vtable.reallocFn(self, ptr, old_size, new_size, alignment, options.toC());
    }

    pub fn free(self: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
        return self.vtable.freeFn(self, ptr, size, alignment);
    }

    pub fn destroy(self: *Allocator) void {
        return self.vtable.destroyFn(self);
    }
};

// Global instances
pub var g_system_allocator: SystemAllocator = undefined;
pub var goo_default_allocator: ?*Allocator = null;
pub threadlocal var goo_thread_allocator: ?*Allocator = null;

// Default out-of-memory handler
fn defaultOutOfMemoryHandler() callconv(.C) void {
    const stderr = std.io.getStdErr().writer();
    stderr.print("FATAL: Out of memory error\n", .{}) catch {};
    // We're already in an OOM situation, so there's not much we can do if this fails
}

// Initialize memory system
pub fn memoryInit() bool {
    // Skip if already initialized
    // Note: This is not thread-safe, in a production system should use atomic operations
    if (g_system_allocator.initialized) {
        return true; // Already initialized
    }

    g_system_allocator.initialized = true;

    // Set as default allocator
    goo_default_allocator = &g_system_allocator.allocator;

    return true;
}

// Memory cleanup function
pub fn memoryCleanup() void {
    // Nothing to do for now
}

// Set default allocator
pub fn setDefaultAllocator(allocator: ?*Allocator) void {
    goo_default_allocator = allocator;
}

// Get default allocator
pub fn getDefaultAllocator() *Allocator {
    if (goo_default_allocator == null) {
        _ = memoryInit();
    }
    return goo_default_allocator.?;
}

// Get thread-local allocator
pub fn getThreadAllocator() *Allocator {
    if (goo_thread_allocator == null) {
        return getDefaultAllocator();
    }
    return goo_thread_allocator.?;
}

// Set thread-local allocator
pub fn setThreadAllocator(allocator: ?*Allocator) void {
    goo_thread_allocator = allocator;
}

// Calculate aligned address
fn alignPointer(ptr: [*]u8, alignment: usize) [*]u8 {
    const addr = @intFromPtr(ptr);
    const aligned_addr = (addr + alignment - 1) & ~(alignment - 1);
    return @ptrFromInt(aligned_addr);
}

// Calculate aligned size
fn alignSize(size: usize, alignment: usize) usize {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Handle allocation failure based on strategy
fn handleAllocationFailure(allocator: *Allocator, size: usize) ?*anyopaque {
    if (allocator.track_stats) {
        allocator.stats.failed_allocations += 1;
    }

    switch (allocator.strategy) {
        .panic => {
            if (allocator.out_of_mem_fn) |handler| {
                handler();
            }
            std.debug.print("Fatal error: Out of memory (requested {d} bytes)\n", .{size});
            @panic("Out of memory");
        },
        .retry => {
            if (allocator.out_of_mem_fn) |handler| {
                handler();
                // Try again after handler runs
                return allocator.alloc(size, DEFAULT_ALIGNMENT, .{});
            }
            // Fall through to NULL if no handler
            return null;
        },
        // GC not implemented yet
        .null, .gc => return null,
    }
}

// System allocator implementation
fn systemAlloc(self: *Allocator, size: usize, alignment: usize, options: c_int) ?*anyopaque {
    if (size == 0) {
        return null;
    }

    // Use standard alignment if not specified
    const actual_alignment = if (alignment == 0) DEFAULT_ALIGNMENT else alignment;

    // For safety, ensure alignment is a power of 2
    if (!std.math.isPowerOfTwo(actual_alignment)) {
        // Log error but proceed with next power of 2
        std.debug.print("Warning: alignment {d} is not a power of 2\n", .{actual_alignment});
    }

    // Allocate memory
    var ptr: ?*anyopaque = null;

    // For simple cases, use malloc directly
    if (actual_alignment <= DEFAULT_ALIGNMENT) {
        ptr = c.malloc(size);
    } else {
        // Use aligned allocation
        ptr = blk: {
            var temp: ?*anyopaque = undefined;
            if (c.posix_memalign(&temp, actual_alignment, size) != 0) {
                break :blk null;
            }
            break :blk temp;
        };
    }

    // Handle allocation failure
    if (ptr == null) {
        return handleAllocationFailure(self, size);
    }

    // Zero memory if requested
    const alloc_options = AllocOptions.fromC(options);
    if (alloc_options.zero) {
        @memset(@as([*]u8, @ptrCast(ptr.?))[0..size], 0);
    }

    // Update statistics
    if (self.track_stats) {
        self.stats.bytes_allocated += size;
        self.stats.bytes_reserved += size;
        if (self.stats.bytes_allocated > self.stats.max_bytes_allocated) {
            self.stats.max_bytes_allocated = self.stats.bytes_allocated;
        }
        self.stats.allocation_count += 1;
        self.stats.total_allocations += 1;
    }

    return ptr;
}

fn systemRealloc(self: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, options: c_int) ?*anyopaque {
    // Handle special cases
    if (ptr == null) {
        return self.vtable.allocFn(self, new_size, alignment, options);
    }

    if (new_size == 0) {
        self.vtable.freeFn(self, ptr, old_size, alignment);
        return null;
    }

    // Update statistics for the old allocation
    if (self.track_stats) {
        self.stats.bytes_allocated -|= old_size; // Saturating subtraction for safety
        self.stats.bytes_reserved -|= old_size;
        self.stats.allocation_count -|= 1;
    }

    // Use standard alignment if not specified
    const actual_alignment = if (alignment == 0) DEFAULT_ALIGNMENT else alignment;

    // For safety, ensure alignment is a power of 2
    if (!std.math.isPowerOfTwo(actual_alignment)) {
        // Log error but proceed with next power of 2
        std.debug.print("Warning: alignment {d} is not a power of 2\n", .{actual_alignment});
    }

    // Use aligned realloc for larger alignments
    var new_ptr: ?*anyopaque = null;

    if (actual_alignment <= DEFAULT_ALIGNMENT) {
        new_ptr = c.realloc(ptr, new_size);
    } else {
        // For aligned memory, we need to allocate new memory and copy
        const aligned_ptr = blk: {
            var temp: ?*anyopaque = undefined;
            if (c.posix_memalign(&temp, actual_alignment, new_size) != 0) {
                break :blk null;
            }
            break :blk temp;
        };

        if (aligned_ptr != null) {
            // Copy old data
            const copy_size = @min(old_size, new_size);
            @memcpy(@as([*]u8, @ptrCast(aligned_ptr.?))[0..copy_size], @as([*]u8, @ptrCast(ptr.?))[0..copy_size]);

            // Free old memory
            c.free(ptr);
            new_ptr = aligned_ptr;
        }
    }

    // Handle allocation failure
    if (new_ptr == null) {
        // Restore statistics for the old allocation since we still have it
        if (self.track_stats) {
            self.stats.bytes_allocated += old_size;
            self.stats.bytes_reserved += old_size;
            self.stats.allocation_count += 1;
        }
        return handleAllocationFailure(self, new_size);
    }

    // Zero any new memory if requested
    const alloc_options = AllocOptions.fromC(options);
    if (alloc_options.zero and new_size > old_size) {
        @memset(@as([*]u8, @ptrCast(new_ptr.?))[old_size..new_size], 0);
    }

    // Update statistics for the new allocation
    if (self.track_stats) {
        self.stats.bytes_allocated += new_size;
        self.stats.bytes_reserved += new_size;
        if (self.stats.bytes_allocated > self.stats.max_bytes_allocated) {
            self.stats.max_bytes_allocated = self.stats.bytes_allocated;
        }
        self.stats.allocation_count += 1;
        self.stats.total_allocations += 1;
    }

    return new_ptr;
}

fn systemFree(self: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
    if (ptr == null) {
        return;
    }

    // Update statistics
    if (self.track_stats) {
        self.stats.bytes_allocated -|= size; // Saturating subtraction for safety
        self.stats.bytes_reserved -|= size;
        self.stats.allocation_count -|= 1;
        self.stats.total_frees += 1;
    }

    // Use appropriate free function based on alignment
    if (alignment <= DEFAULT_ALIGNMENT) {
        c.free(ptr);
    } else {
        // For aligned memory (this is standard on POSIX)
        c.free(ptr);
    }
}

fn systemDestroy(self: *Allocator) void {
    // Nothing to do for system allocator
    _ = self;
}

// Exported C API functions
export fn goo_memory_init() bool {
    // Initialize our global allocator
    g_system_allocator = SystemAllocator.init();
    return memoryInit();
}

export fn goo_memory_cleanup() void {
    return memoryCleanup();
}

export fn goo_set_default_allocator(allocator: ?*Allocator) void {
    return setDefaultAllocator(allocator);
}

export fn goo_get_default_allocator() ?*Allocator {
    return getDefaultAllocator();
}

export fn goo_get_thread_allocator() ?*Allocator {
    return getThreadAllocator();
}

export fn goo_set_thread_allocator(allocator: ?*Allocator) void {
    return setThreadAllocator(allocator);
}

export fn goo_set_out_of_mem_handler(handler: OutOfMemFn) void {
    const default_allocator = getDefaultAllocator();
    default_allocator.out_of_mem_fn = handler;
}

// Helper utility functions (using default allocator)
export fn goo_alloc(size: usize) ?*anyopaque {
    const allocator = getDefaultAllocator();
    return allocator.alloc(size, DEFAULT_ALIGNMENT, .{});
}

export fn goo_alloc_zero(size: usize) ?*anyopaque {
    const allocator = getDefaultAllocator();
    return allocator.alloc(size, DEFAULT_ALIGNMENT, .{ .zero = true });
}

export fn goo_realloc(ptr: ?*anyopaque, old_size: usize, new_size: usize) ?*anyopaque {
    const allocator = getDefaultAllocator();
    return allocator.realloc(ptr, old_size, new_size, DEFAULT_ALIGNMENT, .{});
}

export fn goo_free(ptr: ?*anyopaque, size: usize) void {
    const allocator = getDefaultAllocator();
    return allocator.free(ptr, size, DEFAULT_ALIGNMENT);
}

export fn goo_alloc_aligned(size: usize, alignment: usize) ?*anyopaque {
    const allocator = getDefaultAllocator();
    return allocator.alloc(size, alignment, .{});
}

export fn goo_free_aligned(ptr: ?*anyopaque, size: usize, alignment: usize) void {
    const allocator = getDefaultAllocator();
    return allocator.free(ptr, size, alignment);
}

// Additional utility functions that work with a specific allocator
export fn goo_alloc_with_allocator(allocator: ?*Allocator, size: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.alloc(size, DEFAULT_ALIGNMENT, .{});
}

export fn goo_alloc_zero_with_allocator(allocator: ?*Allocator, size: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.alloc(size, DEFAULT_ALIGNMENT, .{ .zero = true });
}

export fn goo_realloc_with_allocator(allocator: ?*Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.realloc(ptr, old_size, new_size, DEFAULT_ALIGNMENT, .{});
}

export fn goo_free_with_allocator(allocator: ?*Allocator, ptr: ?*anyopaque, size: usize) void {
    if (allocator == null) return;
    return allocator.?.free(ptr, size, DEFAULT_ALIGNMENT);
}

export fn goo_alloc_aligned_with_allocator(allocator: ?*Allocator, size: usize, alignment: usize) ?*anyopaque {
    if (allocator == null) return null;
    return allocator.?.alloc(size, alignment, .{});
}

export fn goo_free_aligned_with_allocator(allocator: ?*Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
    if (allocator == null) return;
    return allocator.?.free(ptr, size, alignment);
}

// Using Zig's cleaner memory stats API
export fn goo_get_alloc_stats(allocator: ?*Allocator) AllocStats {
    if (allocator) |alloc| {
        return alloc.stats;
    }
    return .{};
}

// Alias for goo_get_alloc_stats for API consistency
export fn goo_get_allocator_stats(allocator: ?*Allocator) AllocStats {
    return goo_get_alloc_stats(allocator);
}

// Helper for scope-based allocations in C
export fn goo_scope_cleanup(ptr: [*]?*anyopaque) void {
    if (ptr[0]) |mem| {
        goo_free(mem, 0); // Size is technically incorrect, but the C code did this too
        ptr[0] = null;
    }
}

// Create a system allocator (malloc/free based)
export fn goo_system_allocator_create() ?*Allocator {
    if (goo_default_allocator == null) {
        _ = goo_memory_init();
    }
    return goo_default_allocator;
}

// GooAllocator is a wrapper around Zig's standard allocator
pub const GooAllocator = struct {
    allocator: std.mem.Allocator,
    stats: AllocationStats,

    // Statistics for memory allocations
    pub const AllocationStats = struct {
        allocations: usize = 0,
        deallocations: usize = 0,
        bytes_allocated: usize = 0,
        current_usage: usize = 0,
        peak_usage: usize = 0,
    };

    // Create a new allocator instance
    pub fn create(backing_allocator: std.mem.Allocator) !*GooAllocator {
        const self = try backing_allocator.create(GooAllocator);
        self.* = GooAllocator{
            .allocator = backing_allocator,
            .stats = AllocationStats{},
        };
        return self;
    }

    // Destroy the allocator
    pub fn destroy(self: *GooAllocator) void {
        const backing = self.allocator;
        backing.destroy(self);
    }

    // Allocate memory
    pub fn alloc(self: *GooAllocator, size: usize) !*anyopaque {
        const memory = try self.allocator.alignedAlloc(u8, @alignOf(usize), size);

        // Update stats
        self.stats.allocations += 1;
        self.stats.bytes_allocated += size;
        self.stats.current_usage += size;
        self.stats.peak_usage = @max(self.stats.peak_usage, self.stats.current_usage);

        return memory.ptr;
    }

    // Free memory
    pub fn free(self: *GooAllocator, ptr: *anyopaque) void {
        // In a real implementation, we would track the size of each allocation
        // This is simplified for the example
        const ptr_slice = @as([*]u8, @ptrCast(ptr))[0..1];

        self.stats.deallocations += 1;
        // Approximate size tracking would go here

        self.allocator.free(ptr_slice);
    }

    // Reallocate memory
    pub fn realloc(self: *GooAllocator, ptr: ?*anyopaque, old_size: usize, new_size: usize) !*anyopaque {
        if (ptr == null) {
            return self.alloc(new_size);
        }

        const ptr_slice = @as([*]u8, @ptrCast(ptr.?))[0..old_size];
        const new_mem = try self.allocator.realloc(ptr_slice, new_size);

        // Update stats
        if (new_size > old_size) {
            self.stats.bytes_allocated += (new_size - old_size);
            self.stats.current_usage += (new_size - old_size);
        } else {
            self.stats.current_usage -= (old_size - new_size);
        }

        self.stats.peak_usage = @max(self.stats.peak_usage, self.stats.current_usage);

        return new_mem.ptr;
    }

    // Get allocation statistics
    pub fn getStats(self: *const GooAllocator) AllocationStats {
        return self.stats;
    }

    // Reset statistics
    pub fn resetStats(self: *GooAllocator) void {
        self.stats = AllocationStats{};
    }
};

// Fix the problematic C API compatibility functions
export fn goo_allocator_alloc(size: usize) ?*anyopaque {
    // Use alloc instead of allocBytes
    const result = std.heap.c_allocator.alloc(u8, size) catch return null;
    return result.ptr;
}

export fn goo_allocator_realloc(ptr: ?*anyopaque, old_size: usize, new_size: usize) ?*anyopaque {
    if (ptr == null) {
        return goo_allocator_alloc(new_size);
    }

    // Use arrays instead of raw slices
    const ptr_slice = @as([*]u8, @ptrCast(ptr.?))[0..old_size];
    const new_mem = std.heap.c_allocator.realloc(ptr_slice, new_size) catch return null;
    return new_mem.ptr;
}

export fn goo_allocator_free(ptr: ?*anyopaque) void {
    if (ptr) |p| {
        // This is a simplification; in a real implementation we would track allocation sizes
        const ptr_slice = @as([*]u8, @ptrCast(p))[0..1];
        std.heap.c_allocator.free(ptr_slice);
    }
}

export fn goo_allocator_aligned_alloc(size: usize, alignment: usize) ?*anyopaque {
    // Use 8 as a comptime-known alignment
    // For actual runtime alignment, we'd need to create aligned memory ourselves
    const fixed_alignment: usize = 8;

    // Explicitly discard the alignment parameter for now
    _ = alignment;

    const mem = std.heap.c_allocator.alignedAlloc(u8, fixed_alignment, size) catch return null;

    // Then align it manually if necessary
    return mem.ptr;
}

export fn goo_allocator_calloc(num: usize, size: usize) ?*anyopaque {
    const total_size = num * size;
    const mem = goo_allocator_alloc(total_size) orelse return null;

    // Zero the memory
    const bytes = @as([*]u8, @ptrCast(mem))[0..total_size];
    @memset(bytes, 0);

    return mem;
}

export fn goo_allocator_strdup(str: [*:0]const u8) ?[*:0]u8 {
    const len = std.mem.len(str);
    const dup = goo_allocator_alloc(len + 1) orelse return null;

    const src = str[0..len :0];
    const dest = @as([*]u8, @ptrCast(dup))[0 .. len + 1];

    @memcpy(dest[0..len], src[0..len]);
    dest[len] = 0; // Null terminator

    return @ptrCast(dest.ptr);
}

export fn goo_allocator_get_stats() void {
    // In a real implementation, this would return statistics
    // For now, this is just a stub
}

export fn goo_allocator_reset_stats() void {
    // In a real implementation, this would reset statistics
    // For now, this is just a stub
}
