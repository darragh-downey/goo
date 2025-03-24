const std = @import("std");
const lexer_mod = @import("lexer.zig");
const token_mod = @import("token.zig");

const Lexer = lexer_mod.Lexer;
const Token = token_mod.Token;
const TokenType = token_mod.TokenType;
const TokenValue = token_mod.TokenValue;

/// Global allocator for C API
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const global_allocator = gpa.allocator();

/// C representation of a token
pub const GooToken = extern struct {
    token_type: c_int,
    line: c_int,
    column: c_int,
    int_value: i64,
    float_value: f64,
    bool_value: bool,
    string_value: ?[*:0]u8, // Null-terminated string or NULL
};

/// Opaque handle to a lexer instance
pub const GooLexer = opaque {};

/// Initialize a new lexer with the given source code
pub export fn goo_lexer_new(source: [*:0]const u8) ?*GooLexer {
    // Convert C string to Zig slice
    const source_slice = std.mem.span(source);

    // Create a new lexer
    const lexer_ptr = global_allocator.create(Lexer) catch |err| {
        std.debug.print("Failed to allocate lexer: {}\n", .{err});
        return null;
    };

    // Initialize the lexer
    lexer_ptr.* = Lexer.init(global_allocator, source_slice) catch |err| {
        std.debug.print("Failed to initialize lexer: {}\n", .{err});
        global_allocator.destroy(lexer_ptr);
        return null;
    };

    return @ptrCast(lexer_ptr);
}

/// Free a lexer instance
pub export fn goo_lexer_free(lexer_handle: ?*GooLexer) void {
    if (lexer_handle) |handle| {
        const lexer_ptr = @as(*Lexer, @ptrCast(@alignCast(handle)));
        lexer_ptr.deinit();
        global_allocator.destroy(lexer_ptr);
    }
}

/// Get the next token from the lexer
pub export fn goo_lexer_next_token(lexer_handle: ?*GooLexer) GooToken {
    if (lexer_handle) |handle| {
        const lexer_ptr = @as(*Lexer, @ptrCast(@alignCast(handle)));

        // Get the next token
        const zig_token = lexer_ptr.nextToken() catch |err| {
            std.debug.print("Error getting next token: {}\n", .{err});
            return GooToken{
                .token_type = @intFromEnum(TokenType.ERROR),
                .line = 0,
                .column = 0,
                .int_value = 0,
                .float_value = 0,
                .bool_value = false,
                .string_value = null,
            };
        };

        // Convert token to C representation
        return zigTokenToCToken(zig_token);
    } else {
        return GooToken{
            .token_type = @intFromEnum(TokenType.ERROR),
            .line = 0,
            .column = 0,
            .int_value = 0,
            .float_value = 0,
            .bool_value = false,
            .string_value = null,
        };
    }
}

/// Convert a Zig token to a C token
fn zigTokenToCToken(zig_token: Token) GooToken {
    var c_token = GooToken{
        .token_type = @intFromEnum(zig_token.token_type),
        .line = @intCast(zig_token.line),
        .column = @intCast(zig_token.column),
        .int_value = 0,
        .float_value = 0,
        .bool_value = false,
        .string_value = null,
    };

    // Set the appropriate value based on the token type
    switch (zig_token.value) {
        .int_val => |i| c_token.int_value = i,
        .float_val => |f| c_token.float_value = f,
        .bool_val => |b| c_token.bool_value = b,
        .string_val => |s| {
            // Convert Zig string to C string (null-terminated)
            if (s.len > 0) {
                const c_str = global_allocator.allocSentinel(u8, s.len, 0) catch return c_token;
                @memcpy(c_str, s);
                c_token.string_value = c_str.ptr;
            }
        },
        .no_value => {},
    }

    return c_token;
}

/// Free a token's resources (string value)
pub export fn goo_token_free(c_token: GooToken) void {
    if (c_token.string_value) |string_ptr| {
        const len = std.mem.len(string_ptr);
        const slice = string_ptr[0..len :0];
        global_allocator.free(slice);
    }
}

/// Get the name of a token type
pub export fn goo_token_get_name(token_type: c_int) [*:0]const u8 {
    const zig_token_type = @as(TokenType, @enumFromInt(token_type));
    return @ptrCast(zig_token_type.toString());
}

/// Test the lexer with a sample source
pub export fn goo_test_lexer(source: [*:0]const u8) bool {
    const lexer_handle = goo_lexer_new(source);
    if (lexer_handle == null) {
        return false;
    }
    defer goo_lexer_free(lexer_handle);

    // Read all tokens
    var token_count: usize = 0;
    var error_count: usize = 0;

    while (true) {
        const current_token = goo_lexer_next_token(lexer_handle);
        token_count += 1;

        // Print token information
        if (current_token.token_type == @intFromEnum(TokenType.ERROR)) {
            error_count += 1;
            std.debug.print("Error token at line {}, column {}\n", .{ current_token.line, current_token.column });
        }

        // Free token resources
        goo_token_free(current_token);

        // Stop at EOF
        if (current_token.token_type == @intFromEnum(TokenType.EOF)) {
            break;
        }
    }

    std.debug.print("Lexer test complete: {} tokens processed, {} errors\n", .{ token_count, error_count });
    return error_count == 0;
}
