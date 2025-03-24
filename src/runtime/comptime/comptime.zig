// comptime.zig
//
// Implementation of Goo's compile-time evaluation features.
// This module provides functionality for compile-time code generation,
// evaluation, and reflection.

const std = @import("std");
const Allocator = std.mem.Allocator;
const mem = std.mem;

// Global state
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
var global_allocator: *Allocator = undefined;

// Types of compile-time values
pub const ComptimeValueType = enum {
    Int,
    Float,
    Bool,
    String,
    Type,
    Struct,
    Array,
    Null,
};

// Representation of a compile-time value
pub const ComptimeValue = struct {
    type_tag: ComptimeValueType,
    data: union {
        int: i64,
        float: f64,
        bool: bool,
        string: []const u8,
        type_info: *TypeInfo,
        struct_value: *StructValue,
        array_value: *ArrayValue,
        null_value: void,
    },

    // Create an integer value
    pub fn initInt(allocator: *Allocator, value: i64) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Int,
            .data = .{ .int = value },
        };
        return result;
    }

    // Create a float value
    pub fn initFloat(allocator: *Allocator, value: f64) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Float,
            .data = .{ .float = value },
        };
        return result;
    }

    // Create a boolean value
    pub fn initBool(allocator: *Allocator, value: bool) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Bool,
            .data = .{ .bool = value },
        };
        return result;
    }

    // Create a string value
    pub fn initString(allocator: *Allocator, value: []const u8) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        const string_copy = try allocator.dupe(u8, value);
        result.* = ComptimeValue{
            .type_tag = .String,
            .data = .{ .string = string_copy },
        };
        return result;
    }

    // Create a type value
    pub fn initType(allocator: *Allocator, type_info: *TypeInfo) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Type,
            .data = .{ .type_info = type_info },
        };
        return result;
    }

    // Create a struct value
    pub fn initStruct(allocator: *Allocator, struct_value: *StructValue) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Struct,
            .data = .{ .struct_value = struct_value },
        };
        return result;
    }

    // Create an array value
    pub fn initArray(allocator: *Allocator, array_value: *ArrayValue) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Array,
            .data = .{ .array_value = array_value },
        };
        return result;
    }

    // Create a null value
    pub fn initNull(allocator: *Allocator) !*ComptimeValue {
        const result = try allocator.create(ComptimeValue);
        result.* = ComptimeValue{
            .type_tag = .Null,
            .data = .{ .null_value = {} },
        };
        return result;
    }

    // Destroy a comptime value and free its resources
    pub fn deinit(self: *ComptimeValue, allocator: *Allocator) void {
        switch (self.type_tag) {
            .String => allocator.free(self.data.string),
            .Type => self.data.type_info.deinit(allocator),
            .Struct => self.data.struct_value.deinit(allocator),
            .Array => self.data.array_value.deinit(allocator),
            else => {}, // No additional resources to free
        }
        allocator.destroy(self);
    }

    // Convert to string representation
    pub fn toString(self: *ComptimeValue, allocator: *Allocator) ![]const u8 {
        switch (self.type_tag) {
            .Int => {
                var buf: [64]u8 = undefined;
                const str = try std.fmt.bufPrint(&buf, "{d}", .{self.data.int});
                return allocator.dupe(u8, str);
            },
            .Float => {
                var buf: [64]u8 = undefined;
                const str = try std.fmt.bufPrint(&buf, "{d}", .{self.data.float});
                return allocator.dupe(u8, str);
            },
            .Bool => {
                return allocator.dupe(u8, if (self.data.bool) "true" else "false");
            },
            .String => {
                return allocator.dupe(u8, self.data.string);
            },
            .Type => {
                return self.data.type_info.toString(allocator);
            },
            .Struct => {
                return self.data.struct_value.toString(allocator);
            },
            .Array => {
                return self.data.array_value.toString(allocator);
            },
            .Null => {
                return allocator.dupe(u8, "null");
            },
        }
    }

    // Evaluate a binary operation between two comptime values
    pub fn evalBinaryOp(
        allocator: *Allocator, 
        op: BinaryOp, 
        left: *ComptimeValue, 
        right: *ComptimeValue
    ) !*ComptimeValue {
        switch (op) {
            .Add => {
                if (left.type_tag == .Int and right.type_tag == .Int) {
                    return try ComptimeValue.initInt(allocator, left.data.int + right.data.int);
                } else if (left.type_tag == .Float and right.type_tag == .Float) {
                    return try ComptimeValue.initFloat(allocator, left.data.float + right.data.float);
                } else if (left.type_tag == .String and right.type_tag == .String) {
                    const result_len = left.data.string.len + right.data.string.len;
                    const result = try allocator.alloc(u8, result_len);
                    std.mem.copy(u8, result[0..left.data.string.len], left.data.string);
                    std.mem.copy(u8, result[left.data.string.len..], right.data.string);
                    
                    const value = try ComptimeValue.initString(allocator, result);
                    allocator.free(result);
                    return value;
                } else {
                    return error.TypeMismatch;
                }
            },
            .Subtract => {
                if (left.type_tag == .Int and right.type_tag == .Int) {
                    return try ComptimeValue.initInt(allocator, left.data.int - right.data.int);
                } else if (left.type_tag == .Float and right.type_tag == .Float) {
                    return try ComptimeValue.initFloat(allocator, left.data.float - right.data.float);
                } else {
                    return error.TypeMismatch;
                }
            },
            .Multiply => {
                if (left.type_tag == .Int and right.type_tag == .Int) {
                    return try ComptimeValue.initInt(allocator, left.data.int * right.data.int);
                } else if (left.type_tag == .Float and right.type_tag == .Float) {
                    return try ComptimeValue.initFloat(allocator, left.data.float * right.data.float);
                } else {
                    return error.TypeMismatch;
                }
            },
            .Divide => {
                if (left.type_tag == .Int and right.type_tag == .Int) {
                    if (right.data.int == 0) return error.DivisionByZero;
                    return try ComptimeValue.initInt(allocator, @divTrunc(left.data.int, right.data.int));
                } else if (left.type_tag == .Float and right.type_tag == .Float) {
                    if (right.data.float == 0.0) return error.DivisionByZero;
                    return try ComptimeValue.initFloat(allocator, left.data.float / right.data.float);
                } else {
                    return error.TypeMismatch;
                }
            },
            // Add more operations as needed
        }
    }
};

