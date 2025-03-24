// reflection.zig
//
// Implementation of meta-programming and reflection capabilities for Goo.
// This file provides tools for inspecting and manipulating types and values
// at compile time and runtime.

const std = @import("std");
const Allocator = std.mem.Allocator;
const assert = std.debug.assert;

// Global state
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
var global_allocator: *Allocator = undefined;

/// Possible types that can be reflected
pub const TypeKind = enum {
    Void,
    Bool,
    Int,
    Float,
    Pointer,
    Array,
    Slice,
    Struct,
    Enum,
    Union,
    Function,
    Optional,
    ErrorUnion,
    ErrorSet,
    Vector,
    Opaque,
    // Goo-specific types
    Any,
    Dynamic,
    Interface,
    Null,
    Undefined,
};

/// Information about a reflected type
pub const TypeInfo = struct {
    kind: TypeKind,
    name: []const u8,
    size: usize,
    alignment: usize,
    data: TypeData,

    // Type-specific data
    pub const TypeData = union {
        Void: void,
        Bool: void,
        Int: IntInfo,
        Float: FloatInfo,
        Pointer: PointerInfo,
        Array: ArrayInfo,
        Slice: SliceInfo,
        Struct: StructInfo,
        Enum: EnumInfo,
        Union: UnionInfo,
        Function: FunctionInfo,
        Optional: OptionalInfo,
        ErrorUnion: ErrorUnionInfo,
        ErrorSet: ErrorSetInfo,
        Vector: VectorInfo,
        Opaque: void,
        // Goo-specific types
        Any: void,
        Dynamic: void,
        Interface: InterfaceInfo,
        Null: void,
        Undefined: void,
    };

    // Information about integer types
    pub const IntInfo = struct {
        bits: u16,
        is_signed: bool,
    };

    // Information about floating-point types
    pub const FloatInfo = struct {
        bits: u16,
    };

    // Information about pointer types
    pub const PointerInfo = struct {
        size: enum {
            One,
            Many,
            Slice,
            C,
        },
        is_const: bool,
        is_volatile: bool,
        is_allowzero: bool,
        child: *TypeInfo,
        sentinel: ?*Value,
        alignment: u16,
    };

    // Information about array types
    pub const ArrayInfo = struct {
        len: usize,
        child: *TypeInfo,
        sentinel: ?*Value,
    };

    // Information about slice types
    pub const SliceInfo = struct {
        is_const: bool,
        child: *TypeInfo,
        sentinel: ?*Value,
    };

    // Information about struct types
    pub const StructInfo = struct {
        fields: []FieldInfo,
        decls: []DeclInfo,
        is_tuple: bool,
        layout: StructLayout,

        pub const StructLayout = enum {
            Auto,
            Extern,
            Packed,
        };
    };

    // Information about enum types
    pub const EnumInfo = struct {
        fields: []FieldInfo,
        decls: []DeclInfo,
        tag_type: *TypeInfo,
        is_exhaustive: bool,
    };

    // Information about union types
    pub const UnionInfo = struct {
        fields: []FieldInfo,
        decls: []DeclInfo,
        tag_type: ?*TypeInfo,
        layout: UnionLayout,

        pub const UnionLayout = enum {
            Auto,
            Extern,
            Tagged,
        };
    };

    // Information about function types
    pub const FunctionInfo = struct {
        params: []ParamInfo,
        return_type: *TypeInfo,
        is_var_args: bool,
        calling_convention: CallingConvention,

        pub const CallingConvention = enum {
            Unspecified,
            C,
            Stdcall,
            Fastcall,
            Vectorcall,
            Thiscall,
            Async,
            APCS,
            AAPCS,
            AAPCSVFP,
            Interrupt,
            Signal,
            Naked,
        };
    };

    // Information about optional types
    pub const OptionalInfo = struct {
        child: *TypeInfo,
    };

    // Information about error union types
    pub const ErrorUnionInfo = struct {
        error_set: *TypeInfo,
        payload: *TypeInfo,
    };

    // Information about error set types
    pub const ErrorSetInfo = struct {
        errors: [][]const u8,
    };

    // Information about vector types
    pub const VectorInfo = struct {
        len: u32,
        child: *TypeInfo,
    };

    // Information about interface types (Goo-specific)
    pub const InterfaceInfo = struct {
        methods: []MethodInfo,
        is_marker: bool,
    };

    // Information about fields in structs, unions, and enums
    pub const FieldInfo = struct {
        name: []const u8,
        type: *TypeInfo,
        default_value: ?*Value,
        is_comptime: bool,
        alignment: ?u16,
    };

    // Information about methods
    pub const MethodInfo = struct {
        name: []const u8,
        type: *TypeInfo,
        is_static: bool,
    };

    // Information about parameters in functions
    pub const ParamInfo = struct {
        name: ?[]const u8,
        type: *TypeInfo,
        is_comptime: bool,
        is_noalias: bool,
    };

    // Information about declarations
    pub const DeclInfo = struct {
        name: []const u8,
        is_pub: bool,
        is_exported: bool,
        is_comptime: bool,
    };

    // Free all memory associated with the type info
    pub fn deinit(self: *TypeInfo, allocator: *Allocator) void {
        switch (self.kind) {
            .Pointer => |ptr| {
                ptr.child.deinit(allocator);
                if (ptr.sentinel) |sentinel| {
                    sentinel.deinit(allocator);
                    allocator.destroy(sentinel);
                }
                allocator.free(ptr.child);
            },
            .Array => |arr| {
                arr.child.deinit(allocator);
                if (arr.sentinel) |sentinel| {
                    sentinel.deinit(allocator);
                    allocator.destroy(sentinel);
                }
                allocator.free(arr.child);
            },
            .Slice => |slice| {
                slice.child.deinit(allocator);
                if (slice.sentinel) |sentinel| {
                    sentinel.deinit(allocator);
                    allocator.destroy(sentinel);
                }
                allocator.free(slice.child);
            },
            .Struct => |strct| {
                for (strct.fields) |field| {
                    field.type.deinit(allocator);
                    if (field.default_value) |default_value| {
                        default_value.deinit(allocator);
                        allocator.destroy(default_value);
                    }
                    allocator.free(field.type);
                }
                allocator.free(strct.fields);
                allocator.free(strct.decls);
            },
            .Enum => |enm| {
                enm.tag_type.deinit(allocator);
                for (enm.fields) |field| {
                    field.type.deinit(allocator);
                    if (field.default_value) |default_value| {
                        default_value.deinit(allocator);
                        allocator.destroy(default_value);
                    }
                    allocator.free(field.type);
                }
                allocator.free(enm.fields);
                allocator.free(enm.decls);
                allocator.free(enm.tag_type);
            },
            .Union => |unn| {
                if (unn.tag_type) |tag_type| {
                    tag_type.deinit(allocator);
                    allocator.free(tag_type);
                }
                for (unn.fields) |field| {
                    field.type.deinit(allocator);
                    if (field.default_value) |default_value| {
                        default_value.deinit(allocator);
                        allocator.destroy(default_value);
                    }
                    allocator.free(field.type);
                }
                allocator.free(unn.fields);
                allocator.free(unn.decls);
            },
            .Function => |func| {
                for (func.params) |param| {
                    param.type.deinit(allocator);
                    allocator.free(param.type);
                }
                func.return_type.deinit(allocator);
                allocator.free(func.params);
                allocator.free(func.return_type);
            },
            .Optional => |opt| {
                opt.child.deinit(allocator);
                allocator.free(opt.child);
            },
            .ErrorUnion => |err_union| {
                err_union.error_set.deinit(allocator);
                err_union.payload.deinit(allocator);
                allocator.free(err_union.error_set);
                allocator.free(err_union.payload);
            },
            .ErrorSet => |err_set| {
                for (err_set.errors) |err| {
                    allocator.free(err);
                }
                allocator.free(err_set.errors);
            },
            .Vector => |vec| {
                vec.child.deinit(allocator);
                allocator.free(vec.child);
            },
            .Interface => |intf| {
                for (intf.methods) |method| {
                    method.type.deinit(allocator);
                    allocator.free(method.type);
                }
                allocator.free(intf.methods);
            },
            else => {},
        }
        allocator.free(self.name);
    }
};

