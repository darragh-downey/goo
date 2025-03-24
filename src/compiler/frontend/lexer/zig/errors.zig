const std = @import("std");

/// Enum representing the different types of errors that can occur during lexing.
pub const LexerErrorType = enum {
    /// An unexpected character was encountered
    InvalidCharacter,

    /// A malformed number literal was encountered (hex, octal, binary, decimal, float)
    InvalidNumber,

    /// A malformed identifier was encountered
    InvalidIdentifier,

    /// A string literal wasn't properly closed
    UnterminatedString,

    /// A block comment wasn't properly closed
    UnterminatedComment,

    /// Token that couldn't be recognized
    UnknownToken,

    /// Miscellaneous errors
    Other,

    /// Get a human-readable string representing the error type
    pub fn toString(self: LexerErrorType) []const u8 {
        return switch (self) {
            .InvalidCharacter => "Invalid character",
            .InvalidNumber => "Invalid number",
            .InvalidIdentifier => "Invalid identifier",
            .UnterminatedString => "Unterminated string",
            .UnterminatedComment => "Unterminated comment",
            .UnknownToken => "Unknown token",
            .Other => "Error",
        };
    }
};

/// Set of errors that can occur during lexing operations
pub const LexerError = error{
    InvalidCharacter,
    InvalidNumber,
    InvalidIdentifier,
    UnterminatedString,
    UnterminatedComment,
    InvalidEscapeSequence,
    OutOfMemory,
};

/// A struct containing information about a lexer error
pub const ErrorInfo = struct {
    /// The type of error
    error_type: LexerErrorType,

    /// The line number where the error occurred
    line: usize,

    /// The column number where the error occurred
    column: usize,

    /// The error message
    message: []const u8,

    /// Allocator used for the message
    allocator: std.mem.Allocator,

    /// Free resources used by this error
    pub fn deinit(self: *ErrorInfo) void {
        // Add a safety check to prevent double-free
        // If the message is empty, it might be a static string or already freed
        if (self.message.len > 0) {
            // Mark that we've freed the message by setting it to an empty slice
            const message = self.message;
            self.message = "";
            self.allocator.free(message);
        }
    }
};

/// A struct for collecting and reporting lexer errors
pub const ErrorReporter = struct {
    /// List of errors
    errors: std.ArrayList(ErrorInfo),

    /// Allocator used for error messages
    allocator: std.mem.Allocator,

    /// Initialize a new error reporter
    pub fn init(allocator: std.mem.Allocator) !ErrorReporter {
        return ErrorReporter{
            .errors = std.ArrayList(ErrorInfo).init(allocator),
            .allocator = allocator,
        };
    }

    /// Free resources used by the error reporter
    pub fn deinit(self: *ErrorReporter) void {
        for (self.errors.items) |*err| {
            err.deinit();
        }
        self.errors.deinit();
    }

    /// Report an error with a formatted message
    pub fn reportError(
        self: *ErrorReporter,
        error_type: LexerErrorType,
        line: usize,
        column: usize,
        comptime fmt: []const u8,
        args: anytype,
    ) !void {
        // Create a copy of the message
        const message = try std.fmt.allocPrint(self.allocator, fmt, args);
        errdefer self.allocator.free(message);

        // Add to our list of errors
        try self.errors.append(ErrorInfo{
            .error_type = error_type,
            .line = line,
            .column = column,
            .message = message,
            .allocator = self.allocator,
        });
    }

    /// Check if any errors have been reported
    pub fn hasErrors(self: *const ErrorReporter) bool {
        return self.errors.items.len > 0;
    }

    /// Get the list of errors
    pub fn getErrors(self: *const ErrorReporter) []const ErrorInfo {
        return self.errors.items;
    }

    /// Get formatted error messages
    pub fn getFormattedErrors(self: *const ErrorReporter) ![][]const u8 {
        // Allocate an array for the formatted error messages
        var messages = try self.allocator.alloc([]const u8, self.errors.items.len);
        errdefer {
            for (0..self.errors.items.len) |i| {
                if (i < messages.len) {
                    // Only free messages we've successfully allocated
                    self.allocator.free(messages[i]);
                }
            }
            self.allocator.free(messages);
        }

        // Format each error message
        for (self.errors.items, 0..) |err, i| {
            messages[i] = try std.fmt.allocPrint(
                self.allocator,
                "[{s}] at line {d}, column {d}: {s}",
                .{
                    err.error_type.toString(),
                    err.line,
                    err.column,
                    err.message,
                },
            );
        }

        return messages;
    }
};

/// Create a formatted error message
pub fn formatError(
    allocator: std.mem.Allocator,
    line: usize,
    column: usize,
    error_type: LexerErrorType,
    message: []const u8,
) ![]const u8 {
    return try std.fmt.allocPrint(
        allocator,
        "Line {d}, Column {d}: {s}: {s}",
        .{ line, column, @tagName(error_type), message },
    );
}
