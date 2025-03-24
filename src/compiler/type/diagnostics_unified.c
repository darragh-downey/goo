/**
 * diagnostics_unified.c
 *
 * Unified diagnostics implementation for the Goo compiler
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

// Mock diagnostics context structure
typedef struct GooDiagContext {
    int error_count;
    int warning_count;
    int max_errors;
    bool silent;
    char* current_file;
} GooDiagContext;

// Create a diagnostics context
GooDiagContext* goo_diag_create_context(void) {
    GooDiagContext* ctx = (GooDiagContext*)malloc(sizeof(GooDiagContext));
    if (ctx) {
        ctx->error_count = 0;
        ctx->warning_count = 0;
        ctx->max_errors = 20; // Default max errors
        ctx->silent = false;
        ctx->current_file = NULL;
    }
    return ctx;
}

// Free a diagnostics context
void goo_diag_free_context(GooDiagContext* ctx) {
    if (ctx) {
        if (ctx->current_file) {
            free(ctx->current_file);
        }
        free(ctx);
    }
}

// Set the maximum number of errors
void goo_diag_set_max_errors(GooDiagContext* ctx, int max_errors) {
    if (ctx) {
        ctx->max_errors = max_errors;
    }
}

// Set silent mode (no output)
void goo_diag_set_silent(GooDiagContext* ctx, bool silent) {
    if (ctx) {
        ctx->silent = silent;
    }
}

// Set the current file being processed
void goo_diag_set_current_file(GooDiagContext* ctx, const char* file) {
    if (ctx) {
        if (ctx->current_file) {
            free(ctx->current_file);
        }
        ctx->current_file = file ? strdup(file) : NULL;
    }
}

// Check if the error limit has been reached
bool goo_diag_error_limit_reached(GooDiagContext* ctx) {
    return ctx && ctx->error_count >= ctx->max_errors;
}

// Helper function to format and print a diagnostic message
static void print_diagnostic(const char* level, const char* code, 
                           const char* file, int line, int column,
                           const char* message) {
    printf("%s:%d:%d: %s [%s]: %s\n", 
           file ? file : "<unknown>", 
           line, 
           column, 
           level, 
           code, 
           message);
}

// Report an error with a node
void goo_diag_report_error(GooDiagContext* ctx, const GooAstNode* node, 
                         const char* code, const char* message) {
    if (!ctx || ctx->silent || goo_diag_error_limit_reached(ctx)) {
        return;
    }
    
    const char* file = node && node->file ? node->file : 
                     (ctx->current_file ? ctx->current_file : "<unknown>");
    int line = node ? node->line : 0;
    int column = node ? node->column : 0;
    
    print_diagnostic("error", code, file, line, column, message);
    ctx->error_count++;
    
    if (goo_diag_error_limit_reached(ctx)) {
        printf("Error limit reached (%d errors). Suppressing further errors.\n", 
               ctx->max_errors);
    }
}

// Report a warning with a node
void goo_diag_report_warning(GooDiagContext* ctx, const GooAstNode* node, 
                           const char* code, const char* message) {
    if (!ctx || ctx->silent) {
        return;
    }
    
    const char* file = node && node->file ? node->file : 
                     (ctx->current_file ? ctx->current_file : "<unknown>");
    int line = node ? node->line : 0;
    int column = node ? node->column : 0;
    
    print_diagnostic("warning", code, file, line, column, message);
    ctx->warning_count++;
}

// Add a note to the most recent diagnostic
void goo_diag_add_note(GooDiagContext* ctx, const char* message) {
    if (!ctx || ctx->silent) {
        return;
    }
    
    printf("note: %s\n", message);
}

// Add a suggestion to the most recent diagnostic
void goo_diag_add_suggestion_message(GooDiagContext* ctx, const char* message) {
    if (!ctx || ctx->silent) {
        return;
    }
    
    printf("suggestion: %s\n", message);
}

// Print a summary of diagnostics
void goo_diag_print_summary(GooDiagContext* ctx) {
    if (!ctx || ctx->silent) {
        return;
    }
    
    if (ctx->error_count > 0 || ctx->warning_count > 0) {
        printf("\nDiagnostic summary: %d error(s), %d warning(s)\n", 
               ctx->error_count, ctx->warning_count);
    }
}

// Reset error and warning counts
void goo_diag_reset_counts(GooDiagContext* ctx) {
    if (ctx) {
        ctx->error_count = 0;
        ctx->warning_count = 0;
    }
}

// Get the current error count
int goo_diag_get_error_count(GooDiagContext* ctx) {
    return ctx ? ctx->error_count : 0;
}

// Get the current warning count
int goo_diag_get_warning_count(GooDiagContext* ctx) {
    return ctx ? ctx->warning_count : 0;
}

// Helper function to create a specialized error message
char* goo_diag_format_type_mismatch(const char* expected, const char* found) {
    if (!expected || !found) {
        return strdup("Type mismatch");
    }
    
    size_t len = strlen("Expected type: ") + strlen(expected) + 
                strlen(", found: ") + strlen(found) + 1;
    
    char* result = (char*)malloc(len);
    if (result) {
        sprintf(result, "Expected type: %s, found: %s", expected, found);
    }
    return result;
} 