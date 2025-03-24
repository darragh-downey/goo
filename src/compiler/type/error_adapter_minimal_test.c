/**
 * error_adapter_minimal_test.c
 * 
 * Test file for the minimal type error adapter in the Goo type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "error_adapter_minimal.h"
#include "ast_node_minimal.h"
#include "type_error_codes.h"

// Sample source code for displaying in diagnostics
const char* sample_code =
"function add(a: int, b: int): int {\n"
"    return a + b;\n"
"}\n"
"\n"
"function main() {\n"
"    let x = 42;\n"
"    let y = \"hello\";\n"
"    let z = add(x, y);  // Type error: string passed where int expected\n"
"    \n"
"    if (z) {           // Type error: condition must be boolean\n"
"        print(z);\n"
"    }\n"
"}\n";

// Create a simple type context with a diagnostics context
static GooTypeContext* create_test_context(void) {
    GooTypeContext* ctx = (GooTypeContext*)malloc(sizeof(GooTypeContext));
    memset(ctx, 0, sizeof(GooTypeContext));
    
    // Create a new diagnostics context
    ctx->diagnostics = goo_type_init_diagnostics();
    
    return ctx;
}

// Test basic error reporting
void test_basic_error_reporting(void) {
    printf("Testing basic error reporting...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create a test node
    GooAstNodeMinimal* node = goo_ast_create_minimal("test.goo", 10, 5, 3);
    
    // Report a simple error
    goo_type_report_error_minimal(ctx, node, GOO_ERR_TYPE_MISMATCH, "Type mismatch in expression");
    
    // Check error count
    int error_count = goo_type_error_count(ctx);
    printf("Error count: %d\n", error_count);
    assert(error_count == 1);
    
    // Print the diagnostics
    printf("Diagnostics after error reporting:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_minimal(node);
    if (ctx && ctx->diagnostics) {
        free(ctx->diagnostics);
    }
    free(ctx);
}

// Test type mismatch reporting
void test_type_mismatch_reporting(void) {
    printf("\nTesting type mismatch reporting...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create a test node
    GooAstNodeMinimal* node = goo_ast_create_minimal("test.goo", 15, 10, 8);
    
    // Create type strings
    const char* expected_type = "int";
    const char* found_type = "string";
    
    // Report a type mismatch
    goo_type_report_mismatch_minimal(ctx, node, expected_type, found_type);
    
    // Check error count
    int error_count = goo_type_error_count(ctx);
    printf("Error count: %d\n", error_count);
    assert(error_count == 1);
    
    // Print the diagnostics
    printf("Diagnostics after type mismatch reporting:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_minimal(node);
    if (ctx && ctx->diagnostics) {
        free(ctx->diagnostics);
    }
    free(ctx);
}

// Test a complex error scenario
void test_complex_error_scenario(void) {
    printf("\nTesting complex error scenario...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // 1. Report function argument type mismatch
    GooAstNodeMinimal* arg_node = goo_ast_create_minimal("example.goo", 8, 16, 1);
    goo_type_report_mismatch_minimal(ctx, arg_node, "int", "string");
    
    // 2. Add a suggestion for fixing the mismatch
    goo_type_add_suggestion_minimal(ctx, arg_node, "Try converting the string to an integer", "to_int(y)");
    
    // 3. Report non-boolean condition
    GooAstNodeMinimal* condition_node = goo_ast_create_minimal("example.goo", 10, 9, 1);
    goo_type_report_error_minimal(ctx, condition_node, GOO_ERR_NON_BOOLEAN_CONDITION, 
                                "Non-boolean condition: 'int' used where a boolean is required");
    
    // 4. Add a suggestion for fixing the condition
    goo_type_add_suggestion_minimal(ctx, condition_node, "Try using a comparison", "if (z != 0)");
    
    // Print the diagnostics with source code
    printf("Diagnostics for complex error scenario:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_minimal(arg_node);
    goo_ast_free_minimal(condition_node);
    if (ctx && ctx->diagnostics) {
        free(ctx->diagnostics);
    }
    free(ctx);
}

int main(void) {
    printf("=== Minimal Type Error Adapter Tests ===\n\n");
    
    // Run tests
    test_basic_error_reporting();
    test_type_mismatch_reporting();
    test_complex_error_scenario();
    
    printf("\nAll tests completed.\n");
    return 0;
} 