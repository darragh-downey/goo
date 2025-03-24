const std = @import("std");
const parser = @import("../../frontend/parser/zig/parser.zig");
const ast = parser.ast;
const NodeType = ast.NodeType;

/// Error type for type checking errors
pub const TypeCheckError = error{
    TypeMismatch,
    UndefinedVariable,
    UndefinedFunction,
    UndefinedType,
    InvalidAssignment,
    InvalidOperation,
    InvalidFunctionCall,
    OutOfMemory,
};

/// Symbol type enum
pub const SymbolType = enum {
    VARIABLE,
    CONSTANT,
    FUNCTION,
    TYPE,
    PACKAGE,
    IMPORT,
};

/// Symbol information
pub const Symbol = struct {
    name: []const u8,
    symbol_type: SymbolType,
    data_type: ?*Type,
    is_initialized: bool,
    scope_level: usize,
    node: ?*ast.Node,

    pub fn init(
        allocator: std.mem.Allocator,
        name: []const u8,
        symbol_type: SymbolType,
        data_type: ?*Type,
        scope_level: usize,
        node: ?*ast.Node,
    ) !*Symbol {
        const symbol = try allocator.create(Symbol);
        symbol.* = .{
            .name = try allocator.dupe(u8, name),
            .symbol_type = symbol_type,
            .data_type = data_type,
            .is_initialized = false,
            .scope_level = scope_level,
            .node = node,
        };
        return symbol;
    }

    pub fn deinit(self: *Symbol, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
    }
};

/// Type kind enum
pub const TypeKind = enum {
    VOID,
    BOOL,
    INT,
    FLOAT,
    STRING,
    ARRAY,
    STRUCT,
    FUNCTION,
    CUSTOM,
    ERROR_TYPE, // For error handling
};

/// Type information
pub const Type = struct {
    kind: TypeKind,
    name: []const u8,

    // Function type information
    return_type: ?*Type,
    parameter_types: ?std.ArrayList(*Type),

    // Array type information
    element_type: ?*Type,

    // Struct type information
    fields: ?std.StringHashMap(*Symbol),

    pub fn init(allocator: std.mem.Allocator, kind: TypeKind, name: []const u8) !*Type {
        const typ = try allocator.create(Type);
        typ.* = .{
            .kind = kind,
            .name = try allocator.dupe(u8, name),
            .return_type = null,
            .parameter_types = null,
            .element_type = null,
            .fields = null,
        };
        return typ;
    }

    pub fn deinit(self: *Type, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        if (self.parameter_types) |params| {
            params.deinit();
        }
        if (self.fields) |fields| {
            var iter = fields.valueIterator();
            while (iter.next()) |symbol| {
                symbol.*.deinit(allocator);
                allocator.destroy(symbol.*);
            }
            fields.deinit();
        }
    }

    pub fn equals(self: *const Type, other: *const Type) bool {
        if (self.kind != other.kind) {
            return false;
        }

        switch (self.kind) {
            .VOID, .BOOL, .INT, .FLOAT, .STRING => return true,
            .CUSTOM => return std.mem.eql(u8, self.name, other.name),
            .ARRAY => {
                if (self.element_type == null or other.element_type == null) {
                    return false;
                }
                return self.element_type.?.equals(other.element_type.?);
            },
            .FUNCTION => {
                if (self.return_type == null or other.return_type == null) {
                    return false;
                }
                if (!self.return_type.?.equals(other.return_type.?)) {
                    return false;
                }
                if (self.parameter_types == null or other.parameter_types == null) {
                    return self.parameter_types == null and other.parameter_types == null;
                }
                if (self.parameter_types.?.items.len != other.parameter_types.?.items.len) {
                    return false;
                }
                for (self.parameter_types.?.items, other.parameter_types.?.items) |param1, param2| {
                    if (!param1.equals(param2)) {
                        return false;
                    }
                }
                return true;
            },
            .STRUCT => {
                if (self.fields == null or other.fields == null) {
                    return false;
                }
                if (self.fields.?.count() != other.fields.?.count()) {
                    return false;
                }
                var iter = self.fields.?.iterator();
                while (iter.next()) |entry| {
                    const name = entry.key_ptr.*;
                    const field = entry.value_ptr.*;

                    const other_field = other.fields.?.get(name) orelse return false;
                    if (field.data_type == null or other_field.data_type == null) {
                        return false;
                    }
                    if (!field.data_type.?.equals(other_field.data_type.?)) {
                        return false;
                    }
                }
                return true;
            },
            .ERROR_TYPE => return true,
        }
    }
};

