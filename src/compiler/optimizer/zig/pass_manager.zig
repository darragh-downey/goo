const std = @import("std");
const ir = @import("ir.zig");

const Allocator = std.mem.Allocator;
const Module = ir.Module;
const Function = ir.Function;

/// Interface for optimization passes
pub const OptimizationPass = struct {
    /// Unique name for the optimization pass
    name: []const u8,

    /// Function pointer for pass execution on a module
    run_module_fn: ?*const fn (*OptimizationPass, *Module) anyerror!bool = null,

    /// Function pointer for pass execution on a function
    run_function_fn: ?*const fn (*OptimizationPass, *Function) anyerror!bool = null,

    /// Function pointer for pass initialization
    init_fn: ?*const fn (*OptimizationPass, *PassManager) anyerror!void = null,

    /// Function pointer for pass finalization
    deinit_fn: ?*const fn (*OptimizationPass) void = null,

    /// Pass-specific data
    data: ?*anyopaque = null,

    /// Run the pass on a module
    pub fn runOnModule(self: *OptimizationPass, module: *Module) !bool {
        if (self.run_module_fn) |run_fn| {
            return run_fn(self, module);
        }
        return false;
    }

    /// Run the pass on a function
    pub fn runOnFunction(self: *OptimizationPass, function: *Function) !bool {
        if (self.run_function_fn) |run_fn| {
            return run_fn(self, function);
        }
        return false;
    }

    /// Initialize the pass with the pass manager
    pub fn init(self: *OptimizationPass, pass_manager: *PassManager) !void {
        if (self.init_fn) |init_fn| {
            try init_fn(self, pass_manager);
        }
    }

    /// Clean up pass resources
    pub fn deinit(self: *OptimizationPass) void {
        if (self.deinit_fn) |deinit_fn| {
            deinit_fn(self);
        }
    }
};

/// Target optimization level
pub const OptimizationLevel = enum {
    None, // -O0: No optimizations
    Debug, // -O1: Basic optimizations that don't interfere with debugging
    Default, // -O2: Default optimizations for production code
    Size, // -Os: Optimize for size
    Speed, // -O3: Optimize for speed, potentially at the cost of binary size
};

/// Pass manager configuration
pub const PassManagerConfig = struct {
    /// Optimization level
    optimization_level: OptimizationLevel = .Default,

    /// Size/speed trade-off (0-100, where 0 favors size and 100 favors speed)
    size_speed_tradeoff: u8 = 50,

    /// Enable thread parallelism for function-level passes
    enable_parallelism: bool = false,

    /// Maximum number of threads for parallel execution
    max_threads: u8 = 4,

    /// Enable verbose output
    verbose: bool = false,

    /// Custom allocator for pass allocations
    allocator: ?Allocator = null,

    /// Collect statistics
    collect_statistics: bool = false,
};

/// Pass execution statistics
pub const PassStatistics = struct {
    /// Name of the pass
    pass_name: []const u8,

    /// Number of transformations applied
    transformations: usize = 0,

    /// Execution time in nanoseconds
    execution_time_ns: u64 = 0,

    /// Peak memory usage in bytes
    peak_memory_bytes: usize = 0,

    /// Total functions processed
    total_functions_processed: usize = 0,

    /// Modified functions
    modified_functions: usize = 0,
};

