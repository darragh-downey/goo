#!/bin/bash

# Build script for the Goo compiler optimizer

set -e  # Exit on any error

# Function to display usage
usage() {
    echo "Usage: $0 [options]"
    echo "Options:"
    echo "  --clean     Clean build artifacts"
    echo "  --test      Run tests after building"
    echo "  --docs      Generate documentation"
    echo "  --install   Install to specified prefix (requires PREFIX env var)"
    echo "  --help      Display this help message"
    exit 1
}

# Set default options
CLEAN=0
TEST=0
DOCS=0
INSTALL=0

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)
            CLEAN=1
            shift
            ;;
        --test)
            TEST=1
            shift
            ;;
        --docs)
            DOCS=1
            shift
            ;;
        --install)
            INSTALL=1
            shift
            ;;
        --help)
            usage
            ;;
        *)
            echo "Unknown option: $1"
            usage
            ;;
    esac
done

# Determine script directory (where this script is located)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Clean if requested
if [[ $CLEAN -eq 1 ]]; then
    echo "Cleaning build artifacts..."
    make clean
    exit 0
fi

# Check if Zig is installed
if ! command -v zig &> /dev/null; then
    echo "Error: Zig is not installed or not in PATH."
    echo "Please install Zig from https://ziglang.org/download/"
    exit 1
fi

# Build the optimizer
echo "Building Goo optimizer..."
make build

# Run tests if requested
if [[ $TEST -eq 1 ]]; then
    echo "Running tests..."
    make test
fi

# Generate documentation if requested
if [[ $DOCS -eq 1 ]]; then
    echo "Generating documentation..."
    make docs
fi

# Install if requested
if [[ $INSTALL -eq 1 ]]; then
    if [[ -z "$PREFIX" ]]; then
        echo "Error: PREFIX environment variable not set for installation."
        echo "Example usage: PREFIX=/usr/local $0 --install"
        exit 1
    fi
    
    echo "Installing to $PREFIX..."
    DESTDIR="$PREFIX" make install
fi

echo "Build completed successfully!"
exit 0 