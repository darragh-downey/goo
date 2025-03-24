/**
 * diagnostics_unified.h
 * 
 * Unified diagnostics interface for the type checker and tests
 * with no dependencies on other system components
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_DIAGNOSTICS_UNIFIED_H
#define GOO_DIAGNOSTICS_UNIFIED_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Diagnostic severity levels
typedef enum {
    GOO_DIAG_ERROR,       // Error - prevents compilation
    GOO_DIAG_WARNING,     // Warning - does not prevent compilation
    GOO_DIAG_NOTE,        // Note - additional information
    GOO_DIAG_HELP         // Help - suggestion on how to fix
} GooDiagnosticLevel;

// Source location structure
typedef struct {
    const char* filename;    // Source filename
    int line;                // Line number (1-based)
    int column;              // Column number (1-based)
    int length;              // Length of the span
} GooSourceLocation;

// Forward declaration
typedef struct GooDiagnostic GooDiagnostic;

// Diagnostic structure
struct GooDiagnostic {
    GooDiagnosticLevel level;    // Error, warning, note, etc.
    GooSourceLocation location;  // Primary source location
    const char* message;         // Diagnostic message
    const char* code;            // Error code (e.g., "E0001")
    const char* explanation;     // Detailed explanation
    GooDiagnostic* next;         // Next diagnostic in list
};

// Diagnostic context
typedef struct {
    GooDiagnostic* first;    // First diagnostic in the list
    GooDiagnostic* last;     // Last diagnostic in the list
    int error_count;         // Number of errors
    int warning_count;       // Number of warnings
} GooDiagnosticContext;

// Function declarations

// Create a new diagnostic context
GooDiagnosticContext* goo_diag_context_new(void);

// Free a diagnostic context
void goo_diag_context_free(GooDiagnosticContext* context);

// Create a new diagnostic
GooDiagnostic* goo_diag_new(
    GooDiagnosticLevel level,
    const char* filename,
    int line,
    int column,
    int length,
    const char* message
);

// Set the error code for a diagnostic
void goo_diag_set_code(GooDiagnostic* diag, const char* code, const char* explanation);

// Emit a diagnostic
void goo_diag_emit(GooDiagnosticContext* context, GooDiagnostic* diagnostic);

// Add a note to a diagnostic
void goo_diag_add_note(
    GooDiagnosticContext* context,
    const char* filename,
    int line,
    int column,
    int length,
    const char* message
);

// Add a suggestion to a diagnostic
void goo_diag_add_suggestion(
    GooDiagnosticContext* context,
    const char* filename,
    int line,
    int column,
    int length,
    const char* message,
    const char* replacement
);

// Get the error count
int goo_diag_error_count(GooDiagnosticContext* context);

// Get the warning count
int goo_diag_warning_count(GooDiagnosticContext* context);

// Print all diagnostics
void goo_diag_print_all(GooDiagnosticContext* context);

// Should abort due to too many errors
bool goo_diag_should_abort(GooDiagnosticContext* context);

#endif /* GOO_DIAGNOSTICS_UNIFIED_H */ 