/// Pass manager for organizing and running optimization passes
pub const PassManager = struct {
    /// List of module passes
    module_passes: std.ArrayList(*OptimizationPass),

    /// List of function passes
    function_passes: std.ArrayList(*OptimizationPass),

    /// Pass execution statistics
    statistics: std.ArrayList(PassStatistics),

    /// Configuration
    config: PassManagerConfig,

    /// Allocator for the pass manager
    allocator: Allocator,

    /// Pass execution timer
    timer: std.time.Timer,

    /// Initialize the pass manager
    pub fn init(allocator: Allocator, config: PassManagerConfig) !PassManager {
        var adjusted_config = config;

        // Use the provided allocator or fall back to the one from config
        const alloc = if (config.allocator) |a| a else allocator;
        adjusted_config.allocator = null; // Avoid storing the allocator twice

        return PassManager{
            .module_passes = std.ArrayList(*OptimizationPass).init(alloc),
            .function_passes = std.ArrayList(*OptimizationPass).init(alloc),
            .statistics = std.ArrayList(PassStatistics).init(alloc),
            .config = adjusted_config,
            .allocator = alloc,
            .timer = try std.time.Timer.start(),
        };
    }

    /// Clean up pass manager resources
    pub fn deinit(self: *PassManager) void {
        // Deinitialize all passes
        for (self.module_passes.items) |pass| {
            pass.deinit();
        }

        for (self.function_passes.items) |pass| {
            pass.deinit();
        }

        // Free the lists
        self.module_passes.deinit();
        self.function_passes.deinit();
        self.statistics.deinit();
    }

    /// Add a module pass to the manager
    pub fn addModulePass(self: *PassManager, pass: *OptimizationPass) !void {
        try pass.init(self);
        try self.module_passes.append(pass);
    }

    /// Add a function pass to the manager
    pub fn addFunctionPass(self: *PassManager, pass: *OptimizationPass) !void {
        try pass.init(self);
        try self.function_passes.append(pass);
    }

    /// Run all passes on a module
    pub fn run(self: *PassManager, module: *Module) !bool {
        var modified = false;

        // Run module passes
        for (self.module_passes.items) |pass| {
            const start_time = self.timer.lap();

            if (self.config.verbose) {
                std.debug.print("Running module pass: {s}\n", .{pass.name});
            }

            const pass_modified = try pass.runOnModule(module);
            modified = modified or pass_modified;

            const execution_time = self.timer.lap();

            try self.recordStatistics(pass.name, pass_modified, start_time, execution_time);
        }

        // Run function passes on each function
        for (module.functions.items) |function| {
            const function_modified = try self.runFunctionPasses(function);
            modified = modified or function_modified;
        }

        return modified;
    }

    /// Run function passes on a single function
    pub fn runFunctionPasses(self: *PassManager, function: *Function) !bool {
        var modified = false;

        for (self.function_passes.items) |pass| {
            const start_time = self.timer.lap();

            if (self.config.verbose) {
                std.debug.print("Running function pass on {s}: {s}\n", .{ function.name, pass.name });
            }

            const pass_modified = try pass.runOnFunction(function);
            modified = modified or pass_modified;

            const execution_time = self.timer.lap();

            try self.recordStatistics(pass.name, pass_modified, start_time, execution_time);
        }

        return modified;
    }

    /// Record statistics for a pass execution
    fn recordStatistics(self: *PassManager, pass_name: []const u8, modified: bool, start_time: u64, end_time: u64) !void {
        const execution_time = end_time - start_time;

        // Find existing stats or create new ones
        for (self.statistics.items) |*stat| {
            if (std.mem.eql(u8, stat.pass_name, pass_name)) {
                // Update existing statistics
                stat.execution_time_ns += execution_time;
                if (modified) {
                    stat.transformations += 1;
                }
                return;
            }
        }

        // Create new statistics
        const stats = PassStatistics{
            .pass_name = pass_name,
            .transformations = if (modified) 1 else 0,
            .execution_time_ns = execution_time,
            .peak_memory_bytes = 0, // TODO: Track memory usage
        };

        try self.statistics.append(stats);
    }

    /// Print pass statistics
    pub fn printStatistics(self: *PassManager, writer: anytype) !void {
        try writer.writeAll("Pass Manager Statistics:\n");
        try writer.writeAll("----------------------\n");

        for (self.statistics.items) |stat| {
            const execution_time_ms = @as(f64, @floatFromInt(stat.execution_time_ns)) / 1_000_000.0;
            try writer.print("{s}:\n", .{stat.name});
            try writer.print("  Transformations: {d}\n", .{stat.transformations});
            try writer.print("  Execution time: {d:.2} ms\n", .{execution_time_ms});
            if (stat.peak_memory_bytes > 0) {
                const memory_mb = @as(f64, @floatFromInt(stat.peak_memory_bytes)) / (1024.0 * 1024.0);
                try writer.print("  Peak memory: {d:.2} MB\n", .{memory_mb});
            }
            try writer.writeAll("\n");
        }
    }

    /// Get the current optimization level
    pub fn getOptimizationLevel(self: *PassManager) OptimizationLevel {
        return self.config.optimization_level;
    }

    /// Check if a feature is enabled based on the optimization level
    pub fn isFeatureEnabled(self: *PassManager, feature: []const u8, min_level: OptimizationLevel) bool {
        _ = feature; // Feature name can be used for more granular control
        return @intFromEnum(self.config.optimization_level) >= @intFromEnum(min_level);
    }
};

/// Sample optimization pass that performs constant folding
pub const ConstantFoldingPass = struct {
    base: OptimizationPass = OptimizationPass{
        .init = init,
        .deinit = deinit,
        .runOnFunction = runOnFunction,
        .runOnModule = null,
    },

    max_iterations: u32,
    allocator: Allocator,

    /// Statistics for the pass
    stats: PassStatistics = PassStatistics{
        .name = "Constant Folding",
        .total_functions_processed = 0,
        .modified_functions = 0,
    },

    pub fn create(allocator: Allocator, max_iterations: u32) !*ConstantFoldingPass {
        const pass = try allocator.create(ConstantFoldingPass);
        pass.* = .{
            .max_iterations = max_iterations,
            .allocator = allocator,
        };
        return pass;
    }

    pub fn destroy(self: *ConstantFoldingPass) void {
        self.allocator.destroy(self);
    }

    pub fn getPass(self: *ConstantFoldingPass) *OptimizationPass {
        return &self.base;
    }

    fn init(base: *OptimizationPass) void {
        // self is the parent struct that contains base (ConstantFoldingPass)
        const self: *ConstantFoldingPass = @fieldParentPtr(ConstantFoldingPass, "base", base);
        // Any initialization logic here
        std.debug.print("Initializing ConstantFolding pass with max iterations: {d}\n", .{self.max_iterations});
    }

    fn deinit(base: *OptimizationPass) void {
        const self: *ConstantFoldingPass = @fieldParentPtr(ConstantFoldingPass, "base", base);
        self.allocator.destroy(self);
    }

    fn runOnFunction(base: *OptimizationPass, function: *Function) !bool {
        const self: *ConstantFoldingPass = @fieldParentPtr(ConstantFoldingPass, "base", base);
        self.stats.total_functions_processed += 1;

        // Simulate folding a constant
        self.stats.modified_functions += 1;

        return true;
    }
};

