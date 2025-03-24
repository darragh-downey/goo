const std = @import("std");
const ir = @import("ir.zig");
const pm = @import("pass_manager.zig");

/// The DeadCodeElimination optimization pass removes unreachable code and unused values.
pub const DeadCodeElimination = struct {
    /// Allocator for the pass
    allocator: std.mem.Allocator,
    /// Statistics for the pass
    stats: DCEStats,

    /// Statistics specific to dead code elimination
    pub const DCEStats = struct {
        /// Number of unreachable instructions removed
        removed_instructions: u32 = 0,
        /// Number of unreachable basic blocks removed
        removed_blocks: u32 = 0,
        /// Number of unused values eliminated
        eliminated_values: u32 = 0,
    };

    /// Create a new dead code elimination pass
    pub fn create(allocator: std.mem.Allocator) !*DeadCodeElimination {
        const self = try allocator.create(DeadCodeElimination);
        self.* = DeadCodeElimination{
            .allocator = allocator,
            .stats = DCEStats{},
        };
        return self;
    }

    /// Destroy the dead code elimination pass
    pub fn destroy(self: *DeadCodeElimination) void {
        self.allocator.destroy(self);
    }

    /// Get the optimization pass interface
    pub fn getPass(self: *DeadCodeElimination) pm.OptimizationPass {
        return pm.OptimizationPass{
            .name = "DeadCodeElimination",
            .init = init,
            .deinit = deinit,
            .runOnModule = null, // We run on functions
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

    /// Run the dead code elimination pass on a function
    fn runOnFunction(pass: *pm.OptimizationPass, function: *ir.Function) bool {
        const self = @as(*DeadCodeElimination, @ptrCast(@alignCast(pass.context)));

        // Reset statistics for this function
        self.stats.removed_instructions = 0;
        self.stats.removed_blocks = 0;
        self.stats.eliminated_values = 0;

        // First identify reachable blocks through control flow analysis
        var reachable_blocks = try self.markReachableBlocks(function);
        defer reachable_blocks.deinit();

        // Remove unreachable blocks
        const blocks_removed = try self.removeUnreachableBlocks(function, &reachable_blocks);
        self.stats.removed_blocks += blocks_removed;

        // Identify live values through data flow analysis
        var live_values = try self.markLiveValues(function);
        defer live_values.deinit();

        // Remove dead instructions
        const instructions_removed = try self.removeDeadInstructions(function, &live_values);
        self.stats.removed_instructions += instructions_removed;

        // Record statistics
        if (pass.pass_manager) |pass_manager| {
            pass_manager.recordStat(pass, "removed_instructions", @as(u64, self.stats.removed_instructions));
            pass_manager.recordStat(pass, "removed_blocks", @as(u64, self.stats.removed_blocks));
            pass_manager.recordStat(pass, "eliminated_values", @as(u64, self.stats.eliminated_values));
        }

        // Return true if we made any changes
        return blocks_removed > 0 or instructions_removed > 0;
    }

    /// Mark reachable blocks through control flow analysis
    fn markReachableBlocks(self: *DeadCodeElimination, function: *ir.Function) !std.AutoHashMap(*ir.BasicBlock, void) {
        var reachable = std.AutoHashMap(*ir.BasicBlock, void).init(self.allocator);

        // Find entry block
        var entry_block: ?*ir.BasicBlock = null;
        for (function.basic_blocks.items) |block| {
            if (block.block_type == .ENTRY) {
                entry_block = block;
                break;
            }
        }

        // If no entry block, nothing is reachable
        if (entry_block == null) {
            return reachable;
        }

        // Use a worklist algorithm to mark all reachable blocks
        var worklist = std.ArrayList(*ir.BasicBlock).init(self.allocator);
        defer worklist.deinit();

        // Start with the entry block
        try worklist.append(entry_block.?);
        try reachable.put(entry_block.?, {});

        // Process blocks until the worklist is empty
        while (worklist.items.len > 0) {
            const block = worklist.pop();

            // Add all successors to the worklist if not already processed
            for (block.successors.items) |succ| {
                if (!reachable.contains(succ)) {
                    try reachable.put(succ, {});
                    try worklist.append(succ);
                }
            }
        }

        return reachable;
    }

    /// Remove unreachable blocks from the function
    fn removeUnreachableBlocks(self: *DeadCodeElimination, function: *ir.Function, reachable_blocks: *std.AutoHashMap(*ir.BasicBlock, void)) !u32 {
        var removed: u32 = 0;

        // Use a new list to store reachable blocks
        var new_blocks = std.ArrayList(*ir.BasicBlock).init(self.allocator);
        defer new_blocks.deinit();

        // Keep only reachable blocks
        for (function.basic_blocks.items) |block| {
            if (reachable_blocks.contains(block)) {
                try new_blocks.append(block);
            } else {
                // Block is unreachable, remove it
                // First, remove this block from all predecessor/successor lists
                for (block.predecessors.items) |pred| {
                    _ = pred.removeSuccessor(block);
                }
                for (block.successors.items) |succ| {
                    _ = succ.removePredecessor(block);
                }

                // Clean up the block's instructions
                for (block.instructions.items) |instr| {
                    instr.deinit();
                }
                block.instructions.clearAndFree();
                block.deinit();

                removed += 1;
            }
        }

        // Replace the function's blocks with the new list
        function.basic_blocks.clearAndFree();
        try function.basic_blocks.appendSlice(new_blocks.items);

        return removed;
    }

    /// Mark live values through data flow analysis
    fn markLiveValues(self: *DeadCodeElimination, function: *ir.Function) !std.AutoHashMap(ir.Value, void) {
        var live_values = std.AutoHashMap(ir.Value, void).init(self.allocator);

        // First, mark all function parameters as live
        for (function.parameters.items) |param| {
            try live_values.put(param, {});
        }

        // Mark values used in branches and returns as live
        for (function.basic_blocks.items) |block| {
            for (block.instructions.items) |instr| {
                switch (instr.opcode) {
                    .RETURN, .BRANCH, .JUMP, .CALL => {
                        // Mark operands of control flow instructions as live
                        for (instr.operands.items) |operand| {
                            try live_values.put(operand, {});
                        }
                    },
                    else => {},
                }
            }
        }

        // Use a worklist algorithm to propagate liveness
        var worklist = std.ArrayList(ir.Value).init(self.allocator);
        defer worklist.deinit();

        // Initialize worklist with current live values
        var it = live_values.keyIterator();
        while (it.next()) |key| {
            try worklist.append(key.*);
        }

        // Process values until the worklist is empty
        while (worklist.items.len > 0) {
            const value = worklist.pop();

            // Find instructions that define this value
            for (function.basic_blocks.items) |block| {
                for (block.instructions.items) |instr| {
                    if (instr.result) |result| {
                        if (std.meta.eql(result, value)) {
                            // Mark operands of this instruction as live
                            for (instr.operands.items) |operand| {
                                if (!live_values.contains(operand)) {
                                    try live_values.put(operand, {});
                                    try worklist.append(operand);
                                }
                            }
                        }
                    }
                }
            }
        }

        return live_values;
    }

    /// Remove dead instructions from the function
    fn removeDeadInstructions(self: *DeadCodeElimination, function: *ir.Function, live_values: *std.AutoHashMap(ir.Value, void)) !u32 {
        var removed: u32 = 0;

        // Iterate over all basic blocks
        for (function.basic_blocks.items) |block| {
            var new_instructions = std.ArrayList(ir.Instruction).init(self.allocator);
            defer new_instructions.deinit();

            // Keep only instructions with live results or side effects
            for (block.instructions.items) |instr| {
                var keep_instruction = false;

                // Keep instructions with side effects (e.g., calls, returns, stores)
                switch (instr.opcode) {
                    .CALL, .RETURN, .STORE, .FREE, .GOROUTINE_SPAWN, .GOROUTINE_YIELD, .GOROUTINE_JOIN, .CHANNEL_SEND, .CHANNEL_RECEIVE, .CHANNEL_CLOSE => {
                        keep_instruction = true;
                    },
                    else => {
                        // For other instructions, keep if result is live
                        if (instr.result) |result| {
                            keep_instruction = live_values.contains(result);
                        }
                    },
                }

                if (keep_instruction) {
                    try new_instructions.append(instr);
                } else {
                    // Instruction is dead, can be removed
                    instr.deinit();
                    removed += 1;
                    self.stats.eliminated_values += 1;
                }
            }

            // Replace the block's instructions with the new list
            block.instructions.clearAndFree();
            try block.instructions.appendSlice(new_instructions.items);
        }

        return removed;
    }
};

test "dead code elimination - unreachable blocks" {
    const allocator = std.testing.allocator;

    // Create a test IR with unreachable blocks
    var module = try ir.Module.create(allocator, "test_module");
    defer module.deinit();

    var function = try module.addFunction("test_function");

    // Create basic blocks: entry, block1, block2, unreachable, exit
    var entry_block = try function.addBasicBlock(.ENTRY, "entry");
    var block1 = try function.addBasicBlock(.NORMAL, "block1");
    var block2 = try function.addBasicBlock(.NORMAL, "block2");
    var unreachable_block = try function.addBasicBlock(.NORMAL, "unreachable");
    var exit_block = try function.addBasicBlock(.EXIT, "exit");

    // Create control flow: entry -> block1 -> block2 -> exit
    try entry_block.addSuccessor(block1);
    try block1.addPredecessor(entry_block);

    try block1.addSuccessor(block2);
    try block2.addPredecessor(block1);

    try block2.addSuccessor(exit_block);
    try exit_block.addPredecessor(block2);

    // Create a constant instruction in the unreachable block
    var const_instr = try ir.Instruction.create(null);
    const_instr.opcode = .CONST;
    try unreachable_block.addInstruction(const_instr);

    // Create pass manager
    var pass_manager = try pm.PassManager.create(allocator, .DEFAULT, true);
    defer pass_manager.deinit();

    // Create DCE pass
    var dce_pass = try DeadCodeElimination.create(allocator);
    defer dce_pass.destroy();

    // Add pass to pass manager
    try pass_manager.addFunctionPass(dce_pass.getPass());

    // Run passes
    const changed = try pass_manager.run(module);

    // The pass should have made changes
    try std.testing.expect(changed);

    // Verify the number of blocks in the function
    // The unreachable block should be eliminated
    try std.testing.expectEqual(@as(usize, 4), function.basic_blocks.items.len);

    // Verify the statistics
    try std.testing.expectEqual(@as(u32, 1), dce_pass.stats.removed_blocks);
}

test "dead code elimination - dead instructions" {
    const allocator = std.testing.allocator;

    // Create a test IR with dead instructions
    var module = try ir.Module.create(allocator, "test_module");
    defer module.deinit();

    var function = try module.addFunction("test_function");

    // Create values
    const x = try function.createLocal("x");
    const y = try function.createLocal("y");
    const z = try function.createLocal("z");
    const unused = try function.createLocal("unused");

    // Create a basic block
    var entry_block = try function.addBasicBlock(.ENTRY, "entry");
    var exit_block = try function.addBasicBlock(.EXIT, "exit");

    // Link blocks
    try entry_block.addSuccessor(exit_block);
    try exit_block.addPredecessor(entry_block);

    // Create instructions in entry block
    // x = 10
    var instr_x = try ir.Instruction.create(null);
    instr_x.opcode = .CONST;
    instr_x.result = x;
    try instr_x.addOperand(10);
    try entry_block.addInstruction(instr_x);

    // y = 20
    var instr_y = try ir.Instruction.create(null);
    instr_y.opcode = .CONST;
    instr_y.result = y;
    try instr_y.addOperand(20);
    try entry_block.addInstruction(instr_y);

    // z = x + y (used in return)
    var instr_z = try ir.Instruction.create(null);
    instr_z.opcode = .ADD;
    instr_z.result = z;
    try instr_z.addOperand(x);
    try instr_z.addOperand(y);
    try entry_block.addInstruction(instr_z);

    // unused = x * y (not used anywhere)
    var instr_unused = try ir.Instruction.create(null);
    instr_unused.opcode = .MUL;
    instr_unused.result = unused;
    try instr_unused.addOperand(x);
    try instr_unused.addOperand(y);
    try entry_block.addInstruction(instr_unused);

    // return z
    var return_instr = try ir.Instruction.create(null);
    return_instr.opcode = .RETURN;
    try return_instr.addOperand(z);
    try exit_block.addInstruction(return_instr);

    // Create pass manager
    var pass_manager = try pm.PassManager.create(allocator, .DEFAULT, true);
    defer pass_manager.deinit();

    // Create DCE pass
    var dce_pass = try DeadCodeElimination.create(allocator);
    defer dce_pass.destroy();

    // Add pass to pass manager
    try pass_manager.addFunctionPass(dce_pass.getPass());

    // Run passes
    const changed = try pass_manager.run(module);

    // The pass should have made changes
    try std.testing.expect(changed);

    // Verify the number of instructions in entry block
    // The unused instruction should be eliminated
    try std.testing.expectEqual(@as(usize, 3), entry_block.instructions.items.len);

    // Verify the statistics
    try std.testing.expectEqual(@as(u32, 1), dce_pass.stats.removed_instructions);
}
