#ifndef GOO_AST_H
#define GOO_AST_H

#include <stdbool.h>
#include <stdint.h>
#include "goo_core.h"

// Node types in our AST
typedef enum {
    GOO_NODE_PACKAGE_DECL,        // Package declaration
    GOO_NODE_IMPORT_DECL,         // Import declaration
    GOO_NODE_FUNCTION_DECL,       // Function declaration
    GOO_NODE_KERNEL_FUNC_DECL,    // Kernel function declaration
    GOO_NODE_USER_FUNC_DECL,      // User function declaration 
    GOO_NODE_VAR_DECL,            // Variable declaration
    GOO_NODE_MODULE_DECL,         // Module declaration
    GOO_NODE_ALLOCATOR_DECL,      // Allocator declaration
    GOO_NODE_CHANNEL_DECL,        // Channel declaration
    GOO_NODE_PARAM,               // Function parameter
    GOO_NODE_PARAM_LIST,          // Function parameter list
    GOO_NODE_TYPE_EXPR,           // Type expression
    GOO_NODE_CAP_TYPE_EXPR,       // Capability type expression
    GOO_NODE_BLOCK_STMT,          // Block statement
    GOO_NODE_IF_STMT,             // If statement
    GOO_NODE_FOR_STMT,            // For statement
    GOO_NODE_RETURN_STMT,         // Return statement
    GOO_NODE_GO_STMT,             // Go statement
    GOO_NODE_GO_PARALLEL_STMT,    // Go parallel statement 
    GOO_NODE_SUPERVISE_STMT,      // Supervise statement
    GOO_NODE_TRY_STMT,            // Try statement
    GOO_NODE_RECOVER_BLOCK,       // Recover block
    GOO_NODE_CHANNEL_SEND,        // Channel send operation
    GOO_NODE_CHANNEL_RECV,        // Channel receive operation
    GOO_NODE_EXPR_STMT,           // Expression statement
    GOO_NODE_BINARY_EXPR,         // Binary expression
    GOO_NODE_UNARY_EXPR,          // Unary expression
    GOO_NODE_CALL_EXPR,           // Function call expression
    GOO_NODE_ALLOC_EXPR,          // Allocation expression
    GOO_NODE_FREE_EXPR,           // Free expression
    GOO_NODE_SCOPE_BLOCK,         // Scope block
    GOO_NODE_COMPTIME_VAR_DECL,   // Compile-time variable declaration
    GOO_NODE_COMPTIME_BUILD_DECL, // Compile-time build declaration
    GOO_NODE_COMPTIME_SIMD_DECL,  // Compile-time SIMD declaration
    GOO_NODE_SIMD_TYPE_DECL,      // SIMD type declaration
    GOO_NODE_SIMD_OP_DECL,        // SIMD operation declaration
    GOO_NODE_SUPER_EXPR,          // Super expression
    GOO_NODE_IDENTIFIER,          // Identifier
    GOO_NODE_INT_LITERAL,         // Integer literal
    GOO_NODE_FLOAT_LITERAL,       // Float literal
    GOO_NODE_BOOL_LITERAL,        // Boolean literal
    GOO_NODE_STRING_LITERAL,      // String literal
    GOO_NODE_RANGE_LITERAL,       // Range literal
    GOO_NODE_PACKAGE,             // Package declaration
    GOO_NODE_IMPORT,              // Import declaration
    GOO_NODE_ROOT,                // Root node
    GOO_NODE_MODULE,              // Module node
    GOO_NODE_FUNCTION,            // Function node
    GOO_NODE_STATEMENT,           // Statement node
    GOO_NODE_EXPRESSION,          // Expression node
    GOO_NODE_LITERAL,             // Literal node
    GOO_NODE_WHILE_STMT,          // While statement
    GOO_NODE_EXPORT,              // Export node
    GOO_NODE_VARIABLE_DECL,       // Variable declaration node
    GOO_NODE_GO_PARALLEL,         // Go parallel node
    GOO_NODE_TYPE,                // Type node
    GOO_NODE_CHANNEL,             // Channel node
    GOO_NODE_SEND,                // Send node
    GOO_NODE_RECEIVE,             // Receive node
    // Add more node types as needed
} GooNodeType;

