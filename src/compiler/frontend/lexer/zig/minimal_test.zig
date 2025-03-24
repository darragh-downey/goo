const std = @import("std");
const Lexer = @import("lexer.zig").Lexer;
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const TokenValue = @import("token.zig").TokenValue;

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    // Skip GPA deinit to avoid crashes
    // defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Print header
    try std.io.getStdOut().writer().print("=== Minimal Lexer Test (No Cleanup) ===\n\n", .{});

    // Test processing a valid input with no error handling
    {
        try std.io.getStdOut().writer().print("Test: Valid Input Lexing\n", .{});

        // Use a source code with no errors expected
        const source = "let x = 42";
        var lexer = try Lexer.init(allocator, source);
        // Skip lexer cleanup
        // defer lexer.deinit();

        try std.io.getStdOut().writer().print("  Lexing tokens one at a time...\n", .{});

        // Process all tokens without storing them
        var token_count: usize = 0;
        while (true) {
            const tok = try lexer.nextToken();
            token_count += 1;
            try std.io.getStdOut().writer().print("    Token {d}: {s}\n", .{ token_count, @tagName(tok.type) });

            // DO NOT deinit tokens - this is a memory leak but helps diagnose

            if (tok.type == .EOF) break;
        }

        try std.io.getStdOut().writer().print("  Processed {d} tokens\n", .{token_count});
    }

    try std.io.getStdOut().writer().print("\nTest complete (with memory leaks)\n", .{});
}
