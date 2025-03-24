// Lexer module for the Goo programming language
// This file serves as the main entry point for the lexer module, re-exporting
// components from the specialized modules.

const std = @import("std");

// Re-export the token module
pub const token = @import("token.zig");
pub const TokenType = token.TokenType;
pub const Token = token.Token;
pub const TokenValue = token.TokenValue;

// Re-export the lexer core functionality
pub const lexer_core = @import("lexer_core.zig");
pub const Lexer = lexer_core.Lexer;

// Re-export the error handling module
pub const errors = @import("errors.zig");
pub const ErrorReporter = errors.ErrorReporter;

// Re-export utility modules
pub const character_utils = @import("character_utils.zig");
pub const keywords = @import("keywords.zig");
pub const token_reader = @import("token_reader.zig");

/// Initialize a new lexer with the given input
pub fn init(allocator: std.mem.Allocator, input: []const u8) !Lexer {
    return Lexer.init(allocator, input);
}

/// Read all tokens from the input and return them as an array
pub fn tokenize(allocator: std.mem.Allocator, input: []const u8) ![]Token {
    var lexer = try init(allocator, input);
    defer lexer.deinit();

    var tokens = std.ArrayList(Token).init(allocator);
    defer tokens.deinit();

    while (true) {
        const current_token = try lexer.nextToken();
        try tokens.append(current_token);

        if (current_token.type == .EOF) {
            break;
        }
    }

    return tokens.toOwnedSlice();
}

/// Utility function to create a token for testing or debugging
pub fn createToken(token_type: TokenType, line: usize, column: usize, value: TokenValue) Token {
    return Token.init(token_type, line, column, value);
}

/// Check if a token has the expected type
pub fn tokenHasType(tok: Token, expected_type: TokenType) bool {
    return tok.type == expected_type;
}

/// Format a token for display
pub fn formatToken(tok: Token, allocator: std.mem.Allocator) ![]u8 {
    return tok.format(allocator);
}

// Re-export for tests or other modules that need it
pub const std_lib = std;
