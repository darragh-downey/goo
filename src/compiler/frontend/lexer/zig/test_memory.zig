const std = @import("std");
const lexer_core = @import("lexer_core.zig");
const token = @import("token.zig");

const Lexer = lexer_core.Lexer;
const Token = token.Token;
const TokenValue = token.TokenValue;

/// Test memory management with string literals
pub fn main() !void {
    var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
    const gpa = general_purpose_allocator.allocator();
    defer {
        const deinit_status = general_purpose_allocator.deinit();
        if (deinit_status == .leak) {
            std.debug.print("Memory leaks detected!\n", .{});
            std.process.exit(1);
        } else {
            std.debug.print("No memory leaks detected.\n", .{});
        }
    }

    try std.io.getStdOut().writer().print("=== Goo Lexer Memory Management Test ===\n\n", .{});

    // Test basic string handling
    try std.io.getStdOut().writer().print("Test: Basic String Handling\n", .{});
    {
        const source = "\"Hello, world!\"";
        var lexer = try Lexer.init(gpa, source);
        defer lexer.deinit();

        // Get and deinit token
        var tok = try lexer.nextToken();
        defer tok.deinit(gpa);

        try std.io.getStdOut().writer().print("  Token type: {s}\n", .{tok.getTypeName()});
        const value_str = try tok.getValueString(gpa);
        defer gpa.free(value_str);
        try std.io.getStdOut().writer().print("  Token value: {s}\n", .{value_str});

        // Check for errors
        if (lexer.hasErrors()) {
            const errors = try lexer.getFormattedErrors();
            defer {
                for (errors) |err| {
                    gpa.free(err);
                }
                gpa.free(errors);
            }
            try std.io.getStdOut().writer().print("  Error count: {d}\n", .{errors.len});
            for (errors, 0..) |err, i| {
                try std.io.getStdOut().writer().print("  Error {d}: {s}\n", .{ i + 1, err });
            }
        } else {
            try std.io.getStdOut().writer().print("  No errors reported\n", .{});
        }
    }

    // Test escape sequences
    try std.io.getStdOut().writer().print("\nTest: String Escape Sequences\n", .{});
    {
        const source = "\"Hello\\nWorld\\t\\\"Escaped\\\"\"";
        var lexer = try Lexer.init(gpa, source);
        defer lexer.deinit();

        var tok = try lexer.nextToken();
        defer tok.deinit(gpa);

        try std.io.getStdOut().writer().print("  Token type: {s}\n", .{tok.getTypeName()});
        const value_str = try tok.getValueString(gpa);
        defer gpa.free(value_str);
        try std.io.getStdOut().writer().print("  Token value: {s}\n", .{value_str});

        // Check for errors
        if (lexer.hasErrors()) {
            const errors = try lexer.getFormattedErrors();
            defer {
                for (errors) |err| {
                    gpa.free(err);
                }
                gpa.free(errors);
            }
            try std.io.getStdOut().writer().print("  Error count: {d}\n", .{errors.len});
            for (errors, 0..) |err, i| {
                try std.io.getStdOut().writer().print("  Error {d}: {s}\n", .{ i + 1, err });
            }
        } else {
            try std.io.getStdOut().writer().print("  No errors reported\n", .{});
        }
    }

    // Test error cases
    try std.io.getStdOut().writer().print("\nTest: Error Cases\n", .{});
    {
        const source = "\"Unterminated string";
        var lexer = try Lexer.init(gpa, source);
        defer lexer.deinit();

        var tok = try lexer.nextToken();
        defer tok.deinit(gpa);

        try std.io.getStdOut().writer().print("  Token type: {s}\n", .{tok.getTypeName()});
        const value_str = try tok.getValueString(gpa);
        defer gpa.free(value_str);
        try std.io.getStdOut().writer().print("  Token value: {s}\n", .{value_str});

        // Check for errors
        if (lexer.hasErrors()) {
            const errors = try lexer.getFormattedErrors();
            defer {
                for (errors) |err| {
                    gpa.free(err);
                }
                gpa.free(errors);
            }
            try std.io.getStdOut().writer().print("  Error count: {d}\n", .{errors.len});
            for (errors, 0..) |err, i| {
                try std.io.getStdOut().writer().print("  Error {d}: {s}\n", .{ i + 1, err });
            }
        } else {
            try std.io.getStdOut().writer().print("  No errors reported\n", .{});
        }
    }

    // Test multiple tokens
    try std.io.getStdOut().writer().print("\nTest: Multiple Tokens\n", .{});
    {
        const source = "\"First\" \"Second\" \"Third\"";
        var lexer = try Lexer.init(gpa, source);
        defer lexer.deinit();

        for (0..3) |i| {
            var tok = try lexer.nextToken();
            defer tok.deinit(gpa);

            try std.io.getStdOut().writer().print("  Token {d} type: {s}\n", .{ i + 1, tok.getTypeName() });
            const value_str = try tok.getValueString(gpa);
            defer gpa.free(value_str);
            try std.io.getStdOut().writer().print("  Token {d} value: {s}\n", .{ i + 1, value_str });
        }
    }

    try std.io.getStdOut().writer().print("\nAll memory tests completed.\n", .{});
}
