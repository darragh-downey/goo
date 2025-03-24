#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Helper function to append a node to the end of a linked list
GooNode* append_node(GooNode* list, GooNode* node) {
    if (!list) return node;
    
    GooNode* current = list;
    while (current->next) {
        current = current->next;
    }
    current->next = node;
    
    return list;
}

// Create an identifier node
GooNode* create_identifier_node(const char* name, int line, int column) {
    // Create an identifier node structure
    typedef struct {
        GooNode base;
        char* name;
    } GooIdentifierNode;
    
    GooIdentifierNode* node = malloc(sizeof(GooIdentifierNode));
    if (!node) return NULL;
    
    // Initialize the base node
    node->base.type = GOO_NODE_IDENTIFIER;
    node->base.line = line;
    node->base.column = column;
    node->base.next = NULL;
    
    // Copy the identifier name
    node->name = strdup(name);
    
    return (GooNode*)node;
}

// Create a field access node (e.g., a.b)
GooNode* create_field_access_node(GooNode* expr, const char* field, int line, int column) {
    // Implementation depends on your AST structure
    // This is a placeholder
    return NULL;
}

// Create an index expression node (e.g., a[i])
GooNode* create_index_expr_node(GooNode* expr, GooNode* index, int line, int column) {
    // Implementation depends on your AST structure
    // This is a placeholder
    return NULL;
}

// Create a range literal node (e.g., 1..10)
GooNode* create_range_literal_node(const char* range_str, int line, int column) {
    // Allocate memory for the range literal node
    GooRangeLiteralNode* node = malloc(sizeof(GooRangeLiteralNode));
    if (!node) return NULL;
    
    // Initialize the base node
    node->base.type = GOO_NODE_RANGE_LITERAL;
    node->base.line = line;
    node->base.column = column;
    node->base.next = NULL;
    
    // Parse the range string (e.g., "1..10") into start and end values
    char* dot_pos = strstr(range_str, "..");
    if (!dot_pos) {
        // If there's no ".." in the string, free the node and return NULL
        free(node);
        return NULL;
    }
    
    // Temporarily modify the string to split it
    *dot_pos = '\0';
    
    // Parse the start value
    int64_t start = atoll(range_str);
    
    // Parse the end value (skip the "..")
    int64_t end = atoll(dot_pos + 2);
    
    // Restore the original string
    *dot_pos = '.';
    
    // Use the AST function to create the node
    free(node); // Free our temporary node
    return (GooNode*)goo_ast_create_range_literal_node(start, end, line, column);
}

// Create a range option node for parallel execution
GooNode* create_range_option_node(const char* range_str, int line, int column) {
    // Parse the range string similar to range literal node
    char* dot_pos = strstr(range_str, "..");
    if (!dot_pos) {
        return NULL;
    }
    
    // Temporarily modify the string to split it
    *dot_pos = '\0';
    
    // Parse the start and end values
    int64_t start = atoll(range_str);
    int64_t end = atoll(dot_pos + 2);
    
    // Restore the original string
    *dot_pos = '.';
    
    // Create a range literal node to represent the range option
    return (GooNode*)goo_ast_create_range_literal_node(start, end, line, column);
}

// Create a shared variable node for parallel execution
GooNode* create_shared_var_node(const char* var_name, int line, int column) {
    // Create an identifier node with special markings for shared variables
    // For now, we'll just create a regular identifier node
    return create_identifier_node(var_name, line, column);
}

// Create a private variable node for parallel execution
GooNode* create_private_var_node(const char* var_name, int line, int column) {
    // Create an identifier node with special markings for private variables
    // For now, we'll just create a regular identifier node
    return create_identifier_node(var_name, line, column);
} 