const std = @import("std");

/// TokenType represents all possible token types in the Goo language
pub const TokenType = enum {
    // Special tokens
    ERROR,
    EOF,

    // Operators
    PLUS, // +
    MINUS, // -
    ASTERISK, // *
    SLASH, // /
    PERCENT, // %
    MODULO, // % (alias for PERCENT for better naming)
    ASSIGN, // =
    PLUS_ASSIGN, // +=
    MINUS_ASSIGN, // -=
    ASTERISK_ASSIGN, // *=
    SLASH_ASSIGN, // /=
    MODULO_ASSIGN, // %=
    BANG, // !
    EQ, // ==
    NOT_EQ, // != (Updated to match parser.zig usage)
    LT, // <
    GT, // >
    LT_EQ, // <= (Updated to match parser.zig usage)
    GT_EQ, // >= (Updated to match parser.zig usage)
    AND, // &&
    OR, // ||
    BITAND, // &
    BITOR, // |
    BITXOR, // ^
    BITNOT, // ~
    LSHIFT, // <<
    RSHIFT, // >>
    DOT, // .
    RANGE, // ..
    ARROW, // ->
    DECLARE_ASSIGN, // :=

    // Delimiters
    COMMA, // ,
    SEMICOLON, // ;
    COLON, // :
    LPAREN, // (
    RPAREN, // )
    LBRACE, // {
    RBRACE, // }
    LBRACKET, // [
    RBRACKET, // ]

    // Literals
    IDENTIFIER, // variable name, function name, etc.
    INT_LITERAL, // 42
    FLOAT_LITERAL, // 3.14
    STRING_LITERAL, // "hello"

    // Keywords - control flow
    IF,
    ELSE,
    FOR,
    RETURN,
    SWITCH,
    CASE,
    DEFAULT,
    BREAK,
    CONTINUE,

    // Keywords - declarations
    PACKAGE,
    IMPORT,
    FUNC,
    VAR,
    CONST,
    TYPE, // Changed from MODULE to TYPE to match Go conventions

    // Keywords - Goo extensions
    COMPTIME, // For compile-time features
    GO, // Original Go keyword
    GO_PARALLEL, // Extend Go with parallel construct
    GO_DISTRIBUTED, // Extend Go with distributed construct
    IMPL, // For explicit interface implementations
    ASYNC, // For asynchronous operations
    AWAIT, // For awaiting async operations

    // Keywords - parallel execution
    PARALLEL,
    CHAN,
    SUPERVISE,
    BUILD,
    ACTOR, // Erlang-inspired actors
    CLUSTER, // Distributed systems support
    DISTRIBUTE, // Distributed computing keyword

    // Keywords - memory management
    ALLOCATOR,
    ALLOC,
    FREE,
    SCOPE,
    HEAP,
    ARENA,
    FIXED,
    POOL,
    BUMP,
    CUSTOM,
    DEFER, // For deferred cleanup
    ENSURE, // Guaranteed execution

    // Keywords - error handling
    TRY,
    CATCH,
    RECOVER,
    SUPER,
    PANIC,

    // Keywords - memory safety
    SAFE,
    UNSAFE,
    OWNER, // Ownership semantics
    BORROW, // Borrowing semantics
    MOVE, // Move semantics

    // Keywords - runtime mode
    KERNEL,
    USER,

    // Keywords - access control
    CAP,
    SHARED,
    PRIVATE,
    PUBLIC,
    PROTECTED,
    REFLECT,

    // Keywords - vectorization
    SIMD,
    VECTOR,
    ALIGNED,
    MASK,
    FUSED,
    AUTO,
    ARCH,

    // Keywords - types
    TRUE,
    FALSE,
    INT_TYPE,
    INT8_TYPE,
    INT16_TYPE,
    INT32_TYPE,
    INT64_TYPE,
    UINT_TYPE,
    UINT8_TYPE,
    UINT16_TYPE,
    UINT32_TYPE,
    UINT64_TYPE,
    FLOAT32_TYPE,
    FLOAT64_TYPE,
    COMPLEX64_TYPE,
    COMPLEX128_TYPE,
    BOOL_TYPE,
    STRING_TYPE,
    BYTE_TYPE,
    RUNE_TYPE,
    ANY_TYPE,
    MAYBE, // Optional types
    ENUM, // Enumeration types
    UNION, // Union types
    SUM, // Sum types for algebraic data types
    STRUCT, // Struct declaration
    INTERFACE, // Interface declaration
    MAP, // Map type

    // Keywords - channel patterns
    PUB,
    SUB,
    PUSH,
    PULL,
    REQ,
    REP,
    DEALER,
    ROUTER,
    PAIR,

    // Keywords - pattern matching
    MATCH, // For pattern matching
    PATTERN, // Pattern definition
    AS, // Type assertion
};

