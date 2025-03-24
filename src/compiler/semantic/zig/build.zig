const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Create semantic module
    const semantic_module = b.createModule(.{
        .root_source_file = b.path("./semantic.zig"),
    });

    // Static library
    const semantic_lib = b.addStaticLibrary(.{
        .name = "goosemantic",
        .root_source_file = b.path("./semantic_c_api.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add module to library
    semantic_lib.root_module.addImport("semantic", semantic_module);

    // Link with C standard library
    semantic_lib.linkLibC();

    // Install static library
    b.installArtifact(semantic_lib);

    // Install header file
    b.installFile("./include/goo_semantic.h", "include/goo_semantic.h");

    // Test executable
    const semantic_test = b.addExecutable(.{
        .name = "semantic_test",
        .root_source_file = b.path("./test_semantic.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add module to test executable
    semantic_test.root_module.addImport("semantic", semantic_module);

    // Install test executable
    b.installArtifact(semantic_test);

    // Run test command
    const run_test_cmd = b.addRunArtifact(semantic_test);
    run_test_cmd.step.dependOn(b.getInstallStep());

    const run_test_step = b.step("test", "Run semantic analyzer tests");
    run_test_step.dependOn(&run_test_cmd.step);

    // Shared library for C integration
    const semantic_shared_lib = b.addSharedLibrary(.{
        .name = "goosemantic",
        .root_source_file = b.path("./semantic_c_api.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add module to shared library
    semantic_shared_lib.root_module.addImport("semantic", semantic_module);

    // Export symbols for Windows
    if (target.result.os.tag == .windows) {
        semantic_shared_lib.root_module.addCMacro("GOOSEMANTIC_EXPORT", "1");
    }

    // Link with C standard library
    semantic_shared_lib.linkLibC();

    // Install shared library
    b.installArtifact(semantic_shared_lib);
}
