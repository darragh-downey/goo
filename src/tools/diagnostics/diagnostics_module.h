/**
 * @file diagnostics_module.h
 * @brief Central module for Goo compiler diagnostics
 * 
 * This file provides a unified interface to the diagnostic system components,
 * making it easy to initialize and use the system from other compiler components.
 */

#ifndef GOO_DIAGNOSTICS_MODULE_H
#define GOO_DIAGNOSTICS_MODULE_H

#include "diagnostics.h"
#include "error_catalog.h"
#include "source_highlight.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configuration for the diagnostics module
 */
typedef struct {
    bool enable_colors;          // Whether to enable ANSI colors in output
    bool json_output;            // Whether to output in JSON format
    bool treat_warnings_as_errors; // Whether to treat warnings as errors
    int error_limit;             // Maximum number of errors (0 = no limit)
    int context_lines;           // Number of context lines to show
    bool unicode;                // Whether to use Unicode characters
    bool machine_applicable_only; // Only show machine-applicable suggestions
} GooDiagnosticsConfig;

/**
 * Get the default diagnostics configuration
 */
GooDiagnosticsConfig goo_diagnostics_default_config(void);

/**
 * Initialize the diagnostics module
 * 
 * @param config Configuration for the module (NULL for defaults)
 * @return Context for the diagnostics module, or NULL on failure
 */
GooDiagnosticContext* goo_diagnostics_init(const GooDiagnosticsConfig* config);

/**
 * Clean up the diagnostics module
 * 
 * @param context The context to clean up
 */
void goo_diagnostics_cleanup(GooDiagnosticContext* context);

/**
 * Process command-line arguments and extract flags related to diagnostics
 * 
 * @param argc Argument count
 * @param argv Argument array
 * @param config Config to update based on arguments
 * @return true if successful, false on error
 */
bool goo_diagnostics_process_args(int argc, char** argv, GooDiagnosticsConfig* config);

/**
 * Parse and handle the --explain flag
 * 
 * @param argc Argument count
 * @param argv Argument array
 * @return true if --explain was handled, false otherwise
 */
bool goo_diagnostics_handle_explain(int argc, char** argv);

/**
 * Report an error with source highlighting
 * 
 * @param context Diagnostic context
 * @param filename Source filename
 * @param source Full source code
 * @param line Line number (1-based)
 * @param column Column number (1-based)
 * @param length Length of the error span
 * @param message Error message format string
 * @param ... Additional arguments for the format string
 */
void goo_diagnostics_report_error(
    GooDiagnosticContext* context,
    const char* filename,
    const char* source,
    unsigned int line,
    unsigned int column,
    unsigned int length,
    const char* message,
    ...
);

/**
 * Report a warning with source highlighting
 * 
 * @param context Diagnostic context
 * @param filename Source filename
 * @param source Full source code
 * @param line Line number (1-based)
 * @param column Column number (1-based)
 * @param length Length of the warning span
 * @param message Warning message format string
 * @param ... Additional arguments for the format string
 */
void goo_diagnostics_report_warning(
    GooDiagnosticContext* context,
    const char* filename,
    const char* source,
    unsigned int line,
    unsigned int column,
    unsigned int length,
    const char* message,
    ...
);

/**
 * Print a summary of all diagnostics
 * 
 * @param context Diagnostic context
 * @param source Full source code (NULL if not available)
 */
void goo_diagnostics_print_summary(
    GooDiagnosticContext* context,
    const char* source
);

/**
 * Check if compilation should be aborted due to errors
 * 
 * @param context Diagnostic context
 * @return true if compilation should be aborted
 */
bool goo_diagnostics_should_abort(GooDiagnosticContext* context);

#ifdef __cplusplus
}
#endif

#endif /* GOO_DIAGNOSTICS_MODULE_H */ 