/// TokenValue represents the possible values a token can have
pub const TokenValue = union(enum) {
    none_val: void,
    string_val: []const u8,
    integer_val: i64,
    float_val: f64,
    boolean_val: bool,

    // Add type aliases for clearer parameter types
    pub const StringVal = []const u8;
    pub const IntegerVal = i64;
    pub const FloatVal = f64;
    pub const BooleanVal = bool;

    // Validation errors
    pub const ValidationError = error{
        InvalidString,
        InvalidInteger,
        InvalidFloat,
        InvalidBoolean,
    };

    /// Create an empty token value (for tokens without values)
    pub fn none() TokenValue {
        return TokenValue{ .none_val = {} };
    }

    /// Create a string token value
    pub fn string(s: StringVal) TokenValue {
        return TokenValue{ .string_val = s };
    }

    /// Create an integer token value
    pub fn integer(i: IntegerVal) TokenValue {
        return TokenValue{ .integer_val = i };
    }

    /// Create a float token value
    pub fn float(f: FloatVal) TokenValue {
        return TokenValue{ .float_val = f };
    }

    /// Create a boolean token value
    pub fn boolean(b: BooleanVal) TokenValue {
        return TokenValue{ .boolean_val = b };
    }

    /// Free resources associated with this token value
    pub fn deinit(self: TokenValue, allocator: std.mem.Allocator) void {
        // ONLY free memory for string value tokens with non-empty content
        if (self == .string_val) {
            const str = self.string_val;

            // Very conservative approach - only free non-empty strings
            if (str.len == 0) {
                return; // Skip cleanup for empty strings
            }

            // CAREFULLY free the string
            allocator.free(str);
        }
        // All other token types have no resources that need freeing
    }

    /// Validates that the token value matches the expected type
    pub fn validate(self: TokenValue, expected_type: TokenType) ValidationError!void {
        switch (expected_type) {
            .STRING_LITERAL => {
                if (self != .string_val) return ValidationError.InvalidString;
            },
            .INT_LITERAL => {
                if (self != .integer_val) return ValidationError.InvalidInteger;
            },
            .FLOAT_LITERAL => {
                if (self != .float_val) return ValidationError.InvalidFloat;
            },
            .TRUE, .FALSE => {
                if (self != .boolean_val) return ValidationError.InvalidBoolean;
            },
            else => {},
        }
    }

    /// Strongly typed accessor for string value
    pub fn getString(self: TokenValue) ?StringVal {
        return switch (self) {
            .string_val => |s| s,
            else => null,
        };
    }

    /// Strongly typed accessor for integer value
    pub fn getInteger(self: TokenValue) ?IntegerVal {
        return switch (self) {
            .integer_val => |i| i,
            else => null,
        };
    }

    /// Strongly typed accessor for float value
    pub fn getFloat(self: TokenValue) ?FloatVal {
        return switch (self) {
            .float_val => |f| f,
            else => null,
        };
    }

    /// Strongly typed accessor for boolean value
    pub fn getBoolean(self: TokenValue) ?BooleanVal {
        return switch (self) {
            .boolean_val => |b| b,
            else => null,
        };
    }
};

