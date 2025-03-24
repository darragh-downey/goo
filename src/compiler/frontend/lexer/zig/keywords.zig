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
    .{ .keyword = "package", .token_type = .PACKAGE },
    .{ .keyword = "import", .token_type = .IMPORT },
    .{ .keyword = "func", .token_type = .FUNC },
    .{ .keyword = "var", .token_type = .VAR },
    .{ .keyword = "const", .token_type = .CONST },
    .{ .keyword = "type", .token_type = .TYPE },
    .{ .keyword = "if", .token_type = .IF },
    .{ .keyword = "else", .token_type = .ELSE },
    .{ .keyword = "for", .token_type = .FOR },
    .{ .keyword = "return", .token_type = .RETURN },
    .{ .keyword = "switch", .token_type = .SWITCH },
    .{ .keyword = "case", .token_type = .CASE },
    .{ .keyword = "default", .token_type = .DEFAULT },
    .{ .keyword = "break", .token_type = .BREAK },
    .{ .keyword = "continue", .token_type = .CONTINUE },
    .{ .keyword = "struct", .token_type = .STRUCT },
    .{ .keyword = "interface", .token_type = .INTERFACE },
    .{ .keyword = "map", .token_type = .MAP },

    // Type keywords
    .{ .keyword = "true", .token_type = .TRUE },
    .{ .keyword = "false", .token_type = .FALSE },
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
    .{ .keyword = "any", .token_type = .ANY_TYPE },

    // Goo extensions - base
    .{ .keyword = "go", .token_type = .GO },
    .{ .keyword = "go_parallel", .token_type = .GO_PARALLEL },
    .{ .keyword = "go_distributed", .token_type = .GO_DISTRIBUTED },
    .{ .keyword = "comptime", .token_type = .COMPTIME },
    .{ .keyword = "impl", .token_type = .IMPL },
    .{ .keyword = "async", .token_type = .ASYNC },
    .{ .keyword = "await", .token_type = .AWAIT },
    .{ .keyword = "defer", .token_type = .DEFER },
    .{ .keyword = "ensure", .token_type = .ENSURE },

    // Goo extensions - concurrency
    .{ .keyword = "parallel", .token_type = .PARALLEL },
    .{ .keyword = "chan", .token_type = .CHAN },
    .{ .keyword = "supervise", .token_type = .SUPERVISE },
    .{ .keyword = "actor", .token_type = .ACTOR },
    .{ .keyword = "cluster", .token_type = .CLUSTER },
    .{ .keyword = "distribute", .token_type = .DISTRIBUTE },

    // Goo extensions - memory
    .{ .keyword = "allocator", .token_type = .ALLOCATOR },
    .{ .keyword = "alloc", .token_type = .ALLOC },
    .{ .keyword = "free", .token_type = .FREE },
    .{ .keyword = "scope", .token_type = .SCOPE },
    .{ .keyword = "owner", .token_type = .OWNER },
    .{ .keyword = "borrow", .token_type = .BORROW },
    .{ .keyword = "move", .token_type = .MOVE },
    .{ .keyword = "unsafe", .token_type = .UNSAFE },

    // Goo extensions - types
    .{ .keyword = "maybe", .token_type = .MAYBE },
    .{ .keyword = "enum", .token_type = .ENUM },
    .{ .keyword = "union", .token_type = .UNION },
    .{ .keyword = "sum", .token_type = .SUM },

    // Goo extensions - pattern matching
    .{ .keyword = "match", .token_type = .MATCH },
    .{ .keyword = "pattern", .token_type = .PATTERN },
    .{ .keyword = "as", .token_type = .AS },

    // Goo extensions - messaging
    .{ .keyword = "pub", .token_type = .PUB },
    .{ .keyword = "sub", .token_type = .SUB },
    .{ .keyword = "push", .token_type = .PUSH },
    .{ .keyword = "pull", .token_type = .PULL },
    .{ .keyword = "req", .token_type = .REQ },
    .{ .keyword = "rep", .token_type = .REP },
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
