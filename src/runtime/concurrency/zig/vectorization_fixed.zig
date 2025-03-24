const std = @import("std");
const builtin = @import("builtin");
const memory = @import("../../memory/memory.zig");

// SIMD Type definitions
pub const SimdType = enum(u8) {
    None = 0,
    SSE2 = 1,
    SSE4 = 2,
    AVX = 3,
    AVX2 = 4,
    AVX512 = 5,
    NEON = 6,
    Auto = 255,
};

// Vector data types
pub const VectorDataType = enum(u8) {
    Int8 = 0,
    Int16 = 1,
    Int32 = 2,
    Int64 = 3,
    UInt8 = 4,
    UInt16 = 5,
    UInt32 = 6,
    UInt64 = 7,
    Float32 = 8,
    Float64 = 9,
};

// Vector operations
pub const VectorOp = enum(u8) {
    // Arithmetic operations
    Add = 0,
    Sub = 1,
    Mul = 2,
    Div = 3,

    // Bitwise operations
    And = 10,
    Or = 11,
    Xor = 12,
    Not = 13,

    // Comparison operations
    Eq = 20,
    Ne = 21,
    Lt = 22,
    Le = 23,
    Gt = 24,
    Ge = 25,

    // Mathematical functions
    Sqrt = 30,
    Abs = 31,
    Min = 32,
    Max = 33,

    // Movement operations
    Load = 40,
    Store = 41,
    Gather = 42,
    Scatter = 43,

    // Utility operations
    Blend = 50,
    Shuffle = 51,
    Set1 = 52,
};

// Vector mask for masked operations
pub const VectorMask = struct {
    data: []u8,
    size: usize,
    data_type: VectorDataType,

    pub fn init(size: usize, data_type: VectorDataType) !*VectorMask {
        const allocator = memory.getAllocator();

        // Allocate the mask structure
        const mask = try allocator.create(VectorMask);

        // Determine bytes needed based on size
        const bytes_needed = (size + 7) / 8; // 1 bit per element, rounded up to bytes

        // Allocate data buffer
        mask.data = try allocator.alloc(u8, bytes_needed);
        @memset(mask.data, 0); // Initialize to all zeros

        // Set fields
        mask.size = size;
        mask.data_type = data_type;

        return mask;
    }

    pub fn deinit(self: *VectorMask) void {
        const allocator = memory.getAllocator();
        allocator.free(self.data);
        allocator.destroy(self);
    }

    pub fn setBit(self: *VectorMask, index: usize, value: bool) void {
        if (index >= self.size) {
            return; // Out of bounds
        }

        const byte_index = index / 8;
        const bit_index = @as(u3, @truncate(index % 8));

        if (value) {
            // Set bit
            self.data[byte_index] |= @as(u8, 1) << bit_index;
        } else {
            // Clear bit
            self.data[byte_index] &= ~(@as(u8, 1) << bit_index);
        }
    }

    pub fn getBit(self: *VectorMask, index: usize) bool {
        if (index >= self.size) {
            return false; // Out of bounds
        }

        const byte_index = index / 8;
        const bit_index = @as(u3, @truncate(index % 8));

        return (self.data[byte_index] & (@as(u8, 1) << bit_index)) != 0;
    }
};

// Global state
var active_simd_type: SimdType = .None;
var initialized = false;

// Initialize the vectorization subsystem
pub fn initVectorization(simd_type: u8) bool {
    if (initialized) {
        return true;
    }

    // Convert integer to enum
    var requested_type = SimdType.None;
    if (simd_type <= @intFromEnum(SimdType.NEON)) {
        requested_type = @enumFromInt(simd_type);
    } else if (simd_type == @intFromEnum(SimdType.Auto)) {
        requested_type = SimdType.Auto;
    }

    // Auto-detect if requested
    if (requested_type == .Auto) {
        active_simd_type = detectBestSimdType();
    } else {
        // Check if requested type is supported
        if (isSimdTypeSupported(requested_type)) {
            active_simd_type = requested_type;
        } else {
            // Fall back to best supported
            active_simd_type = detectBestSimdType();
        }
    }

    initialized = true;
    return true;
}

// Clean up the vectorization subsystem
pub fn cleanupVectorization() void {
    if (!initialized) {
        return;
    }

    initialized = false;
    active_simd_type = .None;
}

// Detect available SIMD instruction sets
pub fn detectSimd() u8 {
    return @intFromEnum(detectBestSimdType());
}

// Helper function to detect the best available SIMD type
fn detectBestSimdType() SimdType {
    // Check CPU features based on target architecture
    if (builtin.cpu.arch == .x86_64 or builtin.cpu.arch == .x86) {
        // Check for AVX-512 support
        if (comptime std.Target.x86.featureSetHas(builtin.cpu.features, .avx512f)) {
            return .AVX512;
        }
        // Check for AVX2 support
        if (comptime std.Target.x86.featureSetHas(builtin.cpu.features, .avx2)) {
            return .AVX2;
        }
        // Check for AVX support
        if (comptime std.Target.x86.featureSetHas(builtin.cpu.features, .avx)) {
            return .AVX;
        }
        // Check for SSE4 support
        if (comptime std.Target.x86.featureSetHas(builtin.cpu.features, .sse4_1)) {
            return .SSE4;
        }
        // Check for SSE2 support
        if (comptime std.Target.x86.featureSetHas(builtin.cpu.features, .sse2)) {
            return .SSE2;
        }
    } else if (builtin.cpu.arch == .aarch64 or builtin.cpu.arch == .arm) {
        // Check for NEON support
        if (comptime std.Target.aarch64.featureSetHas(builtin.cpu.features, .neon)) {
            return .NEON;
        }
    }

    // No SIMD support detected
    return .None;
}

