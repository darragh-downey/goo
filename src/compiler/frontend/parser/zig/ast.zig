const std = @import("std");
const lexer = @import("lexer");

/// Program node representing the entire program
pub const Program = struct {
    declarations: std.ArrayList(*Node),

    pub fn init(allocator: std.mem.Allocator) Program {
        return Program{
            .declarations = std.ArrayList(*Node).init(allocator),
        };
    }

    pub fn deinit(self: Program, allocator: std.mem.Allocator) void {
        for (self.declarations.items) |decl| {
            decl.deinit(allocator);
            allocator.destroy(decl);
        }
        self.declarations.deinit();
    }
};

/// Source location in the file
pub const SourceLocation = struct {
    line: usize,
    column: usize,
};

/// Base node type for all AST nodes
pub const Node = struct {
    node_type: NodeType,
    start_loc: SourceLocation,
    end_loc: SourceLocation,

    pub fn deinit(self: Node, allocator: std.mem.Allocator) void {
        _ = self;
        _ = allocator;
    }
};

/// Declaration in the program
pub const Declaration = struct {
    node: Node,
    decl_type: DeclarationType,

    pub fn deinit(self: Declaration, allocator: std.mem.Allocator) void {
        switch (self.decl_type) {
            .package => |pkg| pkg.deinit(allocator),
            .function => |func| func.deinit(allocator),
            .const_decl => |const_decl| const_decl.deinit(allocator),
            .var_decl => |var_decl| var_decl.deinit(allocator),
            .type_decl => |type_decl| type_decl.deinit(allocator),
            .struct_decl => |struct_decl| struct_decl.deinit(allocator),
            .import => |import_decl| import_decl.deinit(allocator),
            .comptime => |comptime_decl| comptime_decl.deinit(allocator),
            .parallel => |parallel_decl| parallel_decl.deinit(allocator),
        }
    }
};

/// Function declaration
pub const FunctionDecl = struct {
    name: []const u8,
    params: std.ArrayList(Parameter),
    return_type: ?[]const u8,
    body: Block,

    pub fn deinit(self: FunctionDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        for (self.params.items) |param| {
            param.deinit(allocator);
        }
        self.params.deinit();
        if (self.return_type) |rt| {
            allocator.free(rt);
        }
        self.body.deinit(allocator);
    }
};

/// Function parameter
pub const Parameter = struct {
    name: []const u8,
    type_name: []const u8,

    pub fn deinit(self: Parameter, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        allocator.free(self.type_name);
    }
};

/// Variable declaration (for var keyword)
pub const VarDecl = struct {
    name: []const u8,
    type_name: ?[]const u8,
    initializer: ?Expression,

    pub fn deinit(self: VarDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        if (self.type_name) |tn| {
            allocator.free(tn);
        }
        if (self.initializer) |init| {
            init.deinit(allocator);
        }
    }
};

/// Constant declaration
pub const ConstDecl = struct {
    name: []const u8,
    type_name: ?[]const u8,
    initializer: ?Expression,

    pub fn deinit(self: ConstDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        if (self.type_name) |tn| {
            allocator.free(tn);
        }
        if (self.initializer) |init| {
            init.deinit(allocator);
        }
    }
};

/// Struct field
pub const StructField = struct {
    name: []const u8,
    type_name: []const u8,

    pub fn deinit(self: StructField, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        allocator.free(self.type_name);
    }
};

/// Import declaration
pub const ImportDecl = struct {
    path: []const u8,

    pub fn deinit(self: ImportDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.path);
    }
};

/// Package declaration
pub const PackageDecl = struct {
    name: []const u8,

    pub fn deinit(self: PackageDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
    }
};

/// Statement
pub const Statement = struct {
    node: Node,
    stmt_type: StatementType,

    pub fn deinit(self: Statement, allocator: std.mem.Allocator) void {
        switch (self.stmt_type) {
            .expression => |expr| expr.deinit(allocator),
            .return_stmt => |ret| if (ret.value) |val| val.deinit(allocator),
            .if_stmt => |if_stmt| if_stmt.deinit(allocator),
            .while_stmt => |while_stmt| while_stmt.deinit(allocator),
            .block => |block| block.deinit(allocator),
            .assignment => |assign| assign.deinit(allocator),
        }
    }
};

/// Block of statements
pub const Block = struct {
    statements: std.ArrayList(Statement),

    pub fn deinit(self: Block, allocator: std.mem.Allocator) void {
        for (self.statements.items) |stmt| {
            stmt.deinit(allocator);
        }
        self.statements.deinit();
    }
};

/// Return statement
pub const ReturnStmt = struct {
    value: ?Expression,
};

/// If statement
pub const IfStmt = struct {
    condition: Expression,
    then_block: Block,
    else_block: ?Block,

    pub fn deinit(self: IfStmt, allocator: std.mem.Allocator) void {
        self.condition.deinit(allocator);
        self.then_block.deinit(allocator);
        if (self.else_block) |else_block| {
            else_block.deinit(allocator);
        }
    }
};

/// While statement
pub const WhileStmt = struct {
    condition: Expression,
    body: Block,

    pub fn deinit(self: WhileStmt, allocator: std.mem.Allocator) void {
        self.condition.deinit(allocator);
        self.body.deinit(allocator);
    }
};

