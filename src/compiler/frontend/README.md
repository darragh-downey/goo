# Goo Compiler Frontend

This directory contains the implementation of the Goo programming language compiler frontend, which is responsible for lexical analysis, parsing, and abstract syntax tree (AST) generation.

## Architecture Overview

The Goo compiler frontend follows a hybrid approach, combining the performance and safety advantages of Zig with the existing C codebase:

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│ Source Code     │     │ Token Stream    │     │ Abstract Syntax │
│ (Goo Language)  │────▶│ (Lexer Output)  │────▶│ Tree (AST)      │
└─────────────────┘     └─────────────────┘     └─────────────────┘
         │                      │                       │
         ▼                      ▼                       ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│ Lexer           │     │ Parser          │     │ Semantic        │
│ (Zig)           │     │ (Zig)           │     │ Analysis (C/Zig)│
└─────────────────┘     └─────────────────┘     └─────────────────┘
         │                      │                       │
         └──────────────────────┼───────────────────────┘
                                ▼
                      ┌─────────────────┐
                      │ C API Layer     │
                      │ (Integration)   │
                      └─────────────────┘
```

## Components

### 1. Lexer (Tokenizer)

Located in `lexer/zig/`, the lexer is implemented in Zig for better performance and safety.

Key files:
- `lexer.zig`: Main module entry point and API
- `lexer_core.zig`: Core lexical analysis implementation 
- `token.zig`: Token definitions
- `lexer_c_api.zig`: C API integration for the lexer

The lexer converts Goo source code into a stream of tokens (identifiers, keywords, literals, etc.) while tracking line and column information.

### 2. Parser

Located in `parser/zig/`, the parser is also implemented in Zig for better type safety and memory safety.

Key files:
- `parser.zig`: Main parser module and entry point
- `ast.zig`: Abstract Syntax Tree definitions
- `expressions.zig`: Parsing expression constructs
- `statements.zig`: Parsing statement constructs
- `declarations.zig`: Parsing declaration constructs
- `parser_c_api.zig`: C API integration for the parser

The parser takes the token stream from the lexer and builds an abstract syntax tree (AST) that represents the program structure.

### 3. C API Integration

The bridge between Zig and C components is managed through carefully designed C API layers:

- `lexer_c_api.zig` and `parser_c_api.zig`: Export Zig functionality to C
- `zig_integration.c`: C code for using the Zig lexer
- `ast_helpers.c`: C helper functions for working with ASTs

This layered approach allows for gradual migration from C to Zig while maintaining full compatibility with existing code.

## Build System

The compiler frontend uses Zig's build system, integrated into the main Goo build system:

```
build.zig
├── goo_runtime (runtime library)
├── goolexer (lexer library)
├── gooparser (parser library)
└── goocompiler (compiler integration)
```

## Testing

The compiler frontend includes extensive testing:

1. Zig-native tests: `test_lexer.zig` and `test_parser.zig`
2. C integration tests: `test_files/goo_test_compiler.c`
3. Targeted component tests in various directories

Run tests using the integrated build system:
```
zig build test-lexer   # Run lexer tests
zig build test-parser  # Run parser tests  
zig build test-compiler # Run full compiler frontend tests
```

## C API Usage Example

```c
// Initialize a lexer
GooLexerHandle lexer = goo_lexer_init(source_code, source_length);

// Tokenize
GooToken token;
while (goo_lexer_next_token(lexer, &token)) {
    // Process token
    if (token.token_type == GOO_TOKEN_EOF) break;
}

// Clean up
goo_lexer_destroy(lexer);

// Initialize a parser
GooParserHandle parser = goo_parser_init(source_code, source_length);

// Parse
GooASTHandle ast = goo_parser_parse(parser);

// Use AST
if (ast != NULL) {
    // Process AST
    goo_ast_destroy(ast);
}

// Clean up
goo_parser_destroy(parser);
```

## Future Improvements

1. **Incremental Parsing**: Support for parsing partial files for IDE integration
2. **Error Recovery**: Better error recovery during parsing for improved diagnostics
3. **Parallel Lexing**: Lexical analysis of large files in parallel
4. **Full Migration**: Complete migration of semantic analysis to Zig

## Directory Layout

- `lexer/`: Lexical analysis components
- `parser/`: Syntax analysis and AST generation
- `include/`: Public C headers for compiler frontend
- `build_all.sh`: Script to build all frontend components
