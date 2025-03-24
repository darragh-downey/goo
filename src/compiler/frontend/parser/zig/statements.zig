const std = @import("std");
const lexer = @import("lexer");
const ast = @import("ast");
const errors = @import("errors");
const expressions = @import("expressions");
const Parser = @import("parser").Parser;
const TokenType = lexer.TokenType;

/// Parse a statement
pub fn parseStatement(parser: *Parser) !*ast.Node {
    return switch (parser.current_token.token_type) {
        .VAR => @ptrCast(try parseVarDeclaration(parser)),
        .CONST => @ptrCast(try parseConstDeclaration(parser)),
        .RETURN => @ptrCast(try parseReturnStatement(parser)),
        .IF => @ptrCast(try parseIfStatement(parser)),
        .FOR => @ptrCast(try parseForStatement(parser)),
        else => @ptrCast(try parseExpressionStatement(parser)),
    };
}

/// Parse a variable declaration
pub fn parseVarDeclaration(parser: *Parser) !*ast.VarDeclNode {
    var var_node = try parser.allocator.create(ast.VarDeclNode);
    var_node.* = ast.VarDeclNode{
        .node = ast.Node{ .node_type = ast.NodeType.VAR_DECL, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 3 } },
        .name = null,
        .var_type = null,
        .value = null,
    };

    // Skip "var" keyword
    parser.nextToken();

    // Get variable name
    if (!parser.currentTokenIs(.IDENTIFIER)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected variable name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    // Create identifier node for variable name
    const name_node = try parser.allocator.create(ast.IdentifierNode);

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

    name_node.* = .{
        .base = parser.createNode(ast.NodeType.IDENTIFIER),
        .value = try parser.allocator.dupe(u8, str_value),
    };
    var_node.name = name_node;

    // Skip to next token
    parser.nextToken();

    // Parse type annotation if present
    if (parser.currentTokenIs(.COLON)) {
        parser.nextToken(); // Skip ':'
        var_node.var_type = try parseTypeExpression(parser);
    }

    // Parse initializer if present
    if (parser.currentTokenIs(.ASSIGN)) {
        parser.nextToken(); // Skip '='
        var_node.value = try expressions.parseExpression(parser, .LOWEST);
    }

    return var_node;
}

/// Parse a constant declaration
pub fn parseConstDeclaration(parser: *Parser) !*ast.ConstDeclNode {
    var const_node = try parser.allocator.create(ast.ConstDeclNode);
    const_node.* = ast.ConstDeclNode{
        .node = ast.Node{ .node_type = ast.NodeType.CONST_DECL, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 5 } },
        .name = null,
        .const_type = null,
        .value = null,
    };

    // Skip "const" keyword
    parser.nextToken();

    // Get constant name
    if (!parser.currentTokenIs(.IDENTIFIER)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected constant name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    // Create identifier node for constant name
    const name_node = try parser.allocator.create(ast.IdentifierNode);

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

    name_node.* = .{
        .base = parser.createNode(ast.NodeType.IDENTIFIER),
        .value = try parser.allocator.dupe(u8, str_value),
    };
    const_node.name = name_node;

    // Skip to next token
    parser.nextToken();

    // Parse type annotation if present
    if (parser.currentTokenIs(.COLON)) {
        parser.nextToken(); // Skip ':'
        const_node.const_type = try parseTypeExpression(parser);
    }

    // Constants must have an initializer
    if (parser.currentTokenIs(.ASSIGN)) {
        parser.nextToken(); // Skip '='
        const_node.value = try expressions.parseExpression(parser, .LOWEST);
    } else {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '=' after constant declaration, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    return const_node;
}

/// Parse a return statement
pub fn parseReturnStatement(parser: *Parser) !*ast.ReturnStmtNode {
    var return_node = try parser.allocator.create(ast.ReturnStmtNode);
    return_node.* = ast.ReturnStmtNode{
        .node = ast.Node{ .node_type = ast.NodeType.RETURN_STMT, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 6 } },
        .value = null,
    };

    // Skip "return" keyword
    parser.nextToken();

    // Parse return value if present (otherwise it's a void return)
    if (!parser.currentTokenIs(.SEMICOLON) and !parser.currentTokenIs(.RBRACE)) {
        return_node.value = try expressions.parseExpression(parser, .LOWEST);
    }

    // Skip semicolon if present
    if (parser.currentTokenIs(.SEMICOLON)) {
        parser.nextToken();
    }

    return return_node;
}

/// Parse an if statement
pub fn parseIfStatement(parser: *Parser) !*ast.IfStmtNode {
    var if_node = try parser.allocator.create(ast.IfStmtNode);
    if_node.* = ast.IfStmtNode{
        .node = ast.Node{ .node_type = ast.NodeType.IF_STMT, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 2 } },
        .condition = null,
        .consequence = null,
        .alternative = null,
    };

    // Skip "if" keyword
    parser.nextToken();

    // Parse condition expression
    if_node.condition = try expressions.parseExpression(parser, .LOWEST);

    // Parse consequence block
    if (!parser.currentTokenIs(.LBRACE)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '{' after if condition, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    if_node.consequence = try parseBlockStatement(parser);

    // Parse else block if present
    if (parser.currentTokenIs(.ELSE)) {
        parser.nextToken(); // Skip "else"

        if (parser.currentTokenIs(.IF)) {
            // Handle "else if" by parsing another if statement
            const else_if = try parseIfStatement(parser);

            // Create a block to contain the else-if
            var block = try parser.allocator.create(ast.BlockNode);
            block.* = ast.BlockNode{
                .node = ast.Node{ .node_type = ast.NodeType.BLOCK, .start_pos = else_if.node.start_pos, .end_pos = else_if.node.end_pos },
                .statements = std.ArrayList(*ast.Node).init(parser.allocator),
            };

            try block.statements.append(@ptrCast(else_if));
            if_node.alternative = block;
        } else if (parser.currentTokenIs(.LBRACE)) {
            // Regular else block
            if_node.alternative = try parseBlockStatement(parser);
        } else {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "expected '{' or 'if' after 'else', got {s} at line {d}:{d}",
                .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        }
    }

    return if_node;
}