// Base AST node structure
typedef struct GooNode {
    GooNodeType type;
    uint32_t line;
    uint32_t column;
    struct GooNode* next;  // For linked lists of nodes
} GooNode;

// Package declaration node
typedef struct {
    GooNode base;
    char* name;
} GooPackageNode;

// Import declaration node
typedef struct {
    GooNode base;
    char* path;               // Import path
} GooImportNode;

// Function declaration node
typedef struct {
    GooNode base;
    char* name;
    struct GooNode* params;    // Parameter list
    struct GooNode* return_type;
    struct GooNode* body;      // Function body block
    bool is_kernel;            // Is this a kernel function
    bool is_user;              // Is this a user function
    bool is_unsafe;            // Is this function unsafe
    struct GooNode* allocator; // Optional allocator parameter
} GooFunctionNode;

// Channel declaration node with pattern
typedef struct {
    GooNode base;
    char* name;
    GooChannelPattern pattern;
    struct GooNode* element_type;
    char* endpoint;            // Optional endpoint string (for distributed channels)
    bool has_capability;       // Whether this channel has capability restrictions
} GooChannelDeclNode;

// Variable declaration node
typedef struct {
    GooNode base;
    char* name;
    struct GooNode* type;
    struct GooNode* init_expr;
    bool is_safe;              // For 'safe var' declarations
    bool is_comptime;          // For 'comptime var' declarations
    struct GooNode* allocator; // Optional allocator for this variable
} GooVarDeclNode;

// Range literal node
typedef struct {
    GooNode base;
    int64_t start;  // Start value of the range
    int64_t end;    // End value of the range
} GooRangeLiteralNode;

// Comptime build declaration node
typedef struct {
    GooNode base;
    struct GooNode* block;    // Block of build declarations
} GooComptimeBuildNode;

// Integer literal node
typedef struct {
    GooNode base;
    int64_t value;
} GooIntLiteralNode;

// Float literal node
typedef struct {
    GooNode base;
    double value;
} GooFloatLiteralNode;

// Boolean literal node
typedef struct {
    GooNode base;
    bool value;
} GooBoolLiteralNode;

// String literal node
typedef struct {
    GooNode base;
    char* value;
} GooStringLiteralNode;

// Channel send node
typedef struct {
    GooNode base;
    struct GooNode* channel;   // Channel expression
    struct GooNode* value;     // Value expression
} GooChannelSendNode;

// Channel receive node
typedef struct {
    GooNode base;
    struct GooNode* channel;   // Channel expression
} GooChannelRecvNode;

// Go statement (goroutine)
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression to run in a goroutine
} GooGoStmtNode;

// Go parallel block
typedef struct {
    GooNode base;
    struct GooNode* body;      // Block to run in parallel
    struct GooNode* options;   // Parallel options (range, shared/private vars)
} GooGoParallelNode;

// Supervise statement
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression to supervise
} GooSuperviseStmtNode;

// Try statement
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression to try
    char* error_type;          // Optional error type name
    struct GooNode* recover_block; // Optional recover block
} GooTryStmtNode;

// Module declaration
typedef struct {
    GooNode base;
    char* name;
    struct GooNode* declarations; // Declarations in the module
} GooModuleDeclNode;

// Type expression node
typedef struct {
    GooNode base;
    GooNodeType type_kind;     // What kind of type
    struct GooNode* elem_type; // For parameterized types (e.g., channels)
    bool is_capability;        // Whether this is a capability type
} GooTypeNode;

// Allocator declaration node
typedef struct {
    GooNode base;
    char* name;
    GooAllocatorType type;     // Type of allocator
    struct GooNode* options;   // Optional configuration options
} GooAllocatorDeclNode;

// Allocation expression node
typedef struct {
    GooNode base;
    struct GooNode* type;      // Type to allocate
    struct GooNode* size;      // Size expression (for arrays)
    struct GooNode* allocator; // Optional explicit allocator
} GooAllocExprNode;

// Free expression node
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression to free
    struct GooNode* allocator; // Optional explicit allocator
} GooFreeExprNode;

// Scoped memory block
typedef struct {
    GooNode base;
    struct GooNode* allocator; // Allocator to use within the block
    struct GooNode* body;      // Block statements
} GooScopeBlockNode;