/// Token represents a lexical token in the Goo language
pub const Token = struct {
    type: TokenType,
    value: TokenValue,
    line: usize,
    column: usize,

    /// Create a new token with the given type, line, column and value
    pub fn init(token_type: TokenType, line: usize, column: usize, value: TokenValue) Token {
        return Token{
            .type = token_type,
            .value = value,
            .line = line,
            .column = column,
        };
    }

    /// Create a new token with validated value
    pub fn initValidated(token_type: TokenType, line: usize, column: usize, value: TokenValue) !Token {
        try value.validate(token_type);
        return init(token_type, line, column, value);
    }

    /// Free resources associated with this token
    pub fn deinit(self: Token, allocator: std.mem.Allocator) void {
        // Skip deinitialization for strings that aren't dynamically allocated
        if (self.type == .IDENTIFIER) {
            // Do not free IDENTIFIER token strings, they're slices from the source
            return;
        }

        // Skip deinitialization for token types that have been problematic
        if (self.type == .ERROR) {
            return; // Just leak ERROR tokens rather than crash
        }

        // For STRING_LITERAL tokens, be extra cautious
        if (self.type == .STRING_LITERAL) {
            const val = self.value;
            if (val == .string_val) {
                const str = val.string_val;

                // ONLY free non-empty string literals
                if (str.len > 0) {
                    allocator.free(str);
                }
            }
            return;
        }

        // For all other token types, use normal deinit
        self.value.deinit(allocator);
    }

    /// Get a string representation of the token type
    pub fn getTypeName(self: Token) []const u8 {
        return @tagName(self.type);
    }

    /// Get a string representation of the token value
    pub fn getValueString(self: Token, allocator: std.mem.Allocator) ![]const u8 {
        switch (self.value) {
            .none_val => return try allocator.dupe(u8, ""),
            .string_val => |s| return try allocator.dupe(u8, s),
            .integer_val => |i| return try std.fmt.allocPrint(allocator, "{d}", .{i}),
            .float_val => |f| return try std.fmt.allocPrint(allocator, "{d}", .{f}),
            .boolean_val => |b| return try allocator.dupe(u8, if (b) "true" else "false"),
        }
    }

    /// Get a string representation of the token
    pub fn format(self: Token, allocator: std.mem.Allocator) ![]const u8 {
        const value_str = try self.getValueString(allocator);
        defer allocator.free(value_str);

        return try std.fmt.allocPrint(allocator, "Type: {s}, Value: {s}, Line: {d}, Column: {d}", .{ self.getTypeName(), value_str, self.line, self.column });
    }

    /// Check if a token is of a specific type
    pub fn isType(self: Token, expected_type: TokenType) bool {
        return self.type == expected_type;
    }

    /// Check if a token is a literal (int, float, string)
    pub fn isLiteral(self: Token) bool {
        return switch (self.type) {
            .INT_LITERAL, .FLOAT_LITERAL, .STRING_LITERAL => true,
            else => false,
        };
    }

    /// Check if a token is an operator
    pub fn isOperator(self: Token) bool {
        return switch (self.type) {
            .PLUS, .MINUS, .ASTERISK, .SLASH, .PERCENT, .ASSIGN, .PLUS_ASSIGN, .MINUS_ASSIGN, .ASTERISK_ASSIGN, .SLASH_ASSIGN, .BANG, .EQ, .NOT_EQ, .LT, .GT, .LT_EQ, .GT_EQ, .AND, .OR, .BITAND, .BITOR, .BITXOR, .BITNOT, .LSHIFT, .RSHIFT => true,
            else => false,
        };
    }

    /// Check if a token is a keyword
    pub fn isKeyword(self: Token) bool {
        return switch (self.type) {
            .IF, .ELSE, .FOR, .RETURN, .SWITCH, .CASE, .DEFAULT, .BREAK, .CONTINUE, .PACKAGE, .IMPORT, .FUNC, .VAR, .CONST, .TYPE => true,

            // Also add Goo-specific keywords
            .COMPTIME, .GO, .GO_PARALLEL, .GO_DISTRIBUTED, .IMPL, .ASYNC, .AWAIT, .PARALLEL, .CHAN, .SUPERVISE, .ACTOR, .CLUSTER, .DISTRIBUTE => true,

            else => switch (self.type) {
                // Memory management keywords
                .ALLOCATOR, .ALLOC, .FREE, .SCOPE, .HEAP, .ARENA, .FIXED, .POOL, .BUMP, .CUSTOM, .DEFER, .ENSURE => true,

                else => false,
            },
        };
    }

    /// Check if a token is an identifier
    pub fn isIdentifier(self: Token) bool {
        return self.type == .IDENTIFIER;
    }
};
