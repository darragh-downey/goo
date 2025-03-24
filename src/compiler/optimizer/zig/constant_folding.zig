const std = @import("std");
const ir = @import("ir.zig");
const pm = @import("pass_manager.zig");

/// The ConstantFolding optimization pass evaluates constant expressions at compile time
/// to reduce runtime computations.
pub const ConstantFolding = struct {
    /// Allocator for the pass
    allocator: std.mem.Allocator,
    /// Maximum iterations for folding
    max_iterations: u32,
    /// Statistics for the pass
    stats: ConstantFoldingStats,

    /// Statistics specific to constant folding
    pub const ConstantFoldingStats = struct {
        /// Number of expressions folded
        folded_expressions: u32 = 0,
        /// Number of instructions removed
        removed_instructions: u32 = 0,
        /// Number of iterations performed
        iterations: u32 = 0,
    };

    /// Create a new constant folding pass
    pub fn create(allocator: std.mem.Allocator, max_iterations: u32) !*ConstantFolding {
        const self = try allocator.create(ConstantFolding);
        self.* = ConstantFolding{
            .allocator = allocator,
            .max_iterations = max_iterations,
            .stats = ConstantFoldingStats{},
        };
        return self;
    }

    /// Destroy the constant folding pass
    pub fn destroy(self: *ConstantFolding) void {
        self.allocator.destroy(self);
    }

    /// Get the optimization pass interface
    pub fn getPass(self: *ConstantFolding) pm.OptimizationPass {
        return pm.OptimizationPass{
            .name = "ConstantFolding",
            .init = init,
            .deinit = deinit,
            .runOnModule = null, // This pass only operates on functions
            .runOnFunction = runOnFunction,
            .context = self,
        };
    }

    /// Initialize the pass
    fn init(pass: *pm.OptimizationPass) bool {
        _ = pass;
        // Nothing to initialize
        return true;
    }

    /// Clean up the pass
    fn deinit(pass: *pm.OptimizationPass) void {
        _ = pass;
        // Nothing to clean up
    }

    /// Run the constant folding pass on a function
    fn runOnFunction(pass: *pm.OptimizationPass, function: *ir.Function) bool {
        const self = @as(*ConstantFolding, @ptrCast(@alignCast(pass.context)));

        // Reset statistics for this function
        self.stats.folded_expressions = 0;
        self.stats.removed_instructions = 0;
        self.stats.iterations = 0;

        // Process until fixed point or max iterations reached
        var changed = false;
        var iteration_changed = true;

        while (iteration_changed and self.stats.iterations < self.max_iterations) {
            iteration_changed = self.foldConstantsInFunction(function);
            changed = changed or iteration_changed;
            self.stats.iterations += 1;
        }

        // Record statistics
        if (pass.pass_manager) |pass_manager| {
            pass_manager.recordStat(pass, "folded_expressions", @as(u64, self.stats.folded_expressions));
            pass_manager.recordStat(pass, "removed_instructions", @as(u64, self.stats.removed_instructions));
            pass_manager.recordStat(pass, "iterations", @as(u64, self.stats.iterations));
        }

        return changed;
    }

    /// Fold constants in a function
    fn foldConstantsInFunction(self: *ConstantFolding, function: *ir.Function) bool {
        var changed = false;

        // Create a map of known constant values
        var constant_map = std.AutoHashMap(ir.Value, i64).init(self.allocator);
        defer constant_map.deinit();

        // Scan all blocks
        for (function.basic_blocks.items) |block| {
            if (self.foldConstantsInBlock(block, &constant_map)) {
                changed = true;
            }
        }

        return changed;
    }

    /// Fold constants in a basic block
    fn foldConstantsInBlock(self: *ConstantFolding, block: *ir.BasicBlock, constant_map: *std.AutoHashMap(ir.Value, i64)) bool {
        var changed = false;
        var i: usize = 0;

        // We use a while loop since we might remove instructions during iteration
        while (i < block.instructions.items.len) {
            const instr = block.instructions.items[i];

            switch (instr.opcode) {
                .CONST => {
                    // Record constant value
                    if (instr.result) |result| {
                        if (instr.operands.items.len > 0) {
                            // Assume the first operand is the constant value (stored as Value)
                            const const_val = @as(i64, @intCast(instr.operands.items[0]));
                            constant_map.put(result, const_val) catch {};
                        }
                    }
                    i += 1;
                },

                .ADD, .SUB, .MUL, .DIV, .MOD => {
                    // Try to fold binary operation if both operands are constants
                    if (instr.operands.items.len >= 2 and instr.result != null) {
                        const lhs_val = constant_map.get(instr.operands.items[0]);
                        const rhs_val = constant_map.get(instr.operands.items[1]);

                        if (lhs_val != null and rhs_val != null) {
                            // Both operands are constants, fold the expression
                            const lhs = lhs_val.?;
                            const rhs = rhs_val.?;

                            const result_val = switch (instr.opcode) {
                                .ADD => lhs + rhs,
                                .SUB => lhs - rhs,
                                .MUL => lhs * rhs,
                                .DIV => if (rhs != 0) lhs / rhs else return false,
                                .MOD => if (rhs != 0) @rem(lhs, rhs) else return false,
                                else => unreachable,
                            };

                            // Create a constant instruction to replace the binary op
                            var const_instr = ir.Instruction.create(instr.source_location) catch {
                                i += 1;
                                continue;
                            };
                            const_instr.opcode = .CONST;
                            const_instr.result = instr.result;
                            const_instr.addOperand(@as(ir.Value, @intCast(result_val))) catch {
                                const_instr.deinit();
                                i += 1;
                                continue;
                            };

                            // Record the constant value in our map
                            constant_map.put(instr.result.?, result_val) catch {};

                            // Replace the instruction
                            block.instructions.items[i].deinit();
                            block.instructions.items[i] = const_instr;

                            // Update statistics
                            self.stats.folded_expressions += 1;
                            changed = true;
                            i += 1;
                        } else {
                            i += 1;
                        }
                    } else {
                        i += 1;
                    }
                },

                .NEG, .NOT => {
                    // Try to fold unary operation if operand is constant
                    if (instr.operands.items.len >= 1 and instr.result != null) {
                        const operand_val = constant_map.get(instr.operands.items[0]);

                        if (operand_val != null) {
                            // Operand is a constant, fold the expression
                            const operand = operand_val.?;

                            const result_val = switch (instr.opcode) {
                                .NEG => -operand,
                                .NOT => ~operand,
                                else => unreachable,
                            };

                            // Create a constant instruction to replace the unary op
                            var const_instr = ir.Instruction.create(instr.source_location) catch {
                                i += 1;
                                continue;
                            };
                            const_instr.opcode = .CONST;
                            const_instr.result = instr.result;
                            const_instr.addOperand(@as(ir.Value, @intCast(result_val))) catch {
                                const_instr.deinit();
                                i += 1;
                                continue;
                            };

                            // Record the constant value in our map
                            constant_map.put(instr.result.?, result_val) catch {};

                            // Replace the instruction
                            block.instructions.items[i].deinit();
                            block.instructions.items[i] = const_instr;

                            // Update statistics
                            self.stats.folded_expressions += 1;
                            changed = true;
                            i += 1;
                        } else {
                            i += 1;
                        }
                    } else {
                        i += 1;
                    }
                },

                else => i += 1,
            }
        }

        return changed;
    }
};

