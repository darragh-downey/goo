/**
 * ast_node_minimal.h
 * 
 * Minimal AST node definition for use with type error reporting
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_AST_NODE_MINIMAL_H
#define GOO_AST_NODE_MINIMAL_H

/**
 * Minimal version of GooAstNode that contains only the fields
 * needed for diagnostic reporting. This avoids conflicts with
 * the full AST definition.
 */
typedef struct GooAstNodeMinimal {
    int type;       // Node type enum
    const char* file;   // Source file
    int line;       // Line number
    int column;     // Column number
    int length;     // Length of the token/expression
    struct GooAstNodeMinimal* next;  // Next node in list
} GooAstNodeMinimal;

/**
 * Convert a full AST node to the minimal version
 * In a real implementation, this would convert from the full AST node type
 */
GooAstNodeMinimal* goo_ast_to_minimal(void* full_node);

/**
 * Create a minimal AST node for testing
 */
GooAstNodeMinimal* goo_ast_create_minimal(const char* file, int line, int column, int length);

/**
 * Free a minimal AST node
 */
void goo_ast_free_minimal(GooAstNodeMinimal* node);

#endif /* GOO_AST_NODE_MINIMAL_H */ 