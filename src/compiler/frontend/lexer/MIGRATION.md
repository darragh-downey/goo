# Lexer Migration Plan: Flex to Zig

This document outlines the plan for migrating the Goo compiler lexer from the current Flex-based implementation to a new implementation in Zig.

## Current Status

The current lexer implementation:
- Uses Flex to generate C code from `src/lexer/lexer.l`
- Outputs generated code to `src/generated/lexer.c`
- Interfaces with the Bison parser via `yylex()` function
- Uses global state for line/column tracking and token values

## Migration Goals

The migration aims to:
1. Replace the Flex-based lexer with a pure Zig implementation
2. Maintain full compatibility with the existing Bison parser
3. Improve error handling and reporting
4. Eliminate the need for Flex as a build dependency
5. Lay groundwork for future migration of the parser to Zig

## Migration Steps

### Phase 1: Zig Implementation (Completed)

- [x] Create token definitions in Zig (`src/lexer/zig/token.zig`)
- [x] Implement the core lexer in Zig (`src/lexer/zig/lexer.zig`)
- [x] Create C bindings for the Zig lexer (`src/lexer/zig/lexer_bindings.zig`)
- [x] Develop tests for the Zig lexer (`src/lexer/zig/lexer_test.zig`)
- [x] Update the build system to build the Zig lexer (`src/lexer/zig/build.zig`)

### Phase 2: Integration (Completed)

- [x] Generate C header file for the Zig lexer bindings (`include/goo_lexer.h`)
- [x] Create a compatibility layer to interface with the Bison parser (`src/lexer/zig_integration.c`)
- [x] Add test and comparison tools for the lexers
- [x] Create integration test program (`src/lexer/test_lexer_integration.c`)
- [x] Develop a build system for integration testing (`src/lexer/Makefile.integration`)
- [x] Create a script to automate integration testing (`src/lexer/run_integration_test.sh`)
- [x] Document the integration process (`src/lexer/INTEGRATION_GUIDE.md`)

### Phase 3: Transition (In Progress)

- [x] Run extensive tests comparing both lexers for identical output
- [x] Create a unified lexer selection interface (`lexer_selection.h` and `lexer_selection.c`)
- [x] Create a Makefile for compiler integration (`Makefile.compiler`)
- [x] Document Makefile modifications for the main compiler (`MAKEFILE_MODIFICATIONS.md`)
- [ ] Update error reporting to use Zig lexer's line/column tracking
- [ ] Switch the default lexer to the Zig implementation
- [ ] Remove Flex as a build dependency

### Phase 4: Cleanup (Upcoming)

- [ ] Remove the old Flex-based lexer code
- [ ] Update all documentation to reflect the new implementation
- [ ] Optimize the Zig lexer for performance
- [ ] Consider future parser migration to Zig

## Integration Strategy

The integration between the Zig lexer and existing C parser is now handled through a unified selection interface:

1. The `lexer_selection.h` and `lexer_selection.c` files provide a common interface for both lexers.
2. The `USE_ZIG_LEXER` preprocessor directive controls which lexer implementation is used.
3. The `zig_integration.c` file is modified to use this interface.
4. The `Makefile.compiler` provides build rules for including in the main compiler.

## Lexer Selection Interface

The lexer selection interface provides the following functions:

```c
// Initialize the lexer with a file
void lexer_set_file(FILE* file);

// Initialize the lexer with a string
void lexer_set_string(const char* source);

// Clean up lexer resources
void lexer_cleanup(void);

// Set debug mode
void lexer_set_debug(int enable);

// Get current line number
int lexer_get_line(void);

// Get current column number
int lexer_get_column(void);

// Report an error at the current position
void lexer_error(const char* msg);

// Report an error at a specific position
void lexer_error_at(int line, int column, const char* msg);

// Set a callback for error handling
void lexer_set_error_callback(void (*callback)(const char*, int, int));
```

## Token Mapping

The token types defined in the Zig implementation (`TokenType` enum in `token.zig`) map directly to the integer values expected by the parser. This mapping is handled by defining the Zig enum values to match those expected by the Bison parser.

## Testing Strategy

Testing is performed in multiple stages:
1. **Unit tests**: Test the Zig lexer in isolation with a variety of inputs
2. **Comparative tests**: Compare token streams from both lexers for identical inputs
3. **Integration tests**: Test the integrated lexer with the existing parser
4. **Regression tests**: Run the full compiler test suite with the new lexer

### Integration Test Tools

We've created comprehensive tools for integration testing:

1. **Integration Test Program** (`test_lexer_integration.c`): A standalone program that tests the Zig lexer with or without a parser, providing detailed output about tokens.

2. **Test Build System** (`Makefile.integration`): Manages building the Zig lexer and test programs.

3. **Automated Test Script** (`run_integration_test.sh`): Builds and runs all integration tests, comparing the Flex and Zig lexers.

4. **Sample Test File** (`test_sample.goo`): A comprehensive Goo file exercising various language features.

5. **Simple Parser** (`parser.y`): A simplified parser for testing the lexer integration.

6. **Unified Lexer Selection** (`lexer_selection.h` and `lexer_selection.c`): Provides a common interface for both lexers.

7. **Compiler Integration Makefile** (`Makefile.compiler`): Facilitates integration with the main compiler.

## Performance Considerations

The Zig lexer is expected to perform at least as well as the Flex-generated lexer, with the following advantages:
- No need to generate code at build time
- Better memory safety and error handling
- More maintainable codebase

## Timeline

- **Week 1**: Complete Phase 1 (Zig Implementation) - DONE
- **Week 2**: Complete Phase 2 (Integration) - DONE
- **Week 3-4**: Complete Phase 3 (Transition) - IN PROGRESS
- **Week 5**: Complete Phase 4 (Cleanup)

## Risks and Mitigation

| Risk | Mitigation |
|------|------------|
| Parser expects different token values | Carefully map token types between Zig and C |
| Performance regression | Benchmark both implementations and optimize if needed |
| Subtle behavior differences | Comprehensive testing with a variety of inputs |
| Build system integration issues | Unified interface with fallback to old lexer |

## Future Considerations

This migration is the first step in a larger plan to migrate parts of the Goo compiler to Zig. After the lexer migration is complete, we'll evaluate:

1. Migrating the parser from Bison to a hand-written Zig implementation
2. Rewriting the AST structures in Zig for better memory safety
3. Improving error reporting throughout the compiler

## References

- Zig lexer implementation: `src/lexer/zig/`
- C bindings: `src/lexer/zig/lexer_bindings.zig`
- Integration example: `src/lexer/zig_integration.c`
- Original Flex lexer: `src/lexer/lexer.l`
- Integration test program: `src/lexer/test_lexer_integration.c`
- Lexer selection interface: `src/lexer/lexer_selection.h` and `src/lexer/lexer_selection.c`
- Integration guide: `src/lexer/INTEGRATION_GUIDE.md`
- Makefile modifications: `src/lexer/MAKEFILE_MODIFICATIONS.md`
- Integration test script: `src/lexer/run_integration_test.sh` 