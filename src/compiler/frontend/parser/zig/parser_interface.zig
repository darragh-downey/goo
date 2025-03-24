const std = @import("std");
const lexer = @import("lexer");
const ast = @import("ast");
const errors = @import("errors");
const TokenType = lexer.TokenType;

/// Parser interface that can be used by modules that would create circular dependencies
pub const ParserInterface = struct {
    current_token: *const lexer.Token,
    peek_token: *const lexer.Token,
    allocator: std.mem.Allocator,
    next_token_fn: *const fn (self: *anyopaque) anyerror!void,
    current_token_is_fn: *const fn (self: *anyopaque, token_type: TokenType) bool,
    peek_token_is_fn: *const fn (self: *anyopaque, token_type: TokenType) bool,
    create_node_fn: *const fn (self: *anyopaque, node_type: ast.NodeType) ast.Node,
    add_error_fn: *const fn (self: *anyopaque, message: []const u8) anyerror!void,
};

/// Create a parser interface from a parser
pub fn createParserInterface(parser: anytype) ParserInterface {
    const P = @TypeOf(parser);
    return ParserInterface{
        .current_token = &parser.current_token,
        .peek_token = &parser.peek_token,
        .allocator = parser.allocator,
        .next_token_fn = struct {
            fn nextToken(self: *anyopaque) anyerror!void {
                const p: *P = @alignCast(@ptrCast(self));
                return p.nextToken();
            }
        }.nextToken,
        .current_token_is_fn = struct {
            fn currentTokenIs(self: *anyopaque, token_type: TokenType) bool {
                const p: *P = @alignCast(@ptrCast(self));
                return p.currentTokenIs(token_type);
            }
        }.currentTokenIs,
        .peek_token_is_fn = struct {
            fn peekTokenIs(self: *anyopaque, token_type: TokenType) bool {
                const p: *P = @alignCast(@ptrCast(self));
                return p.peekTokenIs(token_type);
            }
        }.peekTokenIs,
        .create_node_fn = struct {
            fn createNode(self: *anyopaque, node_type: ast.NodeType) ast.Node {
                const p: *P = @alignCast(@ptrCast(self));
                return p.createNode(node_type);
            }
        }.createNode,
        .add_error_fn = struct {
            fn addError(self: *anyopaque, message: []const u8) anyerror!void {
                const p: *P = @alignCast(@ptrCast(self));
                return try p.error_reporter.addError(message);
            }
        }.addError,
    };
}

/// Next token method for the interface
pub fn nextToken(interface: *const ParserInterface, parser: *anyopaque) !void {
    return interface.next_token_fn(parser);
}

/// Check if current token is of the given type
pub fn currentTokenIs(interface: *const ParserInterface, parser: *anyopaque, token_type: TokenType) bool {
    return interface.current_token_is_fn(parser, token_type);
}

/// Check if peek token is of the given type
pub fn peekTokenIs(interface: *const ParserInterface, parser: *anyopaque, token_type: TokenType) bool {
    return interface.peek_token_is_fn(parser, token_type);
}

/// Create a node with the given type
pub fn createNode(interface: *const ParserInterface, parser: *anyopaque, node_type: ast.NodeType) ast.Node {
    return interface.create_node_fn(parser, node_type);
}

/// Add an error message
pub fn addError(interface: *const ParserInterface, parser: *anyopaque, message: []const u8) !void {
    return interface.add_error_fn(parser, message);
}
