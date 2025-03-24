const std = @import("std");
const semantic = @import("semantic.zig");
const parser_c_api = @import("../../frontend/parser/zig/parser_c_api.zig");
const parser = @import("../../frontend/parser/zig/parser.zig");
const TypeChecker = semantic.TypeChecker;

/// Opaque handle for the semantic analyzer
pub const GooSemanticHandle = *anyopaque;

/// Error codes for semantic analyzer
pub const GooSemanticErrorCode = enum(c_int) {
    GOO_SEMANTIC_SUCCESS = 0,
    GOO_SEMANTIC_TYPE_MISMATCH = 1,
    GOO_SEMANTIC_UNDEFINED_VARIABLE = 2,
    GOO_SEMANTIC_UNDEFINED_FUNCTION = 3,
    GOO_SEMANTIC_UNDEFINED_TYPE = 4,
    GOO_SEMANTIC_INVALID_ASSIGNMENT = 5,
    GOO_SEMANTIC_INVALID_OPERATION = 6,
    GOO_SEMANTIC_INVALID_FUNCTION_CALL = 7,
    GOO_SEMANTIC_GENERAL_ERROR = 8,
};

/// Type ID enum
pub const GooTypeId = enum(c_int) {
    GOO_TYPE_VOID = 0,
    GOO_TYPE_BOOL = 1,
    GOO_TYPE_INT = 2,
    GOO_TYPE_FLOAT = 3,
    GOO_TYPE_STRING = 4,
    GOO_TYPE_ARRAY = 5,
    GOO_TYPE_STRUCT = 6,
    GOO_TYPE_FUNCTION = 7,
    GOO_TYPE_CUSTOM = 8,
    GOO_TYPE_ERROR = 9,
};

/// Semantic analyzer context
const Context = struct {
    checker: *TypeChecker,
    allocator: std.mem.Allocator,
    last_error: GooSemanticErrorCode,
    error_message: ?[]const u8,

    fn init(allocator: std.mem.Allocator) !*Context {
        const checker = try semantic.init(allocator);

        const ctx = try allocator.create(Context);
        ctx.* = .{
            .checker = checker,
            .allocator = allocator,
            .last_error = .GOO_SEMANTIC_SUCCESS,
            .error_message = null,
        };

        return ctx;
    }

    fn deinit(self: *Context) void {
        if (self.error_message) |msg| {
            self.allocator.free(msg);
        }

        semantic.deinit(self.checker);
        self.allocator.destroy(self);
    }

    fn setError(self: *Context, code: GooSemanticErrorCode, message: []const u8) !void {
        self.last_error = code;

        if (self.error_message) |old_msg| {
            self.allocator.free(old_msg);
        }

        self.error_message = try self.allocator.dupe(u8, message);
    }
};

/// Initialize the semantic analyzer
export fn gooSemanticCreate() ?GooSemanticHandle {
    const allocator = std.heap.c_allocator;

    const ctx = Context.init(allocator) catch |err| {
        std.debug.print("Failed to initialize semantic analyzer: {}\n", .{err});
        return null;
    };

    return @ptrCast(ctx);
}

/// Destroy the semantic analyzer
export fn gooSemanticDestroy(handle: ?GooSemanticHandle) void {
    if (handle) |h| {
        const ctx = @as(*Context, @ptrCast(@alignCast(h)));
        ctx.deinit();
    }
}

/// Run semantic analysis on an AST
export fn gooSemanticAnalyze(handle: ?GooSemanticHandle, parser_handle: ?parser_c_api.GooParserHandle) GooSemanticErrorCode {
    if (handle == null or parser_handle == null) {
        return .GOO_SEMANTIC_GENERAL_ERROR;
    }

    const ctx = @as(*Context, @ptrCast(@alignCast(handle.?)));

    // Reset error state
    ctx.last_error = .GOO_SEMANTIC_SUCCESS;
    if (ctx.error_message) |msg| {
        ctx.allocator.free(msg);
        ctx.error_message = null;
    }

    // Extract AST from parser handle
    const parser_ctx = @as(*parser_c_api.ParserContext, @ptrCast(@alignCast(parser_handle.?)));
    const program = parser_ctx.program;

    if (program == null) {
        ctx.setError(.GOO_SEMANTIC_GENERAL_ERROR, "No program available for analysis") catch {};
        return .GOO_SEMANTIC_GENERAL_ERROR;
    }

    // Run semantic analysis
    semantic.analyze(ctx.checker, program.?) catch |err| {
        const error_code = mapErrorCode(err);
        ctx.setError(error_code, "Semantic analysis failed") catch {};
        return error_code;
    };

    // Check if there were any type errors
    if (semantic.hasErrors(ctx.checker)) {
        if (ctx.checker.errors.items.len > 0) {
            ctx.setError(.GOO_SEMANTIC_GENERAL_ERROR, ctx.checker.errors.items[0]) catch {};
        } else {
            ctx.setError(.GOO_SEMANTIC_GENERAL_ERROR, "Type checking failed with unknown errors") catch {};
        }
        return .GOO_SEMANTIC_GENERAL_ERROR;
    }

    return .GOO_SEMANTIC_SUCCESS;
}

/// Get the error message from the last semantic operation
export fn gooSemanticGetErrorMessage(handle: ?GooSemanticHandle) [*c]const u8 {
    if (handle == null) {
        return "Invalid semantic analyzer handle";
    }

    const ctx = @as(*Context, @ptrCast(@alignCast(handle.?)));

    if (ctx.error_message) |msg| {
        return msg.ptr;
    } else {
        return "No error";
    }
}

/// Get the number of semantic errors
export fn gooSemanticGetErrorCount(handle: ?GooSemanticHandle) c_int {
    if (handle == null) {
        return 0;
    }

    const ctx = @as(*Context, @ptrCast(@alignCast(handle.?)));

    return @intCast(ctx.checker.errors.items.len);
}

/// Get a semantic error by index
export fn gooSemanticGetError(handle: ?GooSemanticHandle, index: c_int) [*c]const u8 {
    if (handle == null) {
        return "Invalid semantic analyzer handle";
    }

    const ctx = @as(*Context, @ptrCast(@alignCast(handle.?)));

    if (index < 0 or index >= ctx.checker.errors.items.len) {
        return "Invalid error index";
    }

    return ctx.checker.errors.items[@intCast(index)].ptr;
}

/// Helper to map Zig errors to C API error codes
fn mapErrorCode(err: anyerror) GooSemanticErrorCode {
    return switch (err) {
        semantic.TypeCheckError.TypeMismatch => .GOO_SEMANTIC_TYPE_MISMATCH,
        semantic.TypeCheckError.UndefinedVariable => .GOO_SEMANTIC_UNDEFINED_VARIABLE,
        semantic.TypeCheckError.UndefinedFunction => .GOO_SEMANTIC_UNDEFINED_FUNCTION,
        semantic.TypeCheckError.UndefinedType => .GOO_SEMANTIC_UNDEFINED_TYPE,
        semantic.TypeCheckError.InvalidAssignment => .GOO_SEMANTIC_INVALID_ASSIGNMENT,
        semantic.TypeCheckError.InvalidOperation => .GOO_SEMANTIC_INVALID_OPERATION,
        semantic.TypeCheckError.InvalidFunctionCall => .GOO_SEMANTIC_INVALID_FUNCTION_CALL,
        else => .GOO_SEMANTIC_GENERAL_ERROR,
    };
}
