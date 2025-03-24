const std = @import("std");
const testing = std.testing;
const semantic = @import("semantic.zig");
const semantic_analyzer = @import("semantic_analyzer.zig");
const parser = @import("../../frontend/parser/zig/parser.zig");

fn parseAndAnalyze(allocator: std.mem.Allocator, source: []const u8) !bool {
    // Parse the source code
    var p = try parser.Parser.init(allocator, source);
    defer p.deinit();

    const program = try p.parseProgram();
    defer program.deinit();

    // Run semantic analysis
    return semantic_analyzer.analyzeProgram(allocator, program);
}

test "simple variable declaration" {
    const source =
        \\package main
        \\
        \\func main() {
        \\  var x int = 42
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(result);
}

test "type mismatch in variable declaration" {
    const source =
        \\package main
        \\
        \\func main() {
        \\  var x int = "hello"
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(!result);
}

test "function parameters and return type" {
    const source =
        \\package main
        \\
        \\func add(a int, b int) int {
        \\  return a + b
        \\}
        \\
        \\func main() {
        \\  var result int = add(1, 2)
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(result);
}

test "return type mismatch" {
    const source =
        \\package main
        \\
        \\func getValue() int {
        \\  return "not an int"
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(!result);
}

test "implicit return type" {
    const source =
        \\package main
        \\
        \\func getGreeting() {
        \\  return "hello"
        \\}
        \\
        \\func main() {
        \\  var greeting = getGreeting()
        \\}
    ;

    // This should pass because return type is inferred
    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(result);
}

test "if statement condition" {
    const source =
        \\package main
        \\
        \\func main() {
        \\  var x int = 42
        \\  if x > 0 {
        \\    return
        \\  }
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(result);
}

test "invalid if condition" {
    const source =
        \\package main
        \\
        \\func main() {
        \\  var name string = "John"
        \\  if name {
        \\    return
        \\  }
        \\}
    ;

    const result = try parseAndAnalyze(testing.allocator, source);
    try testing.expect(!result);
}

pub fn main() !void {
    std.debug.print("Running semantic analyzer tests...\n", .{});

    const result = try parseAndAnalyze(std.heap.page_allocator,
        \\package main
        \\
        \\func fibonacci(n int) int {
        \\  if n <= 1 {
        \\    return n
        \\  }
        \\  return fibonacci(n-1) + fibonacci(n-2)
        \\}
        \\
        \\func main() {
        \\  var result int = fibonacci(10)
        \\}
    );

    if (result) {
        std.debug.print("Semantic analysis passed!\n", .{});
    } else {
        std.debug.print("Semantic analysis failed!\n", .{});
    }
}