/// Possible value types
pub const ValueKind = enum {
    Void,
    Bool,
    Int,
    Float,
    Pointer,
    Array,
    Slice,
    Struct,
    Enum,
    Union,
    Function,
    Optional,
    Error,
    ErrorUnion,
    Vector,
    // Goo-specific values
    Any,
    Dynamic,
    Null,
    Undefined,
};

/// A value that can be inspected and manipulated at runtime
pub const Value = struct {
    type_info: *TypeInfo,
    kind: ValueKind,
    data: ValueData,

    // Value-specific data
    pub const ValueData = union {
        Void: void,
        Bool: bool,
        Int: IntValue,
        Float: FloatValue,
        Pointer: PointerValue,
        Array: ArrayValue,
        Slice: SliceValue,
        Struct: StructValue,
        Enum: EnumValue,
        Union: UnionValue,
        Function: FunctionValue,
        Optional: OptionalValue,
        Error: ErrorValue,
        ErrorUnion: ErrorUnionValue,
        Vector: VectorValue,
        // Goo-specific values
        Any: AnyValue,
        Dynamic: DynamicValue,
        Null: void,
        Undefined: void,
    };

    // Integer value
    pub const IntValue = union {
        U8: u8,
        U16: u16,
        U32: u32,
        U64: u64,
        U128: u128,
        I8: i8,
        I16: i16,
        I32: i32,
        I64: i64,
        I128: i128,
    };

    // Floating-point value
    pub const FloatValue = union {
        F16: f16,
        F32: f32,
        F64: f64,
        F128: f128,
    };

    // Pointer value
    pub const PointerValue = struct {
        address: usize,
        pointee: ?*Value,
    };

    // Array value
    pub const ArrayValue = struct {
        elements: []*Value,
    };

    // Slice value
    pub const SliceValue = struct {
        ptr: usize,
        len: usize,
        elements: ?[]*Value,
    };

    // Struct value
    pub const StructValue = struct {
        fields: []FieldValue,

        pub const FieldValue = struct {
            name: []const u8,
            value: *Value,
        };
    };

    // Enum value
    pub const EnumValue = struct {
        tag: IntValue,
        name: []const u8,
    };

    // Union value
    pub const UnionValue = struct {
        tag: ?IntValue,
        field_index: usize,
        field_name: []const u8,
        field_value: *Value,
    };

    // Function value
    pub const FunctionValue = struct {
        address: usize,
        name: ?[]const u8,
    };

    // Optional value
    pub const OptionalValue = struct {
        has_value: bool,
        value: ?*Value,
    };

    // Error value
    pub const ErrorValue = struct {
        name: []const u8,
        value: IntValue,
    };

    // Error union value
    pub const ErrorUnionValue = struct {
        is_error: bool,
        error_value: ?ErrorValue,
        payload: ?*Value,
    };

    // Vector value
    pub const VectorValue = struct {
        elements: []*Value,
    };

    // Any value (Goo-specific)
    pub const AnyValue = struct {
        type_id: usize,
        ptr: usize,
    };

    // Dynamic value (Goo-specific)
    pub const DynamicValue = struct {
        inner: *Value,
    };

    // Free all memory associated with the value
    pub fn deinit(self: *Value, allocator: *Allocator) void {
        switch (self.kind) {
            .Pointer => |ptr| {
                if (ptr.pointee) |pointee| {
                    pointee.deinit(allocator);
                    allocator.destroy(pointee);
                }
            },
            .Array => |arr| {
                for (arr.elements) |element| {
                    element.deinit(allocator);
                    allocator.destroy(element);
                }
                allocator.free(arr.elements);
            },
            .Slice => |slice| {
                if (slice.elements) |elements| {
                    for (elements) |element| {
                        element.deinit(allocator);
                        allocator.destroy(element);
                    }
                    allocator.free(elements);
                }
            },
            .Struct => |strct| {
                for (strct.fields) |field| {
                    field.value.deinit(allocator);
                    allocator.destroy(field.value);
                    allocator.free(field.name);
                }
                allocator.free(strct.fields);
            },
            .Union => |unn| {
                unn.field_value.deinit(allocator);
                allocator.destroy(unn.field_value);
                allocator.free(unn.field_name);
            },
            .Optional => |opt| {
                if (opt.has_value and opt.value != null) {
                    opt.value.?.deinit(allocator);
                    allocator.destroy(opt.value.?);
                }
            },
            .ErrorUnion => |err_union| {
                if (err_union.is_error) {
                    if (err_union.error_value) |err_value| {
                        allocator.free(err_value.name);
                    }
                } else if (err_union.payload) |payload| {
                    payload.deinit(allocator);
                    allocator.destroy(payload);
                }
            },
            .Dynamic => |dyn| {
                dyn.inner.deinit(allocator);
                allocator.destroy(dyn.inner);
            },
            else => {},
        }
        self.type_info.deinit(allocator);
        allocator.free(self.type_info);
    }
};

