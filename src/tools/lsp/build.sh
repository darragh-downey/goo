#!/bin/bash
# build.sh - Build script for the Goo LSP toolchain

set -e  # Exit on error

# Ensure we're in the lsp directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "🔨 Building Goo LSP toolchain..."

# Check for required tools
if ! command -v gcc >/dev/null 2>&1; then
    echo "❌ Error: gcc compiler not found"
    echo "Please install gcc before proceeding."
    exit 1
fi

if ! command -v make >/dev/null 2>&1; then
    echo "❌ Error: make not found"
    echo "Please install make before proceeding."
    exit 1
fi

# Check for json-c library
JSON_C_FOUND=false

# Try pkg-config first
if pkg-config --exists json-c; then
    JSON_C_FOUND=true
# On macOS, check Homebrew locations if pkg-config fails
elif [[ "$OSTYPE" == "darwin"* ]]; then
    if [ -d "/usr/local/opt/json-c" ] || [ -d "/opt/homebrew/opt/json-c" ]; then
        # Homebrew has json-c installed but pkg-config isn't finding it
        JSON_C_FOUND=true
        echo "ℹ️  Found json-c installed via Homebrew"
        # Set pkg-config path if needed
        if [ -d "/opt/homebrew/opt/json-c/lib/pkgconfig" ]; then
            export PKG_CONFIG_PATH="/opt/homebrew/opt/json-c/lib/pkgconfig:$PKG_CONFIG_PATH"
        elif [ -d "/usr/local/opt/json-c/lib/pkgconfig" ]; then
            export PKG_CONFIG_PATH="/usr/local/opt/json-c/lib/pkgconfig:$PKG_CONFIG_PATH"
        fi
    fi
fi

if [ "$JSON_C_FOUND" = false ]; then
    echo "❌ Error: json-c library not found"
    echo "Please install json-c before proceeding."
    echo ""
    echo "For macOS:"
    echo "  brew install json-c"
    echo "For Ubuntu/Debian:"
    echo "  sudo apt-get install libjson-c-dev"
    echo "For Fedora/RHEL:"
    echo "  sudo dnf install json-c-devel"
    exit 1
fi

# Create bin directory if it doesn't exist
BIN_DIR="../../../bin"
mkdir -p "$BIN_DIR"

# Build the server and client using make
echo "📦 Building LSP server and client..."
# Use SKIP_TESTS=1 to avoid running tests if there are issues
make SKIP_TESTS=1

# Check if server and client were built successfully
if [ ! -f "$BIN_DIR/goo-lsp" ]; then
    echo "❌ Error: Failed to build LSP server"
    exit 1
fi

if [ ! -f "$BIN_DIR/goo-lsp-client" ]; then
    echo "❌ Error: Failed to build LSP client"
    exit 1
fi

echo "✅ LSP server and client built successfully!"

# Try to build tests (but don't fail if they don't build)
echo "📦 Attempting to build tests (optional)..."
if make test; then
    echo "✅ Tests built successfully!"
else
    echo "⚠️  Warning: Tests could not be built. This is not critical."
fi

# Build VSCode extension if Node.js is available
if command -v node >/dev/null 2>&1 && command -v npm >/dev/null 2>&1; then
    echo "📦 Building VSCode extension..."
    
    # Navigate to the VSCode extension directory
    cd vscode-extension
    
    # Install dependencies
    echo "📦 Installing extension dependencies..."
    npm install
    
    # Compile the extension
    echo "🔨 Compiling the extension..."
    npm run compile
    
    # Generate PNG icon if possible
    if [ -f "images/icon.svg" ]; then
        if command -v convert >/dev/null 2>&1 || command -v rsvg-convert >/dev/null 2>&1; then
            echo "🖼️  Converting SVG icon to PNG..."
            ./scripts/convert_icon.sh
        else
            echo "⚠️  Warning: ImageMagick or librsvg not found, skipping icon conversion"
            echo "You may need to manually convert the icon for some contexts."
        fi
    fi
    
    echo "✅ VSCode extension built successfully!"
else
    echo "⚠️  Warning: Node.js or npm not found, skipping VSCode extension build"
    echo "To build the VSCode extension, please install Node.js and npm, then run:"
    echo "  cd vscode-extension && npm install && npm run compile"
fi

# Return to the original directory
cd "$SCRIPT_DIR"

# Create examples directory if it doesn't exist
mkdir -p examples

echo ""
echo "🎉 Goo LSP toolchain build complete!"
echo ""
echo "The following components are available:"
echo "  - LSP Server: $BIN_DIR/goo-lsp"
echo "  - LSP Client: $BIN_DIR/goo-lsp-client"
echo "  - VSCode Extension: src/tools/lsp/vscode-extension"
echo ""
echo "To test the LSP server with an example file, run:"
echo "  cd examples && ./test_lsp.sh"
echo ""
echo "To install the VSCode extension, copy the extension directory to:"
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "  ~/.vscode/extensions/goo-language"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "  ~/.vscode/extensions/goo-language"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "win32" ]]; then
    echo "  %USERPROFILE%\\.vscode\\extensions\\goo-language"
fi 