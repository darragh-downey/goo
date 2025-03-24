const std = @import("std");
const lexer = @import("lexer.zig");
const token = @import("token.zig");

const Lexer = lexer.Lexer;
const TokenType = token.TokenType;
const Token = token.Token;
const TokenValue = token.TokenValue;

/// Opaque handle for the lexer
pub const GooLexerHandle = *align(@alignOf(u64)) anyopaque;

/// C-compatible token structure
pub const GooToken = extern struct {
    token_type: c_int,
    line: c_uint,
    column: c_uint,
    has_string_value: bool,
    string_value: [*c]u8,
    string_length: c_uint,
    int_value: c_longlong,
    float_value: f64,
    bool_value: bool,
};

/// Convert Zig TokenType to C integer representation
fn tokenTypeToInt(token_type: TokenType) c_int {
    return @intFromEnum(token_type);
}

/// Convert from C integer to Zig TokenType
fn intToTokenType(value: c_int) TokenType {
    return @enumFromInt(value);
}

/// Global allocator for the C API
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const global_allocator = gpa.allocator();

/// Internal state for each lexer instance
const LexerState = struct {
    lexer: Lexer,
    source_string: []const u8,
    allocator: std.mem.Allocator,
    string_cache: std.ArrayList([]u8),

    fn init(allocator: std.mem.Allocator, source: []const u8) !*LexerState {
        const state = try allocator.create(LexerState);
        state.* = LexerState{
            .lexer = try Lexer.init(allocator, source),
            .source_string = source,
            .allocator = allocator,
            .string_cache = std.ArrayList([]u8).init(allocator),
        };
        return state;
    }

    fn deinit(self: *LexerState) void {
        self.lexer.deinit();

        // Free all cached strings
        for (self.string_cache.items) |str| {
            self.allocator.free(str);
        }
        self.string_cache.deinit();

        // Free the source string (which we own)
        self.allocator.free(self.source_string);

        // Free the state itself
        self.allocator.destroy(self);
    }

    fn addCachedString(self: *LexerState, str: []const u8) ![*c]u8 {
        const string_copy = try self.allocator.dupe(u8, str);
        try self.string_cache.append(string_copy);
        return @ptrCast(string_copy.ptr);
    }
};

/// Initialize a new lexer with the given source code
export fn goo_lexer_init(source: [*c]const u8, source_len: c_uint) ?GooLexerHandle {
    // Copy the source string since we don't know its lifetime
    const source_slice = source[0..source_len];
    const source_copy = global_allocator.dupe(u8, source_slice) catch {
        return null;
    };

    // Create the lexer state
    const state = LexerState.init(global_allocator, source_copy) catch {
        global_allocator.free(source_copy);
        return null;
    };

    return @ptrCast(state);
}

/// Free resources associated with the lexer
export fn goo_lexer_destroy(handle: GooLexerHandle) void {
    const state: *LexerState = @ptrCast(@alignCast(handle));
    state.deinit();
}

/// Get the next token from the lexer
export fn goo_lexer_next_token(handle: GooLexerHandle, token_out: *GooToken) bool {
    const state: *LexerState = @ptrCast(@alignCast(handle));

    // Get the next token from the lexer
    const token_result = state.lexer.nextToken() catch {
        return false;
    };

    // Convert from Zig token to C token
    token_out.token_type = tokenTypeToInt(token_result.type);
    token_out.line = @intCast(token_result.line);
    token_out.column = @intCast(token_result.column);

    // Set all value fields to default values
    token_out.has_string_value = false;
    token_out.string_value = null;
    token_out.string_length = 0;
    token_out.int_value = 0;
    token_out.float_value = 0;
    token_out.bool_value = false;

    // Set C token value based on Zig token value
    switch (token_result.value) {
        .none_val => {},
        .string_val => |s| {
            token_out.has_string_value = true;
            token_out.string_value = state.addCachedString(s) catch {
                return false;
            };
            token_out.string_length = @intCast(s.len);
        },
        .integer_val => |i| {
            token_out.int_value = i;
        },
        .float_val => |f| {
            token_out.float_value = f;
        },
        .boolean_val => |b| {
            token_out.bool_value = b;
        },
    }

    return true;
}

/// Get the token type name as a string
export fn goo_token_type_name(token_type: c_int) [*c]const u8 {
    const zig_token_type = intToTokenType(token_type);
    const name = @tagName(zig_token_type);
    return name.ptr;
}

/// Clean up global resources when the library is unloaded
export fn goo_lexer_cleanup() void {
    _ = gpa.deinit();
}