/// Symbol table for tracking variables, functions, and types
pub const SymbolTable = struct {
    symbols: std.StringArrayHashMap(*Symbol),
    scope_level: usize,
    allocator: std.mem.Allocator,

    pub fn init(allocator: std.mem.Allocator) SymbolTable {
        return .{
            .symbols = std.StringArrayHashMap(*Symbol).init(allocator),
            .scope_level = 0,
            .allocator = allocator,
        };
    }

    pub fn deinit(self: *SymbolTable) void {
        var iter = self.symbols.valueIterator();
        while (iter.next()) |symbol| {
            symbol.*.deinit(self.allocator);
            self.allocator.destroy(symbol.*);
        }
        self.symbols.deinit();
    }

    pub fn enterScope(self: *SymbolTable) void {
        self.scope_level += 1;
    }

    pub fn leaveScope(self: *SymbolTable) void {
        // Remove all symbols from the current scope
        var i: usize = 0;
        while (i < self.symbols.count()) {
            const symbol = self.symbols.values()[i];
            if (symbol.scope_level == self.scope_level) {
                _ = self.symbols.orderedRemove(i);
                symbol.deinit(self.allocator);
                self.allocator.destroy(symbol);
            } else {
                i += 1;
            }
        }

        self.scope_level -= 1;
    }

    pub fn define(
        self: *SymbolTable,
        name: []const u8,
        symbol_type: SymbolType,
        data_type: ?*Type,
        node: ?*ast.Node,
    ) !*Symbol {
        // Check if the symbol already exists in the current scope
        if (self.symbols.get(name)) |existing| {
            if (existing.scope_level == self.scope_level) {
                return TypeCheckError.InvalidAssignment;
            }
        }

        const symbol = try Symbol.init(
            self.allocator,
            name,
            symbol_type,
            data_type,
            self.scope_level,
            node,
        );

        try self.symbols.put(name, symbol);
        return symbol;
    }

    pub fn lookup(self: *SymbolTable, name: []const u8) ?*Symbol {
        return self.symbols.get(name);
    }
};