// Binary expression node
typedef struct {
    GooNode base;
    struct GooNode* left;      // Left operand
    struct GooNode* right;     // Right operand
    int operator;              // Operator (can be an enum in the future)
} GooBinaryExprNode;

// Unary expression node
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression operand
    int operator;              // Operator (can be an enum in the future)
} GooUnaryExprNode;

// Function call node
typedef struct {
    GooNode base;
    struct GooNode* func;      // Function identifier/expression
    struct GooNode* args;      // Arguments (linked list)
} GooCallExprNode;

// Super expression node
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Expression to debug/extend
} GooSuperExprNode;

// Return statement node
typedef struct {
    GooNode base;
    struct GooNode* expr;      // Return expression (NULL for empty return)
} GooReturnStmtNode;

// Block statement node
typedef struct {
    GooNode base;
    struct GooNode* statements; // List of statements in the block
} GooBlockStmtNode;

// If statement node
typedef struct {
    GooNode base;
    struct GooNode* condition; // Condition expression
    struct GooNode* then_block; // Then block
    struct GooNode* else_block; // Else block (can be another if statement)
} GooIfStmtNode;

// For statement node
typedef struct {
    GooNode base;
    struct GooNode* condition; // Loop condition expression or range expression
    struct GooNode* init_expr; // Optional initialization expression
    struct GooNode* update_expr; // Optional update expression
    struct GooNode* body;     // Loop body
    bool is_range;           // Whether this is a range-based for loop
} GooForStmtNode;

// Parameter node
typedef struct {
    GooNode base;
    char* name;               // Parameter name
    struct GooNode* type;     // Parameter type
    bool is_capability;       // Whether this has capability restriction
    bool is_allocator;        // Whether this is an allocator parameter
    GooAllocatorType alloc_type; // Type of allocator (if is_allocator is true)
} GooParamNode;

// Comptime SIMD declaration node
typedef struct {
    GooNode base;
    struct GooNode* block;    // Block of SIMD declarations and configuration
} GooComptimeSIMDNode;

// SIMD type declaration node
typedef struct {
    GooNode base;
    char* name;                     // Name of the SIMD type
    GooVectorDataType data_type;    // Base scalar data type
    int vector_width;               // Width of the vector (elements)
    GooSIMDType simd_type;          // SIMD instruction set to use
    bool is_safe;                   // Whether bounds checking is enabled
    size_t alignment;               // Required alignment
} GooSIMDTypeNode;

// SIMD operation declaration node
typedef struct {
    GooNode base;
    char* name;                     // Name of the operation
    GooVectorOp op;                 // Operation type
    struct GooNode* vec_type;       // Identifier for the vector type
    bool is_masked;                 // Whether this is a masked operation
    bool is_fused;                  // Whether this is a fused operation
} GooSIMDOpNode;

// AST root structure
struct GooAst {
    GooNode* root;
    char* filename;
    GooNode* package;
    GooNode* imports;
    GooNode* declarations;
    GooAllocator* allocator;   // Allocator used for AST nodes
};

// Create a new AST
GooAst* goo_ast_create(const char* filename);
GooAst* goo_ast_create_with_allocator(const char* filename, GooAllocator* allocator);

// Free an AST
void goo_ast_free(GooAst* ast);

// Add a node to the AST
void goo_ast_add_node(GooAst* ast, GooNode* node);

