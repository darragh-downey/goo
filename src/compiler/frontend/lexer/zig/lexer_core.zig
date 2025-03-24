const std = @import("std");
const token = @import("token.zig");
const token_reader = @import("token_reader.zig");
const char_utils = @import("character_utils.zig");
const keywords = @import("keywords.zig");
const errors_module = @import("errors.zig");

const Token = token.Token;
const TokenType = token.TokenType;
const TokenValue = token.TokenValue;
const LexerError = errors_module.LexerError;
const LexerErrorType = errors_module.LexerErrorType;
const ErrorReporter = errors_module.ErrorReporter;

/// Result type for lexer operations that can fail
pub const LexResult = union(enum) {
    token: Token,
    err: LexerError,

    /// Create a token result
    pub fn success(token_value: Token) LexResult {
        return LexResult{ .token = token_value };
    }

    /// Create an error result
    pub fn failure(err: LexerError) LexResult {
        return LexResult{ .err = err };
    }

    /// Unwrap the result to get the token, or return the error
    pub fn unwrap(self: LexResult) LexerError!Token {
        return switch (self) {
            .token => |t| t,
            .err => |e| e,
        };
    }
};

/// Core lexer implementation
pub const Lexer = struct {
    input: []const u8,
    position: usize,
    read_position: usize,
    ch: u8,
    line: usize,
    column: usize,
    string_buffer: std.ArrayList(u8),
    comment_nesting: usize,
    allocator: std.mem.Allocator,
    error_reporter: ErrorReporter,

    /// Initialize a new lexer with the given input
    pub fn init(allocator: std.mem.Allocator, input: []const u8) !Lexer {
        var error_reporter = try ErrorReporter.init(allocator);
        errdefer error_reporter.deinit();

        var lexer = Lexer{
            .input = input,
            .position = 0,
            .read_position = 0,
            .ch = 0,
            .line = 1,
            .column = 1,
            .string_buffer = std.ArrayList(u8).init(allocator),
            .comment_nesting = 0,
            .allocator = allocator,
            .error_reporter = error_reporter,
        };

        try lexer.readChar();
        return lexer;
    }

    /// Free any resources held by the lexer
    pub fn deinit(self: *Lexer) void {
        self.string_buffer.deinit();
        self.error_reporter.deinit();
    }

    /// Read the next character from the input
    fn readChar(self: *Lexer) !void {
        if (self.read_position >= self.input.len) {
            self.ch = 0; // End of file
        } else {
            self.ch = self.input[self.read_position];
        }

        self.position = self.read_position;
        self.read_position += 1;

        if (self.ch == '\n') {
            self.line += 1;
            self.column = 1;
        } else {
            self.column += 1;
        }
    }

    /// Peek at the next character without consuming it
    fn peekChar(self: *Lexer) u8 {
        if (self.read_position >= self.input.len) {
            return 0; // End of file
        } else {
            return self.input[self.read_position];
        }
    }

    /// Skip whitespace characters
    fn skipWhitespace(self: *Lexer) !void {
        while (char_utils.isWhitespace(self.ch)) {
            try self.readChar();
        }
    }

    /// Report an error from the lexer
    fn reportError(self: *Lexer, error_type: LexerErrorType, comptime message_fmt: []const u8, args: anytype) !void {
        try self.error_reporter.reportError(error_type, self.line, self.column, message_fmt, args);
    }

    /// Create an error token
    fn createErrorToken(self: *Lexer, start_column: usize, message: []const u8) !Token {
        try self.reportError(LexerErrorType.UnknownToken, "{s}", .{message});

        // Create a separate copy of the message that will be owned by the token
        // This avoids double-free issues with error messages
        const token_message = try self.allocator.dupe(u8, message);

        return Token.init(.ERROR, self.line, start_column, TokenValue.string(token_message));
    }

    /// Read an identifier token
    fn readIdentifier(self: *Lexer, start_column: usize) !Token {
        const start_pos = self.position;
        const reader_result = token_reader.readIdentifier(self.input, start_pos, self.line, start_column);

        // Advance the lexer position to match the end position returned by the reader
        while (self.position < reader_result.end_pos) {
            try self.readChar();
        }

        // IMPORTANT MEMORY NOTE:
        // Identifier tokens contain string slices that point to the original source.
        // These strings are NOT heap-allocated and should not be freed.
        // Token.deinit() must handle this special case to avoid a BUS error.
        return reader_result.token;
    }

    /// Read a number token (integer or float)
    fn readNumber(self: *Lexer, start_column: usize) !Token {
        const start_pos = self.position;

        const reader_result = try token_reader.readNumber(self.input, start_pos, self.line, start_column);

        // Handle errors from the token reader
        if (reader_result.error_message) |err_msg| {
            try self.reportError(LexerErrorType.InvalidNumber, "{s}", .{err_msg});
        }

        // Advance the lexer position to match the end position returned by the reader
        while (self.position < reader_result.end_pos) {
            try self.readChar();
        }

        return reader_result.token;
    }

    /// Read a string literal token
    fn readString(self: *Lexer, start_column: usize) !Token {
        const start_pos = self.position;

        const reader_result = try token_reader.readString(self.input, start_pos, self.line, start_column, self.allocator);

        // Handle errors from the token reader
        if (reader_result.error_message) |err_msg| {
            try self.reportError(LexerErrorType.UnterminatedString, "{s}", .{err_msg});

            // If result token is an ERROR token, we still need to use it because it contains the partial string
            // This string memory will be freed when the token is deallocated
        }

        // Update lexer state to match the end state returned by the reader
        while (self.position < reader_result.end_pos) {
            try self.readChar();
        }

        self.line = reader_result.new_line;
        self.column = reader_result.new_column;

        return reader_result.token;
    }

    /// Handle block comments (potentially nested)
    fn skipBlockComment(self: *Lexer) !void {
        try self.readChar(); // Skip the opening '/'
        try self.readChar(); // Skip the opening '*'

        // Track our current nesting level
        const current_nesting_level = self.comment_nesting;
        self.comment_nesting += 1;

        // Store starting position for error reporting
        const start_line = self.line;
        const start_column = self.column - 2; // Adjust for the /* we already consumed

        var found_closing = false;

        while (self.ch != 0) {
            if (self.ch == '/' and self.peekChar() == '*') {
                // Entering a nested comment
                try self.skipBlockComment();
                // We continue here since skipBlockComment advances the cursor
                continue;
            } else if (self.ch == '*' and self.peekChar() == '/') {
                // Found closing comment marker
                try self.readChar(); // Skip the '*'
                try self.readChar(); // Skip the '/'

                // Decrement nesting level
                self.comment_nesting -= 1;

                // If we're back to our original nesting level, we're done
                if (self.comment_nesting == current_nesting_level) {
                    found_closing = true;
                    break;
                }
            } else {
                try self.readChar();
            }
        }

        // If we reached the end of input without closing the comment
        if (!found_closing) {
            try self.reportError(LexerErrorType.UnterminatedComment, "Block comment starting at line {d}, column {d} was not closed", .{ start_line, start_column });

            // Restore nesting level to avoid cascading errors
            // This way nested comments that are unterminated only report one error
            self.comment_nesting = current_nesting_level;
        }
    }

    /// Handle line comments
    fn skipLineComment(self: *Lexer) !void {
        try self.readChar(); // Skip the opening '/'
        try self.readChar(); // Skip the opening '/'

        while (self.ch != 0 and self.ch != '\n') {
            try self.readChar();
        }
    }

    /// Get the next token from the input
    pub fn nextToken(self: *Lexer) !Token {
        try self.skipWhitespace();

        if (self.ch == 0) {
            return Token.init(.EOF, self.line, self.column, TokenValue.none());
        }

        const start_column = self.column;

        switch (self.ch) {
            // Single character tokens
            '=' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.EQ, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '%' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.MODULO_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.MODULO, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '+' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.PLUS_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.PLUS, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '-' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.MINUS_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else if (self.peekChar() == '>') {
                    try self.readChar();
                    const tok = Token.init(.ARROW, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.MINUS, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '*' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.ASTERISK_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.ASTERISK, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '/' => {
                if (self.peekChar() == '/') {
                    try self.skipLineComment();
                    return try self.nextToken(); // Get next token after comment
                } else if (self.peekChar() == '*') {
                    try self.skipBlockComment();
                    return try self.nextToken(); // Continue skipping
                } else if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.SLASH_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.SLASH, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '!' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.NOT_EQ, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.BANG, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '<' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.LT_EQ, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else if (self.peekChar() == '<') {
                    try self.readChar();
                    const tok = Token.init(.LSHIFT, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.LT, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '>' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.GT_EQ, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else if (self.peekChar() == '>') {
                    try self.readChar();
                    const tok = Token.init(.RSHIFT, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.GT, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            ';' => {
                const tok = Token.init(.SEMICOLON, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '(' => {
                const tok = Token.init(.LPAREN, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            ')' => {
                const tok = Token.init(.RPAREN, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            ',' => {
                const tok = Token.init(.COMMA, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '{' => {
                const tok = Token.init(.LBRACE, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '}' => {
                const tok = Token.init(.RBRACE, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '[' => {
                const tok = Token.init(.LBRACKET, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            ']' => {
                const tok = Token.init(.RBRACKET, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            ':' => {
                if (self.peekChar() == '=') {
                    try self.readChar();
                    const tok = Token.init(.DECLARE_ASSIGN, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.COLON, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '.' => {
                if (self.peekChar() == '.') {
                    try self.readChar();
                    const tok = Token.init(.RANGE, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.DOT, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '&' => {
                if (self.peekChar() == '&') {
                    try self.readChar();
                    const tok = Token.init(.AND, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.BITAND, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '|' => {
                if (self.peekChar() == '|') {
                    try self.readChar();
                    const tok = Token.init(.OR, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                } else {
                    const tok = Token.init(.BITOR, self.line, start_column, TokenValue.none());
                    try self.readChar();
                    return tok;
                }
            },
            '^' => {
                const tok = Token.init(.BITXOR, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '~' => {
                const tok = Token.init(.BITNOT, self.line, start_column, TokenValue.none());
                try self.readChar();
                return tok;
            },
            '"' => return try self.readString(start_column),

            // Handle identifiers and keywords
            else => {
                if (char_utils.isLetter(self.ch)) {
                    return try self.readIdentifier(start_column);
                } else if (char_utils.isDigit(self.ch)) {
                    return try self.readNumber(start_column);
                } else {
                    const ch = self.ch;
                    try self.readChar();

                    // Report the error
                    try self.reportError(LexerErrorType.InvalidCharacter, "unexpected character: '{c}'", .{ch});

                    // Create a message to use for the token (different allocation from the one in reportError)
                    const token_message = try std.fmt.allocPrint(self.allocator, "unexpected character: '{c}'", .{ch});

                    // Create and return an ERROR token that takes ownership of the token_message
                    return Token.init(.ERROR, self.line, start_column, TokenValue.string(token_message));
                }
            },
        }
    }

    /// Get all errors that occurred during lexing
    pub fn getErrors(self: *const Lexer) []const errors_module.ErrorInfo {
        return self.error_reporter.getErrors();
    }

    /// Check if any errors occurred during lexing
    pub fn hasErrors(self: *const Lexer) bool {
        return self.error_reporter.hasErrors();
    }

    /// Get formatted error messages
    pub fn getFormattedErrors(self: *const Lexer) ![]const []const u8 {
        return try self.error_reporter.getFormattedErrors();
    }
};