// Type information representation
pub const TypeInfo = struct {
    name: []const u8,
    kind: TypeKind,
    data: union {
        struct_info: *StructTypeInfo,
        array_info: *ArrayTypeInfo,
        basic: void,
    },

    pub fn deinit(self: *TypeInfo, allocator: *Allocator) void {
        allocator.free(self.name);
        switch (self.kind) {
            .Struct => self.data.struct_info.deinit(allocator),
            .Array => self.data.array_info.deinit(allocator),
            else => {}, // No additional resources to free
        }
        allocator.destroy(self);
    }

    pub fn toString(self: *TypeInfo, allocator: *Allocator) ![]const u8 {
        return allocator.dupe(u8, self.name);
    }
};

// Types of TypeInfo
pub const TypeKind = enum {
    Int,
    Float,
    Bool,
    String,
    Struct,
    Array,
};

// Struct type information
pub const StructTypeInfo = struct {
    fields: std.StringHashMap(*TypeInfo),

    pub fn deinit(self: *StructTypeInfo, allocator: *Allocator) void {
        var it = self.fields.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.*.deinit(allocator);
        }
        self.fields.deinit();
        allocator.destroy(self);
    }
};

// Array type information
pub const ArrayTypeInfo = struct {
    element_type: *TypeInfo,
    length: ?usize, // null for slices

    pub fn deinit(self: *ArrayTypeInfo, allocator: *Allocator) void {
        self.element_type.deinit(allocator);
        allocator.destroy(self);
    }
};

