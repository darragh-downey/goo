// Main entry point for semantic analysis module
pub const type_checker = @import("type_checker.zig");
pub const type_checker_impl = @import("type_checker_impl.zig");

// Export the type checker types
pub const TypeChecker = type_checker.TypeChecker;
pub const TypeCheckError = type_checker.TypeCheckError;
pub const Type = type_checker.Type;
pub const TypeKind = type_checker.TypeKind;
pub const Symbol = type_checker.Symbol;
pub const SymbolType = type_checker.SymbolType;
pub const SymbolTable = type_checker.SymbolTable;

/// Initialize the semantic analysis module
pub fn init(allocator: @import("std").mem.Allocator) !*TypeChecker {
    // Create a new type checker instance
    return TypeChecker.init(allocator);
}

/// Perform semantic analysis on an AST program
pub fn analyze(checker: *TypeChecker, program: *@import("../../frontend/parser/zig/parser.zig").ast.Program) !bool {
    // Perform type checking on the AST
    return checker.checkProgram(program);
}

/// Clean up semantic analysis resources
pub fn deinit(checker: *TypeChecker) void {
    checker.deinit();
}

/// Get type checking errors
pub fn getErrors(checker: *TypeChecker) [][]const u8 {
    return checker.errors.items;
}

/// Print type checking errors
pub fn printErrors(checker: *TypeChecker) void {
    for (checker.errors.items) |error_msg| {
        @import("std").debug.print("{s}\n", .{error_msg});
    }
}

/// Check if type checking succeeded
pub fn hasErrors(checker: *TypeChecker) bool {
    return checker.errors.items.len > 0;
}
