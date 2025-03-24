/**
 * diagnostics_mock.h
 * 
 * Mock implementation of the diagnostics system for testing
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_DIAGNOSTICS_MOCK_H
#define GOO_DIAGNOSTICS_MOCK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Diagnostic severity levels
typedef enum {
    GOO_DIAG_ERROR,
    GOO_DIAG_WARNING,
    GOO_DIAG_NOTE,
    GOO_DIAG_HELP
} GooDiagnosticLevel;

// Source location structure
typedef struct {
    const char* filename;
    int line;
    int column;
    int length;
} GooSourceLocation;

// Diagnostic structure
typedef struct GooDiagnostic {
    GooDiagnosticLevel level;
    GooSourceLocation location;
    const char* message;
    const char* code;
    const char* explanation;
    struct GooDiagnostic* next;  // Linked list of diagnostics
} GooDiagnostic;

// Diagnostic context
typedef struct GooDiagnosticContext {
    GooDiagnostic* first;
    GooDiagnostic* last;
    int error_count;
    int warning_count;
} GooDiagnosticContext;

// Create a new diagnostic context
GooDiagnosticContext* goo_diag_context_new(void);

// Free a diagnostic context
void goo_diag_context_free(GooDiagnosticContext* ctx);

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
void goo_diag_emit(GooDiagnosticContext* ctx, GooDiagnostic* diag);

// Get the error count
int goo_diag_error_count(GooDiagnosticContext* ctx);

// Get the warning count
int goo_diag_warning_count(GooDiagnosticContext* ctx);

// Print all diagnostics
void goo_diag_print_all(GooDiagnosticContext* ctx);

#endif /* GOO_DIAGNOSTICS_MOCK_H */ 