// Struct value representation
pub const StructValue = struct {
    type_info: *TypeInfo,
    fields: std.StringHashMap(*ComptimeValue),

    pub fn deinit(self: *StructValue, allocator: *Allocator) void {
        var it = self.fields.iterator();
        while (it.next()) |entry| {
            allocator.free(entry.key_ptr.*);
            entry.value_ptr.*.deinit(allocator);
        }
        self.fields.deinit();
        self.type_info.deinit(allocator);
        allocator.destroy(self);
    }

    pub fn toString(self: *StructValue, allocator: *Allocator) ![]const u8 {
        var result = std.ArrayList(u8).init(allocator);
        defer result.deinit();

        try result.appendSlice("{");
        var it = self.fields.iterator();
        var first = true;
        while (it.next()) |entry| {
            if (!first) {
                try result.appendSlice(", ");
            }
            first = false;
            try result.appendSlice(entry.key_ptr.*);
            try result.appendSlice(": ");
            const value_str = try entry.value_ptr.*.toString(allocator);
            defer allocator.free(value_str);
            try result.appendSlice(value_str);
        }
        try result.appendSlice("}");
        return result.toOwnedSlice();
    }
};

// Array value representation
pub const ArrayValue = struct {
    element_type: *TypeInfo,
    elements: std.ArrayList(*ComptimeValue),

    pub fn deinit(self: *ArrayValue, allocator: *Allocator) void {
        for (self.elements.items) |element| {
            element.deinit(allocator);
        }
        self.elements.deinit();
        self.element_type.deinit(allocator);
        allocator.destroy(self);
    }

    pub fn toString(self: *ArrayValue, allocator: *Allocator) ![]const u8 {
        var result = std.ArrayList(u8).init(allocator);
        defer result.deinit();

        try result.appendSlice("[");
        for (self.elements.items) |element, i| {
            if (i > 0) {
                try result.appendSlice(", ");
            }
            const value_str = try element.toString(allocator);
            defer allocator.free(value_str);
            try result.appendSlice(value_str);
        }
        try result.appendSlice("]");
        return result.toOwnedSlice();
    }
};

// Binary operations enum
pub const BinaryOp = enum {
    Add,
    Subtract,
    Multiply,
    Divide,
    // Add more operations as needed
};

// ComptimeExpression represents an expression that can be evaluated at compile time
pub const ComptimeExpression = struct {
    kind: ExpressionKind,
    data: union {
        literal: *ComptimeValue,
        binary_op: BinaryOpExpression,
        field_access: FieldAccessExpression,
        function_call: FunctionCallExpression,
    },

    pub fn eval(self: *ComptimeExpression, allocator: *Allocator) !*ComptimeValue {
        switch (self.kind) {
            .Literal => {
                return self.data.literal;
            },
            .BinaryOp => {
                const left = try self.data.binary_op.left.eval(allocator);
                const right = try self.data.binary_op.right.eval(allocator);
                return try ComptimeValue.evalBinaryOp(
                    allocator,
                    self.data.binary_op.op,
                    left,
                    right
                );
            },
            .FieldAccess => {
                const object = try self.data.field_access.object.eval(allocator);
                if (object.type_tag != .Struct) {
                    return error.NotAStruct;
                }
                const field_name = self.data.field_access.field_name;
                const field = object.data.struct_value.fields.get(field_name) orelse {
                    return error.FieldNotFound;
                };
                return field;
            },
            .FunctionCall => {
                // Evaluate function arguments
                var args = std.ArrayList(*ComptimeValue).init(allocator);
                defer args.deinit();
                
                for (self.data.function_call.args.items) |arg| {
                    const arg_value = try arg.eval(allocator);
                    try args.append(arg_value);
                }
                
                // Call the function with the evaluated arguments
                return try self.data.function_call.func.call(allocator, args.items);
            },
        }
    }

    pub fn deinit(self: *ComptimeExpression, allocator: *Allocator) void {
        switch (self.kind) {
            .Literal => {
                self.data.literal.deinit(allocator);
            },
            .BinaryOp => {
                self.data.binary_op.left.deinit(allocator);
                self.data.binary_op.right.deinit(allocator);
            },
            .FieldAccess => {
                self.data.field_access.object.deinit(allocator);
                allocator.free(self.data.field_access.field_name);
            },
            .FunctionCall => {
                for (self.data.function_call.args.items) |arg| {
                    arg.deinit(allocator);
                }
                self.data.function_call.args.deinit();
                self.data.function_call.func.deinit(allocator);
            },
        }
        allocator.destroy(self);
    }
};

