const std = @import("std");
const Lexer = @import("lexer.zig").Lexer;
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const LexerErrorType = @import("errors.zig").LexerErrorType;
const ErrorReporter = @import("errors.zig").ErrorReporter;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Print header
    try std.io.getStdOut().writer().print("=== Goo Lexer Memory Debug Tests ===\n\n", .{});

    try testSimpleReporting(allocator);
    try testLexerErrorPattern(allocator);

    try std.io.getStdOut().writer().print("\nAll tests completed!\n", .{});
}

fn testSimpleReporting(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Simple Error Reporting\n", .{});

    // Create a controlled test that avoids lexer and only tests the error reporter
    {
        var reporter = try ErrorReporter.init(allocator);
        defer reporter.deinit();

        // Report a simple static error message
        try reporter.reportError(LexerErrorType.InvalidCharacter, 1, 5, "test error", .{});

        // Get the errors without formatting
        const errors = reporter.getErrors();
        try std.io.getStdOut().writer().print("  Found {d} errors\n", .{errors.len});

        // DO NOT get formatted errors in this test to isolate the issue
    }

    // Now test only formatting with a fresh reporter
    {
        var reporter = try ErrorReporter.init(allocator);
        defer reporter.deinit();

        // Report a simple static error message
        try reporter.reportError(LexerErrorType.InvalidCharacter, 1, 5, "test error", .{});

        // First get unformatted errors
        const errors = reporter.getErrors();
        try std.io.getStdOut().writer().print("  Found {d} errors before formatting\n", .{errors.len});

        // Now get formatted errors
        const formatted = try reporter.getFormattedErrors();
        defer {
            for (formatted) |msg| {
                allocator.free(msg);
            }
            allocator.free(formatted);
        }

        try std.io.getStdOut().writer().print("  Got {d} formatted messages: {s}\n", .{ formatted.len, formatted[0] });
    }

    try std.io.getStdOut().writer().print("\n", .{});
}

fn testLexerErrorPattern(allocator: std.mem.Allocator) !void {
    try std.io.getStdOut().writer().print("Test: Lexer Error Pattern - Step by Step\n", .{});

    // Phase 1: Just create a lexer and parse tokens
    {
        try std.io.getStdOut().writer().print("  Phase 1: Basic token handling\n", .{});
        const source = "let @ = 42";
        var lexer = try Lexer.init(allocator, source);
        defer lexer.deinit();

        var token_count: usize = 0;
        while (true) {
            const tok = try lexer.nextToken();
            token_count += 1;
            if (tok.type == .EOF) break;

            // No deinit, just let the lexer handle it
        }

        try std.io.getStdOut().writer().print("    Processed {d} tokens\n", .{token_count});
    }

    // Phase 2: Check if errors exist
    {
        try std.io.getStdOut().writer().print("  Phase 2: Checking for errors\n", .{});
        const source = "let @ = 42";
        var lexer = try Lexer.init(allocator, source);
        defer lexer.deinit();

        // Process all tokens
        while (true) {
            const tok = try lexer.nextToken();
            if (tok.type == .EOF) break;
        }

        // Just check if we have errors without accessing them
        if (lexer.hasErrors()) {
            try std.io.getStdOut().writer().print("    Found errors\n", .{});
        } else {
            try std.io.getStdOut().writer().print("    No errors found\n", .{});
        }
    }

    // Phase 3: Access error information
    {
        try std.io.getStdOut().writer().print("  Phase 3: Accessing error information\n", .{});
        const source = "let @ = 42";
        var lexer = try Lexer.init(allocator, source);
        defer lexer.deinit();

        // Process all tokens
        while (true) {
            const tok = try lexer.nextToken();
            if (tok.type == .EOF) break;
        }

        // Check error count
        if (lexer.hasErrors()) {
            const errors = lexer.getErrors();
            try std.io.getStdOut().writer().print("    Found {d} lexer errors\n", .{errors.len});
        } else {
            try std.io.getStdOut().writer().print("    No errors found\n", .{});
        }
    }

    // Phase 4: Get formatted errors (likely the problem)
    {
        try std.io.getStdOut().writer().print("  Phase 4: Accessing formatted errors\n", .{});
        const source = "let @ = 42";
        var lexer = try Lexer.init(allocator, source);
        defer lexer.deinit();

        // Process all tokens
        while (true) {
            const tok = try lexer.nextToken();
            if (tok.type == .EOF) break;
        }

        // Get formatted errors
        if (lexer.hasErrors()) {
            try std.io.getStdOut().writer().print("    Getting formatted errors...\n", .{});
            const formatted = try lexer.getFormattedErrors();
            // Make sure to free them
            defer {
                for (formatted) |msg| {
                    allocator.free(msg);
                }
                allocator.free(formatted);
            }

            try std.io.getStdOut().writer().print("    First error: {s}\n", .{formatted[0]});
        }
    }

    try std.io.getStdOut().writer().print("\n", .{});
}
