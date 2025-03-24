# Goo Language for Visual Studio Code

This extension provides Goo language support for Visual Studio Code, including syntax highlighting, code completion, diagnostics, and more through the Language Server Protocol (LSP).

## Features

- Syntax highlighting for Goo files (`.goo`)
- Code snippets for common Goo patterns
- Language Server Protocol integration for:
  - Diagnostics (errors and warnings)
  - Code completion
  - Hover information
  - Go to definition
  - Find references
  - Document symbols
  - Workspace symbols
  - Document highlighting
  - Document formatting
  - Rename symbol
  - Signature help

## Requirements

- Visual Studio Code 1.60.0 or newer
- Goo Language Server (`goo-lsp`) executable in your PATH

## Installation

### From VS Code Marketplace

1. Open VS Code
2. Go to Extensions (Ctrl+Shift+X)
3. Search for "Goo Language"
4. Click Install

### From Source

1. Clone this repository
2. Run `npm install`
3. Run `npm run compile`
4. (Optional) If needed, convert the SVG icon to PNG using the provided script:
   ```bash
   ./scripts/convert_icon.sh
   ```
5. Copy the entire directory to your VS Code extensions folder:
   - Windows: `%USERPROFILE%\.vscode\extensions`
   - macOS/Linux: `~/.vscode/extensions`

## Extension Settings

This extension contributes the following settings:

- `goo.lsp.path`: Path to the Goo language server executable
- `goo.lsp.args`: Arguments to pass to the Goo language server
- `goo.lsp.trace.server`: Traces the communication between VS Code and the Goo language server
- `goo.diagnostics.enabled`: Enable/disable diagnostics
- `goo.hover.enabled`: Enable/disable hover information
- `goo.completion.enabled`: Enable/disable code completion
- `goo.definition.enabled`: Enable/disable go-to-definition
- `goo.references.enabled`: Enable/disable find references
- `goo.formatting.enabled`: Enable/disable document formatting
- `goo.symbols.enabled`: Enable/disable document symbols
- `goo.highlight.enabled`: Enable/disable document highlighting
- `goo.rename.enabled`: Enable/disable rename symbol
- `goo.signatureHelp.enabled`: Enable/disable signature help

## Commands

This extension contributes the following commands:

- `goo.restartLSP`: Restart the Goo Language Server

## Snippets

The extension includes code snippets for common Goo patterns:

- `fn`: Function declaration
- `main`: Main function
- `if`: If statement
- `ifelse`: If-else statement
- `for`: For loop
- `while`: While loop
- `struct`: Struct declaration
- `enum`: Enum declaration
- `match`: Match expression
- `import`: Import statement
- `importfrom`: Import from statement
- `let`: Variable declaration
- `const`: Constant declaration
- `print`: Print statement
- `interface`: Interface declaration
- `impl`: Implementation block
- `implfor`: Interface implementation

## Development

### Icon Conversion

The extension uses an SVG icon for high-quality scaling. However, if you need a PNG version for any reason, a conversion script is provided:

```bash
./scripts/convert_icon.sh
```

This script requires either ImageMagick or librsvg to be installed on your system.

## Known Issues

Please report any issues on the [GitHub repository](https://github.com/goo-lang/goo/issues).

## Release Notes

### 0.1.0

Initial release of the Goo language extension.

## Contributing

Contributions are welcome! Please see the [contributing guidelines](https://github.com/goo-lang/goo/blob/master/CONTRIBUTING.md) for more information.

## License

This extension is licensed under the MIT License - see the LICENSE file for details.