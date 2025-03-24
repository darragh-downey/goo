# Goo Language Lexer

This directory contains the lexer implementation for the Goo programming language. The lexer is responsible for transforming source code into a stream of tokens for further processing by the parser.

## Overview

The Goo lexer has two implementations:

1. **Flex-based lexer**: A C implementation using Flex (the fast lexical analyzer)
2. **Zig-based lexer**: A Zig implementation (in development)

The code is structured to allow switching between these implementations at compile time.

## Key Files

- `token_definitions.h`: Common token type definitions shared across all implementations
- `lexer_selection.h/.c`: Interface for selecting between lexer implementations
- `lexer.l`: Flex lexer definition file
- `lexer.c`: Generated C code from the Flex definition
- `zig/`: Directory containing the Zig lexer implementation
- `test_lexer_selection.c`: Tests for the lexer selection interface
- `simple_lexer_test.c`: Simple program demonstrating lexer usage
- `test_flex_lexer.sh`: Script for testing the Flex lexer

## Building

Use the `Makefile.integration` to build the lexer:

```sh
# Build with Flex lexer
make -f Makefile.integration flex-only

# Build with Zig lexer (if available)
make -f Makefile.integration zig-only
```

## Testing

Several test scripts are available:

```sh
# Run the basic lexer tests
./test_flex_lexer.sh

# Run a simple example
./simple_lexer_test "let x = 10;"

# Run with multiple inputs
./simple_lexer_test "let x = 10; function test() { return x + 1; }"
```

## Token Consumption Model

The lexer uses a token consumption model to ensure that tokens are correctly processed:

1. When initialized, the lexer starts with `token_consumed = 1`
2. Each call to `lexer_next_token()` consumes a token and resets the state
3. Consumed tokens are not emitted again

## Token Types

The lexer recognizes various token types, defined in `token_definitions.h`:

- Basic tokens (identifiers, EOF)
- Literals (integer, float, string, boolean)
- Operators (+, -, *, etc.)
- Punctuation (;, :, (, ), etc.)
- Keywords (let, function, return, etc.)

## Token Structure

Each token contains:

- `type`: The token type (defined in `token_definitions.h`)
- `line` and `column`: Position in the source code
- `literal`: The text of the token in the source
- `has_value`: Indicates if the token has an associated value
- `value`: Union containing the value based on token type

## Integration

To integrate the lexer in your code:

```c
#include "token_definitions.h"
#include "lexer_selection.h"

// Initialize with a source string
GooLexer* lexer = lexer_init_string("let x = 10;");

// Process all tokens
GooToken token;
do {
    token = lexer_next_token(lexer);
    // Process token...
} while (token.type != 0);  // Continue until EOF

// Clean up
lexer_free(lexer);
```

## Contributing

When modifying the lexer:

1. Keep both implementations in sync
2. Update tests to verify changes
3. Ensure proper token consumption
4. Maintain consistent behavior across implementations 