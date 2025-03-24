#!/bin/bash
# test_lsp.sh - Demo script for testing the Goo LSP server and client

# Ensure we're in the examples directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Paths
LSP_SERVER="../../../bin/goo-lsp"
LSP_CLIENT="../../../bin/goo-lsp-client"
EXAMPLE_FILE="hello.goo"
FILE_URI="file://$SCRIPT_DIR/$EXAMPLE_FILE"

# Ensure the server and client exist
if [ ! -x "$LSP_SERVER" ]; then
    echo "Error: LSP server not found or not executable at $LSP_SERVER"
    echo "Please run 'make' in the src/tools/lsp directory first."
    exit 1
fi

if [ ! -x "$LSP_CLIENT" ]; then
    echo "Error: LSP client not found or not executable at $LSP_CLIENT"
    echo "Please run 'make' in the src/tools/lsp directory first."
    exit 1
fi

# Ensure the example file exists
if [ ! -f "$EXAMPLE_FILE" ]; then
    echo "Error: Example file not found at $EXAMPLE_FILE"
    exit 1
fi

# Generate a script file for the client
cat > test_commands.txt << EOF
# Initialize the server with the workspace root
initialize

# Send initialized notification
initialized

# Open the example file
open $FILE_URI $EXAMPLE_FILE

# Request document symbols
request 2 textDocument/documentSymbol {"textDocument":{"uri":"$FILE_URI"}}

# Request hover information for "println" at line 9, character 5
request 3 textDocument/hover {"textDocument":{"uri":"$FILE_URI"},"position":{"line":8,"character":5}}

# Request completion at a position
request 4 textDocument/completion {"textDocument":{"uri":"$FILE_URI"},"position":{"line":10,"character":8}}

# Shut down the server properly
request 999 shutdown
exit
EOF

# Run the client with the test commands
echo "Starting LSP client with test commands..."
echo "============================================"
cat test_commands.txt | grep -v "^#" | $LSP_CLIENT -v -r "file://$SCRIPT_DIR"

# Clean up
rm -f test_commands.txt

echo "============================================"
echo "LSP test completed." 