// Create various node types
GooPackageNode* goo_ast_create_package_node(const char* name, uint32_t line, uint32_t column);
GooImportNode* goo_ast_create_import_node(const char* path, uint32_t line, uint32_t column);
GooFunctionNode* goo_ast_create_function_node(const char* name, GooNode* params, GooNode* return_type, GooNode* body, bool is_kernel, bool is_user, bool is_unsafe, GooNode* allocator, uint32_t line, uint32_t column);
GooChannelDeclNode* goo_ast_create_channel_decl_node(const char* name, GooChannelPattern pattern, GooNode* element_type, const char* endpoint, uint32_t line, uint32_t column);
GooVarDeclNode* goo_ast_create_var_decl_node(const char* name, GooNode* type, GooNode* init_expr, bool is_safe, bool is_comptime, GooNode* allocator, uint32_t line, uint32_t column);
GooChannelSendNode* goo_ast_create_channel_send_node(GooNode* channel, GooNode* value, uint32_t line, uint32_t column);
GooChannelRecvNode* goo_ast_create_channel_recv_node(GooNode* channel, uint32_t line, uint32_t column);
GooGoStmtNode* goo_ast_create_go_stmt_node(GooNode* expr, uint32_t line, uint32_t column);
GooGoParallelNode* goo_ast_create_go_parallel_node(GooNode* body, GooNode* options, uint32_t line, uint32_t column);
GooSuperviseStmtNode* goo_ast_create_supervise_stmt_node(GooNode* expr, uint32_t line, uint32_t column);
GooTryStmtNode* goo_ast_create_try_stmt_node(GooNode* expr, const char* error_type, GooNode* recover_block, uint32_t line, uint32_t column);
GooModuleDeclNode* goo_ast_create_module_decl_node(const char* name, GooNode* declarations, uint32_t line, uint32_t column);
GooTypeNode* goo_ast_create_type_node(GooNodeType type_kind, GooNode* elem_type, bool is_capability, uint32_t line, uint32_t column);
GooAllocatorDeclNode* goo_ast_create_allocator_decl_node(const char* name, GooAllocatorType type, GooNode* options, uint32_t line, uint32_t column);
GooAllocExprNode* goo_ast_create_alloc_expr_node(GooNode* type, GooNode* size, GooNode* allocator, uint32_t line, uint32_t column);
GooFreeExprNode* goo_ast_create_free_expr_node(GooNode* expr, GooNode* allocator, uint32_t line, uint32_t column);
GooScopeBlockNode* goo_ast_create_scope_block_node(GooNode* allocator, GooNode* body, uint32_t line, uint32_t column);
GooRangeLiteralNode* goo_ast_create_range_literal_node(int64_t start, int64_t end, uint32_t line, uint32_t column);
GooBinaryExprNode* goo_ast_create_binary_expr_node(GooNode* left, int operator, GooNode* right, uint32_t line, uint32_t column);
GooIntLiteralNode* goo_ast_create_int_literal_node(int64_t value, uint32_t line, uint32_t column);
GooFloatLiteralNode* goo_ast_create_float_literal_node(double value, uint32_t line, uint32_t column);
GooBoolLiteralNode* goo_ast_create_bool_literal_node(bool value, uint32_t line, uint32_t column);
GooStringLiteralNode* goo_ast_create_string_literal_node(const char* value, uint32_t line, uint32_t column);
GooUnaryExprNode* goo_ast_create_unary_expr_node(int operator, GooNode* expr, uint32_t line, uint32_t column);
GooCallExprNode* goo_ast_create_call_expr_node(GooNode* func, GooNode* args, uint32_t line, uint32_t column);
GooSuperExprNode* goo_ast_create_super_expr_node(GooNode* expr, uint32_t line, uint32_t column);
GooReturnStmtNode* goo_ast_create_return_stmt_node(GooNode* expr, uint32_t line, uint32_t column);
GooBlockStmtNode* goo_ast_create_block_stmt_node(GooNode* statements, uint32_t line, uint32_t column);
GooIfStmtNode* goo_ast_create_if_stmt_node(GooNode* condition, GooNode* then_block, GooNode* else_block, uint32_t line, uint32_t column);
GooForStmtNode* goo_ast_create_for_stmt_node(GooNode* condition, GooNode* init_expr, GooNode* update_expr, GooNode* body, bool is_range, uint32_t line, uint32_t column);
GooParamNode* goo_ast_create_param_node(const char* name, GooNode* type, bool is_capability, bool is_allocator, GooAllocatorType alloc_type, uint32_t line, uint32_t column);
GooComptimeBuildNode* goo_ast_create_comptime_build_node(GooNode* block, uint32_t line, uint32_t column);
GooComptimeSIMDNode* goo_ast_create_comptime_simd_node(GooNode* block, uint32_t line, uint32_t column);
GooSIMDTypeNode* goo_ast_create_simd_type_node(const char* name, GooVectorDataType data_type, int width, GooSIMDType simd_type, bool is_safe, size_t alignment, uint32_t line, uint32_t column);
GooSIMDOpNode* goo_ast_create_simd_op_node(const char* name, GooVectorOp op, GooNode* vec_type, bool is_masked, bool is_fused, uint32_t line, uint32_t column);

#endif // GOO_AST_H 