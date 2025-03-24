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
const DEFAULT_REGION_SIZE = 1024 * 1024; // 1 MB
const MIN_REGION_SIZE = 64 * 1024; // 64 KB
const MAX_ALLOCATION_SIZE = 1024 * 1024; // Maximum size for a single allocation

// Region structure
const Region = struct {
    next: ?*Region, // Linked list of regions
    size: usize, // Total size of this region
    used: usize, // Used space in this region
    data: [*]u8, // Memory area

    // Create a new region
    fn create(size: usize) ?*Region {
        // Allocate the region header and data in one block
        const header_size = @sizeOf(Region);
        const total_size = header_size + size;

        const ptr = c.malloc(total_size);
        if (ptr == null) return null;

        // Initialize the region
        const region = @as(*Region, @ptrCast(@alignCast(ptr)));
        region.* = .{
            .next = null,
            .size = size,
            .used = 0,
            .data = @as([*]u8, @ptrCast(@alignCast(ptr))) + header_size,
        };

        return region;
    }

    // Destroy a region
    fn destroy(region: *Region) void {
        c.free(region);
    }

    // Allocate memory from this region
    fn alloc(self: *Region, size: usize, alignment: usize) ?*anyopaque {
        // Ensure alignment
        const current_ptr = @intFromPtr(&self.data[self.used]);
        const aligned_ptr = (current_ptr + alignment - 1) & ~(alignment - 1);
        const padding = aligned_ptr - current_ptr;

        // Check if we have enough space
        const total_needed = padding + size;
        if (self.used + total_needed > self.size) {
            return null;
        }

        // Update used count and return aligned pointer
        self.used += total_needed;
        return @as(*anyopaque, @ptrFromInt(aligned_ptr));
    }

    // Reset a region for reuse
    fn reset(self: *Region) void {
        self.used = 0;
    }
};

// Allocation header for tracking
const AllocHeader = struct {
    size: usize, // Size of this allocation
    payload: [*]u8, // Pointer to the actual data
};