fn initConstantFoldingPass(base: *OptimizationPass) void {
    const self = @fieldParentPtr(ConstantFoldingPass, "base", base);
    self.stats = PassStatistics{
        .name = "Constant Folding",
        .total_functions_processed = 0,
        .modified_functions = 0,
    };
}

fn deinitConstantFoldingPass(base: *OptimizationPass) void {
    const self = @fieldParentPtr(ConstantFoldingPass, "base", base);
    _ = self; // Unused
}

fn runConstantFoldingOnFunction(base: *OptimizationPass, func: *Function) bool {
    const self = @fieldParentPtr(ConstantFoldingPass, "base", base);
    self.stats.total_functions_processed += 1;

    // Simulate folding a constant
    self.stats.modified_functions += 1;

    return true;
}

// Test the pass manager
test "pass manager" {
    const testing = std.testing;

    // Set up allocator
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const allocator = arena.allocator();

    // Create a simple module
    var module = ir.Module.init(allocator, "test_module");
    defer module.deinit();

    const func = try module.addFunction("test_func");

    _ = try func.addBasicBlock(.Entry);
    _ = try func.addBasicBlock(.Exit);

    // Create pass manager
    var pass_manager = try PassManager.init(allocator, .{
        .optimization_level = .Default,
        .verbose = true,
    });
    defer pass_manager.deinit();

    // Create a constant folding pass
    const folding_pass = try ConstantFoldingPass.create(allocator, 10);
    try pass_manager.addFunctionPass(&folding_pass.base);

    // Run the passes
    _ = try pass_manager.run(&module);

    // Check if we have statistics
    try testing.expectEqual(@as(usize, 1), pass_manager.statistics.items.len);
}

/// C FFI for pass manager
const c = struct {
    /// C-compatible pass manager handle
    pub const GooPassManager = opaque {};

    /// C-compatible optimization level enum
    pub const GooOptimizationLevel = c_int;

    /// Create a new pass manager
    pub export fn goo_pass_manager_create(opt_level: GooOptimizationLevel, verbose: bool) ?*GooPassManager {
        // Map C enum to Zig enum
        const level: OptimizationLevel = switch (opt_level) {
            0 => .None,
            1 => .Debug,
            2 => .Default,
            3 => .Size,
            4 => .Speed,
            else => .Default,
        };

        // Create config
        const config = PassManagerConfig{
            .optimization_level = level,
            .verbose = verbose,
        };

        // Create pass manager
        var pass_manager_ptr = std.heap.c_allocator.create(PassManager) catch return null;

        pass_manager_ptr.* = PassManager.init(std.heap.c_allocator, config) catch {
            std.heap.c_allocator.destroy(pass_manager_ptr);
            return null;
        };

        return @ptrCast(pass_manager_ptr);
    }

    /// Destroy a pass manager
    pub export fn goo_pass_manager_destroy(pass_manager_ptr: ?*GooPassManager) void {
        if (pass_manager_ptr) |pm| {
            var pass_manager: *PassManager = @ptrCast(@alignCast(pm));
            pass_manager.deinit();
            std.heap.c_allocator.destroy(pass_manager);
        }
    }

    /// Run all passes on a module
    pub export fn goo_pass_manager_run(pass_manager_ptr: ?*GooPassManager, module_ptr: ?*ir.bindings.GooIRModule) bool {
        if (pass_manager_ptr == null or module_ptr == null) return false;

        var pass_manager: *PassManager = @ptrCast(@alignCast(pass_manager_ptr.?));
        var module: *Module = @ptrCast(@alignCast(module_ptr.?));

        return pass_manager.run(module) catch return false;
    }

    /// Add the constant folding pass to the pass manager
    pub export fn goo_pass_manager_add_constant_folding(pass_manager_ptr: ?*GooPassManager, max_iterations: u32) bool {
        if (pass_manager_ptr == null) return false;

        var pass_manager: *PassManager = @ptrCast(@alignCast(pass_manager_ptr.?));

        const folding_pass = ConstantFoldingPass.create(std.heap.c_allocator, max_iterations) catch return false;

        pass_manager.addFunctionPass(&folding_pass.base) catch return false;

        return true;
    }
};

/// Main function for standalone building
pub fn main() !void {
    // If building this file as an executable, we could test the pass manager here
    std.debug.print("Goo Optimization Pass Manager\n", .{});
}
