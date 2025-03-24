/**
 * type_checker_adapter.c
 * 
 * Adapter for integrating diagnostics with the type checker
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "type_error_codes.h"
#include "ast_node_unified.h"
#include "type_checker_adapter.h"

// Forward declarations for external functions
extern GooDiagContext* goo_diag_create_context(void);
extern void goo_diag_free_context(GooDiagContext* ctx);
extern void goo_diag_report_error(GooDiagContext* ctx, const GooAstNode* node, 
                               const char* code, const char* message);
extern void goo_diag_add_note(GooDiagContext* ctx, const char* message);
extern void goo_diag_add_suggestion_message(GooDiagContext* ctx, const char* message);
extern int goo_diag_get_error_count(GooDiagContext* ctx);
extern bool goo_diag_error_limit_reached(GooDiagContext* ctx);
extern void goo_diag_print_summary(GooDiagContext* ctx);
extern char* goo_diag_format_type_mismatch(const char* expected, const char* found);

// Type checker context implementation
struct GooTypeCheckerContext {
    GooTypeContext* type_ctx;
    GooDiagContext* diag_ctx;
    char* (*type_to_string)(const GooType*);  // Function pointer for type to string conversion
};

// Create a new type checker context
GooTypeCheckerContext* goo_typechecker_create_context(GooTypeContext* type_ctx) {
    GooTypeCheckerContext* ctx = (GooTypeCheckerContext*)malloc(sizeof(GooTypeCheckerContext));
    if (!ctx) return NULL;
    
    ctx->type_ctx = type_ctx;
    ctx->diag_ctx = goo_diag_create_context();
    ctx->type_to_string = NULL;
    
    return ctx;
}

// Free a type checker context
void goo_typechecker_free_context(GooTypeCheckerContext* ctx) {
    if (!ctx) return;
    
    // Free the diagnostics context
    if (ctx->diag_ctx) {
        goo_diag_free_context(ctx->diag_ctx);
    }
    
    // Note: type_ctx is owned externally and not freed here
    
    free(ctx);
}

// Register a type-to-string function
void goo_typechecker_register_type_to_string(GooTypeCheckerContext* ctx, 
                                          char* (*func)(const GooType*)) {
    if (ctx) {
        ctx->type_to_string = func;
    }
}

// Format a type as a string using the registered function
static char* format_type(GooTypeCheckerContext* ctx, const GooType* type) {
    if (!ctx || !type) {
        return strdup("<unknown type>");
    }
    
    if (ctx->type_to_string) {
        return ctx->type_to_string(type);
    }
    
    return strdup("<type>");
}

// Report a general type error
void goo_typechecker_report_error(GooTypeCheckerContext* ctx, 
                                const GooAstNode* node,
                                const char* code, 
                                const char* message) {
    if (!ctx || !ctx->diag_ctx) return;
    
    goo_diag_report_error(ctx->diag_ctx, node, code, message);
}

// Report a type mismatch error
void goo_typechecker_report_type_mismatch(GooTypeCheckerContext* ctx, 
                                        const GooAstNode* node,
                                        const GooType* expected, 
                                        const GooType* found) {
    if (!ctx || !ctx->diag_ctx) return;
    
    char* expected_str = format_type(ctx, expected);
    char* found_str = format_type(ctx, found);
    
    char* message = goo_diag_format_type_mismatch(expected_str, found_str);
    
    goo_diag_report_error(ctx->diag_ctx, node, GOO_ERR_TYPE_MISMATCH, message);
    
    free(expected_str);
    free(found_str);
    free(message);
}

// Add a note to the current error
void goo_typechecker_add_note(GooTypeCheckerContext* ctx, const char* message) {
    if (!ctx || !ctx->diag_ctx) return;
    
    goo_diag_add_note(ctx->diag_ctx, message);
}

// Add a suggestion to the current error
void goo_typechecker_add_suggestion(GooTypeCheckerContext* ctx, const char* message) {
    if (!ctx || !ctx->diag_ctx) return;
    
    goo_diag_add_suggestion_message(ctx->diag_ctx, message);
}

// Get error count
int goo_typechecker_get_error_count(GooTypeCheckerContext* ctx) {
    if (!ctx || !ctx->diag_ctx) return 0;
    
    return goo_diag_get_error_count(ctx->diag_ctx);
}

// Check if we should skip additional errors
bool goo_typechecker_error_limit_reached(GooTypeCheckerContext* ctx) {
    if (!ctx || !ctx->diag_ctx) return false;
    
    return goo_diag_error_limit_reached(ctx->diag_ctx);
}

// Print diagnostic summary
void goo_typechecker_print_diagnostics(GooTypeCheckerContext* ctx) {
    if (!ctx || !ctx->diag_ctx) return;
    
    goo_diag_print_summary(ctx->diag_ctx);
} 