#!/bin/bash
set -e  # Exit on error

echo "=== Building Parser ==="
zig build

echo "=== Running Parser Tests ==="
zig build test

echo "=== Tests completed ===" 