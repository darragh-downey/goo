const std = @import("std");

// This function is the entry point for the Zig build system
pub fn build(b: *std.Build) void {
    // Standard target options
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Create AST module
    const ast_module = b.createModule(.{
        .root_source_file = b.path("ast.zig"),
    });

    // Create errors module
    const errors_module = b.createModule(.{
        .root_source_file = b.path("errors.zig"),
    });

    // Create lexer module dependency - reference the lexer module
    const lexer_module = b.createModule(.{
        .root_source_file = b.path("../../../frontend/lexer/zig/lexer.zig"),
    });

    // Create the parser module
    const parser_module = b.createModule(.{
        .root_source_file = b.path("parser.zig"),
    });
    parser_module.addImport("ast", ast_module);
    parser_module.addImport("errors", errors_module);
    parser_module.addImport("lexer", lexer_module);

    // Create a library
    const parser_lib = b.addStaticLibrary(.{
        .name = "gooparser",
        .root_source_file = b.path("parser_c_api.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add all modules to the library
    parser_lib.root_module.addImport("parser", parser_module);
    parser_lib.root_module.addImport("ast", ast_module);
    parser_lib.root_module.addImport("errors", errors_module);
    parser_lib.root_module.addImport("lexer", lexer_module);

    // Export C API
    parser_lib.linkLibC();

    // Install the library
    b.installArtifact(parser_lib);

    // Build the header file
    b.installFile("goo_parser.h", "include/goo_parser.h");

    // Test executable
    const parser_test = b.addExecutable(.{
        .name = "parser_test",
        .root_source_file = b.path("test_parser.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add all modules to the test executable
    parser_test.root_module.addImport("parser", parser_module);
    parser_test.root_module.addImport("ast", ast_module);
    parser_test.root_module.addImport("errors", errors_module);
    parser_test.root_module.addImport("lexer", lexer_module);

    // Install the test executable
    b.installArtifact(parser_test);

    // Run the test executable
    const run_cmd = b.addRunArtifact(parser_test);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("test", "Run parser tests");
    run_step.dependOn(&run_cmd.step);
}
