const std = @import("std");
const Allocator = std.mem.Allocator;

/// Basic block types that can appear in the IR
pub const BlockType = enum {
    Entry,
    Normal,
    Loop,
    Branch,
    Exit,
};

/// Instruction opcode
pub const Opcode = enum {
    // Core instructions
    Nop,
    Const,
    Move,
    Load,
    Store,
    
    // Arithmetic
    Add,
    Sub,
    Mul,
    Div,
    Mod,
    Neg,
    
    // Logical
    And,
    Or,
    Xor,
    Not,
    Shl,
    Shr,
    
    // Comparison
    Eq,
    Ne,
    Lt,
    Le,
    Gt,
    Ge,
    
    // Control flow
    Jump,
    Branch,
    Return,
    Call,
    TailCall,
    
    // Memory management
    Alloc,
    Free,
    
    // Goo-specific
    GoroutineSpawn,
    GoroutineYield,
    GoroutineJoin,
    ChannelSend,
    ChannelReceive,
    ChannelClose,
};

/// Value representation
pub const Value = struct {
    id: u32,
    name: ?[]const u8 = null,
    
    pub fn format(
        self: Value,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        if (self.name) |name| {
            try writer.print("%{d}:{s}", .{ self.id, name });
        } else {
            try writer.print("%{d}", .{ self.id });
        }
    }
};

/// Type representation
pub const Type = struct {
    kind: TypeKind,
    size: usize,
    align_: u8,
    
    pub const TypeKind = enum {
        Void,
        Bool,
        Int8,
        Int16,
        Int32,
        Int64,
        UInt8,
        UInt16,
        UInt32,
        UInt64,
        Float32,
        Float64,
        Pointer,
        Array,
        Struct,
        Function,
        Channel,
    };
    
    // TODO: Add field accessors and type helpers
};

/// Instruction representation
pub const Instruction = struct {
    id: u32,
    opcode: Opcode,
    operands: std.ArrayList(Value),
    result: ?Value = null,
    type_: ?*Type = null,
    location: ?SourceLocation = null,
    metadata: std.StringHashMap([]const u8),
    
    const Self = @This();
    
    pub fn init(allocator: Allocator, opcode: Opcode) Self {
        return Self{
            .id = 0, // Will be assigned when added to basic block
            .opcode = opcode,
            .operands = std.ArrayList(Value).init(allocator),
            .metadata = std.StringHashMap([]const u8).init(allocator),
        };
    }
    
    pub fn deinit(self: *Self) void {
        self.operands.deinit();
        self.metadata.deinit();
    }
    
    pub fn addOperand(self: *Self, value: Value) !void {
        try self.operands.append(value);
    }
    
    pub fn setResult(self: *Self, value: Value) void {
        self.result = value;
    }
    
    pub fn setMetadata(self: *Self, key: []const u8, value: []const u8) !void {
        try self.metadata.put(key, value);
    }
    
    pub fn format(
        self: Self,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        try writer.print("{d}: ", .{self.id});
        
        if (self.result) |result| {
            try writer.print("{} = ", .{result});
        }
        
        try writer.print("{s}", .{@tagName(self.opcode)});
        
        if (self.operands.items.len > 0) {
            try writer.writeAll(" ");
            for (self.operands.items, 0..) |operand, i| {
                if (i > 0) try writer.writeAll(", ");
                try writer.print("{}", .{operand});
            }
        }
        
        if (self.type_) |ty| {
            try writer.print(" : {s}", .{@tagName(ty.kind)});
        }
        
        if (self.location) |loc| {
            try writer.print(" @ {s}:{d}:{d}", .{ loc.file, loc.line, loc.column });
        }
    }
};

/// Source location information
pub const SourceLocation = struct {
    file: []const u8,
    line: u32,
    column: u32,
};

