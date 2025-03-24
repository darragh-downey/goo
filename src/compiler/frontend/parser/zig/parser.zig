const std = @import("std");
const lexer = @import("lexer");
const ast = @import("ast");
const errors = @import("errors");
const token = lexer.token;
const TokenType = lexer.TokenType;

/// Precedence levels for expressions
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
};

/// Parser for the Goo language
pub const Parser = struct {
    allocator: std.mem.Allocator,
    lex: *lexer.Lexer,
    current_token: lexer.Token,
    peek_token: lexer.Token,
    error_reporter: errors.ErrorReporter,

    /// Initialize a parser with the given lexer
    pub fn init(lex: *lexer.Lexer, allocator: std.mem.Allocator, error_reporter: errors.ErrorReporter) !*Parser {
        var parser = try allocator.create(Parser);
        parser.* = Parser{
            .allocator = allocator,
            .lex = lex,
            .current_token = undefined,
            .peek_token = undefined,
            .error_reporter = error_reporter,
        };

        // Read two tokens to initialize current_token and peek_token
        try parser.nextToken();
        try parser.nextToken();

        return parser;
    }

    /// Free resources used by the parser
    pub fn deinit(_: *Parser) void {
        // No additional cleanup needed yet
    }

    /// Advance to the next token
    pub fn nextToken(self: *Parser) !void {
        self.current_token = self.peek_token;
        self.peek_token = try self.lex.nextToken();
    }

    /// Create a new AST node with the given type
    pub fn createNode(self: *Parser, node_type: ast.NodeType) ast.Node {
        return ast.Node{
            .node_type = node_type,
            .start_pos = .{ .line = self.current_token.line, .column = self.current_token.column },
            .end_pos = .{ .line = self.current_token.line, .column = self.current_token.column },
        };
    }

    /// Check if the current token is of the given type
    pub fn currentTokenIs(self: *Parser, token_type: TokenType) bool {
        return self.current_token.type == token_type;
    }

    /// Check if the peek token is of the given type
    pub fn peekTokenIs(self: *Parser, token_type: TokenType) bool {
        return self.peek_token.type == token_type;
    }

    /// Get the precedence of the current token
    pub fn currentPrecedence(self: *Parser) Precedence {
        return getPrecedence(self.current_token.type);
    }

    /// Get the precedence of the peek token
    pub fn peekPrecedence(self: *Parser) Precedence {
        return getPrecedence(self.peek_token.type);
    }

    /// Parse a program
    pub fn parseProgram(self: *Parser) !*ast.Program {
        var program = try self.allocator.create(ast.Program);
        program.* = ast.Program{
            .declarations = std.ArrayList(*ast.Node).init(self.allocator),
        };

        // Parse declarations until end of file
        while (!self.currentTokenIs(.EOF)) {
            std.debug.print("Program parsing loop, token: {s}, line: {d}\n", .{ @tagName(self.current_token.type), self.current_token.line });

            const maybe_decl = self.parseDeclaration() catch |err| {
                std.debug.print("Error parsing declaration: {s}\n", .{@errorName(err)});
                // Skip to next token to try to continue parsing
                try self.nextToken();

                // If too many errors, abort
                if (self.error_reporter.errors.items.len > 100) {
                    return err;
                }
                continue;
            };

            if (maybe_decl) |decl| {
                try program.declarations.append(@ptrCast(decl));
                std.debug.print("Added declaration to program, now {d} declarations\n", .{program.declarations.items.len});
            } else {
                // If we didn't get a declaration, we need to advance past the current token
                // to avoid an infinite loop
                std.debug.print("No declaration found for token: {s}, skipping\n", .{@tagName(self.current_token.type)});
                try self.nextToken();
            }
        }

        return program;
    }

    /// Parse a declaration
    fn parseDeclaration(self: *Parser) !?*ast.Declaration {
        std.debug.print("Parsing declaration, token: {s}\n", .{@tagName(self.current_token.type)});

        // Check for package declaration
        if (self.currentTokenIs(.PACKAGE)) {
            return try self.parsePackageDeclaration();
        }

        // Check for import declaration
        if (self.currentTokenIs(.IMPORT)) {
            return try self.parseImportDeclaration();
        }

        // Check for const declaration
        if (self.currentTokenIs(.CONST)) {
            return try self.parseConstDeclaration();
        }

        // Check for var declaration
        if (self.currentTokenIs(.VAR)) {
            return try self.parseVarDeclaration();
        }

        // Check for function declaration
        if (self.currentTokenIs(.FUNC)) {
            return try self.parseFunctionDeclaration();
        }

        // Check for type declaration
        if (self.currentTokenIs(.TYPE)) {
            return try self.parseTypeDeclaration();
        }

        // Check for comptime declaration (Goo extension)
        if (self.currentTokenIs(.COMPTIME)) {
            return try self.parseComptimeDeclaration();
        }

        // Check for parallel declaration (Goo extension)
        if (self.currentTokenIs(.GO_PARALLEL) or self.currentTokenIs(.GO_DISTRIBUTED)) {
            return try self.parseParallelDeclaration(self.currentTokenIs(.GO_DISTRIBUTED));
        }

        return null;
    }

    /// Parse a function declaration
    fn parseFunctionDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing function declaration\n", .{});

        // Create function declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .function = ast.FunctionDecl{
                    .name = "",
                    .params = std.ArrayList(ast.Parameter).init(self.allocator),
                    .return_type = null,
                    .body = ast.Block{
                        .statements = std.ArrayList(ast.Statement).init(self.allocator),
                    },
                },
            },
        };

        // Advance past 'func' keyword
        try self.nextToken();

        // Parse function name
        if (!self.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected function name after 'func' keyword at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.ExpectedIdentifier;
        }

        // Extract function name
        const name_val = switch (self.current_token.value) {
            .string_val => |s| s,
            else => {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "invalid identifier format at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            },
        };

        // Set function name
        decl.decl_type.function.name = try self.allocator.dupe(u8, name_val);
        std.debug.print("Function name: {s}\n", .{decl.decl_type.function.name});

        // Move past function name
        try self.nextToken();

        // Parse function parameters
        if (!self.currentTokenIs(.LPAREN)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected '(' after function name at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingClosingParen;
        }

        try self.nextToken(); // Skip the '('

        // Parse parameters until we hit the closing paren
        while (!self.currentTokenIs(.RPAREN) and !self.currentTokenIs(.EOF)) {
            // If we have multiple parameters, each should be separated by commas
            if (decl.decl_type.function.params.items.len > 0) {
                if (!self.currentTokenIs(.COMMA)) {
                    const msg = try std.fmt.allocPrint(
                        self.allocator,
                        "expected ',' between function parameters at line {d}:{d}",
                        .{ self.current_token.line, self.current_token.column },
                    );
                    try self.error_reporter.addError(msg);
                    return errors.ParserError.MissingComma;
                }
                try self.nextToken(); // Skip the comma
            }

            // Parameter should be an identifier
            if (!self.currentTokenIs(.IDENTIFIER)) {
                if (self.currentTokenIs(.RPAREN)) {
                    // It's okay to have no parameters
                    break;
                }
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected parameter name at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedIdentifier;
            }

            // Create a parameter
            var param = ast.Parameter{
                .name = "",
                .type_name = "",
            };

            // Extract parameter name
            const param_name = switch (self.current_token.value) {
                .string_val => |s| s,
                else => {
                    const msg = try std.fmt.allocPrint(
                        self.allocator,
                        "invalid parameter name format at line {d}:{d}",
                        .{ self.current_token.line, self.current_token.column },
                    );
                    try self.error_reporter.addError(msg);
                    return errors.ParserError.InvalidSyntax;
                },
            };

            param.name = try self.allocator.dupe(u8, param_name);
            try self.nextToken(); // Move past parameter name

            // Parse parameter type
            if (self.currentTokenIs(.COLON)) {
                try self.nextToken(); // Skip the ':'
            }

            // Parameter type is required
            if (!self.currentTokenIs(.IDENTIFIER) and !self.currentTokenIs(.INT_TYPE) and
                !self.currentTokenIs(.FLOAT32_TYPE) and !self.currentTokenIs(.FLOAT64_TYPE) and
                !self.currentTokenIs(.BOOL_TYPE) and !self.currentTokenIs(.STRING_TYPE))
            {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected type for parameter '{s}' at line {d}:{d}",
                    .{ param.name, self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedType;
            }

            // Extract type name
            const type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            param.type_name = try self.allocator.dupe(u8, type_val);
            try self.nextToken(); // Move past type

            // Add the parameter to the function declaration
            try decl.decl_type.function.params.append(param);
        }

        if (!self.currentTokenIs(.RPAREN)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected ')' after function parameters at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingClosingParen;
        }

        // Move past ')'
        try self.nextToken();

        // Parse return type if present
        if (self.currentTokenIs(.IDENTIFIER) or self.currentTokenIs(.INT_TYPE) or
            self.currentTokenIs(.FLOAT32_TYPE) or self.currentTokenIs(.FLOAT64_TYPE) or
            self.currentTokenIs(.BOOL_TYPE) or self.currentTokenIs(.STRING_TYPE))
        {
            // Extract return type name
            const return_type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            decl.decl_type.function.return_type = try self.allocator.dupe(u8, return_type_val);
            try self.nextToken(); // Move past return type
        }

        // Parse function body
        if (!self.currentTokenIs(.LBRACE)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected '{{' to start function body at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingClosingBrace;
        }

        // Skip the entire function body for now
        // This should be expanded to properly parse statements
        var brace_count: usize = 1;
        try self.nextToken(); // Skip the '{'

        while (brace_count > 0 and !self.currentTokenIs(.EOF)) {
            if (self.currentTokenIs(.LBRACE)) {
                brace_count += 1;
            } else if (self.currentTokenIs(.RBRACE)) {
                brace_count -= 1;
            }

            if (brace_count > 0) {
                try self.nextToken();
            }
        }

        if (!self.currentTokenIs(.RBRACE)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected '}}' to end function body at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingClosingBrace;
        }

        // Move past '}'
        try self.nextToken();

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Expect semicolon or newline (implicit semicolon)
    fn expectSemicolon(self: *Parser) !void {
        // If there's an explicit semicolon, consume it
        if (self.currentTokenIs(.SEMICOLON)) {
            try self.nextToken();
            return;
        }

        // Go's semicolon insertion rules (§3):
        // 1. Semicolon can be omitted after:
        //    - an identifier
        //    - an integer, floating-point, imaginary, rune, or string literal
        //    - one of the keywords 'break', 'continue', 'fallthrough', or 'return'
        //    - one of the operators '++', '--', ')', ']', or '}'
        const autoSemicolonTokens = [_]TokenType{
            .IDENTIFIER, .INT_LITERAL, .FLOAT_LITERAL, .STRING_LITERAL,
            .RETURN, // Missing: BREAK, CONTINUE, FALLTHROUGH
            .RPAREN,
            .RBRACE,
            .RBRACKET,
            // Missing: ++, --
        };

        for (autoSemicolonTokens) |token_type| {
            if (self.current_token.type == token_type) {
                // Implicit semicolon is inserted, continue parsing
                std.debug.print("Implicit semicolon inserted after {s}\n", .{@tagName(self.current_token.type)});
                return;
            }
        }

        // 2. To allow complex statements to occupy a single line, a semicolon may be
        //    omitted before a closing ")" or "}"
        if (self.peekTokenIs(.RPAREN) or self.peekTokenIs(.RBRACE)) {
            std.debug.print("Semicolon omitted before closing delimiter\n", .{});
            return;
        }

        // If we get here, a semicolon was expected but not found
        const msg = try std.fmt.allocPrint(
            self.allocator,
            "expected ';' after statement at line {d}:{d}, got {s}",
            .{ self.current_token.line, self.current_token.column, @tagName(self.current_token.type) },
        );
        try self.error_reporter.addError(msg);
        return errors.ParserError.MissingSemicolon;
    }

    /// Parse a variable declaration
    fn parseVarDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing var declaration\n", .{});

        // Create variable declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .variable = ast.VariableDecl{
                    .name = "",
                    .type_name = null,
                    .initializer = null,
                },
            },
        };

        // Advance past 'var' keyword
        try self.nextToken();
        std.debug.print("After var keyword, token: {s}\n", .{@tagName(self.current_token.type)});

        // Parse variable name
        if (!self.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected variable name after 'var' keyword at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.ExpectedIdentifier;
        }

        // Extract variable name
        const name_val = switch (self.current_token.value) {
            .string_val => |s| s,
            else => {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "invalid identifier format at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            },
        };

        // Set variable name
        decl.decl_type.variable.name = try self.allocator.dupe(u8, name_val);

        // Move past variable name
        try self.nextToken();

        // Parse variable type if present (':' type_name or directly type_name)
        if (self.currentTokenIs(.COLON)) {
            try self.nextToken(); // Skip the ':'

            if (!self.currentTokenIs(.IDENTIFIER) and !self.currentTokenIs(.INT_TYPE) and
                !self.currentTokenIs(.FLOAT32_TYPE) and !self.currentTokenIs(.FLOAT64_TYPE) and
                !self.currentTokenIs(.BOOL_TYPE) and !self.currentTokenIs(.STRING_TYPE))
            {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected type name after ':' at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedType;
            }

            // Extract type name
            const type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            // Set variable type
            decl.decl_type.variable.type_name = try self.allocator.dupe(u8, type_val);

            // Move past type name
            try self.nextToken();
        } else if (self.currentTokenIs(.IDENTIFIER) or self.currentTokenIs(.INT_TYPE) or
            self.currentTokenIs(.FLOAT32_TYPE) or self.currentTokenIs(.FLOAT64_TYPE) or
            self.currentTokenIs(.BOOL_TYPE) or self.currentTokenIs(.STRING_TYPE))
        {
            // Go-style var declaration with type directly after var name (no colon)
            // e.g., "var x int = 42"

            // Extract type name
            const type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            // Set variable type
            decl.decl_type.variable.type_name = try self.allocator.dupe(u8, type_val);

            // Move past type name
            try self.nextToken();
        }

        // Parse initializer if present ('=' expression)
        if (self.currentTokenIs(.ASSIGN) or self.currentTokenIs(.DECLARE_ASSIGN)) {
            try self.nextToken(); // Skip the '=' or ':='

            // For simplicity, just skip the expression for now
            // This should be expanded to properly parse expressions
            while (!self.currentTokenIs(.SEMICOLON) and !self.currentTokenIs(.EOF) and
                !self.currentTokenIs(.CONST) and !self.currentTokenIs(.VAR) and
                !self.currentTokenIs(.FUNC) and !self.currentTokenIs(.PACKAGE))
            {
                try self.nextToken();
            }
        }

        // Handle semicolon (explicit or implicit)
        if (self.currentTokenIs(.SEMICOLON)) {
            std.debug.print("Found semicolon after var, proceeding\n", .{});
            try self.nextToken(); // Move past ';'
        } else {
            // In Go, semicolons are optional in many cases
            std.debug.print("No explicit semicolon after var, continuing with token: {s}\n", .{@tagName(self.current_token.type)});
        }

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Parse a constant declaration
    fn parseConstDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing const declaration\n", .{});

        // Create constant declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .variable = ast.VariableDecl{
                    .name = "",
                    .type_name = null,
                    .initializer = null,
                },
            },
        };

        // Advance past 'const' keyword
        try self.nextToken();
        std.debug.print("After const keyword, token: {s}\n", .{@tagName(self.current_token.type)});

        // Parse constant name
        if (!self.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected constant name after 'const' keyword at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.ExpectedIdentifier;
        }

        // Extract constant name
        const name_val = switch (self.current_token.value) {
            .string_val => |s| s,
            else => {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "invalid identifier format at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            },
        };

        // Set constant name
        decl.decl_type.variable.name = try self.allocator.dupe(u8, name_val);

        // Move past constant name
        try self.nextToken();

        // Parse constant type if present (':' type_name or directly type_name)
        if (self.currentTokenIs(.COLON)) {
            try self.nextToken(); // Skip the ':'

            if (!self.currentTokenIs(.IDENTIFIER) and !self.currentTokenIs(.INT_TYPE) and
                !self.currentTokenIs(.FLOAT32_TYPE) and !self.currentTokenIs(.FLOAT64_TYPE) and
                !self.currentTokenIs(.BOOL_TYPE) and !self.currentTokenIs(.STRING_TYPE))
            {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected type name after ':' at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedType;
            }

            // Extract type name
            const type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            // Set constant type
            decl.decl_type.variable.type_name = try self.allocator.dupe(u8, type_val);

            // Move past type name
            try self.nextToken();
        } else if (self.currentTokenIs(.IDENTIFIER) or self.currentTokenIs(.INT_TYPE) or
            self.currentTokenIs(.FLOAT32_TYPE) or self.currentTokenIs(.FLOAT64_TYPE) or
            self.currentTokenIs(.BOOL_TYPE) or self.currentTokenIs(.STRING_TYPE))
        {
            // Go-style var declaration with type directly after var name (no colon)
            // e.g., "const x int = 42"

            // Extract type name
            const type_val = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            // Set constant type
            decl.decl_type.variable.type_name = try self.allocator.dupe(u8, type_val);

            // Move past type name
            try self.nextToken();
        }

        // Constants require initializers, expect '='
        if (!self.currentTokenIs(.ASSIGN)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected '=' after constant declaration at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            std.debug.print("Missing initializer, current token: {s}\n", .{@tagName(self.current_token.type)});
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingEquals;
        }

        try self.nextToken(); // Skip the '='

        // For simplicity, just skip the expression for now
        // This should be expanded to properly parse expressions
        while (!self.currentTokenIs(.SEMICOLON) and !self.currentTokenIs(.EOF) and
            !self.currentTokenIs(.CONST) and !self.currentTokenIs(.VAR) and
            !self.currentTokenIs(.FUNC) and !self.currentTokenIs(.PACKAGE))
        {
            try self.nextToken();
        }

        // Handle semicolon (explicit or implicit)
        if (self.currentTokenIs(.SEMICOLON)) {
            std.debug.print("Found semicolon after const, proceeding\n", .{});
            try self.nextToken(); // Move past ';'
        } else {
            // In Go, semicolons are optional in many cases
            std.debug.print("No explicit semicolon after const, continuing with token: {s}\n", .{@tagName(self.current_token.type)});
        }

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Parse a type declaration (struct, interface, type alias)
    fn parseTypeDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing type declaration\n", .{});

        // Create type declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .struct_decl = ast.StructDecl{
                    .name = "",
                    .fields = std.ArrayList(ast.StructField).init(self.allocator),
                },
            },
        };

        // Advance past 'type' keyword
        try self.nextToken();
        std.debug.print("After type keyword, token: {s}\n", .{@tagName(self.current_token.type)});

        // Parse type name
        if (!self.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected type name after 'type' keyword at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.ExpectedIdentifier;
        }

        // Extract type name
        const name_val = switch (self.current_token.value) {
            .string_val => |s| s,
            else => {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "invalid identifier format at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            },
        };

        // Set type name
        decl.decl_type.struct_decl.name = try self.allocator.dupe(u8, name_val);

        // Move past type name
        try self.nextToken();

        // Check if this is an interface declaration
        if (self.currentTokenIs(.LBRACE)) {
            // For simplicity, we're treating all type declarations with immediate braces
            // as struct declarations. In a more complete implementation, we'd first check
            // if it's an interface based on the contents.
            try self.parseStructBody(&decl.decl_type.struct_decl);
        } else {
            // This is a type alias or another form of type declaration
            // This would require expanding the AST to properly represent all possible type
            // declarations in Go, such as aliases, interfaces, etc.

            // Skip the rest of the declaration for now
            while (!self.currentTokenIs(.SEMICOLON) and !self.currentTokenIs(.EOF)) {
                try self.nextToken();
            }

            // Handle semicolon
            try self.expectSemicolon();
        }

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Parse a struct body
    fn parseStructBody(self: *Parser, struct_decl: *ast.StructDecl) !void {
        // Advance past '{'
        try self.nextToken();

        // Parse fields until we hit the closing brace
        while (!self.currentTokenIs(.RBRACE) and !self.currentTokenIs(.EOF)) {
            // Parse field name
            if (!self.currentTokenIs(.IDENTIFIER)) {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected field name in struct at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedIdentifier;
            }

            // Extract field name
            const field_name = switch (self.current_token.value) {
                .string_val => |s| s,
                else => {
                    const msg = try std.fmt.allocPrint(
                        self.allocator,
                        "invalid field name format at line {d}:{d}",
                        .{ self.current_token.line, self.current_token.column },
                    );
                    try self.error_reporter.addError(msg);
                    return errors.ParserError.InvalidSyntax;
                },
            };

            // Move past field name
            try self.nextToken();

            // Parse field type
            if (!self.currentTokenIs(.IDENTIFIER) and !self.currentTokenIs(.INT_TYPE) and
                !self.currentTokenIs(.FLOAT32_TYPE) and !self.currentTokenIs(.FLOAT64_TYPE) and
                !self.currentTokenIs(.BOOL_TYPE) and !self.currentTokenIs(.STRING_TYPE))
            {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected type for field '{s}' at line {d}:{d}",
                    .{ field_name, self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.ExpectedType;
            }

            // Extract field type
            const field_type = switch (self.current_token.value) {
                .string_val => |s| s,
                else => @tagName(self.current_token.type),
            };

            // Create field
            const field = ast.StructField{
                .name = try self.allocator.dupe(u8, field_name),
                .type_name = try self.allocator.dupe(u8, field_type),
            };

            // Add field to struct
            try struct_decl.fields.append(field);

            // Move past field type
            try self.nextToken();

            // Expect semicolon or comma between fields
            if (self.currentTokenIs(.SEMICOLON) or self.currentTokenIs(.COMMA)) {
                try self.nextToken();
            } else {
                // In Go, semicolons are optional in many cases
                std.debug.print("No explicit field separator, continuing with token: {s}\n", .{@tagName(self.current_token.type)});
            }
        }

        if (!self.currentTokenIs(.RBRACE)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected '}}' to close struct declaration at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.MissingClosingBrace;
        }

        // Move past '}'
        try self.nextToken();
    }

    /// Parse an import declaration
    fn parseImportDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing import declaration\n", .{});

        // Create import declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .import = ast.ImportDecl{
                    .path = "",
                },
            },
        };

        // Advance past 'import' keyword
        try self.nextToken();
        std.debug.print("After import keyword, token: {s}\n", .{@tagName(self.current_token.type)});

        // Check if this is a grouped import
        if (self.currentTokenIs(.LPAREN)) {
            try self.nextToken(); // Skip the '('

            // Parse multiple imports until we hit ')'
            while (!self.currentTokenIs(.RPAREN) and !self.currentTokenIs(.EOF)) {
                // Skip any semicolons between imports
                if (self.currentTokenIs(.SEMICOLON)) {
                    try self.nextToken();
                    continue;
                }

                // Each import in a group must be a string literal
                if (!self.currentTokenIs(.STRING_LITERAL)) {
                    const msg = try std.fmt.allocPrint(
                        self.allocator,
                        "expected import path string at line {d}:{d}",
                        .{ self.current_token.line, self.current_token.column },
                    );
                    try self.error_reporter.addError(msg);
                    return errors.ParserError.InvalidSyntax;
                }

                // Extract import path
                const import_path = switch (self.current_token.value) {
                    .string_val => |s| s,
                    else => {
                        const msg = try std.fmt.allocPrint(
                            self.allocator,
                            "invalid import path format at line {d}:{d}",
                            .{ self.current_token.line, self.current_token.column },
                        );
                        try self.error_reporter.addError(msg);
                        return errors.ParserError.InvalidSyntax;
                    },
                };

                // For grouped imports, we're only returning the first one for now
                // A more complete implementation would handle all imports in the group
                decl.decl_type.import.path = try self.allocator.dupe(u8, import_path);

                // Move past the import path
                try self.nextToken();

                // For simplicity, we're only capturing the first import in the group
                // In a complete implementation, we'd create a separate import declaration for each
                break;
            }

            // Check that we found the closing paren
            if (!self.currentTokenIs(.RPAREN)) {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected ')' to close import group at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.MissingClosingParen;
            }

            // Move past ')'
            try self.nextToken();
        } else {
            // Single import - must be a string literal
            if (!self.currentTokenIs(.STRING_LITERAL)) {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "expected import path string at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            }

            // Extract import path
            const import_path = switch (self.current_token.value) {
                .string_val => |s| s,
                else => {
                    const msg = try std.fmt.allocPrint(
                        self.allocator,
                        "invalid import path format at line {d}:{d}",
                        .{ self.current_token.line, self.current_token.column },
                    );
                    try self.error_reporter.addError(msg);
                    return errors.ParserError.InvalidSyntax;
                },
            };

            decl.decl_type.import.path = try self.allocator.dupe(u8, import_path);

            // Move past the import path
            try self.nextToken();
        }

        // Handle semicolon
        try self.expectSemicolon();

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Parse a package declaration
    fn parsePackageDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing package declaration\n", .{});

        // Create package declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up basic node properties
        decl.* = ast.Declaration{
            .node = ast.Node{
                .node_type = .declaration,
                .start_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
                .end_loc = .{
                    .line = self.current_token.line,
                    .column = self.current_token.column,
                },
            },
            .decl_type = .{
                .package = ast.PackageDecl{
                    .name = "",
                },
            },
        };

        // Advance past 'package' keyword
        try self.nextToken();

        // Expect an identifier after 'package'
        if (!self.currentTokenIs(.IDENTIFIER)) {
            const msg = try std.fmt.allocPrint(
                self.allocator,
                "expected package name after 'package' keyword at line {d}:{d}",
                .{ self.current_token.line, self.current_token.column },
            );
            try self.error_reporter.addError(msg);
            return errors.ParserError.ExpectedIdentifier;
        }

        // Extract package name
        const package_name = switch (self.current_token.value) {
            .string_val => |s| s,
            else => {
                const msg = try std.fmt.allocPrint(
                    self.allocator,
                    "invalid package name format at line {d}:{d}",
                    .{ self.current_token.line, self.current_token.column },
                );
                try self.error_reporter.addError(msg);
                return errors.ParserError.InvalidSyntax;
            },
        };

        // Set package name
        decl.decl_type.package.name = try self.allocator.dupe(u8, package_name);
        std.debug.print("Found package name: {s}\n", .{package_name});

        // Advance past the package name
        try self.nextToken();

        // Handle semicolon (explicit or implicit)
        try self.expectSemicolon();

        // Update end location after parsing
        decl.node.end_loc = .{
            .line = self.current_token.line,
            .column = self.current_token.column,
        };

        return decl;
    }

    /// Parse a comptime declaration (Goo extension)
    fn parseComptimeDeclaration(self: *Parser) !*ast.Declaration {
        std.debug.print("Parsing comptime declaration\n", .{});

        // Create the bare declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up the node properties
        decl.node.node_type = .declaration;
        decl.node.start_loc.line = self.current_token.line;
        decl.node.start_loc.column = self.current_token.column;
        decl.node.end_loc.line = self.current_token.line;
        decl.node.end_loc.column = self.current_token.column;

        // These variables would be used in a proper implementation
        // Using underscore prefix to indicate intentionally unused for now
        const _empty_name = try self.allocator.dupe(u8, "");
        var _statements = std.ArrayList(ast.Statement).init(self.allocator);

        // Prevent memory leak since we're not using these yet
        self.allocator.free(_empty_name);
        _statements.deinit();

        // TODO: Leaving decl.decl_type uninitialized to fix compiler issues with union initialization
        // This needs to be properly addressed in a future version
        // The union should be initialized to a comptime field with empty_name and statements

        // Advance past 'comptime' keyword and return
        // This is a placeholder implementation
        try self.nextToken();
        return decl;
    }

    /// Parse a parallel declaration (Goo extension)
    fn parseParallelDeclaration(self: *Parser, is_distributed: bool) !*ast.Declaration {
        std.debug.print("Parsing parallel declaration (distributed: {any})\n", .{is_distributed});

        // Create the bare declaration node
        var decl = try self.allocator.create(ast.Declaration);

        // Set up the node properties
        decl.node.node_type = .declaration;
        decl.node.start_loc.line = self.current_token.line;
        decl.node.start_loc.column = self.current_token.column;
        decl.node.end_loc.line = self.current_token.line;
        decl.node.end_loc.column = self.current_token.column;

        // These variables would be used in a proper implementation
        // Using underscore prefix to indicate intentionally unused for now
        var _statements = std.ArrayList(ast.Statement).init(self.allocator);

        // Prevent memory leak since we're not using these yet
        _statements.deinit();

        // TODO: Leaving decl.decl_type uninitialized to fix compiler issues with union initialization
        // This needs to be properly addressed in a future version
        // The union should be initialized to a parallel field with is_distributed and a block

        // Advance past keyword and return
        // This is a placeholder implementation
        try self.nextToken();
        return decl;
    }
};

/// Get the precedence of a token type
fn getPrecedence(token_type: TokenType) Precedence {
    return switch (token_type) {
        .EQ, .NOT_EQ => .EQUALITY,
        .LT, .GT, .LT_EQ, .GT_EQ => .COMPARISON,
        .PLUS, .MINUS => .SUM,
        .ASTERISK, .SLASH, .PERCENT => .PRODUCT,
        .LPAREN => .CALL,
        .LBRACKET => .INDEX,
        .DOT => .CALL,
        .ASSIGN => .ASSIGN,
        .AND => .LOGICAL_AND,
        .OR => .LOGICAL_OR,
        else => .LOWEST,
    };
}
