const std = @import("std");
const ast = @import("ast");
const errors = @import("errors");
const statements = @import("statements");
const expressions = @import("expressions");
const parser_interface = @import("parser_interface");
const lexer = @import("lexer");
const token = lexer.token;
const TokenType = lexer.TokenType;

/// Parse a declaration
pub fn parseDeclaration(parser: *parser_interface.ParserInterface) !*ast.Node {
    return switch (parser.current_token.*.type) {
        .FUNC => @ptrCast(try parseFunctionDeclaration(parser)),
        .VAR => @ptrCast(try statements.parseVarDeclaration(parser)),
        .CONST => @ptrCast(try statements.parseConstDeclaration(parser)),
        .TYPE => @ptrCast(try parseTypeDeclaration(parser)),
        else => {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "unexpected token {s} for declaration at line {d}:{d}",
                .{ @tagName(parser.current_token.*.type), parser.current_token.*.line, parser.current_token.*.column },
            );
            try parser.add_error_fn(parser, msg);
            return errors.ParserError.InvalidSyntax;
        },
    };
}

/// Parse a function declaration
pub fn parseFunctionDeclaration(parser: *parser_interface.Parser) !*ast.FunctionNode {
    var func_node = try parser.allocator.create(ast.FunctionNode);
    func_node.* = ast.FunctionNode{
        .node = ast.Node{ .node_type = ast.NodeType.FUNCTION_DECL, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column + 4 } },
        .name = null,
        .parameters = std.ArrayList(*ast.ParameterNode).init(parser.allocator),
        .return_type = null,
        .body = null,
    };

    // Skip "func" keyword
    parser.nextToken();

    // Get function name
    if (!parser.currentTokenIs(.IDENTIFIER)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected function name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    // Create identifier node for function name
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
    func_node.name = name_node;

    // Skip to parameters
    parser.nextToken();

    // Parse parameters
    if (!parser.currentTokenIs(.LPAREN)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '(' after function name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    parser.nextToken(); // Skip '('

    // Parse parameters if there are any
    if (!parser.currentTokenIs(.RPAREN)) {
        try parseFunctionParameters(parser, &func_node.parameters);
    }

    if (!parser.currentTokenIs(.RPAREN)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected ')' after parameters, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    parser.nextToken(); // Skip ')'

    // Parse return type if present
    if (parser.currentTokenIs(.COLON)) {
        parser.nextToken(); // Skip ':'
        func_node.return_type = try statements.parseTypeExpression(parser);
    }

    // Parse function body
    if (!parser.currentTokenIs(.LBRACE)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '{' for function body, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    func_node.body = try statements.parseBlockStatement(parser);

    return func_node;
}

/// Parse function parameters
pub fn parseFunctionParameters(parser: *parser_interface.Parser, parameters: *std.ArrayList(*ast.ParameterNode)) !void {
    // Parameter format: name: type, name: type, ...
    while (true) {
        if (!parser.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "expected parameter name, got {s} at line {d}:{d}",
                .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        }

        // Create parameter node
        var param_node = try parser.allocator.create(ast.ParameterNode);
        param_node.* = ast.ParameterNode{
            .node = ast.Node{ .node_type = ast.NodeType.PARAMETER, .start_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column }, .end_pos = .{ .line = parser.current_token.line, .column = parser.current_token.column } },
            .name = null,
            .param_type = null,
        };

        // Create identifier node for parameter name
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
        param_node.name = name_node;

        parser.nextToken(); // Skip parameter name

        // Expect colon after parameter name
        if (!parser.currentTokenIs(.COLON)) {
            const msg = try std.fmt.allocPrint(
                parser.allocator,
                "expected ':' after parameter name, got {s} at line {d}:{d}",
                .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
            );
            try parser.error_reporter.errors.append(msg);
            return errors.ParserError.InvalidSyntax;
        }

        parser.nextToken(); // Skip ':'

        // Parse parameter type
        param_node.param_type = try statements.parseTypeExpression(parser);

        // Add parameter to the list
        try parameters.append(param_node);

        // Check if there are more parameters
        if (parser.currentTokenIs(.COMMA)) {
            parser.nextToken(); // Skip ','
            continue;
        } else {
            break;
        }
    }
}

/// Parse a type declaration
pub fn parseTypeDeclaration(parser: *parser_interface.Parser) !*ast.TypeDeclNode {
    const node = try parser.allocator.create(ast.TypeDeclNode);
    node.* = ast.TypeDeclNode{
        .base = parser.createNode(ast.NodeType.TYPE_DECL),
        .name = null,
        .type_expr = null,
    };

    // Skip 'type' keyword
    parser.nextToken();

    // Parse type name
    if (!parser.currentTokenIs(.IDENTIFIER)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected type name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
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
    node.name = name_node;

    parser.nextToken(); // Skip type name

    // Expect equals sign
    if (!parser.currentTokenIs(.ASSIGN)) {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "expected '=' after type name, got {s} at line {d}:{d}",
            .{ @tagName(parser.current_token.type), parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    parser.nextToken(); // Skip '='

    // Parse type expression
    node.type_expr = try statements.parseTypeExpression(parser);

    return node;
}

/// Parse a package declaration
pub fn parsePackageDeclaration(parser: *parser_interface.Parser) !*ast.PackageNode {
    // Create a package node
    const node = try parser.createNode(.PACKAGE_DECL);
    const package_node = try parser.allocator.create(ast.PackageNode);
    package_node.* = .{
        .base = node.*,
        .name = null,
    };

    // Skip the 'package' keyword
    parser.nextToken();

    // Check if we have a package name
    if (parser.currentTokenIs(.IDENTIFIER)) {
        // Create an identifier node for the package name
        const id_node = try parser.createNode(.IDENTIFIER);
        const identifier = try parser.allocator.create(ast.IdentifierNode);

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

        identifier.* = .{
            .base = id_node.*,
            .value = try parser.allocator.dupe(u8, str_value),
        };

        package_node.name = identifier;

        // Update end position to include the identifier
        package_node.base.end_pos = identifier.base.end_pos;

        // Skip the identifier
        parser.nextToken();
    } else {
        try parser.error_reporter.appendError("Expected package name");
        return errors.ParserError.UnexpectedToken;
    }

    // Expect semicolon
    if (!parser.currentTokenIs(.SEMICOLON)) {
        try parser.error_reporter.appendError("Expected ';' after package declaration");
        return errors.ParserError.MissingToken;
    }

    // Skip the semicolon
    parser.nextToken();

    return package_node;
}

/// Parse an import declaration
pub fn parseImportDeclaration(parser: *parser_interface.Parser) !*ast.ImportNode {
    // Create an import node
    const node = try parser.createNode(.IMPORT_DECL);
    const import_node = try parser.allocator.create(ast.ImportNode);
    import_node.* = .{
        .base = node.*,
        .path = null,
        .alias = null,
    };

    // Skip "import" keyword
    parser.nextToken();

    // Get import path (should be string literal)
    if (!parser.currentTokenIs(.STRING_LITERAL)) {
        try parser.error_reporter.appendError("Expected import path (string literal)");
        return errors.ParserError.UnexpectedToken;
    }

    // Create string literal node for import path
    const str_node = try parser.createNode(.STRING_LITERAL);
    const path_node = try parser.allocator.create(ast.StringLiteralNode);

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

    path_node.* = .{
        .node = str_node.*,
        .value = try parser.allocator.dupe(u8, str_value),
    };
    import_node.path = path_node;

    // Update end position of import node to include the path
    import_node.base.end_pos = path_node.node.end_pos;

    // Check if import has an alias
    parser.nextToken();
    if (parser.currentTokenIs(.AS)) {
        parser.nextToken(); // Skip "as" keyword

        if (!parser.currentTokenIs(.IDENTIFIER)) {
            try parser.error_reporter.appendError("Expected import alias (identifier)");
            return errors.ParserError.UnexpectedToken;
        }

        // Create identifier node for alias
        const id_node = try parser.createNode(.IDENTIFIER);
        const alias_node = try parser.allocator.create(ast.IdentifierNode);

        // Extract the string value based on the TokenValue union type
        const alias_value = switch (parser.current_token.value) {
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

        alias_node.* = .{
            .base = id_node.*,
            .value = try parser.allocator.dupe(u8, alias_value),
        };
        import_node.alias = alias_node;

        // Update end position to include the alias
        import_node.base.end_pos = alias_node.base.end_pos;

        parser.nextToken(); // Move to next token after alias
    }

    // Expect semicolon
    if (!parser.currentTokenIs(.SEMICOLON)) {
        try parser.error_reporter.appendError("Expected ';' after import declaration");
        return errors.ParserError.MissingToken;
    }

    // Skip the semicolon
    parser.nextToken();

    return import_node;
}

/// Parse a complete program
pub fn parseProgram(parser: *parser_interface.Parser) !*ast.Program {
    var program = try parser.allocator.create(ast.Program);
    program.* = ast.Program{
        .base = .{
            .node_type = ast.NodeType.PROGRAM,
            .start_pos = .{ .line = 0, .column = 0 },
            .end_pos = .{ .line = 0, .column = 0 },
        },
        .package = null,
        .imports = std.ArrayList(*ast.ImportNode).init(parser.allocator),
        .declarations = std.ArrayList(*ast.Node).init(parser.allocator),
    };

    // Parse package declaration (must be the first statement)
    if (parser.currentTokenIs(.PACKAGE)) {
        program.package = try parsePackageDeclaration(parser);
    } else {
        const msg = try std.fmt.allocPrint(
            parser.allocator,
            "program must start with a package declaration at line {d}:{d}",
            .{ parser.current_token.line, parser.current_token.column },
        );
        try parser.error_reporter.errors.append(msg);
        return errors.ParserError.InvalidSyntax;
    }

    // Parse imports
    while (parser.currentTokenIs(.IMPORT)) {
        const import_node = try parseImportDeclaration(parser);
        try program.imports.append(import_node);
    }

    // Parse declarations
    while (!parser.currentTokenIs(.EOF)) {
        const decl = try parseDeclaration(parser);
        try program.declarations.append(decl);
    }

    return program;
}
