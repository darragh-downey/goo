/**
 * type_error_test.c
 * 
 * Test file for the type error diagnostics in the Goo type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "goo_type_system.h"
#include "goo_type_utils.h"
#include "type_error_codes.h"
#include "ast_simple.h"
#include "../../../include/goo_diagnostics.h"

// Helper function to test type mismatches
void test_type_mismatch(GooDiagnosticContext* diag_ctx) {
    printf("Testing type mismatch errors...\n");
    
    // Create type system
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    // Create types for testing
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* string_type = goo_type_system_create_string_type(ctx);
    
    // Create a binary expression AST node for testing (int + string)
    GooBinaryExprNode* bin_expr = (GooBinaryExprNode*)malloc(sizeof(GooBinaryExprNode));
    bin_expr->base.type = GOO_NODE_BINARY_EXPR;
    bin_expr->base.file = "test.goo";
    bin_expr->base.line = 10;
    bin_expr->base.column = 5;
    bin_expr->base.length = 3;
    bin_expr->operator = '+';
    
    // Create left operand (int literal)
    GooIntLiteralNode* left = (GooIntLiteralNode*)malloc(sizeof(GooIntLiteralNode));
    left->base.type = GOO_NODE_INT_LITERAL;
    left->base.file = "test.goo";
    left->base.line = 10;
    left->base.column = 5;
    left->base.length = 1;
    left->value = 42;
    
    // Create right operand (string literal)
    GooStringLiteralNode* right = (GooStringLiteralNode*)malloc(sizeof(GooStringLiteralNode));
    right->base.type = GOO_NODE_STRING_LITERAL;
    right->base.file = "test.goo";
    right->base.line = 10;
    right->base.column = 9;
    right->base.length = 7;
    right->value = "hello";
    
    // Link nodes
    bin_expr->left = (GooAstNode*)left;
    bin_expr->right = (GooAstNode*)right;
    
    // Simulate type checking binary expression
    // This should report a type mismatch error
    GooType* result = goo_type_system_check_binary_expr(ctx, (GooAstNode*)bin_expr);
    
    // Print diagnostic results
    printf("Diagnostics after type checking:\n");
    goo_diag_print_all(diag_ctx);
    printf("Error count: %d\n", goo_diag_error_count(diag_ctx));
    
    // Clean up
    free(right);
    free(left);
    free(bin_expr);
    goo_type_system_destroy(ctx);
}

// Helper function to test non-boolean conditions
void test_non_boolean_condition(GooDiagnosticContext* diag_ctx) {
    printf("\nTesting non-boolean condition errors...\n");
    
    // Create type system
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    // Create an if statement with a non-boolean condition
    GooIfStmtNode* if_stmt = (GooIfStmtNode*)malloc(sizeof(GooIfStmtNode));
    if_stmt->base.type = GOO_NODE_IF_STMT;
    if_stmt->base.file = "test.goo";
    if_stmt->base.line = 15;
    if_stmt->base.column = 1;
    if_stmt->base.length = 20;
    
    // Create condition (int literal)
    GooIntLiteralNode* cond = (GooIntLiteralNode*)malloc(sizeof(GooIntLiteralNode));
    cond->base.type = GOO_NODE_INT_LITERAL;
    cond->base.file = "test.goo";
    cond->base.line = 15;
    cond->base.column = 4;
    cond->base.length = 1;
    cond->value = 1;
    
    // Create empty then block
    GooBlockStmtNode* then_block = (GooBlockStmtNode*)malloc(sizeof(GooBlockStmtNode));
    then_block->base.type = GOO_NODE_BLOCK_STMT;
    then_block->base.file = "test.goo";
    then_block->base.line = 15;
    then_block->base.column = 6;
    then_block->base.length = 2;
    then_block->statements = NULL;
    
    // Link nodes
    if_stmt->condition = (GooAstNode*)cond;
    if_stmt->then_block = (GooAstNode*)then_block;
    if_stmt->else_block = NULL;
    
    // Simulate type checking if statement
    // This should report a non-boolean condition error
    goo_type_system_check_if_stmt(ctx, (GooAstNode*)if_stmt);
    
    // Print diagnostic results
    printf("Diagnostics after type checking:\n");
    goo_diag_print_all(diag_ctx);
    printf("Error count: %d\n", goo_diag_error_count(diag_ctx));
    
    // Clean up
    free(then_block);
    free(cond);
    free(if_stmt);
    goo_type_system_destroy(ctx);
}

int main() {
    // Create diagnostics context
    GooDiagnosticContext* diag_ctx = goo_diag_context_new();
    
    // Run tests
    test_type_mismatch(diag_ctx);
    test_non_boolean_condition(diag_ctx);
    
    // Clean up
    goo_diag_context_free(diag_ctx);
    
    return 0;
} 