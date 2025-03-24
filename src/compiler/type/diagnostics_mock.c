/**
 * diagnostics_mock.c
 * 
 * Mock implementation of the diagnostics system for testing
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include "diagnostics_mock.h"

// Create a new diagnostic context
GooDiagnosticContext* goo_diag_context_new(void) {
    GooDiagnosticContext* ctx = (GooDiagnosticContext*)malloc(sizeof(GooDiagnosticContext));
    ctx->first = NULL;
    ctx->last = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    return ctx;
}

// Free a diagnostic context
void goo_diag_context_free(GooDiagnosticContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Free all diagnostics in the linked list
    GooDiagnostic* diag = ctx->first;
    while (diag) {
        GooDiagnostic* next = diag->next;
        
        if (diag->message) free((void*)diag->message);
        if (diag->code) free((void*)diag->code);
        if (diag->explanation) free((void*)diag->explanation);
        
        free(diag);
        diag = next;
    }
    
    free(ctx);
}

// Create a new diagnostic
GooDiagnostic* goo_diag_new(
    GooDiagnosticLevel level,
    const char* filename,
    int line,
    int column,
    int length,
    const char* message
) {
    GooDiagnostic* diag = (GooDiagnostic*)malloc(sizeof(GooDiagnostic));
    
    diag->level = level;
    diag->location.filename = filename ? strdup(filename) : NULL;
    diag->location.line = line;
    diag->location.column = column;
    diag->location.length = length;
    diag->message = message ? strdup(message) : NULL;
    diag->code = NULL;
    diag->explanation = NULL;
    diag->next = NULL;
    
    return diag;
}

// Set the error code for a diagnostic
void goo_diag_set_code(GooDiagnostic* diag, const char* code, const char* explanation) {
    if (!diag) {
        return;
    }
    
    if (diag->code) {
        free((void*)diag->code);
    }
    
    if (diag->explanation) {
        free((void*)diag->explanation);
    }
    
    diag->code = code ? strdup(code) : NULL;
    diag->explanation = explanation ? strdup(explanation) : NULL;
}

// Emit a diagnostic
void goo_diag_emit(GooDiagnosticContext* ctx, GooDiagnostic* diag) {
    if (!ctx || !diag) {
        return;
    }
    
    // Update error/warning count
    if (diag->level == GOO_DIAG_ERROR) {
        ctx->error_count++;
    } else if (diag->level == GOO_DIAG_WARNING) {
        ctx->warning_count++;
    }
    
    // Add to linked list
    if (ctx->last) {
        ctx->last->next = diag;
        ctx->last = diag;
    } else {
        ctx->first = ctx->last = diag;
    }
}

// Get the error count
int goo_diag_error_count(GooDiagnosticContext* ctx) {
    if (!ctx) {
        return 0;
    }
    
    return ctx->error_count;
}

// Get the warning count
int goo_diag_warning_count(GooDiagnosticContext* ctx) {
    if (!ctx) {
        return 0;
    }
    
    return ctx->warning_count;
}

// Helper function to get level name
static const char* get_level_name(GooDiagnosticLevel level) {
    switch (level) {
        case GOO_DIAG_ERROR:
            return "error";
        case GOO_DIAG_WARNING:
            return "warning";
        case GOO_DIAG_NOTE:
            return "note";
        case GOO_DIAG_HELP:
            return "help";
        default:
            return "unknown";
    }
}

// Print all diagnostics
void goo_diag_print_all(GooDiagnosticContext* ctx) {
    if (!ctx) {
        return;
    }
    
    GooDiagnostic* diag = ctx->first;
    
    while (diag) {
        // Print diagnostic level and code if available
        printf("%s", get_level_name(diag->level));
        
        if (diag->code) {
            printf("[%s]", diag->code);
        }
        
        // Print location and message
        printf(": %s:%d:%d: %s\n", 
               diag->location.filename ? diag->location.filename : "unknown", 
               diag->location.line, 
               diag->location.column,
               diag->message ? diag->message : "");
        
        // Print explanation if available
        if (diag->explanation) {
            printf("    = %s\n", diag->explanation);
        }
        
        printf("\n");
        diag = diag->next;
    }
    
    // Print summary
    printf("%d error(s), %d warning(s) found.\n", 
           ctx->error_count, ctx->warning_count);
} 