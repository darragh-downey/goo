/**
 * ast_node_unified.h
 * 
 * Simplified AST node interface for the unified type checker
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_AST_NODE_UNIFIED_H
#define GOO_AST_NODE_UNIFIED_H

// Node types
typedef enum GooNodeType {
    NODE_UNKNOWN = 0,
    NODE_EXPRESSION,
    NODE_STATEMENT,
    NODE_DECLARATION,
    NODE_IDENTIFIER,
    NODE_LITERAL,
    NODE_BINARY_EXPRESSION,
    NODE_UNARY_EXPRESSION,
    NODE_FUNCTION_CALL,
    NODE_ASSIGNMENT,
    NODE_IF_STATEMENT,
    NODE_WHILE_STATEMENT,
    NODE_FOR_STATEMENT,
    NODE_RETURN_STATEMENT,
    NODE_BLOCK,
    // Add more node types as needed
} GooNodeType;

// AST node structure
typedef struct GooAstNode {
    GooNodeType type;
    char* file;
    int line;
    int column;
    int length;
    // Additional fields would be added in a real implementation
} GooAstNode;

/**
 * Create a new AST node
 *
 * @param type The type of node
 * @param file Source file name
 * @param line Line number
 * @param column Column number
 * @param length Source span length
 * @return Newly created AST node
 */
GooAstNode* goo_ast_create_node(
    GooNodeType type,
    const char* file,
    int line,
    int column,
    int length
);

/**
 * Free an AST node
 *
 * @param node Node to free
 */
void goo_ast_free_node(GooAstNode* node);

#endif /* GOO_AST_NODE_UNIFIED_H */ 