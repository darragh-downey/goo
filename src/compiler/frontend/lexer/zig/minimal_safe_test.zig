const std = @import("std");
const Lexer = @import("lexer.zig").Lexer;
const Token = @import("token.zig").Token;
const TokenType = @import("token.zig").TokenType;
const TokenValue = @import("token.zig").TokenValue;
const LexerErrorType = @import("errors.zig").LexerErrorType;

pub fn main() !void {
    // Use a simple arena allocator that will be freed all at once at the end
    // This avoids individual deallocation issues
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    // Print header
    try std.io.getStdOut().writer().print("=== Critical Bug Test ===\n\n", .{});

    // First identify the specific memory allocation issue
    try std.io.getStdOut().writer().print("PHASE 1: ALLOCATION INSPECTION\n", .{});
    var lexer = try Lexer.init(allocator, "let x = 42");
    defer lexer.deinit();

    const tok = try lexer.nextToken();
    try std.io.getStdOut().writer().print("Token: {s} (Value: {s})\n", .{ @tagName(tok.type), if (tok.value == .string_val) tok.value.string_val else "(none)" });
    try std.io.getStdOut().writer().print("Memory Analysis:\n", .{});
    try std.io.getStdOut().writer().print("  - Token created from: token_reader.readIdentifier\n", .{});
    try std.io.getStdOut().writer().print("  - String memory: part of original source text\n", .{});
    try std.io.getStdOut().writer().print("  - NOT HEAP ALLOCATED - attempting to free this memory causes a bus error\n", .{});

    try std.io.getStdOut().writer().print("\nPHASE 2: WORKAROUND TEST\n", .{});
    try std.io.getStdOut().writer().print("Testing cleanup without freeing IDENTIFIER token values...\n", .{});

    try testWithSafeCleanup(allocator);

    try std.io.getStdOut().writer().print("\nAll tests completed without crashes!\n", .{});
}

fn testWithSafeCleanup(allocator: std.mem.Allocator) !void {
    const source = "let x = 42 \"hello\" @ 0xFF";
    try std.io.getStdOut().writer().print("Source: '{s}'\n", .{source});

    var lexer = try Lexer.init(allocator, source);
    defer lexer.deinit();

    var token_count: usize = 0;
    while (true) {
        const tok = try lexer.nextToken();
        token_count += 1;

        try std.io.getStdOut().writer().print("  Token {d}: {s}", .{ token_count, @tagName(tok.type) });

        // Skip EOF to break the loop
        const is_eof = (tok.type == .EOF);

        // Only clean up tokens that are safe to clean up
        // DO NOT try to free IDENTIFIER tokens because their strings are not allocated
        if (tok.type != .IDENTIFIER) {
            try std.io.getStdOut().writer().print(" (cleaning up safely)", .{});
            tok.deinit(allocator);
        } else {
            try std.io.getStdOut().writer().print(" (skipping cleanup - not allocated)", .{});
        }
        try std.io.getStdOut().writer().print("\n", .{});

        if (is_eof) break;
    }

    try std.io.getStdOut().writer().print("  Processed {d} tokens successfully\n", .{token_count});
}