// Region allocator implementation
pub const RegionAllocator = struct {
    // Base allocator interface
    allocator: Allocator,

    // Region-specific state
    regions: ?*Region,
    current_region: ?*Region,
    region_size: usize,
    allow_large_allocations: bool,

    // Initialize a region allocator
    pub fn init(region_size: usize, allow_large_allocations: bool) ?*RegionAllocator {
        // Validate parameters
        const actual_region_size = if (region_size < MIN_REGION_SIZE)
            DEFAULT_REGION_SIZE
        else
            region_size;

        // Allocate the allocator struct
        const self = @as(*RegionAllocator, @ptrCast(@alignCast(c.malloc(@sizeOf(RegionAllocator)) orelse return null)));

        // Initialize fields
        self.* = .{
            .allocator = .{
                .vtable = &regionVTable,
                .strategy = .null,
                .out_of_mem_fn = null,
                .context = self,
                .track_stats = true,
                .stats = .{},
            },
            .regions = null,
            .current_region = null,
            .region_size = actual_region_size,
            .allow_large_allocations = allow_large_allocations,
        };

        // Allocate initial region
        if (!self.addRegion(actual_region_size)) {
            c.free(self);
            return null;
        }

        return self;
    }

    // Add a new region
    fn addRegion(self: *RegionAllocator, min_size: usize) bool {
        // Determine region size (at least min_size, but normally the default size)
        const size = if (min_size > self.region_size) min_size else self.region_size;

        // Create the region
        const region = Region.create(size) orelse return false;

        // Add to the region list
        if (self.regions == null) {
            self.regions = region;
        } else if (self.current_region) |current| {
            current.next = region;
        }

        self.current_region = region;

        // Update stats
        if (self.allocator.track_stats) {
            self.allocator.stats.bytes_reserved += size;
        }

        return true;
    }

    // Get the allocation header to retrieve the original size
    fn getHeader(ptr: ?*anyopaque) ?*AllocHeader {
        if (ptr == null) return null;
        const header = @as(*AllocHeader, @ptrCast(@alignCast(ptr.?)));
        return header;
    }

    // Allocate memory from the region
    fn alloc(allocator: *Allocator, size: usize, alignment: usize, ret_addr: c_int) ?*anyopaque {
        if (size == 0) return null;

        const self = @as(*RegionAllocator, @ptrCast(@alignCast(allocator.context.?)));
        const actual_alignment = if (alignment == 0) DEFAULT_ALIGNMENT else alignment;

        // Calculate aligned sizes for the header
        const header_size = @sizeOf(AllocHeader);
        const header_aligned_size = std.mem.alignForward(usize, header_size, actual_alignment);
        const total_size = header_aligned_size + size;

        // Check for large allocations
        if (total_size > self.region_size) {
            // For very large allocations, we can either fail or create a custom-sized region
            if (!self.allow_large_allocations) {
                if (allocator.track_stats) {
                    allocator.stats.failed_allocations += 1;
                }
                return null;
            }

            // Create a custom-sized region
            if (!self.addRegion(total_size)) {
                if (allocator.track_stats) {
                    allocator.stats.failed_allocations += 1;
                }
                return null;
            }

            // The new region is now the current one
        }

        // Try to allocate from current region
        var ptr: ?*anyopaque = null;
        if (self.current_region) |current| {
            ptr = current.alloc(total_size, actual_alignment);
        }

        // If current region is full, allocate a new one
        if (ptr == null) {
            if (!self.addRegion(total_size)) {
                if (allocator.track_stats) {
                    allocator.stats.failed_allocations += 1;
                }
                return null;
            }

            // Try again with the new region
            ptr = self.current_region.?.alloc(total_size, actual_alignment);
            if (ptr == null) {
                // This should never happen since we just created a region of sufficient size
                if (allocator.track_stats) {
                    allocator.stats.failed_allocations += 1;
                }
                return null;
            }
        }

        // Set up the allocation header
        const header = @as(*AllocHeader, @ptrCast(@alignCast(ptr.?)));
        header.size = size;
        header.payload = @as([*]u8, @ptrCast(ptr.?)) + header_aligned_size;

        // Update stats
        if (allocator.track_stats) {
            allocator.stats.bytes_allocated += size;
            allocator.stats.allocation_count += 1;
            allocator.stats.total_allocations += 1;

            if (allocator.stats.bytes_allocated > allocator.stats.max_bytes_allocated) {
                allocator.stats.max_bytes_allocated = allocator.stats.bytes_allocated;
            }
        }

        // Zero memory if requested
        const alloc_options = AllocOptions.fromC(ret_addr);
        if (alloc_options.zero) {
            @memset(header.payload[0..size], 0);
        }

        return @as(*anyopaque, @ptrCast(header.payload));
    }

    // Realloc is similar to arena - we always allocate new memory
    fn realloc(allocator: *Allocator, ptr: ?*anyopaque, old_size: usize, new_size: usize, alignment: usize, ret_addr: c_int) ?*anyopaque {
        // Special cases
        if (ptr == null) {
            return RegionAllocator.alloc(allocator, new_size, alignment, ret_addr);
        }

        if (new_size == 0) {
            RegionAllocator.free(allocator, ptr, old_size, alignment);
            return null;
        }

        // Allocate new memory
        const new_ptr = RegionAllocator.alloc(allocator, new_size, alignment, ret_addr);
        if (new_ptr == null) {
            return null;
        }

        // Copy data from old to new
        const copy_size = @min(old_size, new_size);
        @memcpy(@as([*]u8, @ptrCast(new_ptr))[0..copy_size], @as([*]u8, @ptrCast(ptr))[0..copy_size]);

        // Update stats for the old allocation
        if (allocator.track_stats) {
            allocator.stats.bytes_allocated -|= old_size;
            allocator.stats.allocation_count -|= 1;
            allocator.stats.total_frees += 1;
        }

        return new_ptr;
    }

    // Individual frees track stats but don't actually free memory
    fn free(allocator: *Allocator, ptr: ?*anyopaque, size: usize, alignment: usize) void {
        _ = alignment;
        if (ptr == null) return;

        // Update stats
        if (allocator.track_stats) {
            // For more accurate stats, we can get the actual size from the header
            // But we can also just use the provided size for simplicity
            allocator.stats.bytes_allocated -|= size;
            allocator.stats.allocation_count -|= 1;
            allocator.stats.total_frees += 1;
        }
    }

    // Reset all regions for reuse
    pub fn reset(self: *RegionAllocator) void {
        var region = self.regions;
        while (region) |r| {
            r.reset();
            region = r.next;
        }

        // Reset the current region to the first one
        self.current_region = self.regions;

        // Reset stats
        if (self.allocator.track_stats) {
            self.allocator.stats.bytes_allocated = 0;
            self.allocator.stats.allocation_count = 0;
        }
    }

    // Destroy the allocator and all its regions
    fn destroy(allocator: *Allocator) void {
        const self = @as(*RegionAllocator, @ptrCast(@alignCast(allocator.context.?)));

        // Free all regions
        var region = self.regions;
        while (region != null) {
            const next = region.?.next;
            Region.destroy(region.?);
            region = next;
        }

        // Free the allocator itself
        c.free(self);
    }
};

// Allocator vtable for region allocator
const regionVTable = allocator_core.AllocatorVTable{
    .allocFn = RegionAllocator.alloc,
    .reallocFn = RegionAllocator.realloc,
    .freeFn = RegionAllocator.free,
    .destroyFn = RegionAllocator.destroy,
};

// Exported C API functions
export fn goo_region_allocator_create(region_size: usize, allow_large_allocations: bool) ?*Allocator {
    if (RegionAllocator.init(region_size, allow_large_allocations)) |region| {
        return &region.allocator;
    }

    return null;
}

export fn goo_region_allocator_reset(allocator: ?*Allocator) void {
    if (allocator == null) return;

    const self = @as(*RegionAllocator, @ptrCast(@alignCast(allocator.?.context.?)));
    self.reset();
}

export fn goo_region_allocator_destroy(allocator: ?*Allocator) void {
    if (allocator == null) return;

    allocator.?.destroy();
}
