/**
 * standalone_diagnostics_test.c
 * 
 * Standalone test for the mock diagnostics system
 * without dependencies on other type system components
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Error codes for type system errors
#define GOO_ERR_TYPE_MISMATCH "E0001"
#define GOO_ERR_UNDEFINED_VARIABLE "E0002"
#define GOO_ERR_CALL_FUNCTION "E0010"

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

// Simple AST node for testing
typedef struct AstNode {
    int type;
    const char* file;
    int line;
    int column;
    int length;
    struct AstNode* next;
} AstNode;

// Test type context
typedef struct {
    GooDiagnosticContext* diagnostics;
    int current_scope_level;
    void* type_table;
} GooTypeContext;

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

// Create a test node
AstNode* create_test_node(const char* file, int line, int column, int length) {
    AstNode* node = (AstNode*)malloc(sizeof(AstNode));
    node->type = 0;
    node->file = file ? strdup(file) : NULL;
    node->line = line;
    node->column = column;
    node->length = length;
    node->next = NULL;
    return node;
}

// Free a test node
void free_test_node(AstNode* node) {
    if (node) {
        if (node->file) {
            free((void*)node->file);
        }
        free(node);
    }
}

// Create a type context for testing
GooTypeContext* create_test_context(void) {
    GooTypeContext* ctx = (GooTypeContext*)malloc(sizeof(GooTypeContext));
    memset(ctx, 0, sizeof(GooTypeContext));
    ctx->diagnostics = goo_diag_context_new();
    return ctx;
}

// Free a type context
void free_test_context(GooTypeContext* ctx) {
    if (ctx) {
        if (ctx->diagnostics) {
            goo_diag_context_free(ctx->diagnostics);
        }
        free(ctx);
    }
}

// Report a type error
void report_type_error(GooTypeContext* ctx, AstNode* node, const char* error_code, const char* message) {
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
    
    goo_diag_set_code(diag, error_code, NULL);
    goo_diag_emit(ctx->diagnostics, diag);
}

// Add a note to the diagnostics
void add_diagnostic_note(GooTypeContext* ctx, AstNode* node, const char* message) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_NOTE,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    goo_diag_emit(ctx->diagnostics, diag);
}

// Add a suggestion to the diagnostics
void add_diagnostic_suggestion(GooTypeContext* ctx, AstNode* node, const char* message, const char* replacement) {
    if (!ctx || !ctx->diagnostics || !node) {
        return;
    }
    
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_HELP,
        node->file,
        node->line,
        node->column,
        node->length,
        message
    );
    
    goo_diag_emit(ctx->diagnostics, diag);
}

// Test basic error reporting
void test_basic_error_reporting(void) {
    printf("Testing basic error reporting...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create a test node
    AstNode* node = create_test_node("test.goo", 10, 5, 3);
    
    // Report a simple error
    report_type_error(ctx, node, GOO_ERR_TYPE_MISMATCH, "Type mismatch in expression");
    
    // Print diagnostics
    printf("Diagnostics after error reporting:\n");
    goo_diag_print_all(ctx->diagnostics);
    
    // Free resources
    free_test_node(node);
    free_test_context(ctx);
}

// Test complex error scenario
void test_complex_error_scenario(void) {
    printf("\nTesting complex error scenario...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create test nodes
    AstNode* expr_node = create_test_node("complex.goo", 15, 10, 8);
    AstNode* call_node = create_test_node("complex.goo", 20, 15, 12);
    
    // Report a type mismatch error
    report_type_error(ctx, expr_node, GOO_ERR_TYPE_MISMATCH, 
                      "Type mismatch: expected 'int', found 'string'");
    
    // Add a note
    add_diagnostic_note(ctx, expr_node, 
                        "String cannot be implicitly converted to int");
    
    // Add a suggestion
    add_diagnostic_suggestion(ctx, expr_node, 
                              "Try using the to_int() function", 
                              "to_int(myString)");
    
    // Report another error
    report_type_error(ctx, call_node, GOO_ERR_CALL_FUNCTION, 
                      "Cannot call a non-function value");
    
    // Print diagnostics
    printf("Diagnostics for complex error scenario:\n");
    goo_diag_print_all(ctx->diagnostics);
    
    // Free resources
    free_test_node(expr_node);
    free_test_node(call_node);
    free_test_context(ctx);
}

int main(void) {
    printf("=== Type Checker Diagnostics Integration Test ===\n\n");
    
    // Run tests
    test_basic_error_reporting();
    test_complex_error_scenario();
    
    printf("\nAll tests completed.\n");
    return 0;
} 