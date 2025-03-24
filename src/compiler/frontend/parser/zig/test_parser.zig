const std = @import("std");
const parser = @import("parser");
const lexer = @import("lexer");
const ast = @import("ast");
const errors = @import("errors");
const testing = std.testing;

const Parser = parser.Parser;
const Lexer = lexer.Lexer;
const ParserError = errors.ParserError;

// Testing equality functions
pub fn expectEqualDeep(expected: anytype, actual: @TypeOf(expected)) !void {
    try testing.expectEqual(expected, actual);
}

pub fn expectEqualStrings(expected: []const u8, actual: []const u8) !void {
    try testing.expectEqualStrings(expected, actual);
}

test "parse package declaration" {
    const source = "package example;";
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes
    // For now, we're letting the test allocator detect leaks

    try testing.expect(program.package != null);
    try testing.expectEqual(ast.NodeType.PACKAGE_DECL, program.package.?.base.node_type);
    try testing.expectEqualStrings("example", program.package.?.name.?.value);
}

test "parse import declaration" {
    const source =
        \\package example;
        \\import "std/io";
    ;
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes

    try testing.expect(program.package != null);
    try testing.expectEqual(@as(usize, 1), program.imports.items.len);
    try testing.expectEqualStrings("std/io", program.imports.items[0].path.?.value);
}

test "parse function declaration" {
    const source =
        \\package example;
        \\
        \\func add(a: int, b: int): int {
        \\    return a + b;
        \\}
    ;
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes

    try testing.expect(program.package != null);
    try testing.expectEqual(@as(usize, 1), program.declarations.items.len);

    const func_node = @as(*ast.FunctionNode, @ptrCast(program.declarations.items[0]));
    try testing.expectEqual(ast.NodeType.FUNCTION_DECL, func_node.node.node_type);
    try testing.expectEqualStrings("add", func_node.name.?.value);
    try testing.expectEqual(@as(usize, 2), func_node.parameters.items.len);
}

test "parse variable declarations" {
    const source =
        \\package example;
        \\
        \\func main() {
        \\    var x: int = 42;
        \\    const y = "hello";
        \\}
    ;
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes

    try testing.expect(program.package != null);
    try testing.expectEqual(@as(usize, 1), program.declarations.items.len);

    const func_node = @as(*ast.FunctionNode, @ptrCast(program.declarations.items[0]));
    try testing.expectEqual(ast.NodeType.FUNCTION_DECL, func_node.node.node_type);
    try testing.expectEqualStrings("main", func_node.name.?.value);
    try testing.expectEqual(@as(usize, 0), func_node.parameters.items.len);

    // Check block contains two statements
    try testing.expectEqual(@as(usize, 2), func_node.body.?.statements.items.len);
}

test "parse control flow" {
    const source =
        \\package example;
        \\
        \\func test(x: int) {
        \\    if x > 0 {
        \\        return x;
        \\    } else {
        \\        return -x;
        \\    }
        \\}
    ;
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes

    try testing.expect(program.package != null);
    try testing.expectEqual(@as(usize, 1), program.declarations.items.len);

    const func_node = @as(*ast.FunctionNode, @ptrCast(program.declarations.items[0]));
    try testing.expectEqual(ast.NodeType.FUNCTION_DECL, func_node.node.node_type);
    try testing.expectEqualStrings("test", func_node.name.?.value);
    try testing.expectEqual(@as(usize, 1), func_node.parameters.items.len);

    // Check block contains one statement (the if statement)
    try testing.expectEqual(@as(usize, 1), func_node.body.?.statements.items.len);

    // Check if statement
    const if_stmt = @as(*ast.IfStmtNode, @ptrCast(func_node.body.?.statements.items[0]));
    try testing.expectEqual(ast.NodeType.IF_STMT, if_stmt.node.node_type);
    try testing.expect(if_stmt.consequence != null);
    try testing.expect(if_stmt.alternative != null);
}

test "parse complete program" {
    const source =
        \\package example;
        \\import "std/io";
        \\
        \\func fibonacci(n: int): int {
        \\    if n <= 1 {
        \\        return n;
        \\    }
        \\    return fibonacci(n - 1) + fibonacci(n - 2);
        \\}
        \\
        \\func main() {
        \\    for i := 0; i < 10; i = i + 1 {
        \\        io.println(fibonacci(i));
        \\    }
        \\}
    ;
    var lex = try Lexer.init(testing.allocator, source);
    var p = try Parser.init(&lex, testing.allocator);
    defer p.deinit();

    const program = try p.parseProgram();
    // Note: In a real implementation, we'd need proper deinit for AST nodes

    try testing.expect(program.package != null);
    try testing.expectEqual(@as(usize, 1), program.imports.items.len);
    try testing.expectEqual(@as(usize, 2), program.declarations.items.len);

    try testing.expectEqualStrings("example", program.package.?.name.?.value);
    try testing.expectEqualStrings("std/io", program.imports.items[0].path.?.value);
}

