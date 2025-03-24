const std = @import("std");
const ir = @import("ir.zig");
const cf_bindings = @import("constant_folding_bindings.zig");
const dce_bindings = @import("dead_code_elimination_bindings.zig");

const Allocator = std.mem.Allocator;
const Module = ir.Module;
const Function = ir.Function;
const BasicBlock = ir.BasicBlock;
const Instruction = ir.Instruction;
const Value = ir.Value;
const Type = ir.Type;
const Opcode = ir.Opcode;
const BlockType = ir.BlockType;
const IRBuilder = ir.IRBuilder;

/// Global allocator for C interop
var global_allocator: ?std.mem.Allocator = null;

/// Initialize the global allocator
pub fn initGlobalAllocator(allocator: Allocator) void {
    global_allocator = allocator;
}

/// Get the global allocator or return a default one
fn getAllocator() Allocator {
    return global_allocator orelse std.heap.c_allocator;
}

/// Exposed C-compatible structs for the IR
/// C-compatible module handle
pub const GooIRModule = opaque {};

/// C-compatible function handle
pub const GooIRFunction = opaque {};

/// C-compatible basic block handle
pub const GooIRBasicBlock = opaque {};

/// C-compatible instruction handle
pub const GooIRInstruction = opaque {};

/// C-compatible value handle
pub const GooIRValue = u32;

/// C-compatible opcode enum
pub const GooIROpcode = c_int;

/// C-compatible block type enum
pub const GooIRBlockType = c_int;

/// C binding functions using exported symbols
/// Create a new module
pub export fn goo_ir_create_module(name: [*:0]const u8) ?*GooIRModule {
    const allocator = getAllocator();
    const module = allocator.create(Module) catch return null;
    module.* = Module.init(allocator, std.mem.span(name)) catch {
        allocator.destroy(module);
        return null;
    };
    return @ptrCast(module);
}

/// Free a module
export fn goo_ir_destroy_module(module_ptr: ?*GooIRModule) void {
    if (module_ptr) |mod| {
        var module: *Module = @ptrCast(@alignCast(mod));
        module.deinit();
        getAllocator().destroy(module);
    }
}

/// Dump a module to a string
pub export fn goo_ir_dump_module(module_ptr: ?*GooIRModule, buffer: [*]u8, buffer_size: usize) usize {
    if (module_ptr == null) return 0;
    const module: *Module = @ptrCast(@alignCast(module_ptr.?));

    var fixed_buffer = std.io.fixedBufferStream(buffer[0..buffer_size]);
    const writer = fixed_buffer.writer();

    module.format(writer) catch return 0;

    return @intCast(fixed_buffer.pos);
}

/// Add a function to a module
pub export fn goo_ir_add_function(module_ptr: ?*GooIRModule, name: [*:0]const u8) ?*GooIRFunction {
    if (module_ptr == null) return null;
    const module: *Module = @ptrCast(@alignCast(module_ptr.?));

    const function = module.addFunction(std.mem.span(name)) catch return null;
    return @ptrCast(function);
}

/// Get a function from a module by name
export fn goo_ir_get_function(module_ptr: ?*GooIRModule, name: [*:0]const u8) ?*GooIRFunction {
    if (module_ptr == null) return null;

    var module: *Module = @ptrCast(@alignCast(module_ptr.?));
    const name_span = std.mem.span(name);

    // Find function by name
    for (module.functions.items) |func| {
        if (std.mem.eql(u8, func.name, name_span)) {
            return @ptrCast(func);
        }
    }

    return null;
}

/// Add a basic block to a function
export fn goo_ir_add_basic_block(function_ptr: ?*GooIRFunction, block_type: GooIRBlockType, name: ?[*:0]const u8) ?*GooIRBasicBlock {
    if (function_ptr == null) return null;

    var function: *Function = @ptrCast(@alignCast(function_ptr.?));

    // Map the block type from C to Zig
    const bt: BlockType = switch (block_type) {
        0 => .Entry,
        1 => .Normal,
        2 => .Loop,
        3 => .Branch,
        4 => .Exit,
        else => .Normal,
    };

    // Add block to function
    const block = function.addBasicBlock(bt) catch return null;

    // Set name if provided
    if (name) |n| {
        block.name = std.mem.span(n);
    }

    return @ptrCast(block);
}

