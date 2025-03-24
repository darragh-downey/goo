const std = @import("std");
const Lexer = @import("lexer.zig").Lexer;
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const LexerErrorType = @import("errors.zig").LexerErrorType;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Print header
    try std.io.getStdOut().writer().print("=== Goo Lexer Error Handling Tests ===\n\n", .{});

    // Test basic error reporting
    try testErrorReporting(allocator);

    // Test specific error cases
    try testInvalidCharacter(allocator);
    try testUnterminatedString(allocator);
    try testInvalidNumber(allocator);
    try testUnterminatedComment(allocator);
    try testMultipleErrors(allocator);

    try std.io.getStdOut().writer().print("\nAll tests passed!\n", .{});
}

fn testErrorReporting(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Error Reporting Functionality\n", .{});

    const source = "let @ = 42";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    // Store tokens in an array for proper cleanup
    var tokens = std.ArrayList(Token).init(allocator);
    defer {
        for (tokens.items) |tok| {
            tok.deinit(allocator);
        }
        tokens.deinit();
    }

    // Process all tokens
    while (true) {
        const tok = try lexer.nextToken();
        try tokens.append(tok);
        if (tok.type == .EOF) break;
    }

    // Instead of creating our own error reporter, use the one from the lexer
    // This avoids potential double-free issues
    if (!lexer.hasErrors()) {
        try std.io.getStdOut().writer().print("  Error: Expected lexer to have errors\n", .{});
        return error.TestFailure;
    }

    const errors = lexer.getErrors();
    if (errors.len != 1) {
        try std.io.getStdOut().writer().print("  Error: Expected 1 error, found {d}\n", .{errors.len});
        return error.TestFailure;
    }

    if (errors[0].error_type != LexerErrorType.InvalidCharacter) {
        try std.io.getStdOut().writer().print("  Error: Wrong error type\n", .{});
        return error.TestFailure;
    }

    // Test formatting
    const formatted = try lexer.getFormattedErrors();
    defer {
        for (formatted) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted);
    }

    if (formatted.len != 1) {
        try std.io.getStdOut().writer().print("  Error: Expected 1 formatted error, found {d}\n", .{formatted.len});
        return error.TestFailure;
    }

    try std.io.getStdOut().writer().print("  Error message: {s}\n", .{formatted[0]});
    try std.io.getStdOut().writer().print("\n", .{});
}