/// A reflection context for managing types and values
pub const ReflectionContext = struct {
    allocator: *Allocator,
    types: std.StringHashMap(*TypeInfo),
    type_ids: std.AutoHashMap(usize, *TypeInfo),

    const Self = @This();

    /// Initialize a new reflection context
    pub fn init(allocator: *Allocator) !*Self {
        const self = try allocator.create(Self);
        self.* = Self{
            .allocator = allocator,
            .types = std.StringHashMap(*TypeInfo).init(allocator),
            .type_ids = std.AutoHashMap(usize, *TypeInfo).init(allocator),
        };
        return self;
    }

    /// Clean up the reflection context
    pub fn deinit(self: *Self) void {
        var types_iter = self.types.valueIterator();
        while (types_iter.next()) |type_info| {
            type_info.*.deinit(self.allocator);
            self.allocator.destroy(type_info.*);
        }
        self.types.deinit();
        self.type_ids.deinit();
        self.allocator.destroy(self);
    }

    /// Register a type with the reflection context
    pub fn registerType(self: *Self, type_info: *TypeInfo, type_id: ?usize) !void {
        try self.types.put(type_info.name, type_info);
        if (type_id) |id| {
            try self.type_ids.put(id, type_info);
        }
    }

    /// Get type information by name
    pub fn getType(self: *Self, name: []const u8) ?*TypeInfo {
        return self.types.get(name);
    }

    /// Get type information by ID
    pub fn getTypeById(self: *Self, type_id: usize) ?*TypeInfo {
        return self.type_ids.get(type_id);
    }

    /// Create a new value from a type
    pub fn createValue(self: *Self, type_info: *TypeInfo) !*Value {
        const value = try self.allocator.create(Value);
        value.* = Value{
            .type_info = type_info,
            .kind = @enumFromInt(@intFromEnum(type_info.kind)),
            .data = undefined,
        };

        // Initialize default value based on type
        switch (type_info.kind) {
            .Void => {
                value.data = .{ .Void = {} };
            },
            .Bool => {
                value.data = .{ .Bool = false };
            },
            .Int => {
                const int_info = type_info.data.Int;
                value.data = .{ .Int = if (int_info.is_signed) .{ .I64 = 0 } else .{ .U64 = 0 } };
            },
            .Float => {
                value.data = .{ .Float = .{ .F64 = 0 } };
            },
            .Optional => {
                value.data = .{ .Optional = .{ .has_value = false, .value = null } };
            },
            .Null => {
                value.data = .{ .Null = {} };
            },
            .Undefined => {
                value.data = .{ .Undefined = {} };
            },
            else => {
                return error.Unimplemented;
            },
        }

        return value;
    }
};

