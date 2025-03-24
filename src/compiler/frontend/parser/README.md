# Goo Parser Module

The parser module is responsible for parsing Goo source code into an Abstract Syntax Tree (AST) that can be used by the rest of the compiler. It is implemented in Zig and provides both Zig and C APIs.

## Features

- Full parsing of Goo syntax including:
  - Package and import declarations
  - Function declarations with parameters and return types
  - Variable and constant declarations
  - Control flow statements (if, else, for)
  - Return statements
  - Expression statements
  - Function calls
  - String, numeric, and boolean literals
  - Binary and unary operators

- Source position tracking for all AST nodes
- Detailed error reporting with line and column information
- C API for integration with other compiler components

## Module Structure

The parser is organized as a Zig module with the following structure:

- `parser.zig`: The main parser implementation
- `parser_c_api.zig`: C API bindings for the parser
- `test_parser.zig`: Tests for the parser
- `build.zig`: Build script for the parser
- `include/goo_parser.h`: C header for the parser API

The parser can be accessed in two ways:

1. From Zig: `const parser = @import("parser");`
2. From C: `#include "goo_parser.h"`

## AST Structure

All AST nodes have a common base structure with source position information:

```zig
pub const Node = struct {
    node_type: NodeType,
    start_pos: Position,
    end_pos: Position,
};

pub const Position = struct {
    line: usize,
    column: usize,
};
```

This enables accurate error reporting and source mapping for IDE integration.

## Building and Testing

To build the parser:

```bash
cd src/compiler/frontend/parser/zig
zig build
```

To run tests:

```bash
./run_tests.sh
```

## C API Usage

Example usage of the C API:

```c
#include "goo_parser.h"

// Initialize parser with source code
GooParserHandle parser = goo_parser_init("package example;\nfunc main() {}");

// Parse the program
GooParserErrorCode result = goo_parser_parse_program(parser);

if (result == GOO_PARSER_SUCCESS) {
    // Get the AST root
    GooAstNodeHandle root = goo_parser_get_ast_root(parser);
    
    // Process the AST...
    GooAstNodeType node_type = goo_ast_get_node_type(root);
} else {
    // Handle error
    const char* error_msg = goo_parser_get_error(parser);
    printf("Parser error: %s\n", error_msg);
}

// Clean up
goo_parser_destroy(parser);
goo_parser_cleanup();
```

## Recent Improvements

- Modular structure with proper Zig module system
- Source position tracking for all AST nodes
- Enhanced test suite with complex program parsing
- C API improvements for better error handling
- Updated build system
