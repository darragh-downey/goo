# Goo Programming Language

Goo is a modern systems programming language drawing inspiration from Go, Rust, Zig, and Erlang. It aims to provide the simplicity of Go with the safety features of Rust, the build-time reflection of Zig, and the concurrency model of Erlang.

## Project Status

The Goo compiler is under active development. Current components include:

- **Lexer**: Tokenizes Goo source code
- **Parser**: Builds an Abstract Syntax Tree from tokens
- **Semantic Analyzer**: Performs type checking and semantic validation

## Getting Started

### Prerequisites

- Zig 0.11.0 or later
- A C compiler (GCC, Clang, or MSVC)
- Make

### Building the Compiler

To build all components:

```bash
./build_and_test.sh
```

This script builds the lexer, parser, semantic analyzer, and example applications, then runs tests to verify functionality.

## Compiler Architecture

The Goo compiler consists of the following components:

1. **Frontend**
   - Lexer: Converts source code to tokens
   - Parser: Builds an Abstract Syntax Tree (AST)

2. **Semantic Analysis**
   - Type Checker: Validates types and expressions
   - Symbol Table: Tracks declarations and scopes
   - Error Reporter: Provides detailed semantic errors

3. **Backend** (In Development)
   - LLVM IR Generator
   - Optimization Pipeline
   - Code Generator

Each component is implemented in Zig and provides a C API for integration.

## Language Features

Goo supports:

- Package system for code organization
- Import statements for dependencies
- Functions with return type annotations
- Variables and constants with type inference
- Control flow (if/else, for loops)
- Strong static typing
- Type expressions

## Example Goo Program

```go
package main

import "std"

func fibonacci(n: int) int {
    if n <= 1 {
        return n
    }
    return fibonacci(n - 1) + fibonacci(n - 2)
}

func main() {
    std.println(fibonacci(10))
}
```

## Contributing

Contributions are welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## Roadmap

For the planned features and development timeline, see the [ROADMAP.md](ROADMAP.md) file.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details. 