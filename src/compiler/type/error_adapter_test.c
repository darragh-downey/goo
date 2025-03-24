/**
 * error_adapter_test.c
 * 
 * Test file for the type error adapter in the Goo type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "type_error_adapter.h"
#include "ast_simple.h"
#include "type_error_codes.h"

// Create a simple AST node for testing
static GooAstNode* create_test_node(const char* file, int line, int column, int length) {
    GooAstNode* node = (GooAstNode*)malloc(sizeof(GooAstNode));
    node->type = GOO_NODE_IDENTIFIER;
    node->file = file;
    node->line = line;
    node->column = column;
    node->length = length;
    node->node_type = NULL;
    node->next = NULL;
    return node;
}

// Create a simple type context with a diagnostics context
static GooTypeContext* create_test_context(void) {
    GooTypeContext* ctx = (GooTypeContext*)malloc(sizeof(GooTypeContext));
    memset(ctx, 0, sizeof(GooTypeContext));
    
    // Create a new diagnostics context
    ctx->diagnostics = goo_type_init_diagnostics();
    
    return ctx;
}

// Create a simple type for testing
static GooType* create_test_type(GooTypeKind kind, const char* name) {
    GooType* type = (GooType*)malloc(sizeof(GooType));
    memset(type, 0, sizeof(GooType));
    
    type->kind = kind;
    
    // Set additional fields based on kind
    if (kind == GOO_TYPE_STRUCT) {
        type->struct_info.name = strdup(name);
    }
    
    return type;
}

// Test basic error reporting
void test_basic_error_reporting(void) {
    printf("Testing basic error reporting...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create a test node
    GooAstNode* node = create_test_node("test.goo", 10, 5, 3);
    
    // Report a simple error
    goo_type_report_error(ctx, node, GOO_ERR_TYPE_MISMATCH, "Type mismatch in expression");
    
    // Check error count
    int error_count = goo_type_error_count(ctx);
    printf("Error count: %d\n", error_count);
    assert(error_count == 1);
    
    // Print the diagnostics
    printf("Diagnostics after error reporting:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    free(node);
    goo_diag_context_free(ctx->diagnostics);
    free(ctx);
}

// Test type mismatch reporting
void test_type_mismatch_reporting(void) {
    printf("\nTesting type mismatch reporting...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create a test node
    GooAstNode* node = create_test_node("test.goo", 15, 10, 8);
    
    // Create test types
    GooType* expected_type = create_test_type(GOO_TYPE_INT, "int");
    GooType* found_type = create_test_type(GOO_TYPE_STRING, "string");
    
    // Report a type mismatch
    goo_type_report_mismatch(ctx, node, expected_type, found_type);
    
    // Check error count
    int error_count = goo_type_error_count(ctx);
    printf("Error count: %d\n", error_count);
    assert(error_count == 1);
    
    // Print the diagnostics
    printf("Diagnostics after type mismatch reporting:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    free(node);
    if (found_type->kind == GOO_TYPE_STRUCT) free(found_type->struct_info.name);
    free(found_type);
    if (expected_type->kind == GOO_TYPE_STRUCT) free(expected_type->struct_info.name);
    free(expected_type);
    goo_diag_context_free(ctx->diagnostics);
    free(ctx);
}

// Test notes and suggestions
void test_notes_and_suggestions(void) {
    printf("\nTesting notes and suggestions...\n");
    
    // Create a test context
    GooTypeContext* ctx = create_test_context();
    
    // Create test nodes
    GooAstNode* error_node = create_test_node("test.goo", 20, 5, 10);
    GooAstNode* note_node = create_test_node("test.goo", 18, 5, 15);
    GooAstNode* suggestion_node = create_test_node("test.goo", 20, 5, 10);
    
    // Report an error
    goo_type_report_error(ctx, error_node, GOO_ERR_UNDEFINED_VARIABLE, "Variable 'foo' is not defined");
    
    // Add a note
    goo_type_add_note(ctx, note_node, "Did you mean to define 'foo' earlier?");
    
    // Add a suggestion
    goo_type_add_suggestion(ctx, suggestion_node, "Consider defining the variable first", "let foo = 42;");
    
    // Check counts
    int error_count = goo_type_error_count(ctx);
    printf("Error count: %d\n", error_count);
    
    // Print the diagnostics
    printf("Diagnostics after notes and suggestions:\n");
    goo_type_print_diagnostics(ctx);
    
    // Clean up
    free(error_node);
    free(note_node);
    free(suggestion_node);
    goo_diag_context_free(ctx->diagnostics);
    free(ctx);
}

int main(void) {
    printf("=== Type Error Adapter Tests ===\n\n");
    
    // Run tests
    test_basic_error_reporting();
    test_type_mismatch_reporting();
    test_notes_and_suggestions();
    
    printf("\nAll tests completed.\n");
    return 0;
} 