const std = @import("std");
const Lexer = @import("lexer.zig").Lexer;
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const LexerErrorType = @import("errors.zig").LexerErrorType;
const SafeTestRunner = @import("safe_test_runner.zig").SafeTestRunner;

pub fn main() !void {
    // Set up an allocator for testing
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Print header
    try std.io.getStdOut().writer().print("=== Goo Lexer Safe Tests ===\n\n", .{});

    // Run memory-safe tests with our fixes
    try testBasicParsing(allocator);
    try testErrorHandling(allocator);
    try testStringHandling(allocator);
    try testComplexProgram(allocator);

    try std.io.getStdOut().writer().print("\nAll tests completed without crashes!\n", .{});
}

/// Test basic code parsing with identifiers, operators, and literals
fn testBasicParsing(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Basic Parsing\n", .{});

    const source = "let x = 42";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    var runner = SafeTestRunner.init(allocator);
    defer runner.deinit();

    const token_count = try runner.processAllTokens(&lexer);
    try std.io.getStdOut().writer().print("  Found {d} tokens\n", .{token_count});

    if (token_count > 0) {
        if (runner.getToken(0)) |tok| {
            try std.io.getStdOut().writer().print("  First token: {s}\n", .{@tagName(tok.type)});

            if (tok.type != .IDENTIFIER) {
                try std.io.getStdOut().writer().print("  ERROR: Expected IDENTIFIER token, got {s}\n", .{@tagName(tok.type)});
            }
        }
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

/// Test error handling with invalid characters
fn testErrorHandling(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Error Handling\n", .{});

    const source = "let @ = 42";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    var runner = SafeTestRunner.init(allocator);
    defer runner.deinit();

    const token_count = try runner.processAllTokens(&lexer);
    try std.io.getStdOut().writer().print("  Found {d} tokens\n", .{token_count});

    var error_count: usize = 0;
    for (runner.tokens.items) |tok| {
        if (tok.type == .ERROR) {
            error_count += 1;
        }
    }

    try std.io.getStdOut().writer().print("  Found {d} error tokens\n", .{error_count});

    if (lexer.hasErrors()) {
        const errors = lexer.getErrors();
        try std.io.getStdOut().writer().print("  Lexer reported {d} errors\n", .{errors.len});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

/// Test string handling with terminated and unterminated strings
fn testStringHandling(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: String Handling\n", .{});

    const source = "\"Hello, world!\" \"Unterminated";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    var runner = SafeTestRunner.init(allocator);
    defer runner.deinit();

    const token_count = try runner.processAllTokens(&lexer);
    try std.io.getStdOut().writer().print("  Found {d} tokens\n", .{token_count});

    var string_count: usize = 0;
    var error_count: usize = 0;
    for (runner.tokens.items) |tok| {
        if (tok.type == .STRING_LITERAL) {
            string_count += 1;
        } else if (tok.type == .ERROR) {
            error_count += 1;
        }
    }

    try std.io.getStdOut().writer().print("  Found {d} string tokens and {d} error tokens\n", .{ string_count, error_count });

    if (lexer.hasErrors()) {
        const errors = lexer.getErrors();
        try std.io.getStdOut().writer().print("  Lexer reported {d} errors\n", .{errors.len});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

/// Test a more complex program with multiple token types and potential errors
fn testComplexProgram(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Complex Program\n", .{});

    const source =
        \\func add(x int, y int) int {
        \\    return x + y
        \\}
        \\
        \\func main() {
        \\    let result = add(40, 2)
        \\    let msg = "Result: " + result
        \\    // Comment line
        \\    /* Block
        \\       comment */
        \\    let hex = 0xFF
        \\    let bin = 0b1010
        \\    let invalid = @
        \\}
    ;

    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    var runner = SafeTestRunner.init(allocator);
    defer runner.deinit();

    const token_count = try runner.processAllTokens(&lexer);
    try std.io.getStdOut().writer().print("  Found {d} tokens\n", .{token_count});

    var type_counts = std.AutoHashMap(TokenType, usize).init(allocator);
    defer type_counts.deinit();

    // Count token types
    for (runner.tokens.items) |tok| {
        const count = type_counts.get(tok.type) orelse 0;
        try type_counts.put(tok.type, count + 1);
    }

    // Report some interesting token counts
    const identifiers = type_counts.get(.IDENTIFIER) orelse 0;
    const strings = type_counts.get(.STRING_LITERAL) orelse 0;
    const ints = type_counts.get(.INT_LITERAL) orelse 0;
    const errors = type_counts.get(.ERROR) orelse 0;

    try std.io.getStdOut().writer().print("  Token breakdown: {d} identifiers, {d} strings, {d} integers, {d} errors\n", .{ identifiers, strings, ints, errors });

    if (lexer.hasErrors()) {
        const lexer_errors = lexer.getErrors();
        try std.io.getStdOut().writer().print("  Lexer reported {d} errors\n", .{lexer_errors.len});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}
