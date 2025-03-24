# Goo Lexer Testing Framework

This document describes the testing framework for the Goo lexer, which supports both the original Flex-based lexer and the new Zig-based lexer. The framework is designed to ensure compatibility between the two implementations.

## Framework Components

### 1. Unified Selection Interface

- **lexer_selection.h**: Header file defining the API for selecting between the Flex and Zig lexers.
- **lexer_selection.c**: Implementation of the selection interface.

### 2. Test Programs

- **test_lexer_selection.c**: Tests the lexer selection interface with both lexers, producing uniform output for direct comparison.
- **test_lexer_integration.c**: Tests the original integration approach.
- **compiler_integration_test.c**: Simulates how the compiler uses the lexer selection interface, allowing for more realistic testing.

### 3. Test Scripts

The following scripts automate testing and comparison between the lexers:

- **run_lexer_tests.sh**: Runs tests on multiple files and generates reports.
  - Options: 
    - `-v, --verbose`: Detailed output
    - `-t, --timing`: Include timing information
    - `--flex-only`: Test only with the Flex lexer
    - `--zig-only`: Test only with the Zig lexer
    - `--test-dir DIR`: Specify test directory

- **test_single_file.sh**: Provides detailed analysis of a single file, comparing output from both lexers.
  - Options:
    - `-v, --verbose`: Show detailed token information
    - `-t, --time`: Display timing information
    - `--debug`: Enable lexer debug output

- **compare_compiler_lexers.sh**: Processes all files in a directory, comparing how each lexer handles them.
  - Options:
    - `--verbose`: Show full output differences
    - `--summary`: Display only summary information
    - `--fail-fast`: Stop after first difference

- **full_integration_test.sh**: Tests the lexer within the full compiler pipeline.
  - Options:
    - `-c, --compiler PATH`: Path to the Goo compiler executable
    - `-d, --test-dir DIR`: Directory containing test files
    - `-o, --output-dir DIR`: Directory to store test results
    - `-v, --verbose`: Enable verbose output
    - `-s, --summary`: Show only summary results
    - `-f, --fail-fast`: Stop after first failure
    - `-m, --max-tests N`: Maximum number of tests to run
    - `-t, --timeout N`: Timeout for each test in seconds

### 4. Makefile Integration

The `Makefile.integration` provides commands for building and running the tests:

```bash
# Build all test programs
make -f Makefile.integration all

# Run lexer selection test
make -f Makefile.integration run-test-selection

# Run lexer integration test
make -f Makefile.integration run-test-integration

# Run compiler integration test
make -f Makefile.integration run-compiler-test

# Clean all test artifacts
make -f Makefile.integration clean
```

### 5. Sample Test Files

- Test files are automatically generated in the `test_files/` directory during testing.
- Additional test files can be added manually to test specific edge cases.

## Using the Testing Framework

### Basic Verification

To verify that both lexers produce identical tokens:

```bash
./run_lexer_tests.sh --test-dir ../../test/
```

### Detailed Comparison

For a detailed analysis of a specific file:

```bash
./test_single_file.sh path/to/file.goo --verbose
```

### Performance Testing

To compare performance:

```bash
./test_single_file.sh path/to/large_file.goo --time
```

### Full Compiler Integration Test

To test the lexer within the full compiler pipeline:

```bash
# Test with default settings
./full_integration_test.sh

# Test with a specific compiler and test directory
./full_integration_test.sh --compiler ../../bin/goo --test-dir ../../test/parser

# Run with verbose output to see all differences
./full_integration_test.sh --verbose

# Generate only a summary of results
./full_integration_test.sh --summary
```

### Full Test Suite Validation

To run the complete set of tests:

```bash
# 1. Build all test programs
make -f Makefile.integration all

# 2. Run the basic lexer tests
./run_lexer_tests.sh

# 3. Compare outputs across compiler test files
./compare_compiler_lexers.sh ../../test/

# 4. Test with the full compiler pipeline
./full_integration_test.sh
```

## Interpreting Test Results

### Success Criteria

- Both lexers produce identical token streams for all input files.
- Error messages and positions are identical for invalid inputs.
- The Zig lexer maintains or improves performance compared to the Flex lexer.

### Common Issues

If discrepancies are found:

1. **Token Type Differences**: Check token mappings in both lexers.
2. **Position Tracking**: Line/column tracking may differ slightly.
3. **Error Handling**: Error messages may have slight formatting differences.
4. **Memory Management**: Verify that both lexers properly handle resources.

## Adding Custom Test Files

You can add custom test files to test specific language features or edge cases:

1. Create a `.goo` file with the test case.
2. Add it to a test directory or specify it directly.
3. Run the appropriate test script to verify behavior.

Example:

```bash
# Create a test file
echo "let x = 123\nlet y = \"string\"" > test_files/custom_test.goo

# Test with both lexers
./test_single_file.sh test_files/custom_test.goo
```

## Test Output

The test scripts generate structured output for comparison:

- **Log files**: Detailed logs in `test_output/` directory.
- **Token dumps**: Token streams from each lexer in a comparable format.
- **Diff files**: Highlighting differences between lexer outputs.
- **Summary reports**: Overview of test results with pass/fail counts.
- **Integration results**: Output from full compiler pipeline tests in the `integration_results/` directory.

## Regression Testing Process

Follow this process to ensure thorough testing:

1. **Unit Testing**: Test individual lexer functions.
2. **Integration Testing**: Test the full lexing process.
3. **Compiler Simulation**: Test how the compiler uses the lexer.
4. **Batch Testing**: Run tests on multiple files at once.
5. **Full Compiler Testing**: Test within the full compiler pipeline.
6. **Compiler Test Suite**: Run against the compiler's test suite.

## Next Steps

After testing:

1. **Fix Discrepancies**: Address any differences between lexers.
2. **Run Full Test Suite**: Validate against the entire compiler test suite.
3. **Integrate With Compiler**: Update the main compiler to use the lexer selection interface.
4. **Remove Flex Dependency**: Once validation is complete.

## References

- **Code Files**: All test files are in the `src/lexer/` directory.
- **Documentation**: See `MAKEFILE_MODIFICATIONS.md` for integration with the main compiler.
- **Integration Plan**: Detailed in `NEXT_STEPS.md`. 