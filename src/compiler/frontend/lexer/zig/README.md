# Goo Lexer in Zig

This directory contains a Zig implementation of the lexer for the Goo programming language. The lexer is responsible for tokenizing source code into a stream of tokens that can be processed by the parser.

## Components

The lexer has been modularized into several files:

- `lexer.zig`: Main module that imports and re-exports functionality from other modules
- `token.zig`: Defines the token types and token structure
- `lexer_core.zig`: Core implementation of the lexer
- `token_reader.zig`: Functions for reading different token types
- `character_utils.zig`: Utility functions for character operations
- `keywords.zig`: Keyword lookup functionality
- `errors.zig`: Error handling system for reporting lexical errors
- `lexer_bindings.zig`: C API bindings for using the lexer from C code
- `lexer_c_api.zig`: C API implementation
- `test_lexer.zig`: Test program to demonstrate the lexer functionality
- `test_error_handling.zig`: Test program to demonstrate the error handling system
- `test_edge_cases.zig`: Test program for lexer edge cases and boundary conditions
- `ERROR_HANDLING.md`: Documentation for the error handling system
- `build.zig`: Build script for the lexer library and test program

## Building

To build the Zig lexer and generate C headers:

```bash
cd src/compiler/frontend/lexer/zig
zig build  # Builds the library and generates headers
```

This will:
1. Build the static and shared libraries
2. Install the C header in `include/goo_lexer.h`
3. Build the test executable

## Testing

Run the lexer tests:

```bash
cd src/compiler/frontend/lexer/zig
zig build test-lexer       # Run regular lexer tests
zig build test-lexer-errors # Run error handling tests
zig build test-lexer-edge-cases # Run edge case tests
zig build test-lexer-all    # Run all lexer tests
```

## Error Handling

The lexer includes a comprehensive error handling system that provides detailed information about lexical errors. The system:

1. Detects and reports syntax errors during lexing
2. Provides detailed error messages with line and column information
3. Allows the lexer to continue after encountering errors to find multiple errors in a single pass

For more details, see the [ERROR_HANDLING.md](ERROR_HANDLING.md) documentation.

Example of using error handling:

```zig
const std = @import("std");
const lexer = @import("lexer.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    const source = "var x := 42 + @";  // Invalid character '@'
    var l = try lexer.Lexer.init(allocator, source);
    defer l.deinit();
    
    // Process all tokens
    while (true) {
        const token = try l.nextToken();
        std.debug.print("{s}: line {d}, col {d}\n", .{
            token.getTypeName(), 
            token.line, 
            token.column
        });
        
        // Don't forget to free string values
        token.deinit(allocator);
        
        if (token.type == .EOF) {
            break;
        }
    }
    
    // Check for errors
    if (l.hasErrors()) {
        const formatted_errors = try l.getFormattedErrors();
        defer {
            for (formatted_errors) |msg| {
                allocator.free(msg);
            }
            allocator.free(formatted_errors);
        }
        
        std.debug.print("Errors found during lexing:\n", .{});
        for (formatted_errors) |msg| {
            std.debug.print("  {s}\n", .{msg});
        }
    }
}
```

## Edge Case Testing

The lexer includes comprehensive edge case tests to ensure it handles difficult parsing scenarios correctly. These tests check:

1. Empty input and whitespace-only input
2. Integer boundary cases (maximum values, overflow)
3. Floating point precision edge cases
4. String escape sequences and Unicode handling
5. Invalid escape sequences and error reporting
6. Very long tokens and deeply nested expressions
7. Nested comment handling

Based on the edge case testing, we have identified several areas for improvement, which are documented in [LEXER_IMPROVEMENTS.md](../../LEXER_IMPROVEMENTS.md).

## Using the Lexer in Zig

To use the lexer from Zig code:

```zig
const std = @import("std");
const lexer = @import("lexer.zig");

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    
    const source = "var x := 42";
    var l = try lexer.Lexer.init(allocator, source);
    defer l.deinit();
    
    while (true) {
        const token = try l.nextToken();
        std.debug.print("{s}: line {d}, col {d}\n", .{
            token.getTypeName(), 
            token.line, 
            token.column
        });
        
        // Don't forget to free string values
        token.deinit(allocator);
        
        if (token.type == .EOF) {
            break;
        }
    }
}
```

## Using the Lexer in C

To use the lexer from C code:

```c
#include <stdio.h>
#include "goo_lexer.h"

int main() {
    const char* source = "var x = 42";
    GooLexer* lexer = goo_lexer_create(source);
    
    while (1) {
        GooToken token = goo_lexer_next_token(lexer);
        printf("Token: %s\n", goo_token_type_name(token.type));
        
        if (token.type == GOO_TOKEN_EOF) {
            break;
        }
    }
    
    goo_lexer_destroy(lexer);
    return 0;
}
```

## Benefits of the Modular Zig Implementation

1. **Memory Safety**: Zig's memory management features help prevent leaks and use-after-free errors
2. **Performance**: Zig compiles to efficient native code with minimal runtime overhead
3. **Maintainability**: The modular design makes it easy to update and extend
4. **Integration**: Easy to integrate with both Zig and C code
5. **Extensibility**: New token types and language features can be added easily 