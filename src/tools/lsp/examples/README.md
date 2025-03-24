# Goo LSP Server Examples

This directory contains examples and test files for the Goo Language Server Protocol (LSP) implementation.

## Contents

- `hello.goo`: A simple Goo language source file that demonstrates various language features
- `test_lsp.sh`: A demonstration script that shows how to use the LSP client with the server

## Prerequisites

Before running the examples, make sure you've built the LSP server and client:

```bash
# From the lsp directory
cd ..
make
```

This will create the `goo-lsp` server and `goo-lsp-client` executables in the `bin` directory.

## Running the Examples

### Automated Test

To see a demonstration of the LSP server and client interaction, run:

```bash
./test_lsp.sh
```

This script will:
1. Start the LSP server
2. Initialize the server
3. Open the example Goo file
4. Request document symbols from the file
5. Request hover information for a position in the file
6. Request completion at a certain position
7. Properly shut down the server

### Manual Testing

You can also manually test the LSP server using the client:

```bash
../../../bin/goo-lsp-client -v
```

This will start an interactive session where you can enter commands to communicate with the server. The `-v` flag enables verbose mode for more detailed output.

Available commands in the client:

- `help`: Show help message
- `exit`: Exit the program
- `initialize`: Send initialize request
- `initialized`: Send initialized notification
- `open <uri> <file>`: Open a document
- `change <uri> <version> <text>`: Change a document
- `request <id> <method> [params]`: Send a custom request
- `notify <method> [params]`: Send a custom notification
- `raw <json>`: Send raw JSON message

Example commands sequence:

```
initialize
initialized
open file:///path/to/hello.goo hello.goo
request 1 textDocument/documentSymbol {"textDocument":{"uri":"file:///path/to/hello.goo"}}
request 999 shutdown
exit
```

## Using with an Editor

The true power of the LSP server is when it's integrated with an editor. For Visual Studio Code, you can use the Goo language extension located in the `src/tools/lsp/vscode-extension` directory.

See the [VSCode extension README](../vscode-extension/README.md) for more information on how to install and use the extension. 