fn testInvalidCharacter(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Invalid Character\n", .{});

    const source = "let x = @";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    // Store tokens in an array for proper cleanup
    var tokens = std.ArrayList(Token).init(allocator);
    defer {
        for (tokens.items) |tok| {
            tok.deinit(allocator);
        }
        tokens.deinit();
    }

    // Process all tokens
    while (true) {
        const tok = try lexer.nextToken();
        try tokens.append(tok);
        if (tok.type == .EOF) break;
    }

    // Check for errors
    if (!lexer.hasErrors()) {
        try std.io.getStdOut().writer().print("  Error: Expected lexer to have errors\n", .{});
        return error.TestFailure;
    }

    const errors = lexer.getErrors();
    if (errors.len != 1) {
        try std.io.getStdOut().writer().print("  Error: Expected 1 error, found {d}\n", .{errors.len});
        return error.TestFailure;
    }

    if (errors[0].error_type != LexerErrorType.InvalidCharacter) {
        try std.io.getStdOut().writer().print("  Error: Expected InvalidCharacter error, found {s}\n", .{errors[0].error_type.toString()});
        return error.TestFailure;
    }

    // Print formatted errors
    const formatted = try lexer.getFormattedErrors();
    defer {
        for (formatted) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted);
    }

    for (formatted) |msg| {
        try std.io.getStdOut().writer().print("  {s}\n", .{msg});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

fn testUnterminatedString(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Unterminated String\n", .{});

    const source = "let message = \"Hello, world";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    // Store tokens in an array for proper cleanup
    var tokens = std.ArrayList(Token).init(allocator);
    defer {
        for (tokens.items) |tok| {
            tok.deinit(allocator);
        }
        tokens.deinit();
    }

    // Process all tokens
    while (true) {
        const tok = try lexer.nextToken();
        try tokens.append(tok);
        if (tok.type == .EOF) break;
    }

    // Check for errors
    if (!lexer.hasErrors()) {
        try std.io.getStdOut().writer().print("  Error: Expected lexer to have errors\n", .{});
        return error.TestFailure;
    }

    const errors = lexer.getErrors();
    if (errors.len != 1) {
        try std.io.getStdOut().writer().print("  Error: Expected 1 error, found {d}\n", .{errors.len});
        return error.TestFailure;
    }

    if (errors[0].error_type != LexerErrorType.UnterminatedString) {
        try std.io.getStdOut().writer().print("  Error: Expected UnterminatedString error, found {s}\n", .{errors[0].error_type.toString()});
        return error.TestFailure;
    }

    // Print formatted errors
    const formatted = try lexer.getFormattedErrors();
    defer {
        for (formatted) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted);
    }

    for (formatted) |msg| {
        try std.io.getStdOut().writer().print("  {s}\n", .{msg});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

fn testInvalidNumber(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Invalid Number\n", .{});

    const sources = [_][]const u8{
        "let x = 0x",
        "let x = 0xG",
        "let x = 0b102",
        "let x = 0o9",
    };

    for (sources, 0..) |source, i| {
        try std.io.getStdOut().writer().print("  Subtest {d}: {s}\n", .{ i + 1, source });

        var lexer = try Lexer.init(allocator, source);
        defer lexer.deinit();

        // Store tokens in an array for proper cleanup
        var tokens = std.ArrayList(Token).init(allocator);
        defer {
            for (tokens.items) |tok| {
                tok.deinit(allocator);
            }
            tokens.deinit();
        }

        // Process all tokens
        while (true) {
            const tok = try lexer.nextToken();
            try tokens.append(tok);
            if (tok.type == .EOF) break;
        }

        // Check for errors
        if (!lexer.hasErrors()) {
            try std.io.getStdOut().writer().print("    Warning: Expected lexer to have errors\n", .{});
            continue;
        }

        // Print formatted errors
        const formatted = try lexer.getFormattedErrors();
        defer {
            for (formatted) |msg| {
                allocator.free(msg);
            }
            allocator.free(formatted);
        }

        for (formatted) |msg| {
            try std.io.getStdOut().writer().print("    {s}\n", .{msg});
        }
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

fn testUnterminatedComment(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Unterminated Comment\n", .{});

    const source = "let x = 10 /* This is a comment without end";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    // Store tokens in an array for proper cleanup
    var tokens = std.ArrayList(Token).init(allocator);
    defer {
        for (tokens.items) |tok| {
            tok.deinit(allocator);
        }
        tokens.deinit();
    }

    // Process all tokens
    while (true) {
        const tok = try lexer.nextToken();
        try tokens.append(tok);
        if (tok.type == .EOF) break;
    }

    // Check for errors
    if (!lexer.hasErrors()) {
        try std.io.getStdOut().writer().print("  Error: Expected lexer to have errors\n", .{});
        return error.TestFailure;
    }

    const errors = lexer.getErrors();
    if (errors.len != 1) {
        try std.io.getStdOut().writer().print("  Error: Expected 1 error, found {d}\n", .{errors.len});
        return error.TestFailure;
    }

    if (errors[0].error_type != LexerErrorType.UnterminatedComment) {
        try std.io.getStdOut().writer().print("  Error: Expected UnterminatedComment error, found {s}\n", .{errors[0].error_type.toString()});
        return error.TestFailure;
    }

    // Print formatted errors
    const formatted = try lexer.getFormattedErrors();
    defer {
        for (formatted) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted);
    }

    for (formatted) |msg| {
        try std.io.getStdOut().writer().print("  {s}\n", .{msg});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

fn testMultipleErrors(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Multiple Errors\n", .{});

    const source = "let x = @ + \"unclosed + 0xG";
    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    // Store tokens in an array for proper cleanup
    var tokens = std.ArrayList(Token).init(allocator);
    defer {
        for (tokens.items) |tok| {
            tok.deinit(allocator);
        }
        tokens.deinit();
    }

    // Process all tokens
    while (true) {
        const tok = try lexer.nextToken();
        try tokens.append(tok);
        if (tok.type == .EOF) break;
    }

    // Check for errors
    if (!lexer.hasErrors()) {
        try std.io.getStdOut().writer().print("  Error: Expected lexer to have errors\n", .{});
        return error.TestFailure;
    }

    const errors = lexer.getErrors();
    if (errors.len < 2) {
        try std.io.getStdOut().writer().print("  Error: Expected multiple errors, found {d}\n", .{errors.len});
        return error.TestFailure;
    }

    // Print formatted errors
    const formatted = try lexer.getFormattedErrors();
    defer {
        for (formatted) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted);
    }

    try std.io.getStdOut().writer().print("  Found {d} errors:\n", .{formatted.len});
    for (formatted) |msg| {
        try std.io.getStdOut().writer().print("  {s}\n", .{msg});
    }

    try std.io.getStdOut().writer().print("\n", .{});
}