/// Type checker context
pub const TypeChecker = struct {
    allocator: std.mem.Allocator,
    symbol_table: SymbolTable,
    errors: std.ArrayList([]const u8),
    builtin_types: std.StringHashMap(*Type),
    current_function: ?*ast.FunctionNode,

    pub fn init(allocator: std.mem.Allocator) !*TypeChecker {
        var checker = try allocator.create(TypeChecker);
        checker.* = .{
            .allocator = allocator,
            .symbol_table = SymbolTable.init(allocator),
            .errors = std.ArrayList([]const u8).init(allocator),
            .builtin_types = std.StringHashMap(*Type).init(allocator),
            .current_function = null,
        };

        try checker.initBuiltinTypes();
        return checker;
    }

    pub fn deinit(self: *TypeChecker) void {
        var builtin_iter = self.builtin_types.valueIterator();
        while (builtin_iter.next()) |typ| {
            typ.*.deinit(self.allocator);
            self.allocator.destroy(typ.*);
        }
        self.builtin_types.deinit();

        for (self.errors.items) |err| {
            self.allocator.free(err);
        }
        self.errors.deinit();

        self.symbol_table.deinit();
    }

    fn initBuiltinTypes(self: *TypeChecker) !void {
        try self.defineBuiltinType("void", .VOID);
        try self.defineBuiltinType("bool", .BOOL);
        try self.defineBuiltinType("int", .INT);
        try self.defineBuiltinType("float", .FLOAT);
        try self.defineBuiltinType("string", .STRING);
    }

    fn defineBuiltinType(self: *TypeChecker, name: []const u8, kind: TypeKind) !void {
        const typ = try Type.init(self.allocator, kind, name);
        try self.builtin_types.put(name, typ);

        // Also add to symbol table as a type
        _ = try self.symbol_table.define(name, .TYPE, typ, null);
    }

    pub fn addError(self: *TypeChecker, node: *ast.Node, msg: []const u8) !void {
        const error_msg = try std.fmt.allocPrint(
            self.allocator,
            "Type error at line {d}:{d}: {s}",
            .{ node.start_pos.line, node.start_pos.column, msg },
        );
        try self.errors.append(error_msg);
    }

    pub fn checkProgram(self: *TypeChecker, program: *ast.Program) !bool {
        // Check package declaration
        if (program.package) |pkg| {
            try self.checkPackageDeclaration(pkg);
        }

        // Enter global scope
        self.symbol_table.enterScope();

        // Check imports
        for (program.imports.items) |import_node| {
            try self.checkImportDeclaration(import_node);
        }

        // Check all declarations
        for (program.declarations.items) |decl| {
            try self.checkNode(decl);
        }

        // Leave global scope
        self.symbol_table.leaveScope();

        return self.errors.items.len == 0;
    }

    pub fn checkNode(self: *TypeChecker, node: *ast.Node) !?*Type {
        switch (node.node_type) {
            .PROGRAM => {
                _ = try self.checkProgram(@fieldParentPtr(ast.Program, "node", node));
                return self.getBuiltinType("void");
            },
            .PACKAGE_DECL => {
                try self.checkPackageDeclaration(@fieldParentPtr(ast.PackageNode, "base", node));
                return self.getBuiltinType("void");
            },
            .IMPORT_DECL => {
                try self.checkImportDeclaration(@fieldParentPtr(ast.ImportNode, "base", node));
                return self.getBuiltinType("void");
            },
            .FUNCTION_DECL => {
                return try self.checkFunctionDeclaration(@fieldParentPtr(ast.FunctionNode, "node", node));
            },
            .VAR_DECL => {
                return try self.checkVarDeclaration(@fieldParentPtr(ast.VarDeclNode, "node", node));
            },
            .CONST_DECL => {
                return try self.checkConstDeclaration(@fieldParentPtr(ast.ConstDeclNode, "node", node));
            },
            .BLOCK => {
                return try self.checkBlockStatement(@fieldParentPtr(ast.BlockNode, "node", node));
            },
            .IF_STMT => {
                return try self.checkIfStatement(@fieldParentPtr(ast.IfStmtNode, "node", node));
            },
            .FOR_STMT => {
                return try self.checkForStatement(@fieldParentPtr(ast.ForStmtNode, "node", node));
            },
            .RETURN_STMT => {
                return try self.checkReturnStatement(@fieldParentPtr(ast.ReturnStmtNode, "node", node));
            },
            .EXPR_STMT => {
                return try self.checkExpressionStatement(@fieldParentPtr(ast.ExprStmtNode, "node", node));
            },
            .CALL_EXPR => {
                return try self.checkCallExpression(@fieldParentPtr(ast.CallExprNode, "node", node));
            },
            .IDENTIFIER => {
                return try self.checkIdentifier(@fieldParentPtr(ast.IdentifierNode, "node", node));
            },
            .INT_LITERAL => return self.getBuiltinType("int"),
            .FLOAT_LITERAL => return self.getBuiltinType("float"),
            .STRING_LITERAL => return self.getBuiltinType("string"),
            .BOOL_LITERAL => return self.getBuiltinType("bool"),
            .PREFIX_EXPR => {
                return try self.checkPrefixExpression(@fieldParentPtr(ast.PrefixExprNode, "node", node));
            },
            .INFIX_EXPR => {
                return try self.checkInfixExpression(@fieldParentPtr(ast.InfixExprNode, "node", node));
            },
            else => {
                try self.addError(node, @tagName(node.node_type));
                return self.getErrorType();
            },
        }
    }

    // Get a builtin type by name
    fn getBuiltinType(self: *TypeChecker, name: []const u8) !*Type {
        if (self.builtin_types.get(name)) |typ| {
            return typ;
        }
        return TypeCheckError.UndefinedType;
    }

    // Get error type for error reporting
    fn getErrorType(self: *TypeChecker) !*Type {
        return try Type.init(self.allocator, .ERROR_TYPE, "error");
    }

    // Implementation of type checking for different node types will be added
    // in subsequent files.
};
