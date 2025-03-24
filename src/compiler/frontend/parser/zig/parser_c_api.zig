const std = @import("std");
const parser = @import("parser");
const ast = @import("ast");
const lexer = @import("lexer");
const errors = @import("errors");

// Define opaque types for the C API
pub const goo_parser_t = opaque {};
pub const goo_lexer_t = opaque {};
pub const goo_ast_program_t = opaque {};

// Global allocator for the C API
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const global_allocator = gpa.allocator();

// Opaque structs for the C API
pub const GooParser = opaque {};
pub const GooAstNode = opaque {};

// Error code enum for C API
pub const GooParserErrorCode = enum(c_int) {
    success = 0,
    unexpected_token = 1,
    missing_token = 2,
    invalid_syntax = 3,
    out_of_memory = 4,
    not_implemented = 5,
    unknown_error = 6,
};

// AST node type enum for C API
pub const GooAstNodeType = enum(c_int) {
    program = 0,
    package_decl = 1,
    import_decl = 2,
    function_decl = 3,
    parameter = 4,
    var_decl = 5,
    const_decl = 6,
    type_decl = 7,
    type_expr = 8,
    block = 9,
    if_stmt = 10,
    for_stmt = 11,
    return_stmt = 12,
    expr_stmt = 13,
    call_expr = 14,
    identifier = 15,
    int_literal = 16,
    float_literal = 17,
    string_literal = 18,
    bool_literal = 19,
    prefix_expr = 20,
    infix_expr = 21,
};

// Internal state for the parser
const ParserState = struct {
    parser: parser.Parser,
    allocator: std.mem.Allocator,
    ast_root: ?*ast.Node,
    error_message: ?[]const u8,
    source_str: []const u8,

    fn init(allocator: std.mem.Allocator, source: []const u8) !*ParserState {
        var state = try allocator.create(ParserState);
        errdefer allocator.destroy(state);

        // Create a copy of the source string
        state.source_str = try allocator.dupe(u8, source);
        errdefer allocator.free(state.source_str);

        // Initialize the parser directly with the source string
        state.parser = try parser.Parser.init(allocator, state.source_str);
        state.allocator = allocator;
        state.ast_root = null;
        state.error_message = null;

        return state;
    }

    fn deinit(self: *ParserState) void {
        if (self.error_message) |msg| {
            self.allocator.free(msg);
        }
        self.parser.deinit();
        self.allocator.free(self.source_str);
        self.allocator.destroy(self);
    }
};

// Helper function to convert Zig errors to C error codes
fn zigErrorToErrorCode(err: anyerror) GooParserErrorCode {
    return switch (err) {
        parser.ParserError.UnexpectedToken => .unexpected_token,
        parser.ParserError.MissingToken => .missing_token,
        parser.ParserError.InvalidSyntax => .invalid_syntax,
        error.OutOfMemory => .out_of_memory,
        parser.ParserError.NotImplemented => .not_implemented,
        else => .unknown_error,
    };
}

// Helper function to get AST node type
fn getAstNodeType(node: *ast.Node) GooAstNodeType {
    return switch (node.node_type) {
        .PROGRAM => .program,
        .PACKAGE_DECL => .package_decl,
        .IMPORT_DECL => .import_decl,
        .FUNCTION_DECL => .function_decl,
        .PARAMETER => .parameter,
        .VAR_DECL => .var_decl,
        .CONST_DECL => .const_decl,
        .TYPE_DECL => .type_decl,
        .TYPE_EXPR => .type_expr,
        .BLOCK => .block,
        .IF_STMT => .if_stmt,
        .FOR_STMT => .for_stmt,
        .RETURN_STMT => .return_stmt,
        .EXPR_STMT => .expr_stmt,
        .CALL_EXPR => .call_expr,
        .IDENTIFIER => .identifier,
        .INT_LITERAL => .int_literal,
        .FLOAT_LITERAL => .float_literal,
        .STRING_LITERAL => .string_literal,
        .BOOL_LITERAL => .bool_literal,
        .PREFIX_EXPR => .prefix_expr,
        .INFIX_EXPR => .infix_expr,
    };
}

// C API Functions

export fn goo_parser_create(lexer_handle: *goo_lexer_t) ?*goo_parser_t {
    // Create the error reporter
    const error_reporter = errors.ErrorReporter.init(std.heap.c_allocator) catch return null;

    // Create the parser
    const parser_instance = parser.Parser.init(@ptrCast(@alignCast(lexer_handle)), std.heap.c_allocator, error_reporter) catch return null;

    // Cast to opaque type and return
    return @ptrCast(@alignCast(parser_instance));
}

export fn goo_parser_destroy(parser_handle: *goo_parser_t) void {
    const parser_instance: *parser.Parser = @ptrCast(@alignCast(parser_handle));
    parser_instance.deinit();
    std.heap.c_allocator.destroy(parser_instance);
}

export fn goo_parser_parse(parser_handle: *goo_parser_t) ?*goo_ast_program_t {
    const parser_instance: *parser.Parser = @ptrCast(@alignCast(parser_handle));

    // Parse the program
    const program = parser_instance.parseProgram() catch return null;

    // Cast to C type and return
    return @ptrCast(@alignCast(program));
}

export fn goo_parser_has_errors(parser_handle: *goo_parser_t) bool {
    const parser_instance: *parser.Parser = @ptrCast(@alignCast(parser_handle));
    return parser_instance.error_reporter.errors.items.len > 0;
}

export fn goo_parser_get_error_count(parser_handle: *goo_parser_t) usize {
    const parser_instance: *parser.Parser = @ptrCast(@alignCast(parser_handle));
    return parser_instance.error_reporter.errors.items.len;
}

export fn goo_parser_get_error(parser_handle: *goo_parser_t, index: usize) [*c]const u8 {
    const parser_instance: *parser.Parser = @ptrCast(@alignCast(parser_handle));
    if (index >= parser_instance.error_reporter.errors.items.len) {
        return null;
    }

    return parser_instance.error_reporter.errors.items[index].ptr;
}

export fn goo_parser_cleanup() void {
    _ = gpa.deinit();
}
