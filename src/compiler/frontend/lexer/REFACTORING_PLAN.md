# Lexer Implementation Refactoring Plan

## Current Issues Identified

1. **Token Consumption Issues**:
   - ✅ The `token_consumed` flag mechanism in `lexer_selection.c` doesn't work properly, leading to duplicate tokens being emitted
   - ✅ Token consumption needs to be properly managed across both lexer implementations (Zig and Flex)

2. **Macro Redefinition Warnings**:
   - ✅ There are multiple definitions of token type macros in different files:
     - `INT_LITERAL`, `FLOAT_LITERAL`, `BOOL_LITERAL`, `STRING_LITERAL`, `RANGE_LITERAL`
   - ✅ These macros are defined in both `test_lexer_selection.c` and other files

3. **Unused Variables Warnings**:
   - ✅ `flex_only` and `zig_only` in test code

4. **Test File Generation/Management**:
   - ✅ The test files are dynamically created but sometimes not found during testing
   - ✅ The test file content seems to differ between what's specified in Makefile and what's tested

5. **Inconsistent Lexer Behavior**:
   - ✅ The Flex and Zig lexers may produce different output for the same input
   - ✅ Test comparisons are failing

## Refactoring Actions

### 1. Fix Token Consumption

- ✅ **Update `lexer_next_token()` in `lexer_selection.c`**:
  ```c
  // Always consume tokens for consistent behavior
  token_consumed = 1;
  ```

- ✅ **Ensure consistent state management** in all initialization functions:
  ```c
  token_consumed = 1; // Always reset consumption state when initializing
  ```

### 2. Fix Macro Redefinitions

- ✅ **Create a common header file** for token type definitions:
  - New file: `src/lexer/token_definitions.h`
  - Move all shared token type definitions to this file
  - Update all other files to include this common header instead of defining their own

### 3. Fix Test File Management

- ✅ **Update the Makefile targets** to ensure test files are created in the correct location
- ✅ **Add explicit creation of test directories** before tests run
- ✅ **Standardize test file content** to match what's being tested

### 4. Consistent Token Value Handling

- ✅ **Update both lexer implementations** to handle token values consistently:
  - Ensure integer, float, boolean, and string values are properly converted and stored
  - Make sure token positions (line, column) are consistent between implementations

### 5. Code Organization

- ✅ **Refactor common lexer code** into shared utility functions
- ✅ **Standardize error handling** across both lexer implementations
- ✅ **Improve test isolation** to make test cases more independent and reliable

### 6. Documentation

- ✅ **Add detailed comments** describing the token consumption model
- ✅ **Document the integration points** between the lexer implementations
- ✅ **Update usage examples** in the README and TESTING.md files

## Implementation Steps

1. ✅ Create the common token_definitions.h file
2. ✅ Fix the token_consumed flag in lexer_selection.c
3. ✅ Update Makefile to ensure proper test file generation
4. ✅ Fix value handling in both lexer implementations
5. ✅ Fix test comparisons to properly detect differences
6. ✅ Add comprehensive test cases covering edge cases
7. ✅ Update documentation with new implementations

## Testing

After each change, we need to:

1. ✅ Rebuild the affected components
2. ✅ Run tests with various inputs
3. ✅ Verify token streams match between implementations
4. ✅ Ensure no duplicate tokens are emitted
5. ✅ Check that all token values are correctly preserved 