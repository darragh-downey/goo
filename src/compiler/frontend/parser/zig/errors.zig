const std = @import("std");

/// Error reporter for parser errors
pub const ErrorReporter = struct {
    allocator: std.mem.Allocator,
    errors: std.ArrayList([]const u8),

    pub fn init(allocator: std.mem.Allocator) !ErrorReporter {
        return ErrorReporter{
            .allocator = allocator,
            .errors = std.ArrayList([]const u8).init(allocator),
        };
    }

    pub fn deinit(self: *ErrorReporter) void {
        for (self.errors.items) |err| {
            self.allocator.free(err);
        }
        self.errors.deinit();
    }

    pub fn addError(self: *ErrorReporter, message: []const u8) !void {
        const msg_copy = try self.allocator.dupe(u8, message);
        try self.errors.append(msg_copy);
    }

    pub fn hasErrors(self: *const ErrorReporter) bool {
        return self.errors.items.len > 0;
    }

    pub fn getErrorCount(self: *const ErrorReporter) usize {
        return self.errors.items.len;
    }

    pub fn getErrors(self: *const ErrorReporter) []const []const u8 {
        return self.errors.items;
    }
};

/// Common parser errors
pub const ParserError = error{
    InvalidSyntax,
    UnexpectedToken,
    ExpectedIdentifier,
    ExpectedExpression,
    ExpectedDeclaration,
    ExpectedType,
    ExpectedOperator,
    ExpectedStatement,
    InvalidAssignmentTarget,
    MissingClosingBrace,
    MissingClosingParen,
    MissingClosingBracket,
    MissingColon,
    MissingSemicolon,
    MissingEquals,
    MissingComma,
    UnsupportedFeature,
    MalformedTypeParameter,
    InvalidGenericConstraint,
    InvalidTypeAssertion,
    MissingTypeArgument,
    TypeMismatch,
    InvalidInterfaceSpec,
    InvalidImportPath,
    InvalidOperand,
    OutOfMemory,
};
