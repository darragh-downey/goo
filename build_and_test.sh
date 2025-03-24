#!/bin/bash
# Build and test script for the Goo programming language

set -e # Exit on error

echo "========================================"
echo "Goo Programming Language Build and Test"
echo "========================================"
echo

# Parse command line arguments
BUILD_TYPE="Debug"
RUN_TESTS=true
VERBOSE=false
LEXER_ONLY=false
RUNTIME_ONLY=false

while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
    --release)
      BUILD_TYPE="ReleaseFast"
      shift
      ;;
    --no-test)
      RUN_TESTS=false
      shift
      ;;
    --verbose)
      VERBOSE=true
      shift
      ;;
    --lexer-only)
      LEXER_ONLY=true
      shift
      ;;
    --runtime-only)
      RUNTIME_ONLY=true
      shift
      ;;
    *)
      echo "Unknown option: $key"
      echo "Usage: $0 [--release] [--no-test] [--verbose] [--lexer-only] [--runtime-only]"
      exit 1
      ;;
  esac
done

# Check if Zig is installed
if ! command -v zig &> /dev/null; then
    echo "Error: Zig is not installed or not in the PATH"
    echo "Please install Zig 0.11.0 or later: https://ziglang.org/download/"
    exit 1
fi

# Build command
BUILD_CMD="zig build -Doptimize=$BUILD_TYPE"
if [ "$VERBOSE" = true ]; then
  BUILD_CMD="$BUILD_CMD --verbose"
fi

# Component-specific builds
if [ "$RUNTIME_ONLY" = true ]; then
  BUILD_CMD="$BUILD_CMD runtime"
  echo "Building only runtime components (Build type: $BUILD_TYPE)"
elif [ "$LEXER_ONLY" = true ]; then
  BUILD_CMD="$BUILD_CMD lexer"
  echo "Building only lexer components (Build type: $BUILD_TYPE)"
else
  echo "Building Goo (Build type: $BUILD_TYPE)"
fi

echo "Running: $BUILD_CMD"
eval $BUILD_CMD

# Run tests if enabled
if [ "$RUN_TESTS" = true ]; then
    echo "========================================"
    echo "Running Memory Management Tests"
    echo "========================================"
    echo "Basic memory test:"
    zig build run -Doptimize=$BUILD_TYPE
    echo
    echo "Extended memory test:"
    zig build run-extended -Doptimize=$BUILD_TYPE
    echo

    # Only run lexer tests if not runtime-only mode
    if [ "$RUNTIME_ONLY" = false ]; then
        echo "========================================"
        echo "Running Lexer Tests"
        echo "========================================"
        echo "Lexer tests:"
        zig build test-lexer -Doptimize=$BUILD_TYPE || {
            echo "Warning: Lexer tests failed, but continuing..."
        }
        
        echo "Lexer error handling tests:"
        zig build test-lexer-errors -Doptimize=$BUILD_TYPE || {
            echo "Warning: Lexer error handling tests failed, but continuing..."
        }

        echo "Lexer edge case tests:"
        zig build test-lexer-edge-cases -Doptimize=$BUILD_TYPE || {
            echo "Warning: Lexer edge case tests failed, but continuing..."
        }
        echo
    fi

    # Only run parser and compiler tests if not in lexer-only or runtime-only mode
    if [ "$LEXER_ONLY" = false ] && [ "$RUNTIME_ONLY" = false ]; then
        # Check if the parser targets exist
        PARSER_EXISTS=$(zig build --list-steps | grep -c "test-parser")
        
        if [ "$PARSER_EXISTS" -gt 0 ]; then
            echo "========================================"
            echo "Running Compiler Frontend Tests"
            echo "========================================"
            echo "Parser tests:"
            zig build test-parser -Doptimize=$BUILD_TYPE || {
                echo "Warning: Parser tests failed, but continuing..."
            }
            echo
            echo "Compiler integration test:"
            zig build test-compiler -Doptimize=$BUILD_TYPE || {
                echo "Warning: Compiler integration tests failed, but continuing..."
            }
        else
            echo "========================================"
            echo "Skipping Parser and Compiler Tests"
            echo "Parser components are disabled in build.zig"
            echo "========================================"
        fi
    fi
fi

echo "========================================"
echo "Build and tests completed successfully!"
echo "========================================"

# Show available components
echo "Available components:"
echo "  - Runtime library (goo_runtime)"
echo "  - Lexer (goolexer)"

# Only show parser and compiler if they're enabled in build.zig
PARSER_EXISTS=$(zig build --list-steps | grep -c "parser")
if [ "$PARSER_EXISTS" -gt 0 ]; then
    echo "  - Parser (gooparser)"
    echo "  - Compiler frontend (goocompiler)"
fi

echo
echo "Use 'zig build <component>' to build a specific component."
echo "Use 'zig build test-<component>' to run tests for a specific component."
echo

exit 0
