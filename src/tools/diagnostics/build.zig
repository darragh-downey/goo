const std = @import("std");

pub fn build(b: *std.Build) void {
    // Standard target options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Configure C compiler
    // We'll use clang instead of gcc
    const use_clang = true;

    // Define common C flags
    const common_c_flags = &[_][]const u8{
        "-std=c2x",
        "-Wall",
        "-Wextra",
    };

    // Add clang-specific flags if using clang
    const c_flags = if (use_clang) &[_][]const u8{
        "-std=c2x",
        "-Wall",
        "-Wextra",
        "-fcolor-diagnostics", // Enable colored diagnostics
        "-fno-sanitize-recover=all", // Make sanitizers fatal
        "-Wno-unused-parameter", // Don't warn about unused parameters
    } else common_c_flags;

    // For external uses outside build system
    _ = if (use_clang) "clang" else "cc";

    // Create static library for the diagnostics
    const lib = b.addStaticLibrary(.{
        .name = "goo_diagnostics",
        .target = target,
        .optimize = optimize,
    });

    // Add C sources to the library
    lib.addCSourceFiles(&.{
        "diagnostics.c",
        "error_catalog.c",
        "source_highlight.c",
        "diagnostics_module.c",
    }, c_flags);

    // Install the library
    b.installArtifact(lib);

    // Create the example executable
    const example_exe = b.addExecutable(.{
        .name = "diagnostics_example",
        .target = target,
        .optimize = optimize,
    });

    // Add the example C source file
    example_exe.addCSourceFiles(&.{"example.c"}, c_flags);

    // Link to our diagnostics library
    example_exe.linkLibrary(lib);

    // Install the example executable
    b.installArtifact(example_exe);

    // Create a run artifact for the example
    const run_example_cmd = b.addRunArtifact(example_exe);
    run_example_cmd.step.dependOn(b.getInstallStep());

    // Add a step to run the example
    const run_example_step = b.step("run", "Run the diagnostics example");
    run_example_step.dependOn(&run_example_cmd.step);

    // Create a test executable for our Zig tests
    const tests = b.addTest(.{
        .root_source_file = .{ .path = "diagnostics_tests.zig" },
        .target = target,
        .optimize = optimize,
    });

    // Link to our diagnostics library
    tests.linkLibrary(lib);

    // Create a run artifact for tests
    const run_tests = b.addRunArtifact(tests);

    // Add a step to run the tests
    const test_step = b.step("test", "Run the diagnostics tests");
    test_step.dependOn(&run_tests.step);

    // Add integration executable
    const integration_exe = b.addExecutable(.{
        .name = "diagnostics_integration",
        .target = target,
        .optimize = optimize,
    });

    // Add the integration C source file
    integration_exe.addCSourceFiles(&.{"integration.c"}, c_flags);

    // Link to our diagnostics library
    integration_exe.linkLibrary(lib);

    // Install the integration executable
    b.installArtifact(integration_exe);

    // Create a run artifact for the integration example
    const run_integration_cmd = b.addRunArtifact(integration_exe);
    run_integration_cmd.step.dependOn(b.getInstallStep());

    // Add a step to run the integration example
    const run_integration_step = b.step("run-integration", "Run the diagnostics integration example");
    run_integration_step.dependOn(&run_integration_cmd.step);
}
