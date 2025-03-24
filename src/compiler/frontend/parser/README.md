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

# Goo Parser Implementation

The Goo parser is designed to handle both the Goo language (with extensions) and standard Go code. This implementation uses a mode-aware parser that can automatically detect the language mode based on file extension or content markers.

## Architecture

The parser infrastructure consists of several components:

1. **Lexer** - Tokenizes the source code, supporting both Go and Goo syntax
2. **File Detector** - Detects whether code should be treated as Go or Goo
3. **Mode-Aware Parser** - Parses according to the detected mode
4. **AST** - Represents the parsed code, including Go and Goo constructs

## Language Mode Detection

The parser determines the language mode using these rules:

1. **File Extension**:
   - `.go` files are treated as Go code
   - `.goo` files are treated as Goo code

2. **Content Markers**:
   - `// goo:enable` or `/* goo:enable */` forces Goo mode
   - `// goo:mode=go` or `// goo:mode=goo` explicitly sets the mode
   - Go build constraints (`//go:build`) indicate Go mode

3. **Manual Override**:
   - The mode can be manually specified via the API

## Go vs Goo Mode

When parsing in Go mode, the parser enforces these constraints:

1. Standard Go syntax only (no Goo extensions)
2. Strict adherence to the Go language specification
3. Error messages tailored to Go compatibility

In Goo mode, the parser allows:

1. All valid Go syntax
2. Goo language extensions (enums, traits, pattern matching, etc.)
3. Additional operators and type system features

## Using the Mode-Aware Parser

```c
// Create a parser
GooParserHandle parser = gooModeAwareParserCreate();

// Parse with automatic mode detection
GooParserResultCode result = gooModeAwareParseFile(parser, "path/to/file.go");

// Check the detected mode
GooLangMode mode = gooParserGetDetectedMode(parser);
if (mode == GOO_LANG_MODE_GO) {
    printf("File was parsed in Go mode\n");
} else {
    printf("File was parsed in Goo mode\n");
}

// Force a specific mode
gooParserForceMode(parser, GOO_LANG_MODE_GOO);

// Set the default mode for ambiguous cases
gooParserSetDefaultMode(parser, GOO_LANG_MODE_GO);

// Clean up
gooParserDestroy(parser);
```

## Testing

The parser includes a test suite that validates:

1. Correct mode detection based on file extension and content
2. Parsing of valid Go code in Go mode
3. Parsing of valid Goo code in Goo mode
4. Rejection of Goo extensions in Go mode
5. Automatic language mode detection

To run the tests:

```bash
cd src/compiler/frontend/parser
make -f Makefile.mode_test run
```

## Integration with the Compiler

The mode-aware parser is integrated into the compiler pipeline to support both Go and Goo source files, enabling Goo to be a true superset of Go. This means:

1. Go code can be compiled without changes
2. Existing Go libraries can be used in Goo projects
3. Goo extensions can be added incrementally to Go codebases

## Future Work

1. Enhanced IDE integration via LSP for both Go and Goo
2. Expanded test suite for all Go language features
3. Documentation tools that understand both Go and Goo syntax
