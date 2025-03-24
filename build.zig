const std = @import("std");

// Build file for Goo programming language
// This creates the runtime library, compiler components, and test executables
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

    // Choose C compiler based on use_clang flag (for external uses outside build system)
    _ = if (use_clang) "clang" else "cc";

    // =======================================
    // Runtime Library
    // =======================================
    const runtime_lib = b.addStaticLibrary(.{
        .name = "goo_runtime",
        .target = target,
        .optimize = optimize,
    });

    // Add runtime C source files
    runtime_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/runtime/memory.c",
            "src/runtime/memory/memory_interface.c",
        },
        .flags = c_flags,
    });

    // Add include directories
    runtime_lib.addIncludePath(.{ .cwd_relative = "include" });
    runtime_lib.addIncludePath(.{ .cwd_relative = "." });

    // =======================================
    // Diagnostics System
    // =======================================
    const diagnostics_lib = b.addStaticLibrary(.{
        .name = "goo_diagnostics",
        .target = target,
        .optimize = optimize,
    });

    // Add diagnostics C source files
    diagnostics_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/tools/diagnostics/diagnostics.c",
            "src/tools/diagnostics/error_catalog.c",
            "src/tools/diagnostics/source_highlight.c",
            "src/tools/diagnostics/diagnostics_module.c",
            "src/goo_diagnostics.c",
            "src/goo_tools.c",
        },
        .flags = c_flags,
    });

    // Add include directories for diagnostics
    diagnostics_lib.addIncludePath(.{ .cwd_relative = "include" });
    diagnostics_lib.addIncludePath(.{ .cwd_relative = "src/tools/diagnostics" });
    diagnostics_lib.linkLibC();

    // Diagnostics example program
    const diagnostics_example = b.addExecutable(.{
        .name = "diagnostics_example",
        .target = target,
        .optimize = optimize,
    });

    // Add diagnostics example source file
    diagnostics_example.addCSourceFile(.{
        .file = .{ .cwd_relative = "src/tools/diagnostics/example.c" },
        .flags = c_flags,
    });

    // Link with diagnostics library
    diagnostics_example.addIncludePath(.{ .cwd_relative = "src/tools/diagnostics" });
    diagnostics_example.linkLibrary(diagnostics_lib);
    diagnostics_example.linkLibC();

    // Diagnostics integration example
    const diagnostics_integration = b.addExecutable(.{
        .name = "diagnostics_integration",
        .target = target,
        .optimize = optimize,
    });

    // Add diagnostics integration source file
    diagnostics_integration.addCSourceFile(.{
        .file = .{ .cwd_relative = "src/tools/diagnostics/integration.c" },
        .flags = c_flags,
    });

    // Link with diagnostics library
    diagnostics_integration.addIncludePath(.{ .cwd_relative = "src/tools/diagnostics" });
    diagnostics_integration.linkLibrary(diagnostics_lib);
    diagnostics_integration.linkLibC();

    // =======================================
    // Test Executables (Runtime only)
    // =======================================
    // Basic test executable
    const test_exe = b.addExecutable(.{
        .name = "goo_test",
        .target = target,
        .optimize = optimize,
    });

    // Add test source file
    test_exe.addCSourceFile(.{
        .file = .{ .cwd_relative = "test_files/goo_test_minimal.c" },
        .flags = c_flags,
    });

    // Add include directories
    test_exe.addIncludePath(.{ .cwd_relative = "include" });
    test_exe.addIncludePath(.{ .cwd_relative = "." });

    // Link with runtime library
    test_exe.linkLibrary(runtime_lib);

    // Extended memory test executable
    const extended_test = b.addExecutable(.{
        .name = "goo_test_extended",
        .target = target,
        .optimize = optimize,
    });

    // Add extended test source file
    extended_test.addCSourceFile(.{
        .file = .{ .cwd_relative = "test_files/goo_test_memory_extended.c" },
        .flags = c_flags,
    });

    // Add include directories
    extended_test.addIncludePath(.{ .cwd_relative = "include" });
    extended_test.addIncludePath(.{ .cwd_relative = "." });

    // Link with runtime library
    extended_test.linkLibrary(runtime_lib);

    // =======================================
    // Install Runtime Artifacts
    // =======================================
    b.installArtifact(runtime_lib);
    b.installArtifact(test_exe);
    b.installArtifact(extended_test);

    // =======================================
    // Install Diagnostics Artifacts
    // =======================================
    b.installArtifact(diagnostics_lib);
    b.installArtifact(diagnostics_example);
    b.installArtifact(diagnostics_integration);

    // =======================================
    // Type System
    // =======================================
    const type_system_lib = b.addStaticLibrary(.{
        .name = "goo_type_system",
        .target = target,
        .optimize = optimize,
    });

    type_system_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/compiler/type/goo_type_system.c",
            "src/compiler/type/goo_type_traits.c",
            "src/compiler/type/goo_type_checker.c",
            "src/compiler/type/goo_type_utils.c",
            "src/compiler/type/type_table_simple.c",
        },
        .flags = c_flags,
    });

    type_system_lib.addIncludePath(b.path("include"));
    type_system_lib.addIncludePath(b.path("src/compiler/type"));
    type_system_lib.addIncludePath(b.path("src/tools/diagnostics"));
    type_system_lib.linkLibC();

    b.installArtifact(type_system_lib);

    const type_system_test = b.addExecutable(.{
        .name = "type_system_test",
        .target = target,
        .optimize = optimize,
    });

    type_system_test.addCSourceFile(.{
        .file = b.path("src/compiler/type/type_system_test.c"),
        .flags = &[_][]const u8{ "-std=c2x", "-Wall", "-Wextra" },
    });

    type_system_test.linkLibrary(type_system_lib);
    type_system_test.linkLibC();

    // =======================================
    // Minimal Diagnostics Test
    // =======================================
    const minimal_diag_test = b.addExecutable(.{
        .name = "minimal_diag_test",
        .target = target,
        .optimize = optimize,
    });

    // Add minimal diagnostics test source file
    minimal_diag_test.addCSourceFile(.{
        .file = .{ .cwd_relative = "src/compiler/type/minimal_test.c" },
        .flags = &[_][]const u8{ "-std=c2x", "-Wall", "-Wextra" },
    });

    // Link with diagnostics library
    minimal_diag_test.addIncludePath(.{ .cwd_relative = "include" });
    minimal_diag_test.addIncludePath(.{ .cwd_relative = "src/tools/diagnostics" });
    minimal_diag_test.linkLibrary(diagnostics_lib);
    minimal_diag_test.linkLibC();

    // Install minimal diagnostics test
    b.installArtifact(minimal_diag_test);

    // Run step for minimal diagnostics test
    const run_min_diag = b.addRunArtifact(minimal_diag_test);
    run_min_diag.step.dependOn(b.getInstallStep());
    const run_min_diag_step = b.step("run-min-diag", "Run the minimal diagnostics test");
    run_min_diag_step.dependOn(&run_min_diag.step);

    // =======================================
    // Run Steps for Runtime Tests
    // =======================================
    // Basic test run step
    const run_cmd = b.addRunArtifact(test_exe);
    run_cmd.step.dependOn(b.getInstallStep());
    const run_step = b.step("run", "Run the minimal test");
    run_step.dependOn(&run_cmd.step);

    // Extended test run step
    const run_extended_cmd = b.addRunArtifact(extended_test);
    run_extended_cmd.step.dependOn(b.getInstallStep());
    const run_extended_step = b.step("run-extended", "Run the extended memory test");
    run_extended_step.dependOn(&run_extended_cmd.step);

    // =======================================
    // Run Steps for Diagnostics Examples
    // =======================================
    // Run step for diagnostics example
    const run_diag_example = b.addRunArtifact(diagnostics_example);
    run_diag_example.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_diag_example.addArgs(args);
    }

    const diag_example_step = b.step("run-diag-example", "Run the diagnostics example");
    diag_example_step.dependOn(&run_diag_example.step);

    // Run step for diagnostics integration
    const run_diag_integration = b.addRunArtifact(diagnostics_integration);
    run_diag_integration.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_diag_integration.addArgs(args);
    }

    const diag_integration_step = b.step("run-diag-integration", "Run the diagnostics integration");
    diag_integration_step.dependOn(&run_diag_integration.step);

    // Component-specific build steps
    const runtime_step = b.step("runtime", "Build only the runtime library");
    runtime_step.dependOn(&runtime_lib.step);

    const diagnostics_step = b.step("diagnostics", "Build only the diagnostics library");
    diagnostics_step.dependOn(&diagnostics_lib.step);

    // =======================================
    // Lexer Component
    // =======================================
    // Create lexer module
    const lexer_module = b.addModule("lexer", .{
        .root_source_file = .{ .src_path = .{
            .owner = b,
            .sub_path = "src/compiler/frontend/lexer/zig/lexer.zig",
        } },
    });

    // Create a static library for the lexer
    const lexer_lib = b.addStaticLibrary(.{
        .name = "goolexer",
        .root_source_file = .{ .src_path = .{
            .owner = b,
            .sub_path = "src/compiler/frontend/lexer/zig/lexer_c_api.zig",
        } },
        .target = target,
        .optimize = optimize,
    });

    // Add the lexer module to the library
    lexer_lib.root_module.addImport("lexer", lexer_module);
    lexer_lib.linkLibC();

    // Lexer test executable
    const lexer_test = b.addExecutable(.{
        .name = "goo_lexer_test",
        .root_source_file = .{ .src_path = .{
            .owner = b,
            .sub_path = "src/compiler/frontend/lexer/zig/test_lexer.zig",
        } },
        .target = target,
        .optimize = optimize,
    });

    // Add the lexer module to the lexer test
    lexer_test.root_module.addImport("lexer", lexer_module);

    // =======================================
    // Install Lexer Components
    // =======================================
    b.installArtifact(lexer_lib);
    b.installArtifact(lexer_test);

    // =======================================
    // Run Steps for Lexer Tests
    // =======================================
    // Lexer test run step
    const run_lexer_cmd = b.addRunArtifact(lexer_test);
    run_lexer_cmd.step.dependOn(b.getInstallStep());
    const run_lexer_step = b.step("test-lexer", "Run the lexer tests");
    run_lexer_step.dependOn(&run_lexer_cmd.step);

    // Lexer error handling test
    const lexer_error_test = b.addExecutable(.{
        .name = "goo_lexer_error_test",
        .root_source_file = .{ .src_path = .{
            .owner = b,
            .sub_path = "src/compiler/frontend/lexer/zig/test_error_handling.zig",
        } },
        .target = target,
        .optimize = optimize,
    });
    lexer_error_test.root_module.addImport("lexer", lexer_module);
    b.installArtifact(lexer_error_test);

    // Run step for lexer error handling test
    const run_lexer_error_cmd = b.addRunArtifact(lexer_error_test);
    run_lexer_error_cmd.step.dependOn(b.getInstallStep());
    const run_lexer_error_step = b.step("test-lexer-errors", "Run the lexer error handling tests");
    run_lexer_error_step.dependOn(&run_lexer_error_cmd.step);

    // Lexer edge case test
    const lexer_edge_test = b.addExecutable(.{
        .name = "goo_lexer_edge_test",
        .root_source_file = .{ .src_path = .{
            .owner = b,
            .sub_path = "src/compiler/frontend/lexer/zig/test_edge_cases.zig",
        } },
        .target = target,
        .optimize = optimize,
    });
    lexer_edge_test.root_module.addImport("lexer", lexer_module);
    b.installArtifact(lexer_edge_test);

    // Run step for lexer edge case test
    const run_lexer_edge_cmd = b.addRunArtifact(lexer_edge_test);
    run_lexer_edge_cmd.step.dependOn(b.getInstallStep());
    const run_lexer_edge_step = b.step("test-lexer-edge-cases", "Run the lexer edge case tests");
    run_lexer_edge_step.dependOn(&run_lexer_edge_cmd.step);

    // Combined lexer tests step (runs both regular lexer tests and error handling tests)
    const combined_lexer_tests_step = b.step("test-lexer-all", "Run all lexer tests including error handling");
    combined_lexer_tests_step.dependOn(&run_lexer_cmd.step);
    combined_lexer_tests_step.dependOn(&run_lexer_error_cmd.step);
    combined_lexer_tests_step.dependOn(&run_lexer_edge_cmd.step);

    // Component-specific build steps
    const lexer_step = b.step("lexer", "Build only the lexer component");
    lexer_step.dependOn(&lexer_lib.step);

    // =======================================
    // Parser Component (Disabled for now)
    // =======================================
    if (false) { // Keep parser disabled until lexer is fixed
        // Create parser module
        const parser_module = b.addModule("parser", .{
            .root_source_file = .{ .src_path = .{
                .owner = b,
                .sub_path = "src/compiler/frontend/parser/zig/parser.zig",
            } },
        });

        // Create a static library for the parser
        const parser_lib = b.addStaticLibrary(.{
            .name = "gooparser",
            .root_source_file = .{ .src_path = .{
                .owner = b,
                .sub_path = "src/compiler/frontend/parser/zig/parser_c_api.zig",
            } },
            .target = target,
            .optimize = optimize,
        });

        // Add the parser module to the library and lexer dependency
        parser_lib.root_module.addImport("parser", parser_module);
        parser_lib.root_module.addImport("lexer", lexer_module);
        parser_lib.linkLibC();

        // Create a static library for the compiler
        const compiler_lib = b.addStaticLibrary(.{
            .name = "goocompiler",
            .target = target,
            .optimize = optimize,
        });

        // Add compiler C source files
        compiler_lib.addCSourceFiles(.{
            .files = &[_][]const u8{
                "src/compiler/frontend/lexer/zig_integration.c",
                "src/compiler/frontend/parser/ast_helpers.c",
            },
            .flags = &[_][]const u8{"-std=c2x"},
        });

        // Add include directories
        compiler_lib.addIncludePath(.{ .cwd_relative = "include" });
        compiler_lib.addIncludePath(.{ .cwd_relative = "src/compiler/frontend/include" });
        compiler_lib.addIncludePath(.{ .cwd_relative = "." });

        // Link with lexer and parser libraries
        compiler_lib.linkLibrary(lexer_lib);
        compiler_lib.linkLibrary(parser_lib);

        // Parser test executable
        const parser_test = b.addExecutable(.{
            .name = "goo_parser_test",
            .root_source_file = .{ .src_path = .{
                .owner = b,
                .sub_path = "src/compiler/frontend/parser/zig/test_parser.zig",
            } },
            .target = target,
            .optimize = optimize,
        });

        // Add the modules to the parser test
        parser_test.root_module.addImport("parser", parser_module);
        parser_test.root_module.addImport("lexer", lexer_module);

        // Compiler frontend test executable
        const compiler_test = b.addExecutable(.{
            .name = "goo_compiler_test",
            .target = target,
            .optimize = optimize,
        });

        // Add compiler test source file
        compiler_test.addCSourceFile(.{
            .file = .{ .cwd_relative = "test_files/goo_test_compiler.c" },
            .flags = &[_][]const u8{"-std=c2x"},
        });

        // Add include directories
        compiler_test.addIncludePath(.{ .cwd_relative = "include" });
        compiler_test.addIncludePath(.{ .cwd_relative = "src/compiler/frontend/include" });
        compiler_test.addIncludePath(.{ .cwd_relative = "." });

        // Link with needed libraries
        compiler_test.linkLibrary(runtime_lib);
        compiler_test.linkLibrary(lexer_lib);
        compiler_test.linkLibrary(parser_lib);
        compiler_test.linkLibrary(compiler_lib);

        // Install Parser Components
        b.installArtifact(parser_lib);
        b.installArtifact(compiler_lib);
        b.installArtifact(parser_test);
        b.installArtifact(compiler_test);

        // Run Steps for Parser Tests
        const run_parser_cmd = b.addRunArtifact(parser_test);
        run_parser_cmd.step.dependOn(b.getInstallStep());
        const run_parser_step = b.step("test-parser", "Run the parser tests");
        run_parser_step.dependOn(&run_parser_cmd.step);

        // Compiler test run step
        const run_compiler_cmd = b.addRunArtifact(compiler_test);
        run_compiler_cmd.step.dependOn(b.getInstallStep());
        const run_compiler_step = b.step("test-compiler", "Run the compiler frontend tests");
        run_compiler_step.dependOn(&run_compiler_cmd.step);

        // Component-specific build steps
        const parser_step = b.step("parser", "Build only the parser component");
        parser_step.dependOn(&parser_lib.step);

        const compiler_step = b.step("compiler", "Build the compiler integration components");
        compiler_step.dependOn(&compiler_lib.step);
    }

    // =======================================
    // Add the unified diagnostics test
    // =======================================
    const unified_diagnostics_test = b.addExecutable(.{
        .name = "unified-diagnostics-test",
        .target = target,
        .optimize = optimize,
    });
    unified_diagnostics_test.addCSourceFiles(.{
        .files = &.{
            "src/compiler/type/type_checker_diagnostics_unified_test.c",
            "src/compiler/type/ast_node_unified.c",
            "src/compiler/type/diagnostics_unified.c",
            "src/compiler/type/type_checker_adapter.c",
        },
        .flags = c_flags,
    });
    unified_diagnostics_test.addIncludePath(.{ .cwd_relative = "src" });
    unified_diagnostics_test.linkLibC();

    // Install the unified diagnostics test
    const install_unified_diagnostics_test = b.addInstallArtifact(unified_diagnostics_test, .{});
    const run_unified_diagnostics_test = b.addRunArtifact(unified_diagnostics_test);
    run_unified_diagnostics_test.step.dependOn(&install_unified_diagnostics_test.step);
    const run_unified_diagnostics_test_step = b.step("run-unified-diagnostics-test", "Run the unified diagnostics test");
    run_unified_diagnostics_test_step.dependOn(&run_unified_diagnostics_test.step);

    // =======================================
    // Compiler Tools
    // =======================================

    // Formatter Library
    const formatter_lib = b.addStaticLibrary(.{
        .name = "goo_formatter",
        .target = target,
        .optimize = optimize,
    });

    formatter_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/tools/formatter/formatter.c",
        },
        .flags = c_flags,
    });

    formatter_lib.addIncludePath(.{ .cwd_relative = "src/tools/formatter" });
    formatter_lib.linkLibC();

    // Formatter CLI tool
    const formatter_cli = b.addExecutable(.{
        .name = "goo-fmt",
        .target = target,
        .optimize = optimize,
    });

    formatter_cli.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/tools/formatter/goo_fmt.c",
        },
        .flags = c_flags,
    });

    formatter_cli.addIncludePath(.{ .cwd_relative = "src/tools/formatter" });
    formatter_cli.linkLibrary(formatter_lib);
    formatter_cli.linkLibC();

    // Symbols Library
    const symbols_lib = b.addStaticLibrary(.{
        .name = "goo_symbols",
        .target = target,
        .optimize = optimize,
    });

    symbols_lib.addCSourceFiles(.{
        .files = &[_][]const u8{
            "src/tools/symbols/symbols.c",
        },
        .flags = c_flags,
    });

    symbols_lib.addIncludePath(.{ .cwd_relative = "src/tools/symbols" });
    symbols_lib.linkLibC();

    // Install compiler tools
    b.installArtifact(formatter_lib);
    b.installArtifact(formatter_cli);
    b.installArtifact(symbols_lib);

    // Run steps for formatter tool
    const run_formatter_cmd = b.addRunArtifact(formatter_cli);
    run_formatter_cmd.step.dependOn(b.getInstallStep());
    const run_formatter_step = b.step("run-fmt", "Run the code formatter");
    run_formatter_step.dependOn(&run_formatter_cmd.step);

    // Component-specific build steps
    const formatter_step = b.step("formatter", "Build only the formatter");
    formatter_step.dependOn(&formatter_lib.step);
    formatter_step.dependOn(&formatter_cli.step);

    const symbols_step = b.step("symbols", "Build only the symbols library");
    symbols_step.dependOn(&symbols_lib.step);

    // =======================================
    // Install Type System Artifacts
    // =======================================
    b.installArtifact(type_system_lib);
    b.installArtifact(type_system_test);

    // =======================================
    // Add the completely standalone diagnostics test
    // =======================================
    const standalone_diag_test = b.addExecutable(.{
        .name = "standalone_diag_test",
        .target = target,
        .optimize = optimize,
    });

    standalone_diag_test.addCSourceFile(.{
        .file = b.path("src/compiler/type/standalone_diagnostics_test.c"),
        .flags = c_flags,
    });

    standalone_diag_test.linkLibC();

    // Install the executable
    b.installArtifact(standalone_diag_test);

    // Create a step to run the standalone diagnostics test
    const run_standalone_diag_test = b.addRunArtifact(standalone_diag_test);
    const run_standalone_diag_step = b.step("run-standalone-diag-test", "Run the completely standalone diagnostics test");
    run_standalone_diag_step.dependOn(&run_standalone_diag_test.step);

    // =======================================
    // Add the type checker diagnostics test
    // =======================================
    const type_checker_diag_test = b.addExecutable(.{
        .name = "type_checker_diagnostics_test",
        .target = target,
        .optimize = optimize,
    });

    type_checker_diag_test.addCSourceFiles(.{
        .files = &.{
            "src/compiler/type/type_checker_diagnostics_test.c",
            "src/compiler/type/ast_node_minimal.c",
            "src/compiler/type/diagnostics_mock.c",
            "src/compiler/type/type_error_adapter.c",
        },
        .flags = c_flags,
    });

    type_checker_diag_test.addIncludePath(b.path("src"));
    type_checker_diag_test.addIncludePath(b.path("src/compiler"));
    type_checker_diag_test.addIncludePath(b.path("src/compiler/type"));
    type_checker_diag_test.linkLibC();

    // Install the executable
    b.installArtifact(type_checker_diag_test);

    // Create a run step for the test
    const run_type_checker_diag_test = b.addRunArtifact(type_checker_diag_test);
    const type_checker_diag_test_step = b.step("run-type-checker-diag-test", "Run type checker diagnostics integration test");
    type_checker_diag_test_step.dependOn(&run_type_checker_diag_test.step);
}
