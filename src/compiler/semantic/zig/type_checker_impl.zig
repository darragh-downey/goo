const std = @import("std");
const parser = @import("../../frontend/parser/zig/parser.zig");
const ast = parser.ast;
const type_checker = @import("type_checker.zig");
const TypeChecker = type_checker.TypeChecker;
const TypeCheckError = type_checker.TypeCheckError;
const Type = type_checker.Type;
const TypeKind = type_checker.TypeKind;
const Symbol = type_checker.Symbol;
const SymbolType = type_checker.SymbolType;

/// Implementation of type checking for package declarations
pub fn checkPackageDeclaration(self: *TypeChecker, node: *ast.PackageNode) !void {
    if (node.name == null) {
        return;
    }

    // Register the package name in the symbol table
    const package_name = node.name.?.value;
    _ = try self.symbol_table.define(
        package_name,
        .PACKAGE,
        try self.getBuiltinType("void"),
        &node.base,
    );
}

/// Implementation of type checking for import declarations
pub fn checkImportDeclaration(self: *TypeChecker, node: *ast.ImportNode) !void {
    if (node.path == null) {
        return;
    }

    // Register the import in the symbol table
    const import_path = node.path.?.value;
    const import_name = if (node.alias) |alias| alias.value else getImportName(import_path);

    _ = try self.symbol_table.define(
        import_name,
        .IMPORT,
        try self.getBuiltinType("void"),
        &node.base,
    );
}

/// Extract import name from path (e.g., "fmt" from "std/fmt")
fn getImportName(path: []const u8) []const u8 {
    // Remove quotes if present
    var clean_path = path;
    if (clean_path.len > 1 and (clean_path[0] == '"' or clean_path[0] == '\'')) {
        clean_path = clean_path[1 .. clean_path.len - 1];
    }

    // Find the last slash and return everything after it
    const last_slash = std.mem.lastIndexOf(u8, clean_path, "/");
    if (last_slash) |idx| {
        return clean_path[idx + 1 ..];
    }

    return clean_path;
}

/// Implementation of type checking for function declarations
pub fn checkFunctionDeclaration(self: *TypeChecker, node: *ast.FunctionNode) !*Type {
    if (node.name == null) {
        try self.addError(&node.node, "Function missing name");
        return try self.getErrorType();
    }

    // Determine return type
    var return_type = try self.getBuiltinType("void");
    if (node.return_type != null) {
        return_type = try self.resolveTypeExpr(node.return_type.?);
    }

    // Create function type
    var func_type = try Type.init(self.allocator, .FUNCTION, "function");
    func_type.return_type = return_type;

    // Add parameters to function type
    func_type.parameter_types = std.ArrayList(*Type).init(self.allocator);

    // Register function in symbol table
    const func_name = node.name.?.value;
    const func_symbol = try self.symbol_table.define(
        func_name,
        .FUNCTION,
        func_type,
        &node.node,
    );

    // Remember the current function
    const prev_function = self.current_function;
    self.current_function = node;

    // Enter a new scope for function body
    self.symbol_table.enterScope();

    // Check parameters
    for (node.parameters.items) |param| {
        if (param.name == null) {
            try self.addError(&param.node, "Parameter missing name");
            continue;
        }

        var param_type = try self.getBuiltinType("void");
        if (param.param_type != null) {
            param_type = try self.resolveTypeExpr(param.param_type.?);
        }

        // Add parameter to function type
        try func_type.parameter_types.?.append(param_type);

        // Register parameter in local scope
        const param_name = param.name.?.value;
        _ = try self.symbol_table.define(
            param_name,
            .VARIABLE,
            param_type,
            &param.node,
        );
    }

    // Check function body
    if (node.body != null) {
        const body_type = try self.checkBlockStatement(node.body.?);

        // Check return type compatibility
        if (!isTypeCompatible(return_type, body_type)) {
            try self.addError(&node.node, "Function body type doesn't match return type");
        }
    }

    // Leave function scope
    self.symbol_table.leaveScope();

    // Restore previous function
    self.current_function = prev_function;

    return func_type;
}