/// Basic block representation
pub const BasicBlock = struct {
    id: u32,
    name: ?[]const u8 = null,
    block_type: BlockType,
    instructions: std.ArrayList(Instruction),
    predecessors: std.ArrayList(*BasicBlock),
    successors: std.ArrayList(*BasicBlock),
    
    const Self = @This();
    
    pub fn init(allocator: Allocator, block_type: BlockType) Self {
        return Self{
            .id = 0, // Will be assigned when added to function
            .block_type = block_type,
            .instructions = std.ArrayList(Instruction).init(allocator),
            .predecessors = std.ArrayList(*BasicBlock).init(allocator),
            .successors = std.ArrayList(*BasicBlock).init(allocator),
        };
    }
    
    pub fn deinit(self: *Self) void {
        for (self.instructions.items) |*instr| {
            instr.deinit();
        }
        self.instructions.deinit();
        self.predecessors.deinit();
        self.successors.deinit();
    }
    
    pub fn addInstruction(self: *Self, instruction: Instruction) !void {
        var instr = instruction;
        instr.id = @intCast(self.instructions.items.len);
        try self.instructions.append(instr);
    }
    
    pub fn addPredecessor(self: *Self, pred: *BasicBlock) !void {
        try self.predecessors.append(pred);
    }
    
    pub fn addSuccessor(self: *Self, succ: *BasicBlock) !void {
        try self.successors.append(succ);
    }
    
    pub fn format(
        self: Self,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        if (self.name) |name| {
            try writer.print("Block {d}:{s} ({s}):\n", .{ self.id, name, @tagName(self.block_type) });
        } else {
            try writer.print("Block {d} ({s}):\n", .{ self.id, @tagName(self.block_type) });
        }
        
        // Print predecessors
        try writer.writeAll("  Predecessors: ");
        for (self.predecessors.items, 0..) |pred, i| {
            if (i > 0) try writer.writeAll(", ");
            try writer.print("{d}", .{pred.id});
        }
        try writer.writeAll("\n");
        
        // Print instructions
        for (self.instructions.items) |instr| {
            try writer.print("  {}\n", .{instr});
        }
        
        // Print successors
        try writer.writeAll("  Successors: ");
        for (self.successors.items, 0..) |succ, i| {
            if (i > 0) try writer.writeAll(", ");
            try writer.print("{d}", .{succ.id});
        }
        try writer.writeAll("\n");
    }
};

/// Function representation
pub const Function = struct {
    name: []const u8,
    params: std.ArrayList(Value),
    return_type: ?*Type,
    basic_blocks: std.ArrayList(*BasicBlock),
    entry_block: ?*BasicBlock = null,
    exit_block: ?*BasicBlock = null,
    locals: std.ArrayList(Value),
    allocator: Allocator,
    
    const Self = @This();
    
    pub fn init(allocator: Allocator, name: []const u8) Self {
        return Self{
            .name = name,
            .params = std.ArrayList(Value).init(allocator),
            .return_type = null,
            .basic_blocks = std.ArrayList(*BasicBlock).init(allocator),
            .locals = std.ArrayList(Value).init(allocator),
            .allocator = allocator,
        };
    }
    
    pub fn deinit(self: *Self) void {
        for (self.basic_blocks.items) |block| {
            block.deinit();
            self.allocator.destroy(block);
        }
        self.basic_blocks.deinit();
        self.params.deinit();
        self.locals.deinit();
    }
    
    pub fn addBasicBlock(self: *Self, block_type: BlockType) !*BasicBlock {
        var block = try self.allocator.create(BasicBlock);
        block.* = BasicBlock.init(self.allocator, block_type);
        block.id = @intCast(self.basic_blocks.items.len);
        
        try self.basic_blocks.append(block);
        
        if (block_type == .Entry and self.entry_block == null) {
            self.entry_block = block;
        } else if (block_type == .Exit and self.exit_block == null) {
            self.exit_block = block;
        }
        
        return block;
    }
    
    pub fn createParam(self: *Self, name: ?[]const u8) !Value {
        const id: u32 = @intCast(self.params.items.len);
        const param = Value{ .id = id, .name = name };
        try self.params.append(param);
        return param;
    }
    
    pub fn createLocal(self: *Self, name: ?[]const u8) !Value {
        const id: u32 = @intCast(self.locals.items.len);
        const local = Value{ .id = id, .name = name };
        try self.locals.append(local);
        return local;
    }
    
    pub fn format(
        self: Self,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        try writer.print("function {s}(", .{self.name});
        
        for (self.params.items, 0..) |param, i| {
            if (i > 0) try writer.writeAll(", ");
            try writer.print("{}", .{param});
        }
        
        try writer.writeAll(") {\n");
        
        for (self.basic_blocks.items) |block| {
            try writer.print("{}\n", .{block});
        }
        
        try writer.writeAll("}\n");
    }
};

/// Module representation (holds multiple functions)
pub const Module = struct {
    name: []const u8,
    functions: std.ArrayList(*Function),
    allocator: Allocator,
    
    const Self = @This();
    
    pub fn init(allocator: Allocator, name: []const u8) Self {
        return Self{
            .name = name,
            .functions = std.ArrayList(*Function).init(allocator),
            .allocator = allocator,
        };
    }
    
    pub fn deinit(self: *Self) void {
        for (self.functions.items) |func| {
            func.deinit();
            self.allocator.destroy(func);
        }
        self.functions.deinit();
    }
    
    pub fn addFunction(self: *Self, name: []const u8) !*Function {
        var func = try self.allocator.create(Function);
        func.* = Function.init(self.allocator, name);
        
        try self.functions.append(func);
        
        return func;
    }
    
    pub fn format(
        self: Self,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;
        
        try writer.print("module {s} {{\n", .{self.name});
        
        for (self.functions.items) |func| {
            try writer.print("{}\n", .{func});
        }
        
        try writer.writeAll("}\n");
    }
};

