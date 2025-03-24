#!/bin/bash
# full_integration_test.sh - Automated testing script for validating full compiler integration
# 
# This script runs the Goo compiler with both lexer implementations and compares outputs
# to ensure the Zig lexer behaves identically to the Flex lexer when integrated with
# the full compiler pipeline.

set -e

# Default values
COMPILER_PATH="../../bin/goo"
TEST_DIR="../../test/parser"
OUTPUT_DIR="./integration_results"
VERBOSE=0
SUMMARY_ONLY=0
FAIL_FAST=0
MAX_TESTS=0  # 0 means all tests
TIMEOUT=30   # seconds

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to display usage information
usage() {
    echo -e "${BLUE}Usage:${NC} $0 [options]"
    echo ""
    echo "Options:"
    echo "  -c, --compiler PATH    Path to the Goo compiler executable (default: $COMPILER_PATH)"
    echo "  -d, --test-dir DIR     Directory containing test files (default: $TEST_DIR)"
    echo "  -o, --output-dir DIR   Directory to store test results (default: $OUTPUT_DIR)"
    echo "  -v, --verbose          Enable verbose output"
    echo "  -s, --summary          Show only summary results"
    echo "  -f, --fail-fast        Stop after first failure"
    echo "  -m, --max-tests N      Maximum number of tests to run (default: all)"
    echo "  -t, --timeout N        Timeout for each test in seconds (default: $TIMEOUT)"
    echo "  -h, --help             Display this help message"
    echo ""
    echo "Example:"
    echo "  $0 --compiler ../../bin/goo --test-dir ../../test/parser --verbose"
    exit 1
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--compiler)
            COMPILER_PATH="$2"
            shift 2
            ;;
        -d|--test-dir)
            TEST_DIR="$2"
            shift 2
            ;;
        -o|--output-dir)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -s|--summary)
            SUMMARY_ONLY=1
            shift
            ;;
        -f|--fail-fast)
            FAIL_FAST=1
            shift
            ;;
        -m|--max-tests)
            MAX_TESTS="$2"
            shift 2
            ;;
        -t|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Error:${NC} Unknown option: $1"
            usage
            ;;
    esac
done

# Verify compiler exists
if [ ! -f "$COMPILER_PATH" ]; then
    echo -e "${RED}Error:${NC} Compiler not found at $COMPILER_PATH"
    echo "Use --compiler option to specify the correct path"
    exit 1
fi

# Verify test directory exists
if [ ! -d "$TEST_DIR" ]; then
    echo -e "${RED}Error:${NC} Test directory not found: $TEST_DIR"
    echo "Use --test-dir option to specify the correct directory"
    exit 1
fi

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/flex"
mkdir -p "$OUTPUT_DIR/zig"
mkdir -p "$OUTPUT_DIR/diffs"

