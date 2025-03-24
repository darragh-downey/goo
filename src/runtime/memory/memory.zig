const std = @import("std");
const builtin = @import("builtin");

/// Global memory management state
var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
var page_allocator = std.heap.page_allocator;
var allocator: std.mem.Allocator = undefined;
var is_initialized = false;

/// Initialize the memory subsystem
pub fn memoryInit() bool {
    if (is_initialized) {
        return true;
    }

    // Use the general purpose allocator by default
    allocator = general_purpose_allocator.allocator();
    is_initialized = true;
    return true;
}

/// Clean up the memory subsystem
pub fn memoryCleanup() void {
    if (!is_initialized) {
        return;
    }

    // Check for memory leaks if using GPA
    _ = general_purpose_allocator.deinit();
    is_initialized = false;
}

/// Allocate aligned memory
pub fn allocAligned(size: usize, _: usize) !*anyopaque {
    if (!is_initialized) {
        _ = memoryInit();
    }

    // Ensure minimal size
    const actual_size = if (size == 0) 1 else size;

    // Note: We're ignoring alignment for now as most allocators naturally align to powers of 2
    // Future improvement: implement proper aligned allocation if needed

    // Allocate the memory with alloc
    const memory = try allocator.alloc(u8, actual_size);
    return memory.ptr;
}

/// Reallocate aligned memory
pub fn reallocAligned(ptr: ?*anyopaque, old_size: usize, new_size: usize, _: usize) !*anyopaque {
    if (!is_initialized) {
        _ = memoryInit();
    }

    // If ptr is null, just allocate new memory
    if (ptr == null) {
        return try allocAligned(new_size, 0);
    }

    // Ensure minimal sizes
    const actual_old_size = if (old_size == 0) 1 else old_size;
    const actual_new_size = if (new_size == 0) 1 else new_size;

    // Create slices from the pointers
    const old_slice = @as([*]u8, @ptrCast(ptr.?))[0..actual_old_size];

    // Reallocate using simple alloc+copy+free approach for now
    const new_memory = try allocator.alloc(u8, actual_new_size);

    // Copy data from old allocation to new
    const copy_size = @min(actual_old_size, actual_new_size);
    @memcpy(new_memory[0..copy_size], old_slice[0..copy_size]);

    // Free the old allocation
    allocator.free(old_slice);
    return new_memory.ptr;
}

/// Free aligned memory
pub fn freeAligned(ptr: *anyopaque, size: usize, _: usize) void {
    if (!is_initialized) {
        return;
    }

    // Ensure minimal size
    const actual_size = if (size == 0) 1 else size;

    // Create slice from the pointer
    const slice = @as([*]u8, @ptrCast(ptr))[0..actual_size];

    // Free the memory
    allocator.free(slice);
}

/// Copy memory
pub fn copyAligned(dest: *anyopaque, src: *const anyopaque, size: usize, _: usize) void {
    if (size == 0) {
        return;
    }

    const dest_bytes = @as([*]u8, @ptrCast(dest));
    const src_bytes = @as([*]const u8, @ptrCast(src));

    @memcpy(dest_bytes[0..size], src_bytes[0..size]);
}

/// Set memory
pub fn setAligned(dest: *anyopaque, value: u8, size: usize, _: usize) void {
    if (size == 0) {
        return;
    }

    const dest_bytes = @as([*]u8, @ptrCast(dest));
    @memset(dest_bytes[0..size], value);
}

/// Get the default allocator
pub fn getAllocator() std.mem.Allocator {
    if (!is_initialized) {
        _ = memoryInit();
    }
    return allocator;
}

/// Get the page allocator (for larger allocations)
pub fn getPageAllocator() std.mem.Allocator {
    return page_allocator;
}
