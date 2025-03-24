/**
 * type_error_adapter.c
 * 
 * Implementation of adapter functions for integrating diagnostics with the type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type_error_adapter.h"
#include "diagnostics_mock.h"  // Use mock implementation
#include "ast_node_minimal.h"  // Use minimal AST node implementation

// Minimal definition of GooTypeContext - just enough to access diagnostics
typedef struct GooType GooType;
struct GooTypeContext {
    void* diagnostics;  // Points to GooDiagnosticContext
    // Other fields are not needed for this adapter
};

// Buffer size for type string representation
#define TYPE_STRING_BUFFER_SIZE 256

// Function pointer for the type to string conversion
// This allows us to call the type system without direct include
typedef void (*TypeToStringFunc)(GooTypeContext*, GooType*, char*, size_t);

// Global function pointer that will be set by the type system
static TypeToStringFunc g_type_to_string_func = NULL;

// Function to register the type to string function
void goo_type_register_to_string_func(TypeToStringFunc func) {
    g_type_to_string_func = func;
}

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
void goo_type_report_error(GooTypeContext* ctx, void* node_ptr,
                         const char* error_code, const char* message) {
    if (!ctx || !node_ptr) {
        return;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return;
    }
    
    // Convert to minimal AST node for diagnostics
    GooAstNodeMinimal* node = goo_ast_to_minimal(node_ptr);
    if (!node) {
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
    goo_diag_emit(diag_ctx, diag);
    
    // Clean up the minimal node since it was created dynamically
    goo_ast_free_minimal(node);
}

// Function to report a type mismatch with expected and found types
void goo_type_report_mismatch(GooTypeContext* ctx, void* node_ptr,
                            GooType* expected, GooType* found) {
    if (!ctx || !node_ptr) {
        return;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return;
    }
    
    // Convert to minimal AST node for diagnostics
    GooAstNodeMinimal* node = goo_ast_to_minimal(node_ptr);
    if (!node) {
        return;
    }
    
    // Format the error message
    char expected_str[TYPE_STRING_BUFFER_SIZE] = "<unknown>";
    char found_str[TYPE_STRING_BUFFER_SIZE] = "<unknown>";
    
    // Use the registered function pointer if available
    if (g_type_to_string_func) {
        g_type_to_string_func(ctx, expected, expected_str, TYPE_STRING_BUFFER_SIZE);
        g_type_to_string_func(ctx, found, found_str, TYPE_STRING_BUFFER_SIZE);
    }
    
    char message[512];
    snprintf(message, sizeof(message), 
             "Type mismatch: expected '%s', found '%s'",
             expected_str, found_str);
    
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
    goo_diag_emit(diag_ctx, diag);
    
    // Clean up the minimal node since it was created dynamically
    goo_ast_free_minimal(node);
}

// Function to add a note to the most recent diagnostic
void goo_type_add_note(GooTypeContext* ctx, void* node_ptr, const char* message) {
    if (!ctx || !node_ptr) {
        return;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return;
    }
    
    // Convert to minimal AST node for diagnostics
    GooAstNodeMinimal* node = goo_ast_to_minimal(node_ptr);
    if (!node) {
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
    goo_diag_emit(diag_ctx, diag);
    
    // Clean up the minimal node since it was created dynamically
    goo_ast_free_minimal(node);
}

// Function to add a suggestion to the most recent diagnostic
void goo_type_add_suggestion(GooTypeContext* ctx, void* node_ptr,
                           const char* message, const char* replacement) {
    if (!ctx || !node_ptr) {
        return;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return;
    }
    
    // Convert to minimal AST node for diagnostics
    GooAstNodeMinimal* node = goo_ast_to_minimal(node_ptr);
    if (!node) {
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
    goo_diag_emit(diag_ctx, diag);
    
    // Clean up the minimal node since it was created dynamically
    goo_ast_free_minimal(node);
}

// Function to check if we should abort due to too many errors
bool goo_type_should_abort(GooTypeContext* ctx) {
    if (!ctx) {
        return false;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return false;
    }
    
    // Arbitrary threshold - could be configurable
    return goo_diag_error_count(diag_ctx) > 100;
}

// Function to get the current error count
int goo_type_error_count(GooTypeContext* ctx) {
    if (!ctx) {
        return 0;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return 0;
    }
    
    return goo_diag_error_count(diag_ctx);
}

// Function to print all diagnostics
void goo_type_print_diagnostics(GooTypeContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Get the diagnostics context from the type context
    GooDiagnosticContext* diag_ctx = (GooDiagnosticContext*)ctx->diagnostics;
    if (!diag_ctx) {
        return;
    }
    
    goo_diag_print_all(diag_ctx);
} 