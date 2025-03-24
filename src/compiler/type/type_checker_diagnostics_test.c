/**
 * type_checker_diagnostics_test.c
 * 
 * Test for integrating diagnostics with the type checker
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "type_error_adapter.h"
#include "type_error_codes.h"
#include "ast_node_minimal.h"
#include "diagnostics_mock.h"

// Define our own GooTypeContext to match the one in the adapter
struct GooTypeContext {
    void* diagnostics;  // Points to GooDiagnosticContext
    // Other fields are not needed for this adapter
};

// Manual implementation of type conversion for testing
void test_type_to_string(struct GooTypeContext* ctx, struct GooType* type, char* buffer, size_t size) {
    (void)ctx; // Unused parameter
    
    // For testing, we'll use simple type names
    if (type == NULL) {
        snprintf(buffer, size, "nil");
        return;
    }
    
    // Simplified type representation for testing
    int type_id = (int)(size_t)type; // Using the pointer value as a type ID for testing
    
    switch (type_id) {
        case 1:
            snprintf(buffer, size, "int");
            break;
        case 2:
            snprintf(buffer, size, "string");
            break;
        case 3:
            snprintf(buffer, size, "bool");
            break;
        case 4:
            snprintf(buffer, size, "function(int) -> string");
            break;
        default:
            snprintf(buffer, size, "unknown");
            break;
    }
}

// Test basic error reporting
static bool test_basic_error() {
    printf("Running basic error test...\n");
    
    // Create a minimal GooTypeContext for testing
    struct GooTypeContext ctx;
    ctx.diagnostics = goo_diag_context_new();
    
    // Register the type to string conversion function
    goo_type_register_to_string_func(test_type_to_string);
    
    // Create a minimal AST node
    GooAstNodeMinimal* node = goo_ast_create_minimal("test.goo", 10, 5, 8);
    
    // Report a simple error
    goo_type_report_error(&ctx, node, GOO_ERR_UNDEFINED_VARIABLE, "Variable 'foo' is not declared in this scope");
    
    // Check error count
    int error_count = goo_type_error_count(&ctx);
    printf("Error count: %d\n", error_count);
    
    // Print all diagnostics
    goo_type_print_diagnostics(&ctx);
    
    // Clean up
    goo_ast_free_minimal(node);
    goo_diag_context_free(ctx.diagnostics);
    
    return error_count == 1;
}

// Test type mismatch reporting
static bool test_type_mismatch() {
    printf("\nRunning type mismatch test...\n");
    
    // Create a minimal GooTypeContext for testing
    struct GooTypeContext ctx;
    ctx.diagnostics = goo_diag_context_new();
    
    // Create a minimal AST node
    GooAstNodeMinimal* node = goo_ast_create_minimal("test.goo", 20, 10, 15);
    
    // Use simple integer values to represent different types for testing
    struct GooType* expected_type = (struct GooType*)1; // int
    struct GooType* found_type = (struct GooType*)2;    // string
    
    // Report a type mismatch
    goo_type_report_mismatch(&ctx, node, expected_type, found_type);
    
    // Add a note
    GooAstNodeMinimal* note_node = goo_ast_create_minimal("test.goo", 20, 10, 15);
    
    goo_type_add_note(&ctx, note_node, "The variable was declared as int on line 5");
    
    // Add a suggestion
    GooAstNodeMinimal* sugg_node = goo_ast_create_minimal("test.goo", 20, 10, 15);
    
    goo_type_add_suggestion(&ctx, sugg_node, "Consider using string conversion", "int(myString)");
    
    // Check error count
    int error_count = goo_type_error_count(&ctx);
    printf("Error count: %d\n", error_count);
    
    // Print all diagnostics
    goo_type_print_diagnostics(&ctx);
    
    // Check if we should abort
    bool should_abort = goo_type_should_abort(&ctx);
    printf("Should abort: %s\n", should_abort ? "yes" : "no");
    
    // Clean up
    goo_ast_free_minimal(node);
    goo_ast_free_minimal(note_node);
    goo_ast_free_minimal(sugg_node);
    goo_diag_context_free(ctx.diagnostics);
    
    return error_count == 1 && !should_abort;
}

int main(void) {
    printf("Starting type checker diagnostics tests...\n\n");
    
    bool basic_test_passed = test_basic_error();
    bool mismatch_test_passed = test_type_mismatch();
    
    printf("\nTest summary:\n");
    printf("Basic error test: %s\n", basic_test_passed ? "PASSED" : "FAILED");
    printf("Type mismatch test: %s\n", mismatch_test_passed ? "PASSED" : "FAILED");
    
    return (basic_test_passed && mismatch_test_passed) ? 0 : 1;
} 