/// Implementation of type checking for variable declarations
pub fn checkVarDeclaration(self: *TypeChecker, node: *ast.VarDeclNode) !*Type {
    if (node.name == null) {
        try self.addError(&node.node, "Variable declaration missing name");
        return try self.getErrorType();
    }

    // Determine variable type
    var var_type: *Type = undefined;

    if (node.var_type != null) {
        // If type is explicitly specified
        var_type = try self.resolveTypeExpr(node.var_type.?);
    } else if (node.value != null) {
        // Infer type from value
        const value_type = try self.checkNode(node.value.?);
        if (value_type != null) {
            var_type = value_type;
        } else {
            var_type = try self.getErrorType();
        }
    } else {
        try self.addError(&node.node, "Variable declaration must have either type or initializer");
        return try self.getErrorType();
    }

    // Register variable in symbol table
    const var_name = node.name.?.value;
    const var_symbol = try self.symbol_table.define(
        var_name,
        .VARIABLE,
        var_type,
        &node.node,
    );

    // If there's an initializer, check type compatibility
    if (node.value != null) {
        const init_type = try self.checkNode(node.value.?);
        if (init_type != null and !isTypeCompatible(var_type, init_type)) {
            try self.addError(&node.node, "Initializer type doesn't match variable type");
        }
        var_symbol.is_initialized = true;
    }

    return var_type;
}

/// Implementation of type checking for constant declarations
pub fn checkConstDeclaration(self: *TypeChecker, node: *ast.ConstDeclNode) !*Type {
    if (node.name == null) {
        try self.addError(&node.node, "Constant declaration missing name");
        return try self.getErrorType();
    }

    if (node.value == null) {
        try self.addError(&node.node, "Constant declaration must have an initializer");
        return try self.getErrorType();
    }

    // Determine constant type
    var const_type: *Type = undefined;

    if (node.const_type != null) {
        // If type is explicitly specified
        const_type = try self.resolveTypeExpr(node.const_type.?);
    } else {
        // Infer type from value
        const value_type = try self.checkNode(node.value.?);
        if (value_type != null) {
            const_type = value_type;
        } else {
            const_type = try self.getErrorType();
        }
    }

    // Register constant in symbol table
    const const_name = node.name.?.value;
    const const_symbol = try self.symbol_table.define(
        const_name,
        .CONSTANT,
        const_type,
        &node.node,
    );

    // Check initializer type compatibility
    const init_type = try self.checkNode(node.value.?);
    if (init_type != null and !isTypeCompatible(const_type, init_type)) {
        try self.addError(&node.node, "Initializer type doesn't match constant type");
    }
    const_symbol.is_initialized = true;

    return const_type;
}

/// Implementation of type checking for block statements
pub fn checkBlockStatement(self: *TypeChecker, node: *ast.BlockNode) !*Type {
    // Enter a new scope for the block
    self.symbol_table.enterScope();

    var block_type = try self.getBuiltinType("void");

    // Check all statements in the block
    for (node.statements.items) |stmt| {
        const stmt_type = try self.checkNode(stmt);

        // The block type is the type of the last statement (useful for expression blocks)
        if (stmt_type != null) {
            block_type = stmt_type;
        }
    }

    // Leave the block scope
    self.symbol_table.leaveScope();

    return block_type;
}

/// Implementation of type checking for if statements
pub fn checkIfStatement(self: *TypeChecker, node: *ast.IfStmtNode) !*Type {
    // Check condition
    if (node.condition != null) {
        const cond_type = try self.checkNode(node.condition.?);
        if (cond_type != null and cond_type.kind != .BOOL) {
            try self.addError(&node.node, "If condition must be a boolean expression");
        }
    } else {
        try self.addError(&node.node, "If statement missing condition");
    }

    // Check consequence block
    if (node.consequence != null) {
        _ = try self.checkBlockStatement(node.consequence.?);
    }

    // Check alternative block if exists
    if (node.alternative != null) {
        _ = try self.checkBlockStatement(node.alternative.?);
    }

    return try self.getBuiltinType("void");
}

/// Implementation of type checking for for statements
pub fn checkForStatement(self: *TypeChecker, node: *ast.ForStmtNode) !*Type {
    // Enter a new scope for the loop
    self.symbol_table.enterScope();

    // Check initialization if present
    if (node.init != null) {
        _ = try self.checkNode(node.init.?);
    }

    // Check condition if present
    if (node.condition != null) {
        const cond_type = try self.checkNode(node.condition.?);
        if (cond_type != null and cond_type.kind != .BOOL) {
            try self.addError(&node.node, "For loop condition must be a boolean expression");
        }
    }

    // Check update if present
    if (node.update != null) {
        _ = try self.checkNode(node.update.?);
    }

    // Check body
    if (node.body != null) {
        _ = try self.checkBlockStatement(node.body.?);
    }

    // Leave the loop scope
    self.symbol_table.leaveScope();

    return try self.getBuiltinType("void");
}

