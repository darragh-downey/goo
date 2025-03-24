# Goo Programming Language Code Organization

This document provides a comprehensive overview of the code organization for the Goo programming language project.

## Directory Structure Overview

The Goo project follows a structured, component-based organization designed for maintainability and scalability:

```
/goo
├── src/                    # Source code directory
│   ├── compiler/           # Compiler implementation
│   │   ├── frontend/       # Frontend components 
│   │   │   ├── lexer/      # Lexical analysis
│   │   │   └── parser/     # Syntax analysis
│   │   ├── ast/            # Abstract syntax tree
│   │   ├── type/           # Type system
│   │   ├── analysis/       # Semantic analysis
│   │   ├── backend/        # Code generation
│   │   └── optimizer/      # Optimizations
│   ├── runtime/            # Runtime library
│   │   ├── memory/         # Memory management
│   │   ├── concurrency/    # Parallelism support
│   │   ├── messaging/      # Channel implementation
│   │   ├── safety/         # Safety features
│   │   └── io/             # I/O operations
│   ├── common/             # Shared utilities
│   └── tools/              # Development tools
│       ├── formatter/      # Code formatting
│       ├── linter/         # Static analysis
│       ├── analyzer/       # Deep code analysis
│       └── lsp/            # Language server protocol
├── include/                # Public headers
│   └── goo/                # Namespaced headers
│       ├── core/           # Core language interfaces
│       └── runtime/        # Runtime interfaces
├── tests/                  # Test suite
│   ├── unit/               # Unit tests
│   ├── integration/        # Integration tests
│   ├── system/             # End-to-end tests
│   └── benchmark/          # Performance tests
├── docs/                   # Documentation
│   ├── api/                # API reference
│   ├── design/             # Design documents
│   ├── examples/           # Documented examples
│   └── internals/          # Implementation details
└── examples/               # Example programs
    ├── beginner/           # Simple examples
    ├── intermediate/       # More complex examples
    └── advanced/           # Advanced examples
```

## Component Descriptions

### Compiler Components

The compiler is organized as a pipeline with the following major components:

- **Frontend**: Responsible for processing source code
  - **Lexer**: Converts source code into tokens
  - **Parser**: Converts tokens into an abstract syntax tree
- **AST**: Data structures representing the structure of the program
- **Type**: Type checking and inference systems
- **Analysis**: Performs semantic analysis on the AST
- **Backend**: Generates code for target platforms
- **Optimizer**: Applies transformations to improve code performance

### Runtime Components

The runtime provides essential services that support Goo programs during execution:

- **Memory**: Memory allocation, deallocation, and garbage collection
- **Concurrency**: Thread management and synchronization primitives
- **Messaging**: Channel-based communication
- **Safety**: Runtime safety checks
- **I/O**: Input/output operations

### Development Tools

Tools to enhance productivity and code quality:

- **Formatter**: Automatic code formatting
- **Linter**: Static analysis to identify issues
- **Analyzer**: Deep code analysis for insights
- **LSP**: Language server protocol implementation

## File Organization Conventions

### Source Files

- **Implementation Files**: Stored in their respective component directories
  - C files: `lowercase_with_underscores.c`
  - Zig files: `camelCase.zig`
- **Header Files**: 
  - Public API: `include/goo/component_name.h`
  - Internal API: `src/component/include/component_internal.h`
- **Test Files**: Named as `component_test.c` or `component_test.zig`

### Documentation Files

- Component-level README.md files in each directory
- API documentation in header files
- Design documents in `docs/design/`
- Usage examples in `docs/examples/`

## Coding Standards

The Goo project adheres to the coding standards documented in [CODING_STYLE.md](CODING_STYLE.md), which covers:

- Naming conventions
- Formatting rules
- Documentation requirements
- Error handling patterns
- Memory management practices
- Testing guidelines

## Compilation and Build

The build system is designed to work with the organized structure:

- Each component can be built independently
- Dependencies between components are explicit
- Tests are automatically discovered and run

## Contributing to the Codebase

When contributing to the Goo project:

1. Place new code in the appropriate component directory
2. Follow the established naming conventions
3. Update relevant documentation
4. Add tests for new functionality
5. Ensure your code builds and passes all tests

## Migration and Legacy Code

As the project transitions to this organization:

1. New code should follow the new structure
2. Legacy code will be gradually migrated
3. The `cleanup_after_reorganization.sh` script can help remove obsolete directories

## Maintenance

To maintain the code organization:

1. Regular reviews of component boundaries
2. Refactoring when responsibilities shift
3. Updating documentation as the structure evolves
4. Automated checks to enforce organizational rules

## References

- [CODING_STYLE.md](CODING_STYLE.md) - Detailed coding standards
- [REORGANIZATION_COMPLETE.md](REORGANIZATION_COMPLETE.md) - Summary of the reorganization process
- [README.md](README.md) - Project overview 