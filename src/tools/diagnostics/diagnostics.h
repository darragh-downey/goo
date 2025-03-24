/**
 * @file diagnostics.h
 * @brief Comprehensive diagnostics system for the Goo compiler
 * 
 * This file defines the diagnostic system interfaces for the Goo compiler,
 * inspired by Rust's diagnostic capabilities.
 */

#ifndef GOO_DIAGNOSTICS_H
#define GOO_DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Severity level for diagnostics
 */
typedef enum {
    GOO_DIAG_ERROR,       // Errors that prevent compilation
    GOO_DIAG_WARNING,     // Warnings about questionable code
    GOO_DIAG_NOTE,        // Additional context information
    GOO_DIAG_HELP,        // Help messages with suggestions
    GOO_DIAG_ICE          // Internal compiler errors
} GooDiagnosticLevel;

/**
 * Applicability of a suggestion
 */
typedef enum {
    GOO_APPLICABILITY_UNSPECIFIED,    // Default, no specific information
    GOO_APPLICABILITY_MACHINE_APPLICABLE, // Can be applied automatically
    GOO_APPLICABILITY_HAS_PLACEHOLDER,  // Contains placeholders
    GOO_APPLICABILITY_NOT_APPLICABLE   // Not automatically applicable
} GooSuggestionApplicability;

/**
 * Source location information
 */
typedef struct {
    const char* filename;  // Source filename
    unsigned int line;     // Line number (1-based)
    unsigned int column;   // Column number (1-based)
    unsigned int length;   // Length of the span in characters
} GooSourceLocation;

/**
 * Code suggestion for diagnostics
 */
typedef struct {
    GooSourceLocation location;             // Where to apply the suggestion
    const char* message;                    // Suggestion message
    const char* suggested_replacement;      // Suggested code replacement
    GooSuggestionApplicability applicability; // How applicable is the suggestion
} GooSuggestion;

/**
 * A single diagnostic message
 */
typedef struct GooDiagnostic {
    GooDiagnosticLevel level;               // Severity level
    GooSourceLocation primary_location;     // Main location
    const char* message;                    // Main message
    
    // Additional information
    struct GooDiagnostic** children;        // Child diagnostics (notes, helps)
    size_t children_count;                  // Number of children
    GooSuggestion** suggestions;            // Suggestions for fixing
    size_t suggestions_count;               // Number of suggestions
    
    // Diagnostic metadata
    const char* code;                       // Diagnostic code (e.g., E0001)
    const char* explanation;                // Detailed explanation
} GooDiagnostic;

/**
 * Diagnostic context/handler
 */
typedef struct GooDiagnosticContext {
    // State
    GooDiagnostic** diagnostics;            // All emitted diagnostics
    size_t diagnostics_count;               // Count of diagnostics
    size_t diagnostics_capacity;            // Capacity of diagnostics array
    
    // Settings
    bool treat_warnings_as_errors;          // -Werror equivalent
    bool json_output;                       // Output as JSON
    bool colored_output;                    // Use colors in terminal
    int error_limit;                        // Max number of errors (0 = no limit)
    
    // Error counts
    int error_count;                        // Number of errors
    int warning_count;                      // Number of warnings
} GooDiagnosticContext;

/**
 * Creates a new diagnostic context
 */
GooDiagnosticContext* goo_diag_context_new(void);

/**
 * Free a diagnostic context and all contained diagnostics
 */
void goo_diag_context_free(GooDiagnosticContext* context);

/**
 * Create a new diagnostic
 */
GooDiagnostic* goo_diag_new(
    GooDiagnosticLevel level,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message
);

/**
 * Add a child diagnostic (note or help) to a diagnostic
 */
void goo_diag_add_child(
    GooDiagnostic* diag, 
    GooDiagnosticLevel level,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message
);

/**
 * Add a suggestion to a diagnostic
 */
void goo_diag_add_suggestion(
    GooDiagnostic* diag,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message,
    const char* replacement,
    GooSuggestionApplicability applicability
);

/**
 * Set a diagnostic code and explanation
 */
void goo_diag_set_code(
    GooDiagnostic* diag,
    const char* code,
    const char* explanation
);

/**
 * Emit a diagnostic to the given context
 */
void goo_diag_emit(GooDiagnosticContext* context, GooDiagnostic* diag);

/**
 * Helper function to create and emit an error
 */
void goo_report_error(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
);

/**
 * Helper function to create and emit a warning
 */
void goo_report_warning(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
);

/**
 * Helper function to create and emit a note
 */
void goo_report_note(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
);

/**
 * Helper function to create and emit a help message
 */
void goo_report_help(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
);

/**
 * Print all diagnostics in the context to stdout/stderr
 */
void goo_diag_print_all(GooDiagnosticContext* context);

/**
 * Print all diagnostics in the context as JSON to stdout
 */
void goo_diag_print_json(GooDiagnosticContext* context);

/**
 * Check if any errors were reported
 */
bool goo_diag_has_errors(GooDiagnosticContext* context);

/**
 * Get the count of errors
 */
int goo_diag_error_count(GooDiagnosticContext* context);

/**
 * Get the count of warnings
 */
int goo_diag_warning_count(GooDiagnosticContext* context);

/**
 * Initialize the diagnostics system with command-line arguments
 */
GooDiagnosticContext* goo_init_diagnostics(int argc, char** argv);

/**
 * Clean up the diagnostics system
 */
void goo_cleanup_diagnostics(GooDiagnosticContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* GOO_DIAGNOSTICS_H */ 