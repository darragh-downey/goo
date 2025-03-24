// scoped_alloc.zig
//
// Implementation of scope-based memory allocation for Goo.
// This file provides allocators that automatically free memory
// when leaving a scope, similar to arena allocators but with
// more fine-grained control.

const std = @import("std");
const Allocator = std.mem.Allocator;
const assert = std.debug.assert;

// Global state
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
var global_allocator: *Allocator = undefined;

/// A scope represents a logical unit of memory allocation that
/// can be freed all at once when the scope ends.
pub const ScopedAllocator = struct {
    parent_allocator: *Allocator,
    allocated_memory: std.ArrayList(*AllocationInfo),
    active: bool,

    const Self = @This();

    /// Information about an individual allocation
    const AllocationInfo = struct {
        ptr: [*]u8,
        len: usize,
        alignment: usize,
    };

    /// Create a new scoped allocator
    pub fn init(parent_allocator: *Allocator) !*Self {
        const self = try parent_allocator.create(Self);
        self.* = Self{
            .parent_allocator = parent_allocator,
            .allocated_memory = std.ArrayList(*AllocationInfo).init(parent_allocator),
            .active = true,
        };
        return self;
    }

    /// Free all memory allocated by this scoped allocator
    pub fn deinit(self: *Self) void {
        self.freeAllAllocations();
        self.allocated_memory.deinit();
        self.parent_allocator.destroy(self);
    }

    /// Create a child scope that inherits from this scope
    pub fn createChildScope(self: *Self) !*Self {
        return init(self.parent_allocator);
    }

    /// Free all allocations made in this scope
    pub fn freeAllAllocations(self: *Self) void {
        for (self.allocated_memory.items) |info| {
            self.parent_allocator.rawFree(info.ptr[0..info.len], info.alignment, @returnAddress());
            self.parent_allocator.destroy(info);
        }
        self.allocated_memory.clearRetainingCapacity();
    }

    /// Enter the scope, making it active
    pub fn enter(self: *Self) void {
        self.active = true;
    }

    /// Exit the scope, optionally freeing all allocations
    pub fn exit(self: *Self, free_all: bool) void {
        self.active = false;
        if (free_all) {
            self.freeAllAllocations();
        }
    }

    /// Implementation of the Allocator interface
    pub fn allocator(self: *Self) Allocator {
        return .{
            .ptr = self,
            .vtable = &.{
                .alloc = alloc,
                .resize = resize,
                .free = free,
            },
        };
    }

    fn alloc(ctx: *anyopaque, len: usize, log2_ptr_align: u8, ret_addr: usize) ?[*]u8 {
        const self: *Self = @ptrCast(@alignCast(ctx));
        if (!self.active) {
            return null;
        }

        const alignment = @as(usize, 1) << @intCast(log2_ptr_align);
        const ptr = self.parent_allocator.rawAlloc(len, alignment, ret_addr) orelse return null;

        // Track this allocation
        const info = self.parent_allocator.create(AllocationInfo) catch {
            self.parent_allocator.rawFree(ptr[0..len], alignment, ret_addr);
            return null;
        };

        info.* = .{
            .ptr = ptr,
            .len = len,
            .alignment = alignment,
        };

        self.allocated_memory.append(info) catch {
            self.parent_allocator.destroy(info);
            self.parent_allocator.rawFree(ptr[0..len], alignment, ret_addr);
            return null;
        };

        return ptr;
    }

    fn resize(ctx: *anyopaque, buf: []u8, log2_buf_align: u8, new_len: usize, ret_addr: usize) bool {
        const self: *Self = @ptrCast(@alignCast(ctx));
        if (!self.active) {
            return false;
        }

        // If growing, make sure we're still active
        if (new_len > buf.len) {
            return self.parent_allocator.rawResize(buf, log2_buf_align, new_len, ret_addr);
        }

        // Find the allocation info
        for (self.allocated_memory.items) |info| {
            if (info.ptr.ptr == buf.ptr) {
                if (self.parent_allocator.rawResize(buf, log2_buf_align, new_len, ret_addr)) {
                    info.len = new_len;
                    return true;
                }
                return false;
            }
        }

        // Not found, probably allocated by parent
        return self.parent_allocator.rawResize(buf, log2_buf_align, new_len, ret_addr);
    }

    fn free(ctx: *anyopaque, buf: []u8, log2_buf_align: u8, ret_addr: usize) void {
        const self: *Self = @ptrCast(@alignCast(ctx));

        // Try to find this allocation in our tracking list
        for (self.allocated_memory.items, 0..) |info, i| {
            if (info.ptr.ptr == buf.ptr) {
                // Remove from our tracking list
                _ = self.allocated_memory.swapRemove(i);
                // Free the memory and the allocation info
                self.parent_allocator.rawFree(buf, log2_buf_align, ret_addr);
                self.parent_allocator.destroy(info);
                return;
            }
        }

        // Not found in our list, pass to parent allocator
        self.parent_allocator.rawFree(buf, log2_buf_align, ret_addr);
    }
};

