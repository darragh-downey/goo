const std = @import("std");
const builtin = @import("builtin");
const c = @cImport({
    @cInclude("goo_core.h");
    @cInclude("goo_vectorization.h");
});

const memory = @import("memory/memory.zig");
const vectorization = @import("concurrency/zig/vectorization_fixed.zig");

// Global allocator
var gpa = std.heap.GeneralPurposeAllocator(.{}){};

// Export version information
pub const VERSION_MAJOR = 0;
pub const VERSION_MINOR = 1;
pub const VERSION_PATCH = 0;

// Export all memory management functions
comptime {
    // No need for the @export statements here since we use pub export functions
}

// ===== Memory Management Integration =====

// Initialize memory system
pub export fn goo_zig_memory_init() callconv(.C) bool {
    return memory.memoryInit();
}

// Cleanup memory system
pub export fn goo_zig_memory_cleanup() callconv(.C) void {
    memory.memoryCleanup();
}

// Allocate aligned memory
pub export fn goo_zig_alloc_aligned(size: usize, alignment: usize) callconv(.C) ?*anyopaque {
    const aligned_size = if (size == 0) 1 else size;
    const result = memory.allocAligned(aligned_size, alignment) catch return null;
    return result;
}

// Reallocate aligned memory
pub export fn goo_zig_realloc_aligned(ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize) callconv(.C) ?*anyopaque {
    const aligned_new_size = if (new_size == 0) 1 else new_size;
    if (ptr == null and old_size == 0) {
        return goo_zig_alloc_aligned(aligned_new_size, alignment);
    }

    const result = memory.reallocAligned(ptr, old_size, aligned_new_size, alignment) catch return null;
    return result;
}

// Free aligned memory
pub export fn goo_zig_free_aligned(ptr: ?*anyopaque, size: usize, alignment: usize) callconv(.C) void {
    if (ptr == null) return;
    memory.freeAligned(ptr.?, size, alignment);
}

// Copy aligned memory
pub export fn goo_zig_copy_aligned(dest: ?*anyopaque, src: ?*const anyopaque, size: usize, alignment: usize) callconv(.C) void {
    if (dest == null or src == null or size == 0) return;
    memory.copyAligned(dest.?, src.?, size, alignment);
}

// Set aligned memory
pub export fn goo_zig_set_aligned(dest: ?*anyopaque, value: u8, size: usize, alignment: usize) callconv(.C) void {
    if (dest == null or size == 0) return;
    memory.setAligned(dest.?, value, size, alignment);
}

// ===== SIMD Vectorization Integration =====

// Initialize vectorization subsystem
pub export fn goo_zig_vectorization_init(simd_type: c_int) callconv(.C) bool {
    return vectorization.initVectorization(@as(u8, @truncate(@as(u32, @intCast(simd_type)))));
}

// Cleanup vectorization subsystem
pub export fn goo_zig_vectorization_cleanup() callconv(.C) void {
    vectorization.cleanupVectorization();
}

// Detect supported SIMD
pub export fn goo_zig_vectorization_detect_simd() callconv(.C) c_int {
    return @as(c_int, @intCast(vectorization.detectSimd()));
}

// Get required alignment for SIMD operations
pub export fn goo_zig_vectorization_get_alignment(simd_type: c_int) callconv(.C) usize {
    return vectorization.getAlignmentForSimd(@as(u8, @truncate(@as(u32, @intCast(simd_type)))));
}

// Check if pointer is aligned for vectorization
pub export fn goo_zig_vectorization_is_aligned(ptr: ?*anyopaque, simd_type: c_int) callconv(.C) bool {
    if (ptr == null) return false;
    return vectorization.isAlignedForSimd(ptr.?, @as(u8, @truncate(@as(u32, @intCast(simd_type)))));
}

// Get vector width
pub export fn goo_zig_vectorization_get_width(data_type: c_int, simd_type: c_int) callconv(.C) usize {
    return vectorization.getVectorWidth(@as(u8, @truncate(@as(u32, @intCast(data_type)))), @as(u8, @truncate(@as(u32, @intCast(simd_type)))));
}

// Check if an operation can be accelerated
pub export fn goo_zig_vectorization_is_accelerated(data_type: c_int, op: c_int, simd_type: c_int) callconv(.C) bool {
    return vectorization.isOperationAccelerated(@as(u8, @truncate(@as(u32, @intCast(data_type)))), @as(u8, @truncate(@as(u32, @intCast(op)))), @as(u8, @truncate(@as(u32, @intCast(simd_type)))));
}

// Create vector mask
pub export fn goo_zig_vectorization_create_mask(size: usize, type_val: c_int) callconv(.C) ?*anyopaque {
    const mask = vectorization.createVectorMask(size, @as(u8, @truncate(@as(u32, @intCast(type_val))))) catch return null;
    return mask;
}

// Destroy vector mask
pub export fn goo_zig_vectorization_free_mask(mask: ?*anyopaque) callconv(.C) void {
    if (mask == null) return;
    vectorization.freeVectorMask(mask.?);
}

// Set vector mask values
pub export fn goo_zig_vectorization_set_mask(mask: ?*anyopaque, indices: [*]usize, count: usize) callconv(.C) bool {
    if (mask == null or count == 0) return false;

    const indices_slice = indices[0..count];
    vectorization.setVectorMask(mask.?, indices_slice) catch return false;

    return true;
}

// Utility function to dispatch Zig memory allocator to C
pub fn createZigAllocatorForC() callconv(.C) ?*c.GooAllocator {
    // Implementation would create a Zig allocator and wrap it for C
    return null; // Placeholder
}

// This function is the main entry point for the Zig runtime wrapper
pub fn main() !void {
    // Initialize Zig components
    _ = goo_zig_memory_init();
    _ = goo_zig_vectorization_init(255); // Auto

    // Cleanup before exit
    goo_zig_vectorization_cleanup();
    goo_zig_memory_cleanup();
}

// Register this runtime with the C runtime components
pub fn registerWithCRuntime() callconv(.C) bool {
    // Placeholder for integration code
    return true;
}