/// Parse a for loop
pub fn parseForStatement(parser: *Parser) !*ast.ForStmtNode {
    var for_node = try parser.allocator.create(ast.ForStmtNode);
    for_node.* = ast.ForStmtNode{
        .node = ast.Node{ .node_type = ast.NodeType.FOR_STMT, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 3 } },
        .init = null,
        .condition = null,
        .update = null,
        .body = null,
    };

    // Skip "for" keyword
    parser.nextToken();

    // Parse "for" clause components
    // Init statement (optional)
    if (!parser.currentTokenIs(.SEMICOLON)) {
        if (parser.currentTokenIs(.VAR) or parser.currentTokenIs(.CONST)) {
            // Parse variable or const declaration as init statement
            if (parser.currentTokenIs(.VAR)) {
                for_node.init = @ptrCast(try parseVarDeclaration(parser));
            } else {
                for_node.init = @ptrCast(try parseConstDeclaration(parser));
            }
        } else {
            // Parse expression statement as init
            for_node.init = @ptrCast(try parseExpressionStatement(parser));
        }

        // Expect semicolon after init statement
        if (!parser.currentTokenIs(.SEMICOLON)) {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "expected ';' after for loop initialization, got {s} at line {d}:{d}",
                .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        }
        parser.nextToken(); // Skip ';'
    } else {
        parser.nextToken(); // Skip ';' if init is omitted
    }

    // Condition expression (optional)
    if (!parser.currentTokenIs(.SEMICOLON)) {
        for_node.condition = try expressions.parseExpression(parser, .LOWEST);

        // Expect semicolon after condition
        if (!parser.currentTokenIs(.SEMICOLON)) {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "expected ';' after for loop condition, got {s} at line {d}:{d}",
                .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        }
    }
    parser.nextToken(); // Skip ';'

    // Update expression (optional)
    if (!parser.currentTokenIs(.LBRACE)) {
        for_node.update = try expressions.parseExpression(parser, .LOWEST);
    }

    // Parse loop body
    if (!parser.currentTokenIs(.LBRACE)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '{' after for loop clauses, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    for_node.body = try parseBlockStatement(parser);
    return for_node;
}

/// Parse an expression statement
pub fn parseExpressionStatement(parser: *Parser) !*ast.ExprStmtNode {
    // Create the statement node
    const stmt_node = try parser.allocator.create(ast.ExprStmtNode);
    stmt_node.* = ast.ExprStmtNode{
        .base = parser.createNode(ast.NodeType.EXPR_STMT),
        .expression = try expressions.parseExpression(parser, .LOWEST),
    };

    // Skip optional semicolon at the end of expression statements
    if (parser.peekTokenIs(.SEMICOLON)) {
        try parser.nextToken();
    }

    return stmt_node;
}

/// Parse a block of statements
pub fn parseBlockStatement(parser: *Parser) !*ast.BlockNode {
    var block_node = try parser.allocator.create(ast.BlockNode);
    block_node.* = ast.BlockNode{
        .node = parser.createNode(ast.NodeType.BLOCK),
        .statements = std.ArrayList(*ast.Node).init(parser.allocator),
    };

    parser.nextToken(); // Skip '{'

    while (!parser.currentTokenIs(.RBRACE) and !parser.currentTokenIs(.EOF)) {
        const stmt = try parseStatement(parser);
        try block_node.statements.append(stmt);
    }

    if (!parser.currentTokenIs(.RBRACE)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '}' for block statement, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    parser.nextToken(); // Skip '}'

    return block_node;
}

/// Parse a type expression
pub fn parseTypeExpression(parser: *Parser) !*ast.TypeExprNode {
    var type_node = try parser.allocator.create(ast.TypeExprNode);
    type_node.* = ast.TypeExprNode{
        .node = parser.createNode(ast.NodeType.TYPE_EXPR),
        .name = null,
    };

    if (!parser.currentTokenIs(.IDENTIFIER)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected type name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.token_type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    // Create identifier node for type name
    const name_node = try parser.allocator.create(ast.IdentifierNode);

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

    name_node.* = .{
        .base = parser.createNode(ast.NodeType.IDENTIFIER),
        .value = try parser.allocator.dupe(u8, str_value),
    };
    type_node.name = name_node;

    parser.nextToken(); // Skip type name

    return type_node;
}

/// Parse a block of statements
pub fn parseBlock(parser: anytype) !ast.Block {
    const block = ast.Block{
        .statements = std.ArrayList(ast.Statement).init(parser.allocator),
    };

    // Use parser here to avoid the warning
    _ = parser.allocator;

    return block;
}