// Types of expressions
pub const ExpressionKind = enum {
    Literal,
    BinaryOp,
    FieldAccess,
    FunctionCall,
};

// Binary operation expression
pub const BinaryOpExpression = struct {
    op: BinaryOp,
    left: *ComptimeExpression,
    right: *ComptimeExpression,
};

// Field access expression
pub const FieldAccessExpression = struct {
    object: *ComptimeExpression,
    field_name: []const u8,
};

// Function call expression
pub const FunctionCallExpression = struct {
    func: *ComptimeFunction,
    args: std.ArrayList(*ComptimeExpression),
};

// Function that can be called at compile time
pub const ComptimeFunction = struct {
    name: []const u8,
    param_types: std.ArrayList(*TypeInfo),
    return_type: *TypeInfo,
    implementation: fn (allocator: *Allocator, args: []const *ComptimeValue) anyerror!*ComptimeValue,

    pub fn call(self: *ComptimeFunction, allocator: *Allocator, args: []const *ComptimeValue) !*ComptimeValue {
        // Check argument count
        if (args.len != self.param_types.items.len) {
            return error.InvalidArgumentCount;
        }
        
        // Check argument types
        // (This is a simplified version; a real implementation would need more sophisticated type checking)
        
        // Call the implementation
        return self.implementation(allocator, args);
    }

    pub fn deinit(self: *ComptimeFunction, allocator: *Allocator) void {
        allocator.free(self.name);
        for (self.param_types.items) |param_type| {
            param_type.deinit(allocator);
        }
        self.param_types.deinit();
        self.return_type.deinit(allocator);
        allocator.destroy(self);
    }
};

// ComptimeContext stores the environment for compile-time evaluation
pub const ComptimeContext = struct {
    allocator: *Allocator,
    types: std.StringHashMap(*TypeInfo),
    values: std.StringHashMap(*ComptimeValue),
    functions: std.StringHashMap(*ComptimeFunction),

    pub fn init(allocator: *Allocator) !*ComptimeContext {
        const ctx = try allocator.create(ComptimeContext);
        ctx.* = ComptimeContext{
            .allocator = allocator,
            .types = std.StringHashMap(*TypeInfo).init(allocator),
            .values = std.StringHashMap(*ComptimeValue).init(allocator),
            .functions = std.StringHashMap(*ComptimeFunction).init(allocator),
        };
        try ctx.registerBuiltins();
        return ctx;
    }

    pub fn deinit(self: *ComptimeContext) void {
        // Free all registered types
        var types_it = self.types.iterator();
        while (types_it.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
            entry.value_ptr.*.deinit(self.allocator);
        }
        self.types.deinit();

        // Free all registered values
        var values_it = self.values.iterator();
        while (values_it.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
            entry.value_ptr.*.deinit(self.allocator);
        }
        self.values.deinit();

        // Free all registered functions
        var functions_it = self.functions.iterator();
        while (functions_it.next()) |entry| {
            self.allocator.free(entry.key_ptr.*);
            entry.value_ptr.*.deinit(self.allocator);
        }
        self.functions.deinit();

        self.allocator.destroy(self);
    }

    pub fn registerType(self: *ComptimeContext, name: []const u8, type_info: *TypeInfo) !void {
        const name_copy = try self.allocator.dupe(u8, name);
        errdefer self.allocator.free(name_copy);
        
        try self.types.put(name_copy, type_info);
    }

    pub fn registerValue(self: *ComptimeContext, name: []const u8, value: *ComptimeValue) !void {
        const name_copy = try self.allocator.dupe(u8, name);
        errdefer self.allocator.free(name_copy);
        
        try self.values.put(name_copy, value);
    }

    pub fn registerFunction(self: *ComptimeContext, name: []const u8, func: *ComptimeFunction) !void {
        const name_copy = try self.allocator.dupe(u8, name);
        errdefer self.allocator.free(name_copy);
        
        try self.functions.put(name_copy, func);
    }

    fn registerBuiltins(self: *ComptimeContext) !void {
        // Register basic types
        {
            const int_type = try self.allocator.create(TypeInfo);
            int_type.* = TypeInfo{
                .name = try self.allocator.dupe(u8, "int"),
                .kind = .Int,
                .data = .{ .basic = {} },
            };
            try self.registerType("int", int_type);
            
            const float_type = try self.allocator.create(TypeInfo);
            float_type.* = TypeInfo{
                .name = try self.allocator.dupe(u8, "float"),
                .kind = .Float,
                .data = .{ .basic = {} },
            };
            try self.registerType("float", float_type);
            
            const bool_type = try self.allocator.create(TypeInfo);
            bool_type.* = TypeInfo{
                .name = try self.allocator.dupe(u8, "bool"),
                .kind = .Bool,
                .data = .{ .basic = {} },
            };
            try self.registerType("bool", bool_type);
            
            const string_type = try self.allocator.create(TypeInfo);
            string_type.* = TypeInfo{
                .name = try self.allocator.dupe(u8, "string"),
                .kind = .String,
                .data = .{ .basic = {} },
            };
            try self.registerType("string", string_type);
        }
        
        // Register builtin functions
        
        // Add any other builtin registrations here
    }

    // Evaluate a compile-time expression in this context
    pub fn eval(self: *ComptimeContext, expr: *ComptimeExpression) !*ComptimeValue {
        return expr.eval(self.allocator);
    }
};

