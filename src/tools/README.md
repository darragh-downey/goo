# Goo Compiler Tools

This directory contains various tools and utilities for the Goo programming language compiler.

## Available Tools

### Diagnostics

The diagnostics system provides comprehensive error reporting with source code highlighting, suggestions, and detailed explanations. It aims to provide helpful and clear error messages to users.

**Key features:**
- Source code highlighting with terminal colors
- Detailed error explanations
- Code suggestions with applicability information
- Support for related notes and context
- Error codes with `--explain` flag support

See the [diagnostics README](diagnostics/README.md) for more details.

### Formatter

The code formatter (`goo-fmt`) provides automatic code formatting for Goo source files, similar to tools like `gofmt` or `rustfmt`. It helps maintain consistent code style across projects.

**Key features:**
- Configurable indentation and spacing
- Consistent brace styling
- Comment formatting
- Support for directory recursion
- Format checking mode

Usage: `goo-fmt [options] file1 [file2 ...]`

### Symbols

The symbols utility provides code structure analysis and symbol management. It tracks functions, types, variables, and other symbols in the codebase to support features like code navigation and auto-completion.

**Key features:**
- Symbol extraction and indexing
- Support for navigating symbol relationships
- Scope-aware symbol lookup
- Support for code completion

### Linter

The linter provides static code analysis to identify potential issues, enforce best practices, and improve code quality.

**Key features:**
- Configurable rule severity
- Multiple rule categories
- Detailed diagnostic messages
- JSON export for integration with other tools

## Integration

All tools can be used both as standalone utilities and as integrated parts of the Goo compiler. The main compiler includes these tools through the unified `goo_tools.h` header.

## Building

The tools are built automatically as part of the main Goo compiler build. You can also build specific tools with:

```bash
zig build formatter  # Build only the formatter
zig build diagnostics  # Build only the diagnostics system
zig build symbols  # Build only the symbols library
```

To run specific tools:

```bash
zig build run-fmt  # Run the formatter
zig build run-diag-example  # Run the diagnostics example
zig build run-diag-integration  # Run the diagnostics integration example
```

## Development Guidelines

When working on tools:

1. Ensure tools are efficient and fast enough for CI pipelines
2. Maintain backwards compatibility where possible
3. Provide clear, actionable error messages
4. Make configuration straightforward and well-documented
5. Follow the [project coding style](../../CODING_STYLE.md)
