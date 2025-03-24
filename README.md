# Goo Programming Language

Goo is a modern, efficient programming language with a focus on memory safety, concurrency, and performance.

## Project Structure

The project has been reorganized with a simplified structure:

```
goo/
├── bin/              # Binary artifacts 
├── docs/             # Documentation
├── examples/         # Example code and demos
├── include/          # Public header files
├── src/              # Source code
│   ├── compiler/     # Compiler implementation
│   └── runtime/      # Runtime implementation
├── test_files/       # Test files and resources
└── third_party/      # Third-party dependencies
```

## Build System

The project has been migrated from Makefiles to using the Zig build system exclusively. This provides better integration between C and Zig components and simplifies the build process.

### Building the Project

To build specific components:

```bash
zig build runtime     # Build only the runtime library
zig build lexer       # Build only the lexer
```

To build the entire project:

```bash
./build_and_test.sh
```

Options:
- `--release`: Build in release mode
- `--no-test`: Skip running tests
- `--verbose`: Show detailed build output
- `--runtime-only`: Build and test only the runtime components
- `--lexer-only`: Build and test only the lexer components

## Testing

Run runtime tests:

```bash
zig build run           # Run basic memory test
zig build run-extended  # Run extended memory test
```

Run lexer tests:

```bash
zig build test-lexer    # Run lexer tests
```

Other tests (pending implementation):

```bash
zig build test-parser    # Run parser tests (not available yet)
zig build test-compiler # Run compiler frontend tests (not available yet)
```

## Project Components

1. **Runtime Library**: Memory management and core functionality (C)
2. **Lexer**: Tokenizes source code (Zig)
3. **Parser**: Parses tokens into AST (Zig, in progress)
4. **Compiler Frontend**: Integrates lexer and parser with C interfaces (in progress)

## Development Status

- ✅ Runtime library fully functional
- ✅ Lexer component operational
- 🔄 Parser component in progress
- 🔄 Compiler frontend integration in progress

## Recent Changes

- Consolidated project structure by removing redundant directories
- Migrated completely to Zig build system
- Unified testing approach
- Reorganized header files
- Improved memory management with a two-layer API
- Fixed lexer component to work with Zig 0.14.0
- Improved build script with component-specific options

## Building with Zig

Goo uses Zig as its build system, leveraging Zig's powerful build capabilities for both C and Zig code.

### Prerequisites

- Zig 0.14.0 or later: [https://ziglang.org/download/](https://ziglang.org/download/)

### Directory Structure

- `include/`: Header files for C code
  - `include/goo/`: Modern API headers
  - `include/*.h`: Legacy compatibility headers
- `src/`: Source code
  - `src/runtime/`: Runtime library (written in C)
  - `src/compiler/frontend/lexer/zig/`: Lexer implementation (written in Zig)
  - `src/compiler/frontend/parser/zig/`: Parser implementation (written in Zig)
- `test_files/`: Test files for the compiler and runtime

## Key Features

- **Native Multi-Threading and Parallelism**: Enhanced `go` keyword with `go parallel` variant for seamless distribution of work across multiple cores.
- **Advanced Memory Management**: Explicit memory control with custom allocators and scope-based memory allocation.
- **First-Class Messaging System**: Native channel types supporting advanced messaging patterns (pub-sub, push-pull, request-reply).
- **Compile-Time Features**: Zig-inspired `comptime` for compile-time evaluation and code generation.
- **Safety and Ownership**: Default thread and memory safety with optional `unsafe` mode for performance-critical sections.
- **Enhanced Concurrency**: Erlang-inspired actors for robust, message-passing concurrency and built-in distributed computing support.
- **Advanced Type System**: Enhanced interfaces, generics, algebraic data types, and pattern matching.

## License

[Your license information here] 