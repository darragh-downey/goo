# Goo Compiler Diagnostics System

This directory contains the implementation of the Goo compiler's diagnostics system, which handles error and warning reporting, source code highlighting, and error explanation.

## Components

The diagnostics system consists of several key components:

- **Core Diagnostics (`diagnostics.h`, `diagnostics.c`)**: Defines the basic structures and functions for creating, managing, and emitting diagnostics.
- **Error Catalog (`error_catalog.h`, `error_catalog.c`)**: Provides a catalog of error codes with explanations, similar to Rust's `--explain` feature.
- **Source Highlighting (`source_highlight.h`, `source_highlight.c`)**: Implements terminal-based source code highlighting for displaying errors and warnings.
- **Diagnostics Module (`diagnostics_module.h`, `diagnostics_module.c`)**: Provides a unified interface for the various diagnostics components.
- **Unified API (`goo_diagnostics.h`, `goo_diagnostics.c`)**: Exposes a simplified API for compiler components to report errors and warnings.

## Features

### Error and Warning Reporting

The system can report errors and warnings with detailed location information:

```c
// Report an error
goo_diagnostics_report_error(
    context, 
    "example.goo",    // Filename
    source_code,      // Source code for highlighting (can be NULL)
    10,               // Line number (1-based)
    5,                // Column number (1-based)
    8,                // Length of the error span
    "Expected '%s' but found '%s'", "int", "string"  // Format string and args
);

// Report a warning
goo_diagnostics_report_warning(
    context, 
    "example.goo",
    source_code,
    15,
    10,
    4,
    "Unused variable '%s'", "x"
);
```

### Source Code Highlighting

When reporting errors and warnings, the system can highlight the relevant portion of the source code:

```
example.goo:10:5: error: Expected 'int' but found 'string'
 9 │ func main() {
10 │     let x: int = "hello"
            ^^^^^^^
11 │     println(x)
```

### Child Diagnostics

Errors and warnings can have child diagnostics attached, which provide additional context:

```c
GooDiagnostic* error = goo_diag_new(...);

// Add a note
goo_diag_add_child(
    error,
    GOO_DIAG_NOTE,
    "example.goo",
    5,
    10,
    2,
    "Variable declared here"
);

// Add a help message
goo_diag_add_child(
    error,
    GOO_DIAG_HELP,
    "example.goo",
    10,
    5,
    8,
    "Try using an integer literal"
);
```

### Fix Suggestions

Diagnostics can include suggestions for fixing the issue, which can be machine-applicable:

```c
// Add a suggestion
goo_diag_add_suggestion(
    error,
    "example.goo",
    10,
    12,
    7,
    "Replace with an integer",
    "42",
    0  // Machine applicable
);
```

### Error Catalog

The error catalog system provides detailed explanations for error codes:

```c
// Register an error code
goo_error_catalog_register(
    "E0001",                 // Error code
    GOO_ERROR_CAT_TYPE,      // Category
    "Type mismatch",         // Short description
    "This error occurs when a value of one type is assigned to a variable of a different type.",
    "let x: int = \"hello\";",  // Example code
    "let x: int = 42;"         // Solution
);

// Look up an error code
const GooErrorCatalogEntry* entry = goo_error_catalog_lookup("E0001");

// Explain an error code (e.g., for --explain command line flag)
goo_error_catalog_explain("E0001");
```

### Configuration

The diagnostics system can be configured with various options:

```c
GooDiagnosticsConfig config = goo_diagnostics_default_config();
config.enable_colors = true;
config.json_output = false;
config.treat_warnings_as_errors = false;
config.error_limit = 10;
config.context_lines = 2;
config.unicode = true;

// Initialize with configuration
GooDiagnosticContext* ctx = goo_diagnostics_init(&config);
```

## Examples

See the `examples` directory for examples of using the diagnostics system.

## Building

The diagnostics system can be built using Zig:

```bash
zig build run-diag-demo  # Build and run the diagnostics demo
```

## Integration with Other Systems

The diagnostics system is designed to be integrated with other compiler components, such as:

- **Parser**: To report syntax errors
- **Type Checker**: To report type errors
- **Borrow Checker**: To report borrowing and lifetime errors
- **Compiler Driver**: To report command-line and I/O errors

## Future Work

- JSON output format for integration with IDEs and other tools
- Integration with LSP (Language Server Protocol)
- Color scheme customization
- Improved terminal rendering for complex error cases 