# Get test files
TEST_FILES=($(find "$TEST_DIR" -name "*.goo" | sort))
TOTAL_FILES=${#TEST_FILES[@]}

if [ $TOTAL_FILES -eq 0 ]; then
    echo -e "${RED}Error:${NC} No test files found in $TEST_DIR"
    exit 1
fi

# Limit number of tests if specified
if [ $MAX_TESTS -gt 0 ] && [ $MAX_TESTS -lt $TOTAL_FILES ]; then
    TEST_FILES=("${TEST_FILES[@]:0:$MAX_TESTS}")
    echo -e "${YELLOW}Note:${NC} Limited to $MAX_TESTS test files (out of $TOTAL_FILES)"
fi

echo -e "${BLUE}======== Goo Compiler Integration Test ========${NC}"
echo "Compiler path: $COMPILER_PATH"
echo "Test directory: $TEST_DIR"
echo "Output directory: $OUTPUT_DIR"
echo "Number of test files: ${#TEST_FILES[@]}"
echo -e "${BLUE}==============================================${NC}"

# Function to run single test with timeout
run_test() {
    local test_file=$1
    local lexer_type=$2
    local output_file=$3
    local errors_file="${output_file%.*}.err"
    
    # Set environment variable to choose lexer
    if [ "$lexer_type" == "zig" ]; then
        export USE_ZIG_LEXER=1
    else
        unset USE_ZIG_LEXER
    fi
    
    # Run compiler with timeout
    timeout $TIMEOUT "$COMPILER_PATH" --ast-dump "$test_file" > "$output_file" 2> "$errors_file" || true
}

# Initialize counters
PASS_COUNT=0
FAIL_COUNT=0
ERROR_COUNT=0

# Create summary file
SUMMARY_FILE="$OUTPUT_DIR/summary.txt"
echo "Integration Test Summary" > "$SUMMARY_FILE"
echo "Date: $(date)" >> "$SUMMARY_FILE"
echo "Compiler: $COMPILER_PATH" >> "$SUMMARY_FILE"
echo "Test directory: $TEST_DIR" >> "$SUMMARY_FILE"
echo "----------------------------------------" >> "$SUMMARY_FILE"

# Process test files
for test_file in "${TEST_FILES[@]}"; do
    # Get base filename
    base_name=$(basename "$test_file")
    
    # Skip if summary only mode is off
    if [ $SUMMARY_ONLY -eq 0 ]; then
        echo -e "${YELLOW}Testing:${NC} $base_name"
    fi
    
    # Define output files
    flex_output="$OUTPUT_DIR/flex/$base_name.out"
    zig_output="$OUTPUT_DIR/zig/$base_name.out"
    diff_file="$OUTPUT_DIR/diffs/$base_name.diff"
    
    # Run tests with both lexers
    run_test "$test_file" "flex" "$flex_output"
    run_test "$test_file" "zig" "$zig_output"
    
    # Compare outputs
    if diff -u "$flex_output" "$zig_output" > "$diff_file"; then
        if [ $SUMMARY_ONLY -eq 0 ]; then
            echo -e "  ${GREEN}✓ PASS${NC}"
        fi
        PASS_COUNT=$((PASS_COUNT + 1))
        echo "✓ PASS: $base_name" >> "$SUMMARY_FILE"
    else
        DIFF_SIZE=$(wc -l < "$diff_file")
        if [ $SUMMARY_ONLY -eq 0 ]; then
            echo -e "  ${RED}✗ FAIL${NC} - $DIFF_SIZE lines different"
            
            if [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Differences:${NC}"
                cat "$diff_file"
                echo ""
            fi
        fi
        
        FAIL_COUNT=$((FAIL_COUNT + 1))
        echo "✗ FAIL: $base_name - $DIFF_SIZE differences" >> "$SUMMARY_FILE"
        
        # Exit if fail fast mode is on
        if [ $FAIL_FAST -eq 1 ]; then
            echo -e "${RED}Stopping after first failure (--fail-fast is enabled)${NC}"
            break
        fi
    fi
    
    # Check for errors in error output
    flex_error="$OUTPUT_DIR/flex/$base_name.err"
    zig_error="$OUTPUT_DIR/zig/$base_name.err"
    
    if [ -s "$flex_error" ] || [ -s "$zig_error" ]; then
        if ! diff -q "$flex_error" "$zig_error" > /dev/null; then
            if [ $SUMMARY_ONLY -eq 0 ] && [ $VERBOSE -eq 1 ]; then
                echo -e "${YELLOW}Error output differences:${NC}"
                echo -e "${BLUE}Flex errors:${NC}"
                cat "$flex_error"
                echo -e "${BLUE}Zig errors:${NC}"
                cat "$zig_error"
                echo ""
            fi
            
            ERROR_COUNT=$((ERROR_COUNT + 1))
            echo "! ERROR DIFF: $base_name - different error outputs" >> "$SUMMARY_FILE"
        fi
    fi
done

# Display summary
echo -e "${BLUE}============== Test Summary ==============${NC}"
echo "Total tests: ${#TEST_FILES[@]}"
echo -e "Passed: ${GREEN}$PASS_COUNT${NC}"
echo -e "Failed: ${RED}$FAIL_COUNT${NC}"
echo -e "Error differences: ${YELLOW}$ERROR_COUNT${NC}"
echo -e "${BLUE}==========================================${NC}"

# Add summary to file
echo "----------------------------------------" >> "$SUMMARY_FILE"
echo "Total tests: ${#TEST_FILES[@]}" >> "$SUMMARY_FILE"
echo "Passed: $PASS_COUNT" >> "$SUMMARY_FILE"
echo "Failed: $FAIL_COUNT" >> "$SUMMARY_FILE"
echo "Error differences: $ERROR_COUNT" >> "$SUMMARY_FILE"

echo -e "${BLUE}Detailed results saved to:${NC} $OUTPUT_DIR"
echo "Summary file: $SUMMARY_FILE"

# Exit with appropriate status code
if [ $FAIL_COUNT -eq 0 ] && [ $ERROR_COUNT -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed. See summary for details.${NC}"
    exit 1
fi 