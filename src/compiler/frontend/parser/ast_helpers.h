#ifndef GOO_AST_HELPERS_H
#define GOO_AST_HELPERS_H

#include "ast.h"

// Helper functions for building the AST during parsing

// Append a node to a linked list of nodes
GooNode* append_node(GooNode* list, GooNode* node);

// Create various node types during parsing
GooNode* create_identifier_node(const char* name, int line, int column);
GooNode* create_field_access_node(GooNode* expr, const char* field, int line, int column);
GooNode* create_index_expr_node(GooNode* expr, GooNode* index, int line, int column);
GooNode* create_range_literal_node(const char* range_str, int line, int column);

// Helper functions for parallel execution options
GooNode* create_range_option_node(const char* range_str, int line, int column);
GooNode* create_shared_var_node(const char* var_name, int line, int column);
GooNode* create_private_var_node(const char* var_name, int line, int column);

#endif // GOO_AST_HELPERS_H 