// Main entry point for test executable
pub fn main() !void {
    var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    // Read input from stdin
    const stdin = std.io.getStdIn();
    const input = try stdin.readToEndAlloc(allocator, 1024 * 1024); // 1MB max
    defer allocator.free(input);

    // Print the input for debugging
    try std.io.getStdOut().writer().print("Input program:\n{s}\n", .{input});

    // Setup error reporter
    var error_reporter = try errors.ErrorReporter.init(allocator);
    defer error_reporter.deinit();

    // Initialize lexer with input
    var lex = try lexer.Lexer.init(allocator, input);
    defer lex.deinit();

    // Debug: print all tokens
    try std.io.getStdOut().writer().print("Tokens:\n", .{});
    var debug_lexer = try lexer.Lexer.init(allocator, input);
    defer debug_lexer.deinit();

    while (true) {
        const token = try debug_lexer.nextToken();
        try std.io.getStdOut().writer().print("  {s}", .{@tagName(token.type)});
        if (token.type == .IDENTIFIER or token.type == .STRING_LITERAL or
            token.type == .INT_LITERAL or token.type == .FLOAT_LITERAL)
        {
            const value_str = try token.getValueString(allocator);
            defer allocator.free(value_str);
            try std.io.getStdOut().writer().print(" ({s})", .{value_str});
        }
        try std.io.getStdOut().writer().print("\n", .{});

        if (token.type == .EOF) break;
    }

    // Initialize parser with lexer
    var p = try parser.Parser.init(&lex, allocator, error_reporter);
    defer p.deinit();

    // Parse the program
    const program = try p.parseProgram();

    // Print any errors
    if (error_reporter.errors.items.len > 0) {
        try std.io.getStdErr().writer().print("Parsing failed with errors:\n", .{});
        for (error_reporter.errors.items) |err| {
            try std.io.getStdErr().writer().print("  {s}\n", .{err});
        }
        return error.ParseFailed;
    }

    // Print the parsed AST
    try std.io.getStdOut().writer().print("Successfully parsed program with {d} declarations\n", .{program.declarations.items.len});

    // Print AST with more detailed information
    try std.io.getStdOut().writer().print("Program with {d} declarations:\n", .{program.declarations.items.len});

    for (program.declarations.items) |node| {
        // Try to cast to Declaration type for better printing
        if (node.node_type == .declaration) {
            const decl = @as(*ast.Declaration, @ptrCast(@alignCast(node)));
            try prettyPrintAST(decl, std.io.getStdOut().writer(), 1);
        } else {
            try prettyPrintAST(node, std.io.getStdOut().writer(), 1);
        }
    }

    return;
}

// Pretty print the AST
fn prettyPrintAST(node: anytype, writer: anytype, indent: usize) !void {
    const T = @TypeOf(node);

    // Print indentation
    try writer.writeByteNTimes(' ', indent * 2);

    // Print node information based on type
    if (T == *ast.Program) {
        try writer.print("Program with {d} declarations:\n", .{node.declarations.items.len});
        for (node.declarations.items) |decl| {
            try prettyPrintAST(decl, writer, indent + 1);
        }
    } else if (T == *ast.Node) {
        try writer.print("Node type: {s}\n", .{@tagName(node.node_type)});
    } else if (T == *ast.Declaration) {
        const decl_type_name = switch (node.decl_type) {
            .function => "Function",
            .variable => "Variable",
            .struct_decl => "Struct",
            .import => "Import",
        };
        try writer.print("Declaration ({s}) at line {d}:{d}\n", .{
            decl_type_name,
            node.node.start_loc.line,
            node.node.start_loc.column,
        });

        switch (node.decl_type) {
            .function => |func| {
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Name: {s}\n", .{func.name});

                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Parameters ({d}):\n", .{func.params.items.len});

                // Print each parameter
                for (func.params.items) |param| {
                    try writer.writeByteNTimes(' ', (indent + 2) * 2);
                    try writer.print("{s}: {s}\n", .{ param.name, param.type_name });
                }

                if (func.return_type) |return_type| {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Return type: {s}\n", .{return_type});
                } else {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Return type: none\n", .{});
                }

                // Print the body statements count
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Body statements: {d}\n", .{func.body.statements.items.len});
            },
            .variable => |var_decl| {
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Name: {s}\n", .{var_decl.name});

                if (var_decl.type_name) |type_name| {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Type: {s}\n", .{type_name});
                } else {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Type: inferred\n", .{});
                }

                if (var_decl.initializer != null) {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Has initializer: true\n", .{});
                } else {
                    try writer.writeByteNTimes(' ', (indent + 1) * 2);
                    try writer.print("Has initializer: false\n", .{});
                }
            },
            .struct_decl => |struct_decl| {
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Name: {s}\n", .{struct_decl.name});
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Fields: {d}\n", .{struct_decl.fields.items.len});
            },
            .import => |import_decl| {
                try writer.writeByteNTimes(' ', (indent + 1) * 2);
                try writer.print("Path: {s}\n", .{import_decl.path});
            },
        }
    } else {
        try writer.print("Unknown node type: {s}\n", .{@typeName(T)});
    }
}
