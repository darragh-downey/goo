const std = @import("std");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Create a module for the lexer
    // Note: We define this module for future use, even though it's not used yet
    _ = b.addModule("lexer", .{
        .root_source_file = b.path("lexer.zig"),
    });

    // Create a static library
    const lib = b.addStaticLibrary(.{
        .name = "goo_lexer",
        .root_source_file = b.path("lexer_c_api.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(lib);

    // Create a shared library
    const dylib = b.addSharedLibrary(.{
        .name = "goo_lexer",
        .root_source_file = b.path("lexer_c_api.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(dylib);

    // Generate the C header
    const headers = b.addInstallHeaderFile(b.path("lexer_bindings.h"), "include/goo_lexer.h");
    b.getInstallStep().dependOn(&headers.step);

    // Add a step for the test program
    const test_lexer_exe = b.addExecutable(.{
        .name = "goo_lexer_test",
        .root_source_file = b.path("test_lexer.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(test_lexer_exe);

    // Add a step for the lexer error handling test program
    const test_lexer_errors_exe = b.addExecutable(.{
        .name = "goo_lexer_error_test",
        .root_source_file = b.path("test_error_handling.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(test_lexer_errors_exe);

    // Add a step for the lexer edge cases test program
    const test_lexer_edge_cases_exe = b.addExecutable(.{
        .name = "goo_lexer_edge_case_test",
        .root_source_file = b.path("test_edge_cases.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(test_lexer_edge_cases_exe);

    // Add a run step for the test program
    const run_lexer_step = b.addRunArtifact(test_lexer_exe);
    if (b.args) |args| {
        run_lexer_step.addArgs(args);
    }
    const run_lexer_cmd = b.step("test-lexer", "Run lexer test program");
    run_lexer_cmd.dependOn(&run_lexer_step.step);

    // Add a run step for the error handling test program
    const run_lexer_error_step = b.addRunArtifact(test_lexer_errors_exe);
    if (b.args) |args| {
        run_lexer_error_step.addArgs(args);
    }
    const run_lexer_error_cmd = b.step("test-lexer-errors", "Run lexer error handling tests");
    run_lexer_error_cmd.dependOn(&run_lexer_error_step.step);

    // Add a run step for the edge cases test program
    const run_lexer_edge_cases_step = b.addRunArtifact(test_lexer_edge_cases_exe);
    if (b.args) |args| {
        run_lexer_edge_cases_step.addArgs(args);
    }
    const run_lexer_edge_cases_cmd = b.step("test-lexer-edge-cases", "Run lexer edge case tests");
    run_lexer_edge_cases_cmd.dependOn(&run_lexer_edge_cases_step.step);

    // Add a combined step for all lexer tests
    const combined_lexer_tests_step = b.step("test-lexer-all", "Run all lexer tests");
    combined_lexer_tests_step.dependOn(&run_lexer_step.step);
    combined_lexer_tests_step.dependOn(&run_lexer_error_step.step);
    combined_lexer_tests_step.dependOn(&run_lexer_edge_cases_step.step);

    // Memory test that doesn't need the header installation
    const memory_test = b.addExecutable(.{
        .name = "test-memory",
        .root_source_file = b.path("test_memory.zig"),
        .target = target,
        .optimize = optimize,
    });

    const run_memory_test = b.addRunArtifact(memory_test);
    // Don't depend on install step which includes the header installation
    const test_memory_step = b.step("test-memory", "Run memory management tests");
    test_memory_step.dependOn(&run_memory_test.step);

    // Run unit tests as well
    const unit_tests = b.addTest(.{
        .root_source_file = b.path("lexer.zig"),
        .target = target,
        .optimize = optimize,
    });
    const run_unit_tests = b.addRunArtifact(unit_tests);
    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_unit_tests.step);
}
