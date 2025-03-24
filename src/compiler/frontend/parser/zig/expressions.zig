const std = @import("std");
const lexer = @import("lexer");
const ast = @import("ast");
const errors = @import("errors");
const Parser = @import("parser").Parser;
const token = lexer.token;
const TokenType = lexer.TokenType;

/// Expression parser for the Goo language
pub const Precedence = enum(u8) {
    LOWEST = 0,
    ASSIGN = 1, // =
    LOGICAL_OR = 2, // or
    LOGICAL_AND = 3, // and
    EQUALITY = 4, // == !=
    COMPARISON = 5, // > >= < <=
    SUM = 6, // + -
    PRODUCT = 7, // * / %
    PREFIX = 8, // ! -
    CALL = 9, // myFunction(x)
    INDEX = 10, // array[index]
    MEMBER = 11, // foo.bar
};

/// Get the precedence of a token type
pub fn getTokenPrecedence(token_type: TokenType) Precedence {
    return switch (token_type) {
        .ASSIGN => .ASSIGN,
        .PLUS, .MINUS => .SUM,
        .ASTERISK, .SLASH, .PERCENT => .PRODUCT,
        .EQ, .NEQ => .EQUALITY,
        .LT, .GT, .LEQ, .GEQ => .COMPARISON,
        .AND => .LOGICAL_AND,
        .OR => .LOGICAL_OR,
        .LPAREN => .CALL,
        else => .LOWEST,
    };
}

/// Parse an expression with the parser
pub fn parseExpression(parser: anytype, precedence: Precedence) !ast.Expression {
    // Placeholder function that we'll implement later
    _ = parser;
    _ = precedence;

    return ast.Expression{
        .node = ast.Node{
            .node_type = .expression,
            .start_loc = .{ .line = 0, .column = 0 },
            .end_loc = .{ .line = 0, .column = 0 },
        },
        .expr_type = .{ .identifier = try std.heap.c_allocator.dupe(u8, "placeholder") },
    };
}

/// Helper function to parse a declaration
pub fn parseDeclaration(parser: anytype) !ast.Declaration {
    _ = parser;
    return error.NotImplemented;
}

