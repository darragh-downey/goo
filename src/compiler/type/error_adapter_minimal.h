/**
 * error_adapter_minimal.h
 * 
 * Adapter for integrating the diagnostics system with the type checker
 * using a minimal AST node definition to avoid conflicts
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_ERROR_ADAPTER_MINIMAL_H
#define GOO_ERROR_ADAPTER_MINIMAL_H

#include <stdbool.h>
#include "ast_node_minimal.h"
#include "type_error_codes.h"

// Forward declarations for types to avoid including headers that cause conflicts
typedef struct GooDiagnosticContext GooDiagnosticContext;
typedef struct GooType GooType;

// Define the GooTypeContext structure for this adapter
typedef struct GooTypeContext {
    GooDiagnosticContext* diagnostics;
    int current_scope_level;
    void* type_table;
    // Other fields would be here in the real implementation
} GooTypeContext;

// Function to initialize a diagnostics context for type checking
GooDiagnosticContext* goo_type_init_diagnostics(void);

// Function to free a diagnostics context
void goo_diag_context_free(GooDiagnosticContext* ctx);

// Function to create and emit a type error
void goo_type_report_error_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                                 const char* error_code, const char* message);

// Function to report a type mismatch with expected and found types
void goo_type_report_mismatch_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                                    const char* expected_type, const char* found_type);

// Function to add a note to the most recent diagnostic
void goo_type_add_note_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node, 
                             const char* message);

// Function to add a suggestion to the most recent diagnostic
void goo_type_add_suggestion_minimal(GooTypeContext* ctx, GooAstNodeMinimal* node,
                                   const char* message, const char* replacement);

// Function to check if we should abort due to too many errors
bool goo_type_should_abort(GooTypeContext* ctx);

// Function to get the current error count
int goo_type_error_count(GooTypeContext* ctx);

// Function to print all diagnostics
void goo_type_print_diagnostics(GooTypeContext* ctx);

#endif /* GOO_ERROR_ADAPTER_MINIMAL_H */ 