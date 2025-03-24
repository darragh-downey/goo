/**
 * ast_node_minimal.c
 * 
 * Implementation of minimal AST node functions for use with type error reporting
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdlib.h>
#include <string.h>
#include "ast_node_minimal.h"

/**
 * Convert a full AST node to the minimal version
 * In a real implementation, this would extract the required fields
 * from the full AST node and create a new minimal node.
 */
GooAstNodeMinimal* goo_ast_to_minimal(void* full_node) {
    // Check for null input
    if (!full_node) {
        return NULL;
    }
    
    // This is a simplified implementation that extracts basic diagnostic info
    // from the full AST node structure
    
    // Define a basic AST node structure to allow accessing common fields
    // This is a minimal version that matches the fields we know are common
    typedef struct {
        int type;            // Node type 
        const char* file;    // Source file
        int line;            // Line number
        int column;          // Column number
        int length;          // Length of the token/expression
        void* next;          // Next node in list
    } BasicAstNode;
    
    // Cast to the basic structure to access fields
    BasicAstNode* node = (BasicAstNode*)full_node;
    
    // Create a new minimal node
    GooAstNodeMinimal* minimal = (GooAstNodeMinimal*)malloc(sizeof(GooAstNodeMinimal));
    if (!minimal) {
        return NULL;
    }
    
    // Copy needed fields
    minimal->type = node->type;
    minimal->file = node->file ? strdup(node->file) : NULL;
    minimal->line = node->line;
    minimal->column = node->column;
    minimal->length = node->length;
    minimal->next = NULL; // We don't copy the next pointer
    
    return minimal;
}

/**
 * Create a minimal AST node for testing
 */
GooAstNodeMinimal* goo_ast_create_minimal(const char* file, int line, int column, int length) {
    GooAstNodeMinimal* node = (GooAstNodeMinimal*)malloc(sizeof(GooAstNodeMinimal));
    
    node->type = 0;  // Default to an arbitrary type
    node->file = file ? strdup(file) : NULL;
    node->line = line;
    node->column = column;
    node->length = length;
    node->next = NULL;
    
    return node;
}

/**
 * Free a minimal AST node
 */
void goo_ast_free_minimal(GooAstNodeMinimal* node) {
    if (!node) {
        return;
    }
    
    if (node->file) {
        free((void*)node->file);
    }
    
    free(node);
} 