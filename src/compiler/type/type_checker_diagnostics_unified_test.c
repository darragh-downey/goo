/**
 * type_checker_diagnostics_unified_test.c
 *
 * Unified test for the type checker diagnostics integration
 *
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ast_node_unified.h"
#include "type_checker_adapter.h"
#include "type_error_codes.h"

// Mock types
#define TYPE_UNKNOWN 0
#define TYPE_INT     1
#define TYPE_FLOAT   2
#define TYPE_STRING  3
#define TYPE_BOOL    4
#define TYPE_FUNC    5
#define TYPE_ARRAY   6
#define TYPE_STRUCT  7

// Mock type system context
typedef struct GooTypeContext {
    int dummy;  // Just a placeholder
} GooTypeContext;

// Mock type structure
typedef struct GooType {
    int kind;
    char* name;
    // Other fields would be here in a real implementation
} GooType;

// Create a test type
static GooType* create_test_type(int kind, const char* name) {
    GooType* type = (GooType*)malloc(sizeof(GooType));
    type->kind = kind;
    type->name = name ? strdup(name) : NULL;
    return type;
}

// Free a test type
static void free_test_type(GooType* type) {
    if (type) {
        if (type->name) free(type->name);
        free(type);
    }
}

// Mock type-to-string conversion
static char* test_type_to_string(const GooType* type) {
    if (!type) return strdup("<null>");
    
    switch (type->kind) {
        case TYPE_INT:    return strdup("int");
        case TYPE_FLOAT:  return strdup("float");
        case TYPE_STRING: return strdup("string");
        case TYPE_BOOL:   return strdup("bool");
        case TYPE_FUNC:   return strdup("function");
        case TYPE_ARRAY:  return strdup("array");
        case TYPE_STRUCT: return strdup("struct");
        default:          return strdup("unknown");
    }
}

// Create a test node
static GooAstNode* create_test_node(GooNodeType type, const char* file, int line, int column) {
    return goo_ast_create_node(type, file, line, column, 1);
}

// Test 1: Basic error reporting
static void test_basic_error_reporting(void) {
    printf("\n--- Test: Basic Error Reporting ---\n");
    
    // Create type context
    GooTypeContext type_ctx = { 0 };
    
    // Create type checker context
    GooTypeCheckerContext* ctx = goo_typechecker_create_context(&type_ctx);
    goo_typechecker_register_type_to_string(ctx, test_type_to_string);
    
    // Create a test node
    GooAstNode* node = create_test_node(NODE_EXPRESSION, "test.goo", 10, 15);
    
    // Report a type error
    goo_typechecker_report_error(ctx, node, GOO_ERR_TYPE_MISMATCH, "Type mismatch in expression");
    
    // Check error count
    int errors = goo_typechecker_get_error_count(ctx);
    printf("Error count: %d (expected: 1)\n", errors);
    
    // Print diagnostics
    goo_typechecker_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_node(node);
    goo_typechecker_free_context(ctx);
}

// Test 2: Type mismatch reporting
static void test_type_mismatch_reporting(void) {
    printf("\n--- Test: Type Mismatch Reporting ---\n");
    
    // Create type context
    GooTypeContext type_ctx = { 0 };
    
    // Create type checker context
    GooTypeCheckerContext* ctx = goo_typechecker_create_context(&type_ctx);
    goo_typechecker_register_type_to_string(ctx, test_type_to_string);
    
    // Create a test node
    GooAstNode* node = create_test_node(NODE_ASSIGNMENT, "test.goo", 15, 10);
    
    // Create types for testing
    GooType* expected_type = create_test_type(TYPE_INT, "int");
    GooType* found_type = create_test_type(TYPE_STRING, "string");
    
    // Report a type mismatch
    goo_typechecker_report_type_mismatch(ctx, node, expected_type, found_type);
    goo_typechecker_add_note(ctx, "Assignment requires compatible types");
    goo_typechecker_add_suggestion(ctx, "Try using a type conversion function");
    
    // Check error count
    int errors = goo_typechecker_get_error_count(ctx);
    printf("Error count: %d (expected: 1)\n", errors);
    
    // Print diagnostics
    goo_typechecker_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_node(node);
    free_test_type(expected_type);
    free_test_type(found_type);
    goo_typechecker_free_context(ctx);
}

// Test 3: Complex error scenario
static void test_complex_error_scenario(void) {
    printf("\n--- Test: Complex Error Scenario ---\n");
    
    // Create type context
    GooTypeContext type_ctx = { 0 };
    
    // Create type checker context
    GooTypeCheckerContext* ctx = goo_typechecker_create_context(&type_ctx);
    goo_typechecker_register_type_to_string(ctx, test_type_to_string);
    
    // Create test nodes
    GooAstNode* assign_node = create_test_node(NODE_ASSIGNMENT, "complex.goo", 20, 5);
    GooAstNode* call_node = create_test_node(NODE_FUNCTION_CALL, "complex.goo", 25, 10);
    
    // Create types for testing
    GooType* int_type = create_test_type(TYPE_INT, "int");
    GooType* string_type = create_test_type(TYPE_STRING, "string");
    GooType* bool_type = create_test_type(TYPE_BOOL, "bool");
    GooType* func_type = create_test_type(TYPE_FUNC, "function");
    
    // Report multiple errors
    goo_typechecker_report_type_mismatch(ctx, assign_node, int_type, string_type);
    goo_typechecker_add_note(ctx, "Variable 'count' is declared as integer");
    goo_typechecker_add_suggestion(ctx, "Use the 'parse_int' function to convert string to int");
    
    goo_typechecker_report_error(ctx, call_node, GOO_ERR_INVALID_OPERANDS, 
                                "Cannot call non-function type");
    goo_typechecker_add_note(ctx, "Expression is of type 'bool'");
    goo_typechecker_add_suggestion(ctx, "Check that you're using the correct variable name");
    
    // Check error count
    int errors = goo_typechecker_get_error_count(ctx);
    printf("Error count: %d (expected: 2)\n", errors);
    
    // Print diagnostics
    goo_typechecker_print_diagnostics(ctx);
    
    // Clean up
    goo_ast_free_node(assign_node);
    goo_ast_free_node(call_node);
    free_test_type(int_type);
    free_test_type(string_type);
    free_test_type(bool_type);
    free_test_type(func_type);
    goo_typechecker_free_context(ctx);
}

int main(void) {
    printf("Type Checker Diagnostics Unified Test\n");
    printf("=====================================\n");
    
    // Run the tests
    test_basic_error_reporting();
    test_type_mismatch_reporting();
    test_complex_error_scenario();
    
    printf("\nAll tests completed.\n");
    return 0;
} 