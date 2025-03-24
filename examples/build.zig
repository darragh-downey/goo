const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Get the Goo lexer module from the main project
    const lexer_module = b.createModule(.{
        .root_source_file = b.path("../src/compiler/frontend/lexer/zig/lexer.zig"),
    });

    // Hello world example
    const hello_example = b.addExecutable(.{
        .name = "hello",
        .root_source_file = b.path("hello.zig"),
        .target = target,
        .optimize = optimize,
    });
    b.installArtifact(hello_example);

    // Lexer C API example
    const lexer_c_example = b.addExecutable(.{
        .name = "lexer_c_example",
        .source_files = &.{"lexer_c_example.c"},
        .target = target,
        .optimize = optimize,
    });
    
    lexer_c_example.addIncludePath(b.path("../include"));
    lexer_c_example.linkLibPath(b.path("../zig-out/lib"));
    lexer_c_example.linkSystemLibrary("goolexer");
    b.installArtifact(lexer_c_example);

    // Run commands
    const run_hello = b.addRunArtifact(hello_example);
    run_hello.step.dependOn(b.getInstallStep());
    
    const run_lexer_example = b.addRunArtifact(lexer_c_example);
    run_lexer_example.step.dependOn(b.getInstallStep());

    // Run step
    const run_step = b.step("run-hello", "Run the hello example");
    run_step.dependOn(&run_hello.step);

    const run_lexer_step = b.step("run-lexer", "Run the lexer C API example");
    run_lexer_step.dependOn(&run_lexer_example.step);
}
