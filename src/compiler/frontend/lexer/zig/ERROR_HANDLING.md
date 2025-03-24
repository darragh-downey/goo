# Lexer Error Handling System

The Goo lexer includes a comprehensive error handling system that provides detailed information about lexical errors. The system is designed to:

1. Detect and report syntax errors during lexing
2. Provide meaningful error messages with location information
3. Continue lexing after encountering errors to find as many errors as possible
4. Collect all errors for reporting to the user

## Error Types

The error handling system defines various error types in `errors.zig`:

- `LexerErrorType` - An enum of all possible error categories:
  - `InvalidCharacter` - Unexpected character in the input
  - `InvalidNumber` - Malformed numeric literal (hex, octal, binary, decimal, float)
  - `InvalidIdentifier` - Malformed identifier
  - `UnterminatedString` - String literal without closing quote
  - `UnterminatedComment` - Block comment without closing delimiter
  - `UnknownToken` - Token that couldn't be recognized
  - `Other` - Miscellaneous errors

- `LexerError` - A Zig error set for lexer operations that can fail

## Error Reporting

The `ErrorReporter` struct is responsible for collecting and managing error information:

```zig
// Create a new error reporter
var error_reporter = try ErrorReporter.init(allocator);

// Report an error
try error_reporter.reportError(
    LexerErrorType.InvalidCharacter,
    line,
    column,
    "unexpected character: '{c}'",
    .{ch}
);

// Check if any errors occurred
if (error_reporter.hasErrors()) {
    // Get all errors
    const errors = error_reporter.getErrors();
    
    // Get formatted error messages
    const formatted_errors = try error_reporter.getFormattedErrors();
    defer {
        for (formatted_errors) |msg| {
            allocator.free(msg);
        }
        allocator.free(formatted_errors);
    }
    
    // Display errors
    for (formatted_errors) |msg| {
        std.debug.print("{s}\n", .{msg});
    }
}
```

## Integration with Lexer

The lexer automatically reports errors when they are encountered:

1. Invalid characters are reported and returned as ERROR tokens
2. Malformed numeric literals are detected and reported
3. Unterminated strings are handled gracefully
4. Nested comments are supported, and unterminated comments are reported

Example usage:

```zig
var lexer = try Lexer.init(allocator, source_code);
defer lexer.deinit();

// Process all tokens
while (true) {
    const tok = try lexer.nextToken();
    if (tok.type == .EOF) break;
    
    // Handle the token...
}

// Check for errors
if (lexer.hasErrors()) {
    const formatted_errors = try lexer.getFormattedErrors();
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
```

## Error Format

Formatted error messages include:
- Error type (e.g., "Invalid character")
- Line and column number
- Descriptive message

Example:
```
[Invalid character] at line 1, column 5: unexpected character: '@'
[Unterminated string] at line 3, column 10: String literal was not closed
```

## Error Collection and Recovery

The lexer is designed to continue processing input even after encountering errors. This allows it to collect multiple errors in a single pass, which is particularly useful for providing comprehensive feedback to users.

### Error Recovery Strategies

1. **Invalid Characters**: When an invalid character is encountered, it's skipped, and the lexer continues.
2. **Unterminated Strings**: If a string literal isn't properly closed, the lexer reports the error and continues from the end of the available string content.
3. **Invalid Numbers**: Malformed numeric literals are reported with specific error messages, and the lexer continues after the problematic token.
4. **Unterminated Comments**: Nested comments are supported, and unterminated block comments are detected and reported.

## Testing Error Handling

The `test_error_handling.zig` file provides comprehensive tests for the error handling system:

```bash
zig build test-lexer-errors
```

This will run tests for:
- Invalid characters
- Unterminated strings
- Invalid numeric literals
- Nested and unterminated comments
- Invalid escape sequences in strings

## Type Safety Improvements

The lexer also includes enhanced type safety for token values:

1. Strong typing for token values with the `TokenValue` union
2. Validation to ensure token values match their expected types
3. Type-specific accessors for token values:
   - `getString()` - Get string value
   - `getInteger()` - Get integer value
   - `getFloat()` - Get float value
   - `getBoolean()` - Get boolean value

Example:

```zig
// Create a token with a string value
const tok = Token.init(.IDENTIFIER, 1, 1, TokenValue.string("name"));

// Safely access the string value
if (tok.value.getString()) |str| {
    std.debug.print("Identifier: {s}\n", .{str});
}

// Create a token with validated value
const validated_tok = try Token.initValidated(.INT_LITERAL, 1, 1, TokenValue.integer(42));
```

These type safety improvements help catch errors early and provide more robust lexing. 