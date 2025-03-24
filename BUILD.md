# Goo Build System

The Goo programming language uses Zig's build system for both C and Zig components. This document explains how to build and test the Goo compiler and its components.

## Requirements

- Zig 0.14.0 or later
- A C compiler (for C components)
- LLVM 18 (optional, for optimized backend)

## Building Goo

To build the entire Goo compiler and all its components:

```sh
zig build all
```

This will build the compiler, all libraries, tools, and examples.

## Build Options

- `-Doptimize=<mode>`: Set the optimization mode (Debug, ReleaseSafe, ReleaseFast, ReleaseSmall)
- `-Dtarget=<target>`: Set the target triple for cross-compilation

Example:

```sh
zig build all -Doptimize=ReleaseFast
```

## Component-Specific Builds

You can build specific components:

- `zig build lexer`: Build just the lexer module
- `zig build parser`: Build just the parser module
- `zig build tools`: Build all tools (LSP server, etc.)
- `zig build examples`: Build all examples
- `zig build benchmark`: Build all benchmarks
- `zig build minimal`: Build only the core components

## Running Tests

To run all tests:

```sh
zig build test
```

Or run component-specific tests:

- `zig build test-lexer`: Run only lexer tests
- `zig build test-parser`: Run only parser tests
- `zig build test-runtime`: Run runtime component tests
- `zig build test-memory`: Run memory allocator tests
- `zig build test-simd`: Run SIMD tests

## Helper Script

The `build_and_test.sh` script provides a convenient way to build and test:

```sh
./build_and_test.sh [--release] [--no-test] [--verbose]
```

Options:
- `--release`: Build in release mode (ReleaseFast)
- `--no-test`: Skip running tests
- `--verbose`: Enable verbose output

## Directory Structure

- `src/`: Source code for the Goo compiler and runtime
  - `compiler/`: Compiler components (frontend, optimizer, backend)
  - `runtime/`: Runtime libraries
  - `tools/`: Tools like the LSP server
- `examples/`: Example programs
- `benchmark/`: Benchmark programs
- `include/`: Public header files
- `tests/`: Test files and test infrastructure

## Contributing

When adding new components to the Goo project:

1. Create a new module in the appropriate subdirectory
2. Add the module to the main `build.zig` file
3. Add appropriate test targets
4. Update documentation if needed

For larger components, create a separate `build.zig` in your component directory and use the main build system as a reference.