/// Link two basic blocks (predecessor -> successor)
export fn goo_ir_link_blocks(pred_ptr: ?*GooIRBasicBlock, succ_ptr: ?*GooIRBasicBlock) bool {
    if (pred_ptr == null or succ_ptr == null) return false;

    var pred: *BasicBlock = @ptrCast(@alignCast(pred_ptr.?));
    var succ: *BasicBlock = @ptrCast(@alignCast(succ_ptr.?));

    // Add successor and predecessor
    pred.addSuccessor(succ) catch return false;
    succ.addPredecessor(pred) catch return false;

    return true;
}

/// Create a new local variable in a function
export fn goo_ir_create_local(function_ptr: ?*GooIRFunction, name: ?[*:0]const u8) GooIRValue {
    if (function_ptr == null) return 0;

    var function: *Function = @ptrCast(@alignCast(function_ptr.?));

    // Create local variable
    var name_slice: ?[]const u8 = null;
    if (name) |n| {
        name_slice = std.mem.span(n);
    }

    const local = function.createLocal(name_slice) catch return 0;

    return local.id;
}

/// Create a new parameter in a function
export fn goo_ir_create_param(function_ptr: ?*GooIRFunction, name: ?[*:0]const u8) GooIRValue {
    if (function_ptr == null) return 0;

    var function: *Function = @ptrCast(@alignCast(function_ptr.?));

    // Create parameter
    var name_slice: ?[]const u8 = null;
    if (name) |n| {
        name_slice = std.mem.span(n);
    }

    const param = function.createParam(name_slice) catch return 0;

    return param.id;
}

/// Create a new instruction
pub export fn goo_ir_create_instruction(block_ptr: ?*GooIRBasicBlock, opcode: u32) ?*GooIRInstruction {
    if (block_ptr == null) return null;
    const block: *BasicBlock = @ptrCast(@alignCast(block_ptr.?));

    const op: Opcode = @enumFromInt(@as(u8, @intCast(opcode)));

    const instruction = block.createInstruction(op) catch return null;
    return @ptrCast(instruction);
}

/// Add an operand to an instruction
export fn goo_ir_add_operand(instr_ptr: ?*GooIRInstruction, value: GooIRValue) bool {
    if (instr_ptr == null) return false;

    var instr: *Instruction = @ptrCast(@alignCast(instr_ptr.?));

    // Create value and add it as operand
    const val = Value{ .id = value, .name = null };
    instr.addOperand(val) catch return false;

    return true;
}

/// Set the result of an instruction
export fn goo_ir_set_result(instr_ptr: ?*GooIRInstruction, value: GooIRValue) bool {
    if (instr_ptr == null) return false;

    var instr: *Instruction = @ptrCast(@alignCast(instr_ptr.?));

    // Create value and set as result
    const val = Value{ .id = value, .name = null };
    instr.setResult(val);

    return true;
}

/// Add an instruction to a basic block
pub export fn goo_ir_add_instruction(block_ptr: ?*GooIRBasicBlock, instruction_ptr: ?*GooIRInstruction) bool {
    if (block_ptr == null or instruction_ptr == null) return false;
    const block: *BasicBlock = @ptrCast(@alignCast(block_ptr.?));
    const instr: *Instruction = @ptrCast(@alignCast(instruction_ptr.?));

    const instr_copy = instr.*;
    block.addInstruction(instr_copy) catch return false;

    return true;
}

/// Helper function to create a constant value instruction
export fn goo_ir_create_const(block_ptr: ?*GooIRBasicBlock, value: i64, result: GooIRValue) bool {
    if (block_ptr == null) return false;

    var block: *BasicBlock = @ptrCast(@alignCast(block_ptr.?));
    const allocator = getAllocator();

    // Create const instruction
    var instr = Instruction.init(allocator, .Const);

    // Add constant as operand
    const val = Value{ .id = @intCast(value), .name = null };
    instr.addOperand(val) catch return false;

    // Set result
    const res = Value{ .id = result, .name = null };
    instr.setResult(res);

    // Add to block
    block.addInstruction(instr) catch return false;

    return true;
}