// Exported C API functions

/// Initialize the reflection system
pub export fn reflectionInit() bool {
    global_allocator = &gpa.allocator;
    return true;
}

/// Clean up the reflection system
export fn reflectionCleanup() void {
    _ = gpa.deinit();
}

/// Create a new reflection context
export fn reflectionContextCreate() ?*ReflectionContext {
    return ReflectionContext.init(global_allocator) catch null;
}

/// Destroy a reflection context
export fn reflectionContextDestroy(context: *ReflectionContext) void {
    context.deinit();
}

/// Register a basic type with the reflection context
export fn reflectionRegisterBasicType(
    context: *ReflectionContext,
    kind: u32,
    name: [*:0]const u8,
    size: usize,
    alignment: usize,
    type_id: usize,
) bool {
    const type_kind: TypeKind = @enumFromInt(kind);
    const name_slice = std.mem.span(name);

    const type_info = global_allocator.create(TypeInfo) catch return false;
    const name_copy = global_allocator.dupe(u8, name_slice) catch {
        global_allocator.destroy(type_info);
        return false;
    };

    type_info.* = TypeInfo{
        .kind = type_kind,
        .name = name_copy,
        .size = size,
        .alignment = alignment,
        .data = undefined,
    };

    // Initialize type-specific data
    switch (type_kind) {
        .Void, .Bool, .Null, .Undefined, .Any, .Dynamic, .Opaque => {
            type_info.data = switch (type_kind) {
                .Void => .{ .Void = {} },
                .Bool => .{ .Bool = {} },
                .Opaque => .{ .Opaque = {} },
                .Any => .{ .Any = {} },
                .Dynamic => .{ .Dynamic = {} },
                .Null => .{ .Null = {} },
                .Undefined => .{ .Undefined = {} },
                else => unreachable,
            };
        },
        else => {
            global_allocator.free(name_copy);
            global_allocator.destroy(type_info);
            return false;
        },
    }

    context.registerType(type_info, type_id) catch {
        type_info.deinit(global_allocator);
        global_allocator.destroy(type_info);
        return false;
    };

    return true;
}