/// Implementation of type checking for return statements
pub fn checkReturnStatement(self: *TypeChecker, node: *ast.ReturnStmtNode) !*Type {
    // Make sure we're in a function
    if (self.current_function == null) {
        try self.addError(&node.node, "Return statement outside of function");
        return try self.getErrorType();
    }

    // Determine the expected return type from the current function
    const func_symbol = self.symbol_table.lookup(self.current_function.?.name.?.value);
    if (func_symbol == null or func_symbol.?.data_type == null) {
        try self.addError(&node.node, "Cannot determine function return type");
        return try self.getErrorType();
    }

    const expected_type = func_symbol.?.data_type.?.return_type.?;

    // Check the actual return value
    if (node.value != null) {
        const actual_type = try self.checkNode(node.value.?);
        if (actual_type != null and !isTypeCompatible(expected_type, actual_type)) {
            try self.addError(&node.node, "Return value type doesn't match function return type");
        }
        return actual_type;
    } else {
        // Void return
        const void_type = try self.getBuiltinType("void");
        if (!isTypeCompatible(expected_type, void_type)) {
            try self.addError(&node.node, "Function must return a value");
        }
        return void_type;
    }
}

/// Implementation of type checking for expression statements
pub fn checkExpressionStatement(self: *TypeChecker, node: *ast.ExprStmtNode) !*Type {
    if (node.expression != null) {
        return try self.checkNode(node.expression.?);
    }
    return try self.getBuiltinType("void");
}

/// Implementation of type checking for call expressions
pub fn checkCallExpression(self: *TypeChecker, node: *ast.CallExprNode) !*Type {
    if (node.function == null) {
        try self.addError(&node.node, "Call expression missing function");
        return try self.getErrorType();
    }

    // Get the function's type
    const func_type = try self.checkNode(node.function.?);
    if (func_type == null or func_type.kind != .FUNCTION) {
        try self.addError(&node.node, "Calling a non-function value");
        return try self.getErrorType();
    }

    // Check argument count
    const expected_arg_count = func_type.parameter_types.?.items.len;
    const actual_arg_count = node.arguments.items.len;

    if (expected_arg_count != actual_arg_count) {
        try self.addError(&node.node, "Function call has wrong number of arguments");
    }

    // Check argument types
    const min_arg_count = @min(expected_arg_count, actual_arg_count);
    for (0..min_arg_count) |i| {
        const expected_type = func_type.parameter_types.?.items[i];
        const arg = node.arguments.items[i];
        const actual_type = try self.checkNode(arg);

        if (actual_type != null and !isTypeCompatible(expected_type, actual_type)) {
            try self.addError(&node.node, "Argument type doesn't match parameter type");
        }
    }

    // Return the function's return type
    return func_type.return_type.?;
}

/// Implementation of type checking for identifier references
pub fn checkIdentifier(self: *TypeChecker, node: *ast.IdentifierNode) !*Type {
    const symbol = self.symbol_table.lookup(node.value);
    if (symbol == null) {
        try self.addError(&node.node, "Undefined identifier");
        return try self.getErrorType();
    }

    if (symbol.?.data_type == null) {
        try self.addError(&node.node, "Identifier has no type");
        return try self.getErrorType();
    }

    // Check if the variable is initialized
    if (symbol.?.symbol_type == .VARIABLE and !symbol.?.is_initialized) {
        try self.addError(&node.node, "Variable used before initialization");
    }

    return symbol.?.data_type.?;
}

/// Implementation of type checking for prefix expressions
pub fn checkPrefixExpression(self: *TypeChecker, node: *ast.PrefixExprNode) !*Type {
    if (node.right == null) {
        try self.addError(&node.node, "Prefix expression missing operand");
        return try self.getErrorType();
    }

    const right_type = try self.checkNode(node.right.?);
    if (right_type == null) {
        return try self.getErrorType();
    }

    // Check operator validity based on operand type
    const op = node.operator;

    if (std.mem.eql(u8, op, "!")) {
        if (right_type.kind != .BOOL) {
            try self.addError(&node.node, "Logical NOT requires boolean operand");
            return try self.getErrorType();
        }
        return right_type;
    } else if (std.mem.eql(u8, op, "-")) {
        if (right_type.kind != .INT and right_type.kind != .FLOAT) {
            try self.addError(&node.node, "Negation requires numeric operand");
            return try self.getErrorType();
        }
        return right_type;
    } else {
        try self.addError(&node.node, "Unsupported prefix operator");
        return try self.getErrorType();
    }
}

