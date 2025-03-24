const std = @import("std");
const cf = @import("constant_folding.zig");
const pm = @import("pass_manager.zig");

/// Global allocator for C API
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
const allocator = gpa.allocator();

/// Add constant folding pass to pass manager
pub export fn goo_pass_manager_add_constant_folding(pass_manager: *pm.PassManager, max_iterations: u32) bool {
    // Create the constant folding pass
    var cf_pass = cf.ConstantFolding.create(allocator, max_iterations) catch {
        return false;
    };

    // Get the pass interface
    const pass = cf_pass.getPass();

    // Add the pass to the pass manager
    pass_manager.addFunctionPass(pass) catch {
        cf_pass.destroy();
        return false;
    };

    return true;
}

/// Get statistics from the constant folding pass
pub export fn goo_constant_folding_get_stats(pass_manager: *pm.PassManager, folded_expressions: *u32, removed_instructions: *u32, iterations: *u32) bool {
    // Find the constant folding pass
    for (pass_manager.function_passes.items) |*pass| {
        if (std.mem.eql(u8, pass.name, "ConstantFolding")) {
            // Extract statistics
            const folded = pass_manager.getStatValue(pass, "folded_expressions") orelse {
                return false;
            };

            const removed = pass_manager.getStatValue(pass, "removed_instructions") orelse {
                return false;
            };

            const iters = pass_manager.getStatValue(pass, "iterations") orelse {
                return false;
            };

            // Update out parameters
            folded_expressions.* = @as(u32, @intCast(folded));
            removed_instructions.* = @as(u32, @intCast(removed));
            iterations.* = @as(u32, @intCast(iters));

            return true;
        }
    }

    return false;
}

/// Test the constant folding pass with a simple IR
pub export fn goo_test_constant_folding() bool {
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
    const const1 = function.createLocal("const1") catch return false;
    const const2 = function.createLocal("const2") catch return false;
    const sum = function.createLocal("sum") catch return false;
    const product = function.createLocal("product") catch return false;
    const result = function.createLocal("result") catch return false;

    // Create a basic block
    var entry_block = function.addBasicBlock(.ENTRY, "entry") catch return false;

    // Create instructions: const1 = 10, const2 = 20
    var const1_instr = @import("ir.zig").Instruction.create(null) catch return false;
    const1_instr.opcode = .CONST;
    const1_instr.result = const1;
    const1_instr.addOperand(10) catch return false;
    entry_block.addInstruction(const1_instr) catch return false;

    var const2_instr = @import("ir.zig").Instruction.create(null) catch return false;
    const2_instr.opcode = .CONST;
    const2_instr.result = const2;
    const2_instr.addOperand(20) catch return false;
    entry_block.addInstruction(const2_instr) catch return false;

    // Create sum = const1 + const2
    var sum_instr = @import("ir.zig").Instruction.create(null) catch return false;
    sum_instr.opcode = .ADD;
    sum_instr.result = sum;
    sum_instr.addOperand(const1) catch return false;
    sum_instr.addOperand(const2) catch return false;
    entry_block.addInstruction(sum_instr) catch return false;

    // Create product = const1 * const2
    var product_instr = @import("ir.zig").Instruction.create(null) catch return false;
    product_instr.opcode = .MUL;
    product_instr.result = product;
    product_instr.addOperand(const1) catch return false;
    product_instr.addOperand(const2) catch return false;
    entry_block.addInstruction(product_instr) catch return false;

    // Create result = sum + product
    var result_instr = @import("ir.zig").Instruction.create(null) catch return false;
    result_instr.opcode = .ADD;
    result_instr.result = result;
    result_instr.addOperand(sum) catch return false;
    result_instr.addOperand(product) catch return false;
    entry_block.addInstruction(result_instr) catch return false;

    // Create pass manager
    var pass_manager = @import("pass_manager.zig").PassManager.create(std.testing.allocator, .DEFAULT, true) catch return false;
    defer pass_manager.deinit();

    // Add constant folding pass
    if (!goo_pass_manager_add_constant_folding(&pass_manager, 3)) {
        return false;
    }

    // Run passes
    const changed = pass_manager.run(module) catch return false;

    // Get statistics
    var folded_expressions: u32 = 0;
    var removed_instructions: u32 = 0;
    var iterations: u32 = 0;

    if (!goo_constant_folding_get_stats(&pass_manager, &folded_expressions, &removed_instructions, &iterations)) {
        return false;
    }

    // Check if the pass made changes and folded at least one expression
    return changed and folded_expressions > 0;
}