// Exported C API functions

export fn comptimeInit() bool {
    global_allocator = &gpa.allocator;
    return true;
}

export fn comptimeCleanup() void {
    _ = gpa.deinit();
}

export fn comptimeContextCreate() ?*ComptimeContext {
    return ComptimeContext.init(global_allocator) catch null;
}

export fn comptimeContextDestroy(ctx: *ComptimeContext) void {
    ctx.deinit();
}

export fn comptimeCreateIntValue(ctx: *ComptimeContext, value: i64) ?*ComptimeValue {
    return ComptimeValue.initInt(ctx.allocator, value) catch null;
}

export fn comptimeCreateFloatValue(ctx: *ComptimeContext, value: f64) ?*ComptimeValue {
    return ComptimeValue.initFloat(ctx.allocator, value) catch null;
}

export fn comptimeCreateBoolValue(ctx: *ComptimeContext, value: bool) ?*ComptimeValue {
    return ComptimeValue.initBool(ctx.allocator, value) catch null;
}

export fn comptimeCreateStringValue(ctx: *ComptimeContext, value: [*:0]const u8) ?*ComptimeValue {
    const slice = std.mem.span(value);
    return ComptimeValue.initString(ctx.allocator, slice) catch null;
}

export fn comptimeDestroyValue(ctx: *ComptimeContext, value: *ComptimeValue) void {
    value.deinit(ctx.allocator);
}

export fn comptimeValueToString(ctx: *ComptimeContext, value: *ComptimeValue, out_len: *usize) ?[*]const u8 {
    const result = value.toString(ctx.allocator) catch return null;
    out_len.* = result.len;
    return result.ptr;
}

export fn comptimeFreeString(ctx: *ComptimeContext, str: [*]const u8, len: usize) void {
    const slice = str[0..len];
    ctx.allocator.free(slice);
}

export fn comptimeEvalBinaryAdd(ctx: *ComptimeContext, left: *ComptimeValue, right: *ComptimeValue) ?*ComptimeValue {
    return ComptimeValue.evalBinaryOp(ctx.allocator, .Add, left, right) catch null;
}

export fn comptimeEvalBinarySub(ctx: *ComptimeContext, left: *ComptimeValue, right: *ComptimeValue) ?*ComptimeValue {
    return ComptimeValue.evalBinaryOp(ctx.allocator, .Subtract, left, right) catch null;
}

export fn comptimeEvalBinaryMul(ctx: *ComptimeContext, left: *ComptimeValue, right: *ComptimeValue) ?*ComptimeValue {
    return ComptimeValue.evalBinaryOp(ctx.allocator, .Multiply, left, right) catch null;
}

export fn comptimeEvalBinaryDiv(ctx: *ComptimeContext, left: *ComptimeValue, right: *ComptimeValue) ?*ComptimeValue {
    return ComptimeValue.evalBinaryOp(ctx.allocator, .Divide, left, right) catch null;
} 