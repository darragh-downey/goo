# Zig Lexer Integration Guide

This guide provides detailed instructions for integrating the new Zig-based lexer with the existing Goo compiler.

## Setup

1. Build the Zig lexer library and generate C header files:

```bash
cd src/lexer
make zig-lexer
```

This will compile the Zig lexer and place the header file in `src/include/goo_lexer.h`.

## Integration Steps

### 1. Include the Zig Lexer Header

In the main parser or lexer integration file, include the Zig lexer header:

```c
#include "goo_lexer.h"
```

### 2. Modify the Parser's Lexer Interface

The Bison parser expects a function called `yylex()` to provide tokens. Implement this function using the Zig lexer:

```c
// Global lexer instance
GooLexer *current_lexer = NULL;
GooToken current_token;

// Implement yylex for Bison integration
int yylex(void) {
    // Free the previous token's resources
    goo_token_free(current_token);
    
    // Get the next token
    if (current_lexer == NULL) {
        fprintf(stderr, "Lexer not initialized\n");
        return 0; // EOF
    }
    
    current_token = goo_lexer_next_token(current_lexer);
    
    // Update yylval and yylloc for Bison
    // yylval.ident = current_token.string_value; // For identifiers
    // yylval.intval = current_token.int_value;   // For integer literals
    // yylval.floatval = current_token.float_value; // For float literals
    // yylval.boolval = current_token.bool_value;   // For boolean literals
    
    // Update line and column information
    // yylloc.first_line = current_token.line;
    // yylloc.first_column = current_token.column;
    
    return current_token.token_type;
}
```

### 3. Initialize the Lexer

Update the compiler's initialization code to use the new lexer:

```c
// Instead of using Flex's yyin
// FILE* yyin = fopen(filename, "r");

// Use the Zig lexer
char* source = read_file_to_string(filename);
current_lexer = goo_lexer_new(source);
free(source); // Zig lexer makes its own copy

if (!current_lexer) {
    fprintf(stderr, "Failed to initialize lexer\n");
    return 1;
}
```

### 4. Clean Up Resources

When parsing is complete, clean up the lexer resources:

```c
// Clean up
goo_token_free(current_token);
goo_lexer_free(current_lexer);
```

### 5. Update the Build System

Update your build system to link against the Zig lexer:

In a Makefile:
```makefile
LIBS += -L$(SRCDIR)/lexer/zig/zig-out/lib -lgoo_lexer
INCLUDES += -I$(SRCDIR)/include
```

Or in CMake:
```cmake
target_link_libraries(${your_target} ${SRCDIR}/lexer/zig/zig-out/lib/libgoo_lexer.a)
include_directories(${SRCDIR}/include)
```

## Token Values

The Zig lexer provides token values through the GooToken struct:

```c
// Accessing token values based on token type
switch (current_token.token_type) {
    case INT_LITERAL:
        printf("Integer value: %ld\n", current_token.int_value);
        break;
    case FLOAT_LITERAL:
        printf("Float value: %f\n", current_token.float_value);
        break;
    case BOOL_LITERAL:
        printf("Boolean value: %s\n", current_token.bool_value ? "true" : "false");
        break;
    case STRING_LITERAL:
    case IDENTIFIER:
    case RANGE_LITERAL:
        if (current_token.string_value) {
            printf("String value: \"%s\"\n", current_token.string_value);
        }
        break;
}
```

## Error Handling

The Zig lexer provides improved error handling:

```c
// Check for lexer errors
if (current_token.token_type == ERROR) {
    fprintf(stderr, "Lexer error at line %d, column %d\n", 
            current_token.line, current_token.column);
    // Handle the error...
}
```

## Testing the Integration

After completing the integration steps, it's important to verify that the Zig lexer works correctly with the existing parser. The test program `test_lexer_integration.c` provides a simple way to test the integration.

### Building the Test Program

1. Navigate to the lexer directory:
   ```bash
   cd src/lexer
   ```

2. Build the test program using the provided Makefile:
   ```bash
   make -f Makefile.integration
   ```

3. This will create an executable `test_lexer_integration` that can be used to test the Zig lexer.

### Running the Test Program

The test program can be run in two modes:

1. **Tokenize-only mode**: This mode only tokenizes the input file and displays each token without parsing.
   ```bash
   ./test_lexer_integration test_sample.goo --tokenize-only
   ```

2. **Parse mode**: This mode attempts to parse the input using the provided parser implementation.
   ```bash
   ./test_lexer_integration test_sample.goo
   ```

Add the `--debug` flag to enable debug output:
```bash
./test_lexer_integration test_sample.goo --debug
```

### Interpreting the Results

The test program will output information about each token encountered, including:
- Token type
- Line and column numbers
- Token value (for literals and identifiers)

If running in parse mode, it will also attempt to parse the input and report success or failure.

### Creating a More Complete Integration Test

For a more complete test with the actual parser, you can:

1. Generate parser tables from the simplified parser.y:
   ```bash
   make -f Makefile.integration parser
   ```

2. Build the test with the parser:
   ```bash
   make -f Makefile.integration test_with_parser
   ```

3. Run the integrated test:
   ```bash
   ./test_with_parser test_sample.goo
   ```

### Expected Results

A successful integration should show:
- All tokens correctly identified with proper values
- No lexer errors
- If parsing, the parse should complete successfully (result code 0)

If you encounter errors, check:
- Token type values match between the Zig lexer and Bison parser
- Token values are correctly converted to the types expected by the parser
- Memory management is correct (no leaks or double-frees)

### Advanced Testing

For more thorough testing:

1. Try with different sample files that exercise all features of the language
2. Compare token streams between the Flex lexer and Zig lexer
3. Run with memory checkers like Valgrind to detect memory issues
4. Test error handling by providing invalid inputs

## Debugging

If you encounter issues with the integration:

1. Enable verbose logging in the Zig lexer by modifying `src/lexer/zig/lexer_bindings.zig`.
2. Compare token streams between the Flex and Zig lexers:
```bash
cd src/lexer
./migrate_lexer.sh
```
3. Check the results directory for token stream comparisons.

## Transition Strategy

For a smooth transition, consider:

1. Making the Zig lexer optional with a compile-time flag initially
2. Running both lexers in parallel and comparing outputs
3. Gradually phasing out the Flex lexer as confidence in the Zig lexer grows

## Common Issues

1. **Token Value Mismatches**: Ensure token type values in `src/lexer/zig/token.zig` match those expected by the parser.
2. **Memory Leaks**: Always call `goo_token_free()` after processing each token.
3. **Build Issues**: Ensure the compiled lexer library is properly linked.

## References

- Zig lexer source: `src/lexer/zig/`
- Zig lexer C API: `src/include/goo_lexer.h`
- Integration example: `src/lexer/zig_integration.c`
- Migration tools: `src/lexer/migrate_lexer.sh` 