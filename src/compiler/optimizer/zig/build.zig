const std = @import("std");

// Build options
pub fn build(b: *std.Build) void {
    // Standard target options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Create modules
    const ir_mod = b.createModule(.{
        .root_source_file = b.path("ir.zig"),
    });

    const pass_manager_mod = b.createModule(.{
        .root_source_file = b.path("pass_manager.zig"),
        .imports = &.{
            .{ .name = "ir", .module = ir_mod },
        },
    });

    const dce_mod = b.createModule(.{
        .root_source_file = b.path("dead_code_elimination.zig"),
        .imports = &.{
            .{ .name = "ir", .module = ir_mod },
            .{ .name = "pass_manager", .module = pass_manager_mod },
        },
    });

    const dce_bindings_mod = b.createModule(.{
        .root_source_file = b.path("dead_code_elimination_bindings.zig"),
        .imports = &.{
            .{ .name = "ir", .module = ir_mod },
            .{ .name = "pass_manager", .module = pass_manager_mod },
            .{ .name = "dead_code_elimination", .module = dce_mod },
        },
    });

    // Build shared library for C API
    const lib = b.addSharedLibrary(.{
        .name = "goo_optimizer",
        .root_source_file = b.path("bindings.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add dependencies to modules
    lib.root_module.addImport("ir", ir_mod);
    lib.root_module.addImport("pass_manager", pass_manager_mod);
    lib.root_module.addImport("dead_code_elimination", dce_mod);
    lib.root_module.addImport("dead_code_elimination_bindings", dce_bindings_mod);

    // Install the library in the standard location
    b.installArtifact(lib);

    // Create test executable for IR module
    const ir_tests = b.addTest(.{
        .root_source_file = b.path("ir.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Create test executable for pass manager module
    const pass_manager_tests = b.addTest(.{
        .root_source_file = b.path("pass_manager.zig"),
        .target = target,
        .optimize = optimize,
    });
    pass_manager_tests.root_module.addImport("ir", ir_mod);

    // Create test executable for dead code elimination
    const dce_tests = b.addTest(.{
        .root_source_file = b.path("dead_code_elimination.zig"),
        .target = target,
        .optimize = optimize,
    });
    dce_tests.root_module.addImport("ir", ir_mod);
    dce_tests.root_module.addImport("pass_manager", pass_manager_mod);

    // Create test executable for bindings
    const bindings_tests = b.addTest(.{
        .root_source_file = b.path("bindings.zig"),
        .target = target,
        .optimize = optimize,
    });
    bindings_tests.root_module.addImport("ir", ir_mod);
    bindings_tests.root_module.addImport("pass_manager", pass_manager_mod);
    bindings_tests.root_module.addImport("dead_code_elimination", dce_mod);

    // Create test executable for DCE bindings
    const dce_bindings_tests = b.addTest(.{
        .root_source_file = b.path("dead_code_elimination_bindings.zig"),
        .target = target,
        .optimize = optimize,
    });
    dce_bindings_tests.root_module.addImport("ir", ir_mod);
    dce_bindings_tests.root_module.addImport("pass_manager", pass_manager_mod);
    dce_bindings_tests.root_module.addImport("dead_code_elimination", dce_mod);

    // Create test step
    const test_step = b.step("test", "Run the unit tests");
    test_step.dependOn(&b.addRunArtifact(ir_tests).step);
    test_step.dependOn(&b.addRunArtifact(pass_manager_tests).step);
    test_step.dependOn(&b.addRunArtifact(dce_tests).step);
    test_step.dependOn(&b.addRunArtifact(bindings_tests).step);
    test_step.dependOn(&b.addRunArtifact(dce_bindings_tests).step);

    // Create a step for running C tests
    const c_test_step = b.step("ctest", "Build and run C API tests");

    // Command to build C test
    const build_c_test = b.addSystemCommand(&.{ "gcc", "-o", "../test/optimizer_test", "../test/optimizer_test.c", "-L../../zig-out/lib", "-lgoo_optimizer", "-I..", "-Wl,-rpath,../../zig-out/lib" });
    build_c_test.step.dependOn(&lib.step);
    c_test_step.dependOn(&build_c_test.step);

    // Command to run C test
    const run_c_test = b.addSystemCommand(&.{"../test/optimizer_test"});
    run_c_test.step.dependOn(&build_c_test.step);
    c_test_step.dependOn(&run_c_test.step);

    // Generate documentation
    const docs = b.addExecutable(.{
        .name = "gen_docs",
        .root_source_file = b.path("bindings.zig"),
        .target = target,
        .optimize = optimize,
    });
    docs.root_module.addImport("ir", ir_mod);
    docs.root_module.addImport("pass_manager", pass_manager_mod);
    docs.root_module.addImport("dead_code_elimination", dce_mod);
    docs.root_module.addImport("dead_code_elimination_bindings", dce_bindings_mod);

    const docs_step = b.step("docs", "Generate documentation");
    const install_docs = b.addInstallDirectory(.{
        .source_dir = docs.getEmittedDocs(),
        .install_dir = .prefix,
        .install_subdir = "docs",
    });
    docs_step.dependOn(&install_docs.step);
}