// Tests for the constant folding pass
test "constant folding" {
    const allocator = std.testing.allocator;

    // Create a test IR
    var module = try ir.Module.create(allocator, "test_module");
    defer module.deinit();

    var function = try module.addFunction("test_function");

    // Create values
    const const1 = try function.createLocal("const1");
    const const2 = try function.createLocal("const2");
    const sum = try function.createLocal("sum");
    const product = try function.createLocal("product");
    const result = try function.createLocal("result");

    // Create a basic block
    var entry_block = try function.addBasicBlock(.ENTRY, "entry");

    // Create instructions: const1 = 10, const2 = 20
    var const1_instr = try ir.Instruction.create(null);
    const1_instr.opcode = .CONST;
    const1_instr.result = const1;
    try const1_instr.addOperand(10);
    try entry_block.addInstruction(const1_instr);

    var const2_instr = try ir.Instruction.create(null);
    const2_instr.opcode = .CONST;
    const2_instr.result = const2;
    try const2_instr.addOperand(20);
    try entry_block.addInstruction(const2_instr);

    // Create sum = const1 + const2
    var sum_instr = try ir.Instruction.create(null);
    sum_instr.opcode = .ADD;
    sum_instr.result = sum;
    try sum_instr.addOperand(const1);
    try sum_instr.addOperand(const2);
    try entry_block.addInstruction(sum_instr);

    // Create product = const1 * const2
    var product_instr = try ir.Instruction.create(null);
    product_instr.opcode = .MUL;
    product_instr.result = product;
    try product_instr.addOperand(const1);
    try product_instr.addOperand(const2);
    try entry_block.addInstruction(product_instr);

    // Create result = sum + product
    var result_instr = try ir.Instruction.create(null);
    result_instr.opcode = .ADD;
    result_instr.result = result;
    try result_instr.addOperand(sum);
    try result_instr.addOperand(product);
    try entry_block.addInstruction(result_instr);

    // Create pass manager
    var pass_manager = try pm.PassManager.create(allocator, .DEFAULT, true);
    defer pass_manager.deinit();

    // Create constant folding pass
    var cf_pass = try ConstantFolding.create(allocator, 3);
    defer cf_pass.destroy();

    // Add pass to pass manager
    try pass_manager.addFunctionPass(cf_pass.getPass());

    // Run passes
    const changed = try pass_manager.run(module);

    // The pass should have made changes
    try std.testing.expect(changed);

    // Verify the number of instructions in entry_block
    // After folding, we should have 3 CONST instructions instead of 5 instructions
    // (const1 = 10, const2 = 20, result = 230)
    try std.testing.expectEqual(@as(usize, 3), entry_block.instructions.items.len);

    // The third instruction should be CONST with the folded result
    const folded_instr = entry_block.instructions.items[2];
    try std.testing.expectEqual(ir.Opcode.CONST, folded_instr.opcode);
    try std.testing.expectEqual(result, folded_instr.result.?);

    // The folded value should be 230 (10 + 20 + 10 * 20)
    const folded_value = folded_instr.operands.items[0];
    try std.testing.expectEqual(@as(ir.Value, 230), folded_value);
}
