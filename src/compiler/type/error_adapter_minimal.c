/**
 * error_adapter_minimal.c
 * 
 * Implementation of adapter functions for integrating diagnostics with the type system
 * using the minimal AST node definition
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "error_adapter_minimal.h"
#include "diagnostics_mock.h"

// Buffer size for type string representation
#define TYPE_STRING_BUFFER_SIZE 256

// Function to initialize a diagnostics context for type checking
GooDiagnosticContext* goo_type_init_diagnostics(void) {
    return goo_diag_context_new();
}

// Helper function to create a source location from an AST node
static GooSourceLocation create_source_location_minimal(GooAstNodeMinimal* node) {
    GooSourceLocation loc;
    loc.filename = node->file;
    loc.line = node->line;
    loc.column = node->column;
    loc.length = node->length;
    return loc;
}

// Function to create and emit a type error
void goo_type_report_error_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                               const char* error_code, const char* message) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_ERROR,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    // Set the error code and explanation
    goo_diag_set_code(diag, error_code, NULL); // No detailed explanation for now
    
    // Emit the diagnostic
    goo_diag_emit(ctx->diagnostics, diag);
}

// Function to report a type mismatch with expected and found types
void goo_type_report_mismatch_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                                  const char* expected_type, const char* found_type) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    // Format the error message
    char message[512];
    snprintf(message, sizeof(message), 
             "Type mismatch: expected '%s', found '%s'",
             expected_type, found_type);
    
    // Create and emit the diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_ERROR,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    // Set the error code
    goo_diag_set_code(diag, GOO_ERR_TYPE_MISMATCH, NULL);
    
    // Emit the diagnostic
    goo_diag_emit(ctx->diagnostics, diag);
}

// Function to add a note to the most recent diagnostic
void goo_type_add_note_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node, const char* message) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    // The diagnostics system doesn't have a direct way to access the most recent diagnostic,
    // so we'll create a new note diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_NOTE,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    // Emit the note as a separate diagnostic
    goo_diag_emit(ctx->diagnostics, diag);
}

// Function to add a suggestion to the most recent diagnostic
void goo_type_add_suggestion_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                                 const char* message, const char* replacement) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    // Create a help diagnostic with the suggestion
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_HELP,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    // Emit the suggestion as a separate diagnostic
    goo_diag_emit(ctx->diagnostics, diag);
}

// Function to check if we should abort due to too many errors
bool goo_type_should_abort(GooTypeContext* ctx) {
    if (!ctx || !ctx->diagnostics) {
        return false;
    }
    
    // Arbitrary threshold - could be configurable
    return goo_diag_error_count(ctx->diagnostics) > 100;
}

// Function to get the current error count
int goo_type_error_count(GooTypeContext* ctx) {
    if (!ctx || !ctx->diagnostics) {
        return 0;
    }
    
    return goo_diag_error_count(ctx->diagnostics);
}

// Function to print all diagnostics
void goo_type_print_diagnostics(GooTypeContext* ctx) {
    if (!ctx || !ctx->diagnostics) {
        return;
    }
    
    goo_diag_print_all(ctx->diagnostics);
} 