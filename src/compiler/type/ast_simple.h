/**
 * ast_simple.h
 * 
 * Simplified AST definitions for the Goo type system tests
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_AST_SIMPLE_H
#define GOO_AST_SIMPLE_H

#include <stdint.h>
#include <stdbool.h>

// Forward declaration for location information
typedef struct GooSourceLoc {
    const char* filename;
    int line;
    int column;
} GooSourceLoc;

// Node kind enumeration
typedef enum {
    GOO_NODE_PROGRAM,
    GOO_NODE_FUNCTION_DECL,
    GOO_NODE_VAR_DECL,
    GOO_NODE_PARAM_DECL,
    GOO_NODE_BLOCK_STMT,
    GOO_NODE_EXPR_STMT,
    GOO_NODE_IF_STMT,
    GOO_NODE_FOR_STMT,
    GOO_NODE_WHILE_STMT,
    GOO_NODE_RETURN_STMT,
    GOO_NODE_BINARY_EXPR,
    GOO_NODE_UNARY_EXPR,
    GOO_NODE_CALL_EXPR,
    GOO_NODE_MEMBER_EXPR,
    GOO_NODE_INDEX_EXPR,
    GOO_NODE_LITERAL,
    GOO_NODE_IDENTIFIER,
    GOO_NODE_TYPE_REF
} GooNodeKind;

// Forward declaration for node type
typedef struct GooNode GooNode;
typedef struct GooAstNode GooAstNode;

// Node types
typedef enum {
    GOO_NODE_BINARY_EXPR,
    GOO_NODE_UNARY_EXPR,
    GOO_NODE_CALL_EXPR,
    GOO_NODE_VAR_DECL,
    GOO_NODE_FUNCTION_DECL,
    GOO_NODE_IF_STMT,
    GOO_NODE_FOR_STMT,
    GOO_NODE_WHILE_STMT,
    GOO_NODE_RETURN_STMT,
    GOO_NODE_BLOCK_STMT,
    GOO_NODE_CHANNEL_SEND,
    GOO_NODE_CHANNEL_RECV,
    GOO_NODE_INT_LITERAL,
    GOO_NODE_FLOAT_LITERAL,
    GOO_NODE_BOOL_LITERAL,
    GOO_NODE_STRING_LITERAL,
    GOO_NODE_IDENTIFIER
} GooAstNodeType;

// Operator types
typedef enum {
    GOO_OP_ADD = '+',
    GOO_OP_SUB = '-',
    GOO_OP_MUL = '*',
    GOO_OP_DIV = '/',
    GOO_OP_MOD = '%',
    GOO_OP_EQ = '=',
    GOO_OP_LT = '<',
    GOO_OP_GT = '>',
    GOO_OP_NOT = '!',
    GOO_OP_REF = '&',
    // Special operators (beyond ASCII range)
    GOO_OP_EQ_EQ = 256,    // ==
    GOO_OP_NEQ,            // !=
    GOO_OP_LTE,            // <=
    GOO_OP_GTE,            // >=
    GOO_OP_AND,            // &&
    GOO_OP_OR,             // ||
    GOO_OP_MUT_REF         // &mut
} GooOperator;

// Base AST node structure
typedef struct GooAstNode {
    GooAstNodeType type;          // Node type
    const char* file;             // Source file
    int line;                     // Line number
    int column;                   // Column number
    int length;                   // Node length
    struct GooType* node_type;    // Type of the node (filled during type checking)
    struct GooAstNode* next;      // Next node in a list (for statements, args, etc.)
} GooAstNode;

// Binary expression node
typedef struct {
    GooAstNode base;              // Base node
    GooOperator operator;         // Binary operator
    GooAstNode* left;             // Left operand
    GooAstNode* right;            // Right operand
} GooBinaryExprNode;

// Unary expression node
typedef struct {
    GooAstNode base;              // Base node
    GooOperator operator;         // Unary operator
    GooAstNode* operand;          // Operand
} GooUnaryExprNode;

// Function call node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* func;             // Function expression
    GooAstNode* args;             // Arguments (linked list)
} GooCallExprNode;

// Variable declaration node
typedef struct {
    GooAstNode base;              // Base node
    const char* name;             // Variable name
    GooAstNode* type_expr;        // Type expression (optional)
    GooAstNode* init_expr;        // Initialization expression
    bool is_mutable;              // Whether the variable is mutable
} GooVarDeclNode;

// Function declaration node
typedef struct {
    GooAstNode base;              // Base node
    const char* name;             // Function name
    GooAstNode* params;           // Parameters (linked list)
    GooAstNode* return_type;      // Return type expression
    GooAstNode* body;             // Function body
    bool is_unsafe;               // Whether the function is unsafe
    bool is_kernel;               // Whether the function is a kernel function
} GooFunctionDeclNode;

// If statement node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* condition;        // Condition expression
    GooAstNode* then_block;       // Then block
    GooAstNode* else_block;       // Else block (optional)
} GooIfStmtNode;

// For statement node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* init;             // Initialization expression
    GooAstNode* condition;        // Condition expression
    GooAstNode* update;           // Update expression
    GooAstNode* body;             // Loop body
    bool is_range;                // Whether this is a range-based for loop
} GooForStmtNode;

// While statement node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* condition;        // Condition expression
    GooAstNode* body;             // Loop body
} GooWhileStmtNode;

// Return statement node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* expr;             // Return expression (optional)
} GooReturnStmtNode;

// Block statement node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* statements;       // Statements (linked list)
} GooBlockStmtNode;

// Channel send node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* channel;          // Channel expression
    GooAstNode* value;            // Value to send
} GooChannelSendNode;

// Channel receive node
typedef struct {
    GooAstNode base;              // Base node
    GooAstNode* channel;          // Channel expression
} GooChannelRecvNode;

// Integer literal node
typedef struct {
    GooAstNode base;              // Base node
    long long value;              // Integer value
} GooIntLiteralNode;

// Float literal node
typedef struct {
    GooAstNode base;              // Base node
    double value;                 // Float value
} GooFloatLiteralNode;

// Boolean literal node
typedef struct {
    GooAstNode base;              // Base node
    bool value;                   // Boolean value
} GooBoolLiteralNode;

// String literal node
typedef struct {
    GooAstNode base;              // Base node
    const char* value;            // String value
} GooStringLiteralNode;

// Identifier node
typedef struct {
    GooAstNode base;              // Base node
    const char* name;             // Identifier name
} GooIdentifierNode;

// Helper functions
GooAstNode* goo_ast_create_int_literal(long long value);
GooAstNode* goo_ast_create_float_literal(double value);
GooAstNode* goo_ast_create_bool_literal(bool value);
GooAstNode* goo_ast_create_string_literal(const char* value);
GooAstNode* goo_ast_create_identifier(const char* name);
GooAstNode* goo_ast_create_binary_expr(GooAstNode* left, GooOperator op, GooAstNode* right);
GooAstNode* goo_ast_create_unary_expr(GooOperator op, GooAstNode* operand);
GooAstNode* goo_ast_create_call_expr(GooAstNode* func, GooAstNode* args);
GooAstNode* goo_ast_create_var_decl(const char* name, GooAstNode* type_expr, GooAstNode* init_expr, bool is_mutable);
GooAstNode* goo_ast_create_function_decl(const char* name, GooAstNode* params, GooAstNode* return_type, GooAstNode* body, bool is_unsafe, bool is_kernel);
GooAstNode* goo_ast_create_if_stmt(GooAstNode* condition, GooAstNode* then_block, GooAstNode* else_block);
GooAstNode* goo_ast_create_for_stmt(GooAstNode* init, GooAstNode* condition, GooAstNode* update, GooAstNode* body, bool is_range);
GooAstNode* goo_ast_create_while_stmt(GooAstNode* condition, GooAstNode* body);
GooAstNode* goo_ast_create_return_stmt(GooAstNode* expr);
GooAstNode* goo_ast_create_block_stmt(GooAstNode* statements);
GooAstNode* goo_ast_create_channel_send(GooAstNode* channel, GooAstNode* value);
GooAstNode* goo_ast_create_channel_recv(GooAstNode* channel);

// Free an AST node (and all its children)
void goo_ast_free_node(GooAstNode* node);

#endif /* GOO_AST_SIMPLE_H */ 