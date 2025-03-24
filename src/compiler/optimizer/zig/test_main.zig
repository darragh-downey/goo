const std = @import("std");
const testing = std.testing;
const ir = @import("ir.zig");
const pass_manager = @import("pass_manager.zig");
const constant_folding = @import("constant_folding.zig");
const dead_code_elimination = @import("dead_code_elimination.zig");

test "IR Module creation" {
    var allocator = std.testing.allocator;
    var module = try ir.Module.create(allocator, "test_module");
    defer module.destroy();

    try testing.expectEqualStrings("test_module", module.name);
    try testing.expect(module.instructions.items.len == 0);
}

test "IR Instruction creation" {
    var instr = try ir.Instruction.create(null);
    defer instr.destroy();

    instr.opcode = .ADD;
    instr.operands.append(try ir.Value.createConstant(i32, 5)) catch unreachable;
    instr.operands.append(try ir.Value.createConstant(i32, 7)) catch unreachable;

    try testing.expect(instr.opcode == .ADD);
    try testing.expect(instr.operands.items.len == 2);
    try testing.expect(instr.operands.items[0].kind == .CONSTANT);
    try testing.expect(instr.operands.items[1].kind == .CONSTANT);
    try testing.expect(instr.operands.items[0].value.integer == 5);
    try testing.expect(instr.operands.items[1].value.integer == 7);
}

test "PassManager creation" {
    var allocator = std.testing.allocator;
    var pm = try pass_manager.PassManager.create(allocator, .DEFAULT, true);
    defer pm.destroy();

    try testing.expect(pm.passes.items.len > 0);
    try testing.expect(pm.debug_mode);
}

test "Constant Folding basic addition" {
    var allocator = std.testing.allocator;
    var module = try ir.Module.create(allocator, "test_module");
    defer module.destroy();

    var const1 = try ir.Value.createConstant(i32, 5);
    var const2 = try ir.Value.createConstant(i32, 7);

    var instr_add = try ir.Instruction.create(null);
    instr_add.opcode = .ADD;
    try instr_add.operands.append(const1);
    try instr_add.operands.append(const2);

    try module.instructions.append(instr_add);

    var cf = try constant_folding.ConstantFolding.create(allocator);
    defer cf.destroy();

    try cf.run(&module);

    // Verify the instruction was folded
    try testing.expect(module.instructions.items.len == 1);
    try testing.expect(module.instructions.items[0].opcode == .CONSTANT);
    try testing.expect(module.instructions.items[0].operands.items.len == 1);
    try testing.expect(module.instructions.items[0].operands.items[0].kind == .CONSTANT);
    try testing.expect(module.instructions.items[0].operands.items[0].value.integer == 12);
}

test "Dead Code Elimination" {
    var allocator = std.testing.allocator;
    var module = try ir.Module.create(allocator, "test_module");
    defer module.destroy();

    // Create variables
    var var_x = try ir.Value.createVariable(i32, "x");
    var var_y = try ir.Value.createVariable(i32, "y");
    var var_z = try ir.Value.createVariable(i32, "z");
    var var_unused = try ir.Value.createVariable(i32, "unused");

    // Define x = 10
    var instr_x = try ir.Instruction.create(null);
    instr_x.opcode = .ASSIGN;
    try instr_x.operands.append(var_x);
    try instr_x.operands.append(try ir.Value.createConstant(i32, 10));
    try module.instructions.append(instr_x);

    // Define y = 20
    var instr_y = try ir.Instruction.create(null);
    instr_y.opcode = .ASSIGN;
    try instr_y.operands.append(var_y);
    try instr_y.operands.append(try ir.Value.createConstant(i32, 20));
    try module.instructions.append(instr_y);

    // Define z = x + y (used)
    var instr_z = try ir.Instruction.create(null);
    instr_z.opcode = .ADD;
    try instr_z.operands.append(var_z);
    try instr_z.operands.append(var_x);
    try instr_z.operands.append(var_y);
    try module.instructions.append(instr_z);

    // Define unused = 30 (should be eliminated)
    var instr_unused = try ir.Instruction.create(null);
    instr_unused.opcode = .ASSIGN;
    try instr_unused.operands.append(var_unused);
    try instr_unused.operands.append(try ir.Value.createConstant(i32, 30));
    try module.instructions.append(instr_unused);

    // Add return z instruction (marks z as used)
    var instr_return = try ir.Instruction.create(null);
    instr_return.opcode = .RETURN;
    try instr_return.operands.append(var_z);
    try module.instructions.append(instr_return);

    // Run DCE
    var dce = try dead_code_elimination.DeadCodeElimination.create(allocator);
    defer dce.destroy();

    try dce.run(&module);

    // Verify unused assignment was eliminated
    try testing.expect(module.instructions.items.len == 4);

    // The instructions should be: assign x, assign y, add z, return z
    try testing.expect(module.instructions.items[0].opcode == .ASSIGN);
    try testing.expect(module.instructions.items[1].opcode == .ASSIGN);
    try testing.expect(module.instructions.items[2].opcode == .ADD);
    try testing.expect(module.instructions.items[3].opcode == .RETURN);

    // Verify the operands of the remaining instructions
    try testing.expectEqualStrings("x", module.instructions.items[0].operands.items[0].name);
    try testing.expectEqualStrings("y", module.instructions.items[1].operands.items[0].name);
    try testing.expectEqualStrings("z", module.instructions.items[2].operands.items[0].name);
    try testing.expectEqualStrings("z", module.instructions.items[3].operands.items[0].name);
}