/// A collection of nested scopes for more complex memory management
pub const ScopeStack = struct {
    allocator: *Allocator,
    scopes: std.ArrayList(*ScopedAllocator),
    current_scope: ?*ScopedAllocator,

    const Self = @This();

    /// Initialize a new scope stack
    pub fn init(allocator: *Allocator) !*Self {
        const self = try allocator.create(Self);
        self.* = Self{
            .allocator = allocator,
            .scopes = std.ArrayList(*ScopedAllocator).init(allocator),
            .current_scope = null,
        };
        return self;
    }

    /// Clean up all scopes and the stack itself
    pub fn deinit(self: *Self) void {
        for (self.scopes.items) |scope| {
            scope.deinit();
        }
        self.scopes.deinit();
        self.allocator.destroy(self);
    }

    /// Push a new scope onto the stack
    pub fn pushScope(self: *Self) !*ScopedAllocator {
        const scope = try ScopedAllocator.init(self.allocator);
        try self.scopes.append(scope);
        self.current_scope = scope;
        return scope;
    }

    /// Pop the current scope from the stack, optionally freeing all allocations
    pub fn popScope(self: *Self, free_all: bool) !void {
        if (self.scopes.items.len == 0) {
            return error.NoScopeAvailable;
        }

        const last_idx = self.scopes.items.len - 1;
        const scope = self.scopes.items[last_idx];
        _ = self.scopes.pop();

        if (free_all) {
            scope.freeAllAllocations();
        }
        scope.deinit();

        self.current_scope = if (self.scopes.items.len > 0)
            self.scopes.items[self.scopes.items.len - 1]
        else
            null;
    }

    /// Get the current active scope
    pub fn getCurrentScope(self: *Self) ?*ScopedAllocator {
        return self.current_scope;
    }

    /// Get an allocator for the current scope
    pub fn scopedAllocator(self: *Self) !Allocator {
        if (self.current_scope) |scope| {
            return scope.allocator();
        }
        return error.NoScopeAvailable;
    }

    /// Free all allocations in all scopes
    pub fn freeAllAllocations(self: *Self) void {
        for (self.scopes.items) |scope| {
            scope.freeAllAllocations();
        }
    }
};

/// A defer-based scope guard that automatically frees memory when it goes out of scope
pub const ScopeGuard = struct {
    scope: *ScopedAllocator,
    free_on_exit: bool,

    const Self = @This();

    /// Create a new scope guard that will automatically free memory when it goes out of scope
    pub fn init(parent_allocator: *Allocator, free_on_exit: bool) !Self {
        const scope = try ScopedAllocator.init(parent_allocator);
        return Self{
            .scope = scope,
            .free_on_exit = free_on_exit,
        };
    }

    /// Get an allocator for this scope
    pub fn allocator(self: *Self) Allocator {
        return self.scope.allocator();
    }

    /// Free all allocations in this scope
    pub fn freeAllAllocations(self: *Self) void {
        self.scope.freeAllAllocations();
    }

    /// Manually release the guard without freeing memory
    pub fn release(self: *Self) void {
        self.free_on_exit = false;
    }

    /// Destructor called when the guard goes out of scope
    pub fn deinit(self: *Self) void {
        if (self.free_on_exit) {
            self.scope.freeAllAllocations();
        }
        self.scope.deinit();
    }
};

// Exported C API functions

pub export fn scopedAllocInit() bool {
    global_allocator = &gpa.allocator;
    return true;
}

export fn scopedAllocCleanup() void {
    _ = gpa.deinit();
}

export fn scopedAllocCreate() ?*ScopedAllocator {
    return ScopedAllocator.init(global_allocator) catch null;
}

export fn scopedAllocDestroy(scope: *ScopedAllocator) void {
    scope.deinit();
}

export fn scopedAllocEnter(scope: *ScopedAllocator) void {
    scope.enter();
}

export fn scopedAllocExit(scope: *ScopedAllocator, free_all: bool) void {
    scope.exit(free_all);
}

export fn scopedAllocFreeAll(scope: *ScopedAllocator) void {
    scope.freeAllAllocations();
}

export fn scopedAllocMalloc(scope: *ScopedAllocator, size: usize) ?*anyopaque {
    const allocator = scope.allocator();
    const memory = allocator.alloc(u8, size) catch return null;
    return memory.ptr;
}

export fn scopedAllocFree(scope: *ScopedAllocator, ptr: *anyopaque, size: usize) void {
    const allocator = scope.allocator();
    const memory = @as([*]u8, @ptrCast(ptr))[0..size];
    allocator.free(memory);
}

export fn scopedAllocRealloc(scope: *ScopedAllocator, ptr: ?*anyopaque, old_size: usize, new_size: usize) ?*anyopaque {
    const allocator = scope.allocator();

    if (ptr) |p| {
        const old_memory = @as([*]u8, @ptrCast(p))[0..old_size];
        const new_memory = allocator.realloc(old_memory, new_size) catch return null;
        return new_memory.ptr;
    } else {
        const new_memory = allocator.alloc(u8, new_size) catch return null;
        return new_memory.ptr;
    }
}

export fn scopeStackCreate() ?*ScopeStack {
    return ScopeStack.init(global_allocator) catch null;
}

export fn scopeStackDestroy(stack: *ScopeStack) void {
    stack.deinit();
}

export fn scopeStackPush(stack: *ScopeStack) ?*ScopedAllocator {
    return stack.pushScope() catch null;
}

export fn scopeStackPop(stack: *ScopeStack, free_all: bool) bool {
    return stack.popScope(free_all) catch return false;
}

export fn scopeStackGetCurrent(stack: *ScopeStack) ?*ScopedAllocator {
    return stack.getCurrentScope();
}

export fn scopeStackFreeAll(stack: *ScopeStack) void {
    stack.freeAllAllocations();
}
