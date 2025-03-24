const std = @import("std");
const semantic = @import("semantic.zig");
const type_checker = @import("type_checker.zig");
const type_checker_impl = @import("type_checker_impl.zig");
const parser = @import("../../frontend/parser/zig/parser.zig");

/// Main entry point for semantic analysis
pub fn analyzeProgram(allocator: std.mem.Allocator, program: *parser.ast.Program) !bool {
    const checker = try semantic.init(allocator);
    defer semantic.deinit(checker);

    return semantic.analyze(checker, program);
}

/// Print type checking errors
pub fn printErrors(checker: *semantic.TypeChecker) void {
    semantic.printErrors(checker);
}

/// Get type checking errors
pub fn getErrors(checker: *semantic.TypeChecker) [][]const u8 {
    return semantic.getErrors(checker);
}

/// Check if type checking succeeded
pub fn hasErrors(checker: *semantic.TypeChecker) bool {
    return semantic.hasErrors(checker);
} 