/// Create a binary operation instruction
pub export fn goo_ir_create_binary_op(block_ptr: ?*GooIRBasicBlock, opcode: u32, lhs: GooIRValue, rhs: GooIRValue, result: GooIRValue) bool {
    if (block_ptr == null) return false;
    const block: *BasicBlock = @ptrCast(@alignCast(block_ptr.?));

    const op: Opcode = @enumFromInt(@as(u8, @intCast(opcode)));

    // Convert to internal values
    const lhs_val = Value{ .id = lhs };
    const rhs_val = Value{ .id = rhs };
    const result_val = Value{ .id = result };

    block.createBinaryOp(op, lhs_val, rhs_val, result_val) catch return false;

    return true;
}

/// Helper function to create a return instruction
export fn goo_ir_create_return(block_ptr: ?*GooIRBasicBlock, value: ?GooIRValue) bool {
    if (block_ptr == null) return false;

    var block: *BasicBlock = @ptrCast(@alignCast(block_ptr.?));
    const allocator = getAllocator();

    // Create return instruction
    var instr = Instruction.init(allocator, .Return);

    // Add operand if provided
    if (value) |v| {
        const val = Value{ .id = v, .name = null };
        instr.addOperand(val) catch return false;
    }

    // Add to block
    block.addInstruction(instr) catch return false;

    return true;
}

/// Opcode conversion functions for C compatibility
/// Convert Zig Opcode to C GooIROpcode
fn goo_ir_opcode_to_c(opcode: ir.Opcode) u32 {
    return @intFromEnum(@as(Opcode, opcode));
}

/// Convert C GooIROpcode to Zig Opcode
fn goo_ir_opcode_from_c(opcode: u32) ir.Opcode {
    return @enumFromInt(@as(u8, @intCast(opcode)));
}

/// BlockType conversion functions for C compatibility
/// Convert Zig BlockType to C GooIRBlockType
fn goo_ir_block_type_to_c(block_type: ir.BlockType) u32 {
    return @intFromEnum(@as(BlockType, block_type));
}

/// Convert C GooIRBlockType to Zig BlockType
fn goo_ir_block_type_from_c(block_type: u32) ir.BlockType {
    return @enumFromInt(@as(u8, @intCast(block_type)));
}

/// Initialize the IR system
export fn goo_ir_init() bool {
    // Use C allocator if no custom one is set
    if (global_allocator == null) {
        global_allocator = std.heap.c_allocator;
    }

    return true;
}

/// Shutdown the IR system
export fn goo_ir_shutdown() void {
    // Clear allocator reference
    global_allocator = null;
}

/// Simple test function to verify C bindings
export fn goo_ir_test_bindings() bool {
    if (!goo_ir_init()) return false;

    const module = goo_ir_create_module("test_module") orelse return false;
    defer goo_ir_destroy_module(module);

    const func = goo_ir_add_function(module, "test_function") orelse return false;

    const entry_block = goo_ir_add_basic_block(func, goo_ir_block_type_to_c(.Entry), "entry") orelse return false;
    const exit_block = goo_ir_add_basic_block(func, goo_ir_block_type_to_c(.Exit), "exit") orelse return false;

    // Link blocks
    if (!goo_ir_link_blocks(entry_block, exit_block)) return false;

    // Create a constant instruction
    const instr = goo_ir_create_instruction(entry_block, goo_ir_opcode_to_c(.Const)) orelse return false;
    if (!goo_ir_add_instruction(entry_block, instr)) return false;

    // Shut down
    goo_ir_shutdown();

    return true;
}

/// Generate C header (requires running with Zig build system)
pub fn main() !void {
    // This is a placeholder if we want to generate C headers
    // from this file using Zig's build system
    std.debug.print("Goo IR Bindings\n", .{});
}

/// Lookup the function with the given name in a module
pub export fn goo_ir_lookup_function(module_ptr: ?*GooIRModule, name: [*:0]const u8) ?*GooIRFunction {
    if (module_ptr == null) return null;
    const module: *Module = @ptrCast(@alignCast(module_ptr.?));

    // Find the function with the given name
    const func_name = std.mem.span(name);
    for (module.functions.items) |*func| {
        if (std.mem.eql(u8, func.name, func_name)) {
            return @ptrCast(func);
        }
    }

    return null;
}
