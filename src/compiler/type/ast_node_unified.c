/**
 * ast_node_unified.c
 * 
 * Implementation of the simplified AST node interface
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast_node_unified.h"

/**
 * Create a new AST node
 */
GooAstNode* goo_ast_create_node(
    GooNodeType type,
    const char* file,
    int line,
    int column,
    int length
) {
    GooAstNode* node = (GooAstNode*)malloc(sizeof(GooAstNode));
    if (node) {
        node->type = type;
        node->file = file ? strdup(file) : NULL;
        node->line = line;
        node->column = column;
        node->length = length;
    }
    return node;
}

/**
 * Free an AST node
 */
void goo_ast_free_node(GooAstNode* node) {
    if (node) {
        if (node->file) {
            free(node->file);
        }
        free(node);
    }
} 