#!/bin/bash
# Run the semantic analyzer tests

# Exit on error
set -e

echo "Building semantic analyzer..."
zig build

echo "Running tests..."
zig build test

echo "Semantic analyzer tests completed successfully!" 