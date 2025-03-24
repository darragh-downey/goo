#!/bin/bash
set -e  # Exit on error

echo "=== Building Goo Compiler Frontend ==="

echo "Building Lexer..."
cd lexer/zig
zig build
cd ../..

echo "Building Parser..."
cd parser/zig
zig build
cd ../..

echo "Frontend build completed successfully!" 