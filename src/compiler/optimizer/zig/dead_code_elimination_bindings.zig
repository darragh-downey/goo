const std = @import("std");
const dce = @import("dead_code_elimination.zig");
const pm = @import("pass_manager.zig");

/// Global allocator for C API
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const allocator = gpa.allocator();

/// Add dead code elimination pass to pass manager
pub export fn goo_pass_manager_add_dead_code_elimination(pass_manager: *pm.PassManager) bool {
    // Create the dead code elimination pass
    var dce_pass = dce.DeadCodeElimination.create(allocator) catch {
        return false;
    };

    // Get the pass interface
    const pass = dce_pass.getPass();

    // Add the pass to the pass manager
    pass_manager.addFunctionPass(pass) catch {
        dce_pass.destroy();
        return false;
    };

    return true;
}

/// Get statistics from the dead code elimination pass
pub export fn goo_dead_code_elimination_get_stats(pass_manager: *pm.PassManager, removed_instructions: *u32, removed_blocks: *u32, eliminated_values: *u32) bool {
    // Find the DCE pass
    for (pass_manager.function_passes.items) |*pass| {
        if (std.mem.eql(u8, pass.name, "DeadCodeElimination")) {
            // Extract statistics
            const instructions = pass_manager.getStatValue(pass, "removed_instructions") orelse {
                return false;
            };

            const blocks = pass_manager.getStatValue(pass, "removed_blocks") orelse {
                return false;
            };

            const values = pass_manager.getStatValue(pass, "eliminated_values") orelse {
                return false;
            };

            // Update out parameters
            removed_instructions.* = @as(u32, @intCast(instructions));
            removed_blocks.* = @as(u32, @intCast(blocks));
            eliminated_values.* = @as(u32, @intCast(values));

            return true;
        }
    }

    return false;
}

/// Test the dead code elimination pass with a simple IR
pub export fn goo_test_dead_code_elimination() bool {
    // Create a test IR
    var module = std.testing.allocator.create(@import("ir.zig").Module) catch {
        return false;
    };
    module.* = @import("ir.zig").Module.create(std.testing.allocator, "test_module") catch {
        std.testing.allocator.destroy(module);
        return false;
    };
    defer module.deinit();

    // Add a function
    var function = module.addFunction("test_function") catch {
        return false;
    };

    // Create values
    const x = function.createLocal("x") catch return false;
    const y = function.createLocal("y") catch return false;
    const z = function.createLocal("z") catch return false;
    const unused = function.createLocal("unused") catch return false;

    // Create basic blocks
    var entry_block = function.addBasicBlock(.ENTRY, "entry") catch return false;
    var unreachable_block = function.addBasicBlock(.NORMAL, "unreachable") catch return false;
    var exit_block = function.addBasicBlock(.EXIT, "exit") catch return false;

    // Only connect entry to exit
    entry_block.addSuccessor(exit_block) catch return false;
    exit_block.addPredecessor(entry_block) catch return false;

    // Create instructions in entry block
    var instr_x = @import("ir.zig").Instruction.create(null) catch return false;
    instr_x.opcode = .CONST;
    instr_x.result = x;
    instr_x.addOperand(10) catch return false;
    entry_block.addInstruction(instr_x) catch return false;

    var instr_y = @import("ir.zig").Instruction.create(null) catch return false;
    instr_y.opcode = .CONST;
    instr_y.result = y;
    instr_y.addOperand(20) catch return false;
    entry_block.addInstruction(instr_y) catch return false;

    var instr_z = @import("ir.zig").Instruction.create(null) catch return false;
    instr_z.opcode = .ADD;
    instr_z.result = z;
    instr_z.addOperand(x) catch return false;
    instr_z.addOperand(y) catch return false;
    entry_block.addInstruction(instr_z) catch return false;

    // Create unused instruction
    var instr_unused = @import("ir.zig").Instruction.create(null) catch return false;
    instr_unused.opcode = .MUL;
    instr_unused.result = unused;
    instr_unused.addOperand(x) catch return false;
    instr_unused.addOperand(y) catch return false;
    entry_block.addInstruction(instr_unused) catch return false;

    // Create instruction in unreachable block
    var unused_instr = @import("ir.zig").Instruction.create(null) catch return false;
    unused_instr.opcode = .CONST;
    unreachable_block.addInstruction(unused_instr) catch return false;

    // Return instruction
    var return_instr = @import("ir.zig").Instruction.create(null) catch return false;
    return_instr.opcode = .RETURN;
    return_instr.addOperand(z) catch return false;
    exit_block.addInstruction(return_instr) catch return false;

    // Create pass manager
    var pass_manager = @import("pass_manager.zig").PassManager.create(std.testing.allocator, .DEFAULT, true) catch return false;
    defer pass_manager.deinit();

    // Add DCE pass
    if (!goo_pass_manager_add_dead_code_elimination(&pass_manager)) {
        return false;
    }

    // Run passes
    const changed = pass_manager.run(module) catch return false;

    // Get statistics
    var removed_instructions: u32 = 0;
    var removed_blocks: u32 = 0;
    var eliminated_values: u32 = 0;

    if (!goo_dead_code_elimination_get_stats(&pass_manager, &removed_instructions, &removed_blocks, &eliminated_values)) {
        return false;
    }

    // Check if the pass made changes and removed at least one block and one instruction
    return changed and removed_blocks > 0 and removed_instructions > 0;
}