/// Parse an identifier
pub fn parseIdentifier(parser: *Parser) !*ast.Node {
    const id_node = try parser.createNode(.IDENTIFIER);
    const ident = try parser.allocator.create(ast.IdentifierNode);

    // Extract the string value based on the TokenValue union type
    const str_value = switch (parser.current_token.value) {
        .string_val => |s| s,
        else => {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "Expected string value for identifier at line {d}:{d}",
                .{ parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
    };

    ident.* = .{
        .base = id_node.*,
        .value = try parser.allocator.dupe(u8, str_value),
    };
    parser.nextToken();
    return &ident.base;
}

/// Parse an integer literal
pub fn parseIntegerLiteral(parser: *Parser) !*ast.Node {
    const lit = try parser.allocator.create(ast.IntLiteralNode);

    // Extract the integer value based on the TokenValue union type
    const value = switch (parser.current_token.value) {
        .integer_val => |i| i,
        .string_val => |s| std.fmt.parseInt(i64, s, 10) catch |err| {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "could not parse {s} as integer at line {d}:{d}: {s}",
                .{
                    s,
                    parser.current_token.line,
                    parser.current_token.column,
                    @errorName(err),
                },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
        else => {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "Expected integer value at line {d}:{d}",
                .{ parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
    };

    lit.* = ast.IntLiteralNode{
        .node = parser.createNode(ast.NodeType.INT_LITERAL),
        .value = value,
    };
    parser.nextToken();
    return &lit.node;
}

/// Parse a float literal
pub fn parseFloatLiteral(parser: *Parser) !*ast.Node {
    const lit = try parser.allocator.create(ast.FloatLiteralNode);

    // Extract the float value based on the TokenValue union type
    const value = switch (parser.current_token.value) {
        .float_val => |f| f,
        .string_val => |s| std.fmt.parseFloat(f64, s) catch |err| {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "could not parse {s} as float at line {d}:{d}: {s}",
                .{
                    s,
                    parser.current_token.line,
                    parser.current_token.column,
                    @errorName(err),
                },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
        else => {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "Expected float value at line {d}:{d}",
                .{ parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
    };

    lit.* = ast.FloatLiteralNode{
        .node = parser.createNode(ast.NodeType.FLOAT_LITERAL),
        .value = value,
    };
    parser.nextToken();
    return &lit.node;
}

/// Parse a string literal
pub fn parseStringLiteral(parser: *Parser) !*ast.Node {
    const str_node = try parser.allocator.create(ast.StringLiteralNode);

    // Extract the string value based on the TokenValue union type
    const str_value = switch (parser.current_token.value) {
        .string_val => |s| s,
        else => {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "Expected string value for string literal at line {d}:{d}",
                .{ parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        },
    };

    str_node.* = ast.StringLiteralNode{
        .node = parser.createNode(ast.NodeType.STRING_LITERAL),
        .value = try parser.allocator.dupe(u8, str_value),
    };

    parser.nextToken();
    return &str_node.node;
}

/// Parse a boolean literal
pub fn parseBooleanLiteral(parser: *Parser) !*ast.Node {
    const bool_node = try parser.allocator.create(ast.BooleanLiteralNode);

    bool_node.* = ast.BooleanLiteralNode{
        .node = parser.createNode(ast.NodeType.BOOLEAN_LITERAL),
        .value = parser.current_token.token_type == .TRUE,
    };

    parser.nextToken();
    return &bool_node.node;
}

/// Parse a grouped expression (in parentheses)
pub fn parseGroupedExpression(parser: *Parser) !*ast.Node {
    parser.nextToken(); // Skip '('

    const expr = try parser.parseExpression(.LOWEST);

    if (!parser.expectPeek(.RPAREN)) {
        return errors.ParserError.MissingToken;
    }

    return expr;
}

/// Parse a prefix expression (e.g., !x, -10)
pub fn parsePrefixExpression(parser: *Parser) !*ast.Node {
    const expr = try parser.allocator.create(ast.PrefixExprNode);

    // Extract the operator string
    const operator = switch (parser.current_token.value) {
        .string_val => |s| s,
        else => @tagName(parser.current_token.token_type),
    };

    expr.* = ast.PrefixExprNode{
        .node = parser.createNode(ast.NodeType.PREFIX_EXPR),
        .operator = try parser.allocator.dupe(u8, operator),
        .right = null,
    };

    // Move to the expression after the prefix
    parser.nextToken();

    expr.right = try parser.parseExpression(.PREFIX);
    return &expr.node;
}

/// Parse an infix expression (e.g., a + b, x == y)
pub fn parseInfixExpression(parser: *Parser, left: *ast.Node) !*ast.Node {
    const expr = try parser.allocator.create(ast.InfixExprNode);

    // Extract the operator string
    const operator = switch (parser.current_token.value) {
        .string_val => |s| s,
        else => @tagName(parser.current_token.token_type),
    };

    expr.* = ast.InfixExprNode{
        .node = parser.createNode(ast.NodeType.INFIX_EXPR),
        .left = left,
        .operator = try parser.allocator.dupe(u8, operator),
        .right = null,
    };

    // Remember the precedence of the current operator
    const precedence = parser.currentPrecedence();

    // Move to the expression after the operator
    parser.nextToken();

    expr.right = try parser.parseExpression(precedence);
    return &expr.node;
}

/// Parse a function call expression (e.g., foo(), bar(1, 2))
pub fn parseCallExpression(parser: *Parser, function: *ast.Node) !*ast.Node {
    const expr = try parser.allocator.create(ast.CallExprNode);
    expr.* = ast.CallExprNode{
        .node = parser.createNode(ast.NodeType.CALL_EXPR),
        .function = function,
        .arguments = std.ArrayList(*ast.Node).init(parser.allocator),
    };

    try parser.parseCallArguments(&expr.arguments);
    return &expr.node;
}

/// Parse call arguments
pub fn parseCallArguments(parser: *Parser, arguments: *std.ArrayList(*ast.Node)) !void {
    // Empty argument list
    if (parser.peekTokenIs(.RPAREN)) {
        parser.nextToken();
        return;
    }

    // Skip '('
    parser.nextToken();

    // Parse first argument
    try arguments.append(try parser.parseExpression(.LOWEST));

    // Parse remaining arguments
    while (parser.peekTokenIs(.COMMA)) {
        parser.nextToken(); // Skip comma
        parser.nextToken(); // Move to next argument
        try arguments.append(try parser.parseExpression(.LOWEST));
    }

    if (!parser.expectPeek(.RPAREN)) {
        return errors.ParserError.MissingToken;
    }
}
