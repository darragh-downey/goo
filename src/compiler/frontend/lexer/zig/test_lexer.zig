const std = @import("std");
const lexer = @import("lexer.zig");
const token = @import("token.zig");

const Lexer = lexer.Lexer;
const TokenType = token.TokenType;
const TokenValue = token.TokenValue;

pub fn main() !void {
    // Create an allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();

    // Create an arena allocator that will free all allocations at once
    var arena = std.heap.ArenaAllocator.init(gpa.allocator());
    defer arena.deinit();
    const allocator = arena.allocator();

    const stdout = std.io.getStdOut().writer();
    try stdout.print("\n=== Goo Lexer Test Runner ===\n\n", .{});

    // Test 1: Simple Tokens
    {
        const source = "= + - * / % { } ( ) [ ] , ; : := == != < > <= >= && ||";
        try stdout.print("=== Test 1: Simple Tokens ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 2: Keywords
    {
        const source = "package import func var const if else for return go parallel chan";
        try stdout.print("\n=== Test 2: Keywords ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 3: Literals
    {
        const source = "42 3.14 \"hello world\" `raw string` true false identifier";
        try stdout.print("\n=== Test 3: Literals ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 4: Advanced Operators
    {
        const source = "+= -= *= /= -> .. << >> & | ^ ~";
        try stdout.print("\n=== Test 4: Advanced Operators ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 5: Comments
    {
        const source =
            \\// This is a line comment
            \\x // Another comment
            \\/* Block comment */ y /* Nested /* comment */ */ z
        ;
        try stdout.print("\n=== Test 5: Comments ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 6: Realistic Code Snippet
    {
        const source =
            \\package main
            \\
            \\import "std"
            \\
            \\func fibonacci(n: int) -> int {
            \\    if n <= 1 {
            \\        return n
            \\    }
            \\    return fibonacci(n-1) + fibonacci(n-2)
            \\}
            \\
            \\func main() {
            \\    // Print first 10 Fibonacci numbers
            \\    for i := 0; i < 10; i += 1 {
            \\        std.println(fibonacci(i))
            \\    }
            \\}
        ;
        try stdout.print("\n=== Test 6: Realistic Code Snippet ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 7: String Escapes
    {
        const source = "\"Hello\\nWorld\" \"Quotes: \\\"nested\\\"\" \"Backslash: \\\\\"";
        try stdout.print("\n=== Test 7: String Escapes ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});
        try testLexer(allocator, stdout, source);
    }

    // Test 8: Error Handling
    {
        const source = "@ \"unterminated string"; // Removed the */ to prevent mixed issues
        try stdout.print("\n=== Test 8: Error Handling ===\n\n", .{});
        try stdout.print("Source: {s}\n\n", .{source});

        // Special handling for the error test
        try testLexerWithErrors(allocator, stdout, source);
    }
}

fn testLexer(allocator: std.mem.Allocator, writer: anytype, source: []const u8) !void {
    var our_lexer = try Lexer.init(allocator, source);
    defer our_lexer.deinit();

    try writer.print("| {s:<15} | {s:<20} | {s:<5} | {s:<5} |\n", .{ "Token Type", "Value", "Line", "Col" });
    try writer.print("|{s}|{s}|{s}|{s}|\n", .{ "-----------------", "----------------------", "-------", "-------" });

    // Read all tokens
    var count: usize = 0;
    const max_tokens = 100; // Prevent infinite loops
    while (count < max_tokens) : (count += 1) {
        const tok = try our_lexer.nextToken();

        // Format the token value based on its type
        const value_str = try formatTokenValue(allocator, tok.value);

        try writer.print("| {s:<15} | {s:<20} | {d:<5} | {d:<5} |\n", .{
            tok.getTypeName(),
            value_str,
            tok.line,
            tok.column,
        });

        // Break if we've reached EOF
        if (tok.type == .EOF) {
            break;
        }
    }

    try writer.print("\nTotal tokens: {d}\n", .{count});
}

// Special version of testLexer that handles expected errors
fn testLexerWithErrors(allocator: std.mem.Allocator, writer: anytype, source: []const u8) !void {
    var our_lexer = try Lexer.init(allocator, source);
    defer our_lexer.deinit();

    try writer.print("| {s:<15} | {s:<20} | {s:<5} | {s:<5} |\n", .{ "Token Type", "Value", "Line", "Col" });
    try writer.print("|{s}|{s}|{s}|{s}|\n", .{ "-----------------", "----------------------", "-------", "-------" });

    // Read all tokens
    var count: usize = 0;
    const max_tokens = 100; // Prevent infinite loops
    while (count < max_tokens) : (count += 1) {
        const tok = our_lexer.nextToken() catch |err| {
            try writer.print("| {s:<15} | {s:<20} | {s:<5} | {s:<5} |\n", .{
                "ERROR",
                "Lexer Error",
                "-",
                "-",
            });
            try writer.print("\nEncountered error: {s}\n", .{@errorName(err)});
            break;
        };

        // Format the token value based on its type
        const value_str = try formatTokenValue(allocator, tok.value);

        try writer.print("| {s:<15} | {s:<20} | {d:<5} | {d:<5} |\n", .{
            tok.getTypeName(),
            value_str,
            tok.line,
            tok.column,
        });

        // Break if we've reached EOF
        if (tok.type == .EOF) {
            break;
        }
    }

    try writer.print("\nTotal tokens: {d}\n", .{count});
}

fn formatTokenValue(allocator: std.mem.Allocator, value: TokenValue) ![]const u8 {
    return switch (value) {
        .none_val => try allocator.dupe(u8, ""),
        .string_val => |s| blk: {
            if (s.len > 17) {
                // Truncate long strings for display
                var result = try allocator.alloc(u8, 17);
                @memcpy(result[0..14], s[0..14]);
                result[14] = '.';
                result[15] = '.';
                result[16] = '.';
                break :blk result;
            } else {
                break :blk try allocator.dupe(u8, s);
            }
        },
        .integer_val => |i| try std.fmt.allocPrint(allocator, "{d}", .{i}),
        .float_val => |f| try std.fmt.allocPrint(allocator, "{d}", .{f}),
        .boolean_val => |b| try allocator.dupe(u8, if (b) "true" else "false"),
    };
}