// Test the IR module
test "IR Module" {
    const testing = std.testing;
    
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    
    const allocator = arena.allocator();
    
    var module = Module.init(allocator, "test_module");
    defer module.deinit();
    
    const func = try module.addFunction("test_func");
    
    const entry = try func.addBasicBlock(.Entry);
    const exit = try func.addBasicBlock(.Exit);
    
    try entry.addSuccessor(exit);
    try exit.addPredecessor(entry);
    
    const x = try func.createLocal("x");
    const y = try func.createLocal("y");
    const z = try func.createLocal("z");
    
    var instr = Instruction.init(allocator, .Const);
    try instr.addOperand(Value{ .id = 42, .name = null });
    instr.setResult(x);
    try entry.addInstruction(instr);
    
    instr = Instruction.init(allocator, .Const);
    try instr.addOperand(Value{ .id = 58, .name = null });
    instr.setResult(y);
    try entry.addInstruction(instr);
    
    instr = Instruction.init(allocator, .Add);
    try instr.addOperand(x);
    try instr.addOperand(y);
    instr.setResult(z);
    try entry.addInstruction(instr);
    
    instr = Instruction.init(allocator, .Return);
    try instr.addOperand(z);
    try exit.addInstruction(instr);
    
    // Verify the structure
    try testing.expectEqual(@as(usize, 1), module.functions.items.len);
    try testing.expectEqual(@as(usize, 2), func.basic_blocks.items.len);
    try testing.expectEqual(@as(usize, 3), entry.instructions.items.len);
    try testing.expectEqual(@as(usize, 1), exit.instructions.items.len);
}

/// IR builder for easier construction
pub const IRBuilder = struct {
    current_module: ?*Module = null,
    current_function: ?*Function = null,
    current_block: ?*BasicBlock = null,
    allocator: Allocator,
    
    const Self = @This();
    
    pub fn init(allocator: Allocator) Self {
        return Self{
            .allocator = allocator,
        };
    }
    
    pub fn createModule(self: *Self, name: []const u8) !*Module {
        var module = try self.allocator.create(Module);
        module.* = Module.init(self.allocator, name);
        self.current_module = module;
        return module;
    }
    
    pub fn createFunction(self: *Self, name: []const u8) !*Function {
        if (self.current_module) |module| {
            const func = try module.addFunction(name);
            self.current_function = func;
            return func;
        }
        
        return error.NoCurrentModule;
    }
    
    pub fn createBasicBlock(self: *Self, block_type: BlockType) !*BasicBlock {
        if (self.current_function) |func| {
            const block = try func.addBasicBlock(block_type);
            self.current_block = block;
            return block;
        }
        
        return error.NoCurrentFunction;
    }
    
    pub fn createInstruction(self: *Self, opcode: Opcode) !*Instruction {
        if (self.current_block) |block| {
            var instr = try self.allocator.create(Instruction);
            instr.* = Instruction.init(self.allocator, opcode);
            return instr;
        }
        
        return error.NoCurrentBlock;
    }
    
    pub fn addInstruction(self: *Self, instruction: *Instruction) !void {
        if (self.current_block) |block| {
            var instr = instruction.*;
            try block.addInstruction(instr);
            instruction.deinit();
            self.allocator.destroy(instruction);
        } else {
            return error.NoCurrentBlock;
        }
    }
};

// Test the IR builder
test "IR builder" {
    const testing = std.testing;
    
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    
    const allocator = arena.allocator();
    
    var builder = IRBuilder.init(allocator);
    
    const module = try builder.createModule("test_module");
    defer module.deinit();
    
    _ = try builder.createFunction("test_func");
    
    _ = try builder.createBasicBlock(.Entry);
    
    var instr = try builder.createInstruction(.Const);
    try instr.addOperand(Value{ .id = 42, .name = null });
    try builder.addInstruction(instr);
    
    // Verify the structure
    try testing.expectEqual(@as(usize, 1), module.functions.items.len);
    try testing.expectEqual(@as(usize, 1), module.functions.items[0].basic_blocks.items.len);
    try testing.expectEqual(@as(usize, 1), module.functions.items[0].basic_blocks.items[0].instructions.items.len);
}

test "create simple IR" {
    const testing = std.testing;
    // ... existing code ...
} 