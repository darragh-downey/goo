const std = @import("std");
const parser = @import("parser");
const ast = @import("ast");
const errors = @import("errors");

// C API for the parser

/// Error codes for the parser
pub const GooParserErrorCode = enum(c_int) {
    /// No error
    GOO_PARSER_OK = 0,
    /// Invalid input
    GOO_PARSER_INVALID_INPUT = 1,
    /// Syntax error
    GOO_PARSER_SYNTAX_ERROR = 2,
    /// Memory allocation error
    GOO_PARSER_MEMORY_ERROR = 3,
    /// Internal error
    GOO_PARSER_INTERNAL_ERROR = 4,
};

/// Handle for the parser
pub const GooParserHandle = *anyopaque;

/// Parse error information
pub const GooParseError = extern struct {
    code: GooParserErrorCode,
    line: c_uint,
    column: c_uint,
    message: [256]u8,
};

/// Initialize a parser with the given source code
export fn goo_parser_init(source: [*:0]const u8) ?GooParserHandle {
    const allocator = std.heap.c_allocator;

    const parser_context = allocator.create(parser.Parser) catch {
        return null;
    };

    parser_context.* = parser.Parser.init(allocator, source) catch {
        allocator.destroy(parser_context);
        return null;
    };

    return @ptrCast(parser_context);
}

/// Parse the source code into an AST
export fn goo_parser_parse(handle: GooParserHandle, error_out: ?*GooParseError) bool {
    if (handle == null) return false;

    const parser_context: *parser.Parser = @ptrCast(@alignCast(handle));

    parser_context.parse() catch |err| {
        if (error_out != null) {
            var parse_error = &error_out.?.*;

            // Set error code based on the error
            parse_error.code = switch (err) {
                error.InvalidInput => GooParserErrorCode.GOO_PARSER_INVALID_INPUT,
                error.SyntaxError => GooParserErrorCode.GOO_PARSER_SYNTAX_ERROR,
                error.OutOfMemory => GooParserErrorCode.GOO_PARSER_MEMORY_ERROR,
                else => GooParserErrorCode.GOO_PARSER_INTERNAL_ERROR,
            };

            // Set error location
            if (parser_context.error_location) |loc| {
                parse_error.line = @intCast(loc.line);
                parse_error.column = @intCast(loc.column);
            } else {
                parse_error.line = 0;
                parse_error.column = 0;
            }

            // Set error message
            if (parser_context.error_message.len > 0) {
                @memcpy(parse_error.message[0..@min(parser_context.error_message.len, 255)], parser_context.error_message[0..@min(parser_context.error_message.len, 255)]);
                parse_error.message[@min(parser_context.error_message.len, 255)] = 0;
            } else {
                @memcpy(parse_error.message[0..@min(@errorName(err).len, 255)], @errorName(err)[0..@min(@errorName(err).len, 255)]);
                parse_error.message[@min(@errorName(err).len, 255)] = 0;
            }
        }

        return false;
    };

    return true;
}

/// Free the parser's resources
export fn goo_parser_destroy(handle: GooParserHandle) void {
    if (handle == null) return;

    const parser_context: *parser.Parser = @ptrCast(@alignCast(handle));
    const allocator = std.heap.c_allocator;

    parser_context.deinit();
    allocator.destroy(parser_context);
}
