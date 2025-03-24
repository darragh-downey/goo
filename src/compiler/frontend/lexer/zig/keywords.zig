const std = @import("std");
const token = @import("token.zig");

const TokenType = token.TokenType;

// Define a keyword mapping structure
const KeywordMapping = struct {
    keyword: []const u8,
    token_type: TokenType,
};

// Define the list of keywords
const keyword_list = [_]KeywordMapping{
    // Go keywords
    .{ .keyword = "break", .token_type = .BREAK },
    .{ .keyword = "case", .token_type = .CASE },
    .{ .keyword = "chan", .token_type = .CHAN },
    .{ .keyword = "const", .token_type = .CONST },
    .{ .keyword = "continue", .token_type = .CONTINUE },
    .{ .keyword = "default", .token_type = .DEFAULT },
    .{ .keyword = "defer", .token_type = .DEFER },
    .{ .keyword = "else", .token_type = .ELSE },
    .{ .keyword = "fallthrough", .token_type = .FALLTHROUGH },
    .{ .keyword = "for", .token_type = .FOR },
    .{ .keyword = "func", .token_type = .FUNC },
    .{ .keyword = "go", .token_type = .GO },
    .{ .keyword = "goto", .token_type = .GOTO },
    .{ .keyword = "if", .token_type = .IF },
    .{ .keyword = "import", .token_type = .IMPORT },
    .{ .keyword = "interface", .token_type = .INTERFACE },
    .{ .keyword = "map", .token_type = .MAP },
    .{ .keyword = "package", .token_type = .PACKAGE },
    .{ .keyword = "range", .token_type = .RANGE },
    .{ .keyword = "return", .token_type = .RETURN },
    .{ .keyword = "select", .token_type = .SELECT },
    .{ .keyword = "struct", .token_type = .STRUCT },
    .{ .keyword = "switch", .token_type = .SWITCH },
    .{ .keyword = "type", .token_type = .TYPE },
    .{ .keyword = "var", .token_type = .VAR },

    // Go literals
    .{ .keyword = "true", .token_type = .TRUE },
    .{ .keyword = "false", .token_type = .FALSE },
    .{ .keyword = "nil", .token_type = .NIL },

    // Go 1.18+ keywords
    .{ .keyword = "any", .token_type = .ANY_TYPE },

    // Built-in types (not keywords in Go but useful to recognize)
    .{ .keyword = "int", .token_type = .INT_TYPE },
    .{ .keyword = "int8", .token_type = .INT8_TYPE },
    .{ .keyword = "int16", .token_type = .INT16_TYPE },
    .{ .keyword = "int32", .token_type = .INT32_TYPE },
    .{ .keyword = "int64", .token_type = .INT64_TYPE },
    .{ .keyword = "uint", .token_type = .UINT_TYPE },
    .{ .keyword = "uint8", .token_type = .UINT8_TYPE },
    .{ .keyword = "uint16", .token_type = .UINT16_TYPE },
    .{ .keyword = "uint32", .token_type = .UINT32_TYPE },
    .{ .keyword = "uint64", .token_type = .UINT64_TYPE },
    .{ .keyword = "float32", .token_type = .FLOAT32_TYPE },
    .{ .keyword = "float64", .token_type = .FLOAT64_TYPE },
    .{ .keyword = "complex64", .token_type = .COMPLEX64_TYPE },
    .{ .keyword = "complex128", .token_type = .COMPLEX128_TYPE },
    .{ .keyword = "string", .token_type = .STRING_TYPE },
    .{ .keyword = "bool", .token_type = .BOOL_TYPE },
    .{ .keyword = "byte", .token_type = .BYTE_TYPE },
    .{ .keyword = "rune", .token_type = .RUNE_TYPE },

    // Goo extensions - core
    .{ .keyword = "enum", .token_type = .ENUM },
    .{ .keyword = "extend", .token_type = .EXTEND },
    .{ .keyword = "match", .token_type = .MATCH },
    .{ .keyword = "trait", .token_type = .TRAIT },
    .{ .keyword = "nullable", .token_type = .NULLABLE },
    .{ .keyword = "pattern", .token_type = .PATTERN },

    // Goo extensions - concurrency
    .{ .keyword = "go_parallel", .token_type = .GO_PARALLEL },
    .{ .keyword = "go_distributed", .token_type = .GO_DISTRIBUTED },
    .{ .keyword = "parallel", .token_type = .PARALLEL },
    .{ .keyword = "supervise", .token_type = .SUPERVISE },
    .{ .keyword = "actor", .token_type = .ACTOR },
    .{ .keyword = "cluster", .token_type = .CLUSTER },
    .{ .keyword = "distribute", .token_type = .DISTRIBUTE },

    // Goo extensions - memory management
    .{ .keyword = "allocator", .token_type = .ALLOCATOR },
    .{ .keyword = "alloc", .token_type = .ALLOC },
    .{ .keyword = "free", .token_type = .FREE },
    .{ .keyword = "scope", .token_type = .SCOPE },
    .{ .keyword = "heap", .token_type = .HEAP },
    .{ .keyword = "arena", .token_type = .ARENA },
    .{ .keyword = "fixed", .token_type = .FIXED },
    .{ .keyword = "pool", .token_type = .POOL },
    .{ .keyword = "bump", .token_type = .BUMP },
    .{ .keyword = "custom", .token_type = .CUSTOM },
    .{ .keyword = "ensure", .token_type = .ENSURE },

    // Goo extensions - memory safety
    .{ .keyword = "safe", .token_type = .SAFE },
    .{ .keyword = "unsafe", .token_type = .UNSAFE },
    .{ .keyword = "owner", .token_type = .OWNER },
    .{ .keyword = "borrow", .token_type = .BORROW },
    .{ .keyword = "move", .token_type = .MOVE },

    // Goo extensions - async
    .{ .keyword = "async", .token_type = .ASYNC },
    .{ .keyword = "await", .token_type = .AWAIT },
    .{ .keyword = "comptime", .token_type = .COMPTIME },

    // Goo extensions - types
    .{ .keyword = "maybe", .token_type = .MAYBE },
    .{ .keyword = "union", .token_type = .UNION },
    .{ .keyword = "sum", .token_type = .SUM },
    .{ .keyword = "impl", .token_type = .IMPL },

    // Goo extensions - pattern matching
    .{ .keyword = "as", .token_type = .AS },

    // Goo extensions - messaging patterns
    .{ .keyword = "pub", .token_type = .PUB },
    .{ .keyword = "sub", .token_type = .SUB },
    .{ .keyword = "push", .token_type = .PUSH },
    .{ .keyword = "pull", .token_type = .PULL },
    .{ .keyword = "req", .token_type = .REQ },
    .{ .keyword = "rep", .token_type = .REP },
    .{ .keyword = "dealer", .token_type = .DEALER },
    .{ .keyword = "router", .token_type = .ROUTER },
    .{ .keyword = "pair", .token_type = .PAIR },
};

// Lookup function to check if a string is a keyword
pub fn lookupIdentifier(ident: []const u8) TokenType {
    for (keyword_list) |mapping| {
        if (std.mem.eql(u8, ident, mapping.keyword)) {
            return mapping.token_type;
        }
    }
    return .IDENTIFIER;
}

// Re-export standard library for use in tests or other modules
pub const std_lib = std;