// Check if a SIMD type is supported
fn isSimdTypeSupported(simd_type: SimdType) bool {
    const best = detectBestSimdType();

    // No SIMD support
    if (best == .None) {
        return simd_type == .None;
    }

    // Check x86 hierarchy
    if (simd_type == .SSE2) {
        return @intFromEnum(best) >= @intFromEnum(SimdType.SSE2);
    } else if (simd_type == .SSE4) {
        return @intFromEnum(best) >= @intFromEnum(SimdType.SSE4);
    } else if (simd_type == .AVX) {
        return @intFromEnum(best) >= @intFromEnum(SimdType.AVX);
    } else if (simd_type == .AVX2) {
        return @intFromEnum(best) >= @intFromEnum(SimdType.AVX2);
    } else if (simd_type == .AVX512) {
        return @intFromEnum(best) >= @intFromEnum(SimdType.AVX512);
    } else if (simd_type == .NEON) {
        return best == .NEON;
    }

    return false;
}

// Get required alignment for SIMD operations
pub fn getAlignmentForSimd(simd_type: u8) usize {
    const type_enum = if (simd_type <= @intFromEnum(SimdType.NEON))
        @as(SimdType, @enumFromInt(simd_type))
    else
        active_simd_type;

    return switch (type_enum) {
        .None => 1,
        .SSE2, .SSE4 => 16,
        .AVX, .AVX2 => 32,
        .AVX512 => 64,
        .NEON => 16,
        .Auto => getAlignmentForSimd(@intFromEnum(active_simd_type)),
    };
}

// Check if a pointer is properly aligned for SIMD
pub fn isAlignedForSimd(ptr: *anyopaque, simd_type: u8) bool {
    const alignment = getAlignmentForSimd(simd_type);
    const addr = @intFromPtr(ptr);
    return addr % alignment == 0;
}

// Get optimal vector width for a data type with SIMD
pub fn getVectorWidth(data_type: u8, simd_type: u8) usize {
    // Don't need type_enum here since we're only using data_enum

    const data_enum = if (data_type <= @intFromEnum(VectorDataType.Float64))
        @as(VectorDataType, @enumFromInt(data_type))
    else
        VectorDataType.Float32;

    const alignment = getAlignmentForSimd(simd_type);

    return switch (data_enum) {
        .Int8, .UInt8 => alignment,
        .Int16, .UInt16 => alignment / 2,
        .Int32, .UInt32, .Float32 => alignment / 4,
        .Int64, .UInt64, .Float64 => alignment / 8,
    };
}

// Check if an operation can be accelerated with SIMD
pub fn isOperationAccelerated(data_type: u8, op: u8, simd_type: u8) bool {
    const type_enum = if (simd_type <= @intFromEnum(SimdType.NEON))
        @as(SimdType, @enumFromInt(simd_type))
    else
        active_simd_type;

    // If no SIMD support, nothing is accelerated
    if (type_enum == .None) {
        return false;
    }

    const data_enum = if (data_type <= @intFromEnum(VectorDataType.Float64))
        @as(VectorDataType, @enumFromInt(data_type))
    else
        VectorDataType.Float32;

    const op_enum = if (op <= 52)
        @as(VectorOp, @enumFromInt(op))
    else
        VectorOp.Add;

    // Most basic operations are supported across SIMD types
    return switch (op_enum) {
        .Add, .Sub, .Mul, .And, .Or, .Xor, .Not, .Eq, .Ne, .Lt, .Le, .Gt, .Ge, .Min, .Max, .Load, .Store, .Set1 => true,

        // Division is often only supported for floating point
        .Div => switch (data_enum) {
            .Float32, .Float64 => true,
            else => false,
        },

        // Sqrt is typically only for floating point
        .Sqrt => switch (data_enum) {
            .Float32, .Float64 => true,
            else => false,
        },

        // Abs is supported for most types
        .Abs => true,

        // Gather/scatter typically need AVX2 or better
        .Gather, .Scatter => @intFromEnum(type_enum) >= @intFromEnum(SimdType.AVX2),

        // Blend and shuffle are generally supported
        .Blend, .Shuffle => true,
    };
}

// Create a vector mask
pub fn createVectorMask(size: usize, type_val: u8) !*anyopaque {
    const data_type = if (type_val <= @intFromEnum(VectorDataType.Float64))
        @as(VectorDataType, @enumFromInt(type_val))
    else
        VectorDataType.Float32;

    const mask = try VectorMask.init(size, data_type);
    return mask;
}

// Free a vector mask
pub fn freeVectorMask(mask: *anyopaque) void {
    const typed_mask = @as(*VectorMask, @ptrCast(@alignCast(mask)));
    typed_mask.deinit();
}

// Set mask values
pub fn setVectorMask(mask: *anyopaque, indices: []const usize) !void {
    const typed_mask = @as(*VectorMask, @ptrCast(@alignCast(mask)));

    // Clear the mask first
    @memset(typed_mask.data, 0);

    // Set bits for each index
    for (indices) |index| {
        if (index < typed_mask.size) {
            typed_mask.setBit(index, true);
        }
    }
}