/// Assignment statement
pub const AssignmentStmt = struct {
    target: Expression,
    value: Expression,

    pub fn deinit(self: AssignmentStmt, allocator: std.mem.Allocator) void {
        self.target.deinit(allocator);
        self.value.deinit(allocator);
    }
};

/// Expression
pub const Expression = struct {
    node: Node,
    expr_type: ExpressionType,

    pub fn deinit(self: Expression, allocator: std.mem.Allocator) void {
        switch (self.expr_type) {
            .identifier => |id| allocator.free(id),
            .int_literal => {},
            .float_literal => {},
            .string_literal => |str| allocator.free(str),
            .bool_literal => {},
            .prefix => |prefix| prefix.deinit(allocator),
            .infix => |infix| infix.deinit(allocator),
            .call => |call| call.deinit(allocator),
            .index => |index| index.deinit(allocator),
            .member => |member| member.deinit(allocator),
        }
    }
};

/// Prefix expression
pub const PrefixExpr = struct {
    operator: []const u8,
    right: *Expression,

    pub fn deinit(self: PrefixExpr, allocator: std.mem.Allocator) void {
        allocator.free(self.operator);
        self.right.deinit(allocator);
        allocator.destroy(self.right);
    }
};

/// Infix expression
pub const InfixExpr = struct {
    left: *Expression,
    operator: []const u8,
    right: *Expression,

    pub fn deinit(self: InfixExpr, allocator: std.mem.Allocator) void {
        self.left.deinit(allocator);
        allocator.destroy(self.left);
        allocator.free(self.operator);
        self.right.deinit(allocator);
        allocator.destroy(self.right);
    }
};

/// Function call expression
pub const CallExpr = struct {
    function: *Expression,
    arguments: std.ArrayList(*Expression),

    pub fn deinit(self: CallExpr, allocator: std.mem.Allocator) void {
        self.function.deinit(allocator);
        allocator.destroy(self.function);

        for (self.arguments.items) |arg| {
            arg.deinit(allocator);
            allocator.destroy(arg);
        }
        self.arguments.deinit();
    }
};

/// Array or map index expression
pub const IndexExpr = struct {
    array: *Expression,
    index: *Expression,

    pub fn deinit(self: IndexExpr, allocator: std.mem.Allocator) void {
        self.array.deinit(allocator);
        allocator.destroy(self.array);
        self.index.deinit(allocator);
        allocator.destroy(self.index);
    }
};

/// Member access expression
pub const MemberExpr = struct {
    object: *Expression,
    member: []const u8,

    pub fn deinit(self: MemberExpr, allocator: std.mem.Allocator) void {
        self.object.deinit(allocator);
        allocator.destroy(self.object);
        allocator.free(self.member);
    }
};

/// Types of nodes
pub const NodeType = enum {
    program,
    declaration,
    statement,
    expression,
};

/// Types of declarations
pub const DeclarationType = union(enum) {
    package: PackageDecl,
    function: FunctionDecl,
    const_decl: ConstDecl,
    var_decl: VarDecl,
    type_decl: TypeDecl,
    struct_decl: StructDecl,
    import: ImportDecl,
    comptime: ComptimeDecl,  // For Goo's comptime feature
    parallel: ParallelDecl,  // For Goo's parallel construct
};

/// Types of statements
pub const StatementType = union(enum) {
    expression: Expression,
    return_stmt: ReturnStmt,
    if_stmt: IfStmt,
    while_stmt: WhileStmt,
    block: Block,
    assignment: AssignmentStmt,
};

/// Types of expressions
pub const ExpressionType = union(enum) {
    identifier: []const u8,
    int_literal: i64,
    float_literal: f64,
    string_literal: []const u8,
    bool_literal: bool,
    prefix: PrefixExpr,
    infix: InfixExpr,
    call: CallExpr,
    index: IndexExpr,
    member: MemberExpr,
};

/// Struct declaration
pub const StructDecl = struct {
    name: []const u8,
    fields: std.ArrayList(StructField),

    pub fn deinit(self: StructDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        for (self.fields.items) |field| {
            field.deinit(allocator);
        }
        self.fields.deinit();
    }
};

/// Type declaration
pub const TypeDecl = struct {
    name: []const u8,
    is_struct: bool,
    type_value: []const u8, // For type aliases
    fields: std.ArrayList(StructField), // For structs

    pub fn deinit(self: TypeDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        allocator.free(self.type_value);
        for (self.fields.items) |field| {
            field.deinit(allocator);
        }
        self.fields.deinit();
    }
};

/// Comptime declaration (for Goo's comptime feature)
pub const ComptimeDecl = struct {
    name: []const u8,
    body: Block,

    pub fn deinit(self: ComptimeDecl, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        self.body.deinit(allocator);
    }
};

/// Parallel construct declaration (for Goo's parallel feature)
pub const ParallelDecl = struct {
    is_distributed: bool, // true for distributed parallelism
    body: Block,

    pub fn deinit(self: ParallelDecl, allocator: std.mem.Allocator) void {
        self.body.deinit(allocator);
    }
};