/// Register an integer type with the reflection context
export fn reflectionRegisterIntType(
    context: *ReflectionContext,
    name: [*:0]const u8,
    size: usize,
    alignment: usize,
    bits: u16,
    is_signed: bool,
    type_id: usize,
) bool {
    const name_slice = std.mem.span(name);

    const type_info = global_allocator.create(TypeInfo) catch return false;
    const name_copy = global_allocator.dupe(u8, name_slice) catch {
        global_allocator.destroy(type_info);
        return false;
    };

    type_info.* = TypeInfo{
        .kind = .Int,
        .name = name_copy,
        .size = size,
        .alignment = alignment,
        .data = .{
            .Int = .{
                .bits = bits,
                .is_signed = is_signed,
            },
        },
    };

    context.registerType(type_info, type_id) catch {
        type_info.deinit(global_allocator);
        global_allocator.destroy(type_info);
        return false;
    };

    return true;
}

/// Register a floating-point type with the reflection context
export fn reflectionRegisterFloatType(
    context: *ReflectionContext,
    name: [*:0]const u8,
    size: usize,
    alignment: usize,
    bits: u16,
    type_id: usize,
) bool {
    const name_slice = std.mem.span(name);

    const type_info = global_allocator.create(TypeInfo) catch return false;
    const name_copy = global_allocator.dupe(u8, name_slice) catch {
        global_allocator.destroy(type_info);
        return false;
    };

    type_info.* = TypeInfo{
        .kind = .Float,
        .name = name_copy,
        .size = size,
        .alignment = alignment,
        .data = .{
            .Float = .{
                .bits = bits,
            },
        },
    };

    context.registerType(type_info, type_id) catch {
        type_info.deinit(global_allocator);
        global_allocator.destroy(type_info);
        return false;
    };

    return true;
}

/// Create a value from a type
export fn reflectionCreateValue(
    context: *ReflectionContext,
    type_name: [*:0]const u8,
) ?*Value {
    const name_slice = std.mem.span(type_name);

    const type_info = context.getType(name_slice) orelse return null;
    return context.createValue(type_info) catch null;
}

/// Destroy a value
export fn reflectionDestroyValue(
    value: *Value,
) void {
    value.deinit(global_allocator);
    global_allocator.destroy(value);
}
