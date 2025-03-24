# Goo Language Server Protocol Implementation

This directory contains the Language Server Protocol (LSP) implementation for the Goo programming language. The LSP server enables IDE integration, providing features like code completion, diagnostics, hover information, and more.

## Components

The implementation consists of several key components:

- **LSP Server**: Core implementation of the LSP protocol for Goo
- **LSP Client**: A command-line client for testing the server
- **VSCode Extension**: Extensions for Visual Studio Code that uses the LSP server
- **Protocol Definitions**: Constants and enums representing the LSP protocol
- **Examples**: Sample files and scripts to demonstrate usage

## Building

To build all components, run:

```bash
./build.sh
```

Or use the Makefile directly:

```bash
make
```

This will compile both the LSP server and client binaries to the `bin` directory.

## Usage

### Starting the LSP Server

You can start the LSP server directly from the command line:

```bash
goo-lsp [OPTIONS]
```

Available options:

- `-h, --help`: Display help information
- `-v, --verbose`: Enable verbose logging
- `-s, --std-lib PATH`: Path to the Goo standard library
- Various `--no-*` options to disable specific features

### Using the LSP Client

The LSP client can be used for testing and debugging:

```bash
goo-lsp-client [OPTIONS]
```

Once started, you can enter commands like:
- `help`: Show help information
- `initialize`: Initialize the server
- `open URI FILE`: Open a document

See the [examples directory](examples/) for more information.

### VSCode Extension

For Visual Studio Code integration, see the [vscode-extension](vscode-extension/) directory.

## Directory Structure

```
lsp/
├── goo_lsp_server.h        # Server header file
├── goo_lsp_server.c        # Server implementation
├── goo_lsp_client.c        # Test client implementation
├── goo_lsp_main.c          # Main entry point for the server
├── goo_lsp_protocol.h      # LSP protocol definitions
├── Makefile                # Build instructions
├── build.sh                # Build script
├── examples/               # Example files and scripts
│   ├── hello.goo           # Sample Goo source file
│   ├── test_lsp.sh         # Test script
│   └── README.md           # Examples documentation
└── vscode-extension/       # VSCode extension
    ├── package.json        # Extension metadata
    ├── src/                # TypeScript source
    ├── syntaxes/           # Syntax definitions
    ├── snippets/           # Code snippets
    └── README.md           # Extension documentation
```

## Requirements

- C compiler (gcc/clang)
- json-c library
- pthread library
- make

## Testing

To run tests:

```bash
make test
```

Or to run and see the test output:

```bash
make run-tests
```

## License

This implementation is licensed under the MIT License.