/// Implementation of type checking for infix expressions
pub fn checkInfixExpression(self: *TypeChecker, node: *ast.InfixExprNode) !*Type {
    if (node.left == null or node.right == null) {
        try self.addError(&node.node, "Infix expression missing operand");
        return try self.getErrorType();
    }

    const left_type = try self.checkNode(node.left.?);
    const right_type = try self.checkNode(node.right.?);

    if (left_type == null or right_type == null) {
        return try self.getErrorType();
    }

    // Check operator compatibility with operand types
    const op = node.operator;

    // Arithmetic operators
    if (std.mem.eql(u8, op, "+") or std.mem.eql(u8, op, "-") or
        std.mem.eql(u8, op, "*") or std.mem.eql(u8, op, "/"))
    {

        // Allow numeric operations
        if ((left_type.kind == .INT or left_type.kind == .FLOAT) and
            (right_type.kind == .INT or right_type.kind == .FLOAT))
        {

            // If either operand is a float, the result is a float
            if (left_type.kind == .FLOAT or right_type.kind == .FLOAT) {
                return try self.getBuiltinType("float");
            } else {
                return try self.getBuiltinType("int");
            }
        }

        // Special case for string concatenation with +
        if (std.mem.eql(u8, op, "+") and left_type.kind == .STRING and right_type.kind == .STRING) {
            return try self.getBuiltinType("string");
        }

        try self.addError(&node.node, "Operands must be numeric (or strings for +)");
        return try self.getErrorType();
    }

    // Comparison operators
    if (std.mem.eql(u8, op, "==") or std.mem.eql(u8, op, "!=") or
        std.mem.eql(u8, op, "<") or std.mem.eql(u8, op, ">") or
        std.mem.eql(u8, op, "<=") or std.mem.eql(u8, op, ">="))
    {

        // Equality checks can be performed on any two values of the same type
        if (std.mem.eql(u8, op, "==") or std.mem.eql(u8, op, "!=")) {
            if (!isTypeCompatible(left_type, right_type)) {
                try self.addError(&node.node, "Cannot compare values of different types");
            }
        } else {
            // Order comparisons require numeric or string types
            if (!((left_type.kind == .INT or left_type.kind == .FLOAT or left_type.kind == .STRING) and
                (right_type.kind == .INT or right_type.kind == .FLOAT or right_type.kind == .STRING)))
            {
                try self.addError(&node.node, "Order comparison requires numeric or string operands");
            }

            // Cannot compare strings with numbers
            if ((left_type.kind == .STRING and right_type.kind != .STRING) or
                (left_type.kind != .STRING and right_type.kind == .STRING))
            {
                try self.addError(&node.node, "Cannot compare string with non-string");
            }
        }

        return try self.getBuiltinType("bool");
    }

    // Logical operators
    if (std.mem.eql(u8, op, "&&") or std.mem.eql(u8, op, "||")) {
        if (left_type.kind != .BOOL or right_type.kind != .BOOL) {
            try self.addError(&node.node, "Logical operators require boolean operands");
        }
        return try self.getBuiltinType("bool");
    }

    try self.addError(&node.node, "Unsupported infix operator");
    return try self.getErrorType();
}

/// Helper to resolve a type expression node to a Type
pub fn resolveTypeExpr(self: *TypeChecker, node: *ast.TypeExprNode) !*Type {
    if (node.name == null) {
        try self.addError(&node.node, "Type expression missing name");
        return try self.getErrorType();
    }

    const type_name = node.name.?.value;
    const type_symbol = self.symbol_table.lookup(type_name);

    if (type_symbol == null or type_symbol.?.symbol_type != .TYPE) {
        try self.addError(&node.node, "Undefined type");
        return try self.getErrorType();
    }

    return type_symbol.?.data_type.?;
}

/// Helper to check if two types are compatible
fn isTypeCompatible(expected: ?*Type, actual: ?*Type) bool {
    if (expected == null or actual == null) {
        return false;
    }

    // Error type is compatible with anything to avoid cascading errors
    if (actual.?.kind == .ERROR_TYPE) {
        return true;
    }

    return expected.?.equals(actual.?);
}
