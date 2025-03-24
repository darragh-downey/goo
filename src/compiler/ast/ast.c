#include <stdlib.h>
#include <string.h>
#include "ast.h"

// Default allocator implementation
static void* default_alloc(void* ctx, size_t size, size_t alignment) {
    (void)ctx;     // Unused
    (void)alignment; // Unused in simple implementation
    return malloc(size);
}

static void* default_realloc(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t alignment) {
    (void)ctx;     // Unused
    (void)old_size; // Unused in simple implementation
    (void)alignment; // Unused in simple implementation
    return realloc(ptr, new_size);
}

static void default_free(void* ctx, void* ptr, size_t size) {
    (void)ctx;  // Unused
    (void)size; // Unused in simple implementation
    free(ptr);
}

// Create a default allocator
static GooAllocator* create_default_allocator() {
    GooAllocator* allocator = malloc(sizeof(GooAllocator));
    if (!allocator) return NULL;
    
    allocator->type = GOO_ALLOC_HEAP;
    allocator->context = NULL;
    allocator->alloc = default_alloc;
    allocator->realloc = default_realloc;
    allocator->free = default_free;
    
    return allocator;
}

// Create a new AST
GooAst* goo_ast_create(const char* filename) {
    GooAllocator* allocator = create_default_allocator();
    if (!allocator) return NULL;
    
    return goo_ast_create_with_allocator(filename, allocator);
}

// Create a new AST with a specific allocator
GooAst* goo_ast_create_with_allocator(const char* filename, GooAllocator* allocator) {
    if (!allocator) return NULL;
    
    GooAst* ast = (GooAst*)allocator->alloc(allocator->context, sizeof(GooAst), 8);
    if (!ast) return NULL;
    
    ast->root = NULL;
    ast->filename = strdup(filename);
    ast->package = NULL;
    ast->imports = NULL;
    ast->declarations = NULL;
    ast->allocator = allocator;
    
    return ast;
}

// Free an AST
void goo_ast_free(GooAst* ast) {
    if (!ast) return;
    
    // Save the allocator for last
    GooAllocator* allocator = ast->allocator;
    
    // TODO: Implement proper recursive node freeing
    
    free(ast->filename);
    
    // Free the AST itself
    allocator->free(allocator->context, ast, sizeof(GooAst));
    
    // Free the allocator if it's our default
    if (allocator->type == GOO_ALLOC_HEAP && !allocator->context) {
        free(allocator);
    }
}

// Add a node to the AST
void goo_ast_add_node(GooAst* ast, GooNode* node) {
    if (!ast || !node) return;
    
    // Set root if not already set
    if (!ast->root) {
        ast->root = node;
    }
    
    // Categorize node based on type
    switch (node->type) {
        case GOO_NODE_PACKAGE:
            ast->package = node;
            break;
        case GOO_NODE_IMPORT:
            if (!ast->imports) {
                ast->imports = node;
            } else {
                // Add to end of imports list
                GooNode* current = ast->imports;
                while (current->next) {
                    current = current->next;
                }
                current->next = node;
            }
            break;
        default:
            // Add to declarations
            if (!ast->declarations) {
                ast->declarations = node;
            } else {
                // Add to end of declarations list
                GooNode* current = ast->declarations;
                while (current->next) {
                    current = current->next;
                }
                current->next = node;
            }
            break;
    }
}

// Initialize a base node
static void init_node(GooNode* node, GooNodeType type, uint32_t line, uint32_t column) {
    node->type = type;
    node->line = line;
    node->column = column;
    node->next = NULL;
}

// Create a package declaration node
GooPackageNode* goo_ast_create_package_node(const char* name, uint32_t line, uint32_t column) {
    GooPackageNode* node = malloc(sizeof(GooPackageNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_PACKAGE, line, column);
    node->name = strdup(name);
    
    return node;
}

// Create an import declaration node
GooImportNode* goo_ast_create_import_node(const char* path, uint32_t line, uint32_t column) {
    GooImportNode* node = malloc(sizeof(GooImportNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_IMPORT_DECL, line, column);
    node->path = strdup(path);
    
    return node;
}

// Create a function declaration node
GooFunctionNode* goo_ast_create_function_node(const char* name, GooNode* params, GooNode* return_type, GooNode* body, bool is_kernel, bool is_user, bool is_unsafe, GooNode* allocator, uint32_t line, uint32_t column) {
    GooFunctionNode* node = malloc(sizeof(GooFunctionNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_FUNCTION_DECL, line, column);
    node->name = strdup(name);
    node->params = params;
    node->return_type = return_type;
    node->body = body;
    node->is_kernel = is_kernel;
    node->is_user = is_user;
    node->is_unsafe = is_unsafe;
    node->allocator = allocator;
    
    return node;
}

// Create a channel declaration node
GooChannelDeclNode* goo_ast_create_channel_decl_node(const char* name, GooChannelPattern pattern, GooNode* element_type, const char* endpoint, uint32_t line, uint32_t column) {
    GooChannelDeclNode* node = malloc(sizeof(GooChannelDeclNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_CHANNEL_DECL, line, column);
    node->name = strdup(name);
    node->pattern = pattern;
    node->element_type = element_type;
    node->endpoint = endpoint ? strdup(endpoint) : NULL;
    node->has_capability = false; // Default to false
    
    return node;
}

// Create a variable declaration node
GooVarDeclNode* goo_ast_create_var_decl_node(const char* name, GooNode* type, GooNode* init_expr, bool is_safe, bool is_comptime, GooNode* allocator, uint32_t line, uint32_t column) {
    GooVarDeclNode* node = malloc(sizeof(GooVarDeclNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_VARIABLE_DECL, line, column);
    node->name = strdup(name);
    node->type = type;
    node->init_expr = init_expr;
    node->is_safe = is_safe;
    node->is_comptime = is_comptime;
    node->allocator = allocator;
    
    return node;
}

// Create a channel send node
GooChannelSendNode* goo_ast_create_channel_send_node(GooNode* channel, GooNode* value, uint32_t line, uint32_t column) {
    GooChannelSendNode* node = malloc(sizeof(GooChannelSendNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_CHANNEL_SEND, line, column);
    node->channel = channel;
    node->value = value;
    
    return node;
}

// Create a channel receive node
GooChannelRecvNode* goo_ast_create_channel_recv_node(GooNode* channel, uint32_t line, uint32_t column) {
    GooChannelRecvNode* node = malloc(sizeof(GooChannelRecvNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_CHANNEL_RECV, line, column);
    node->channel = channel;
    
    return node;
}

// Create a goroutine node
GooGoStmtNode* goo_ast_create_go_stmt_node(GooNode* expr, uint32_t line, uint32_t column) {
    GooGoStmtNode* node = malloc(sizeof(GooGoStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_GO_STMT, line, column);
    node->expr = expr;
    
    return node;
}

// Create a parallel execution node
GooGoParallelNode* goo_ast_create_go_parallel_node(GooNode* body, GooNode* options, uint32_t line, uint32_t column) {
    GooGoParallelNode* node = malloc(sizeof(GooGoParallelNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_GO_PARALLEL, line, column);
    node->body = body;
    node->options = options;
    
    return node;
}

// Create a supervise statement node
GooSuperviseStmtNode* goo_ast_create_supervise_stmt_node(GooNode* expr, uint32_t line, uint32_t column) {
    GooSuperviseStmtNode* node = malloc(sizeof(GooSuperviseStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_SUPERVISE_STMT, line, column);
    node->expr = expr;
    
    return node;
}

// Create a try statement node
GooTryStmtNode* goo_ast_create_try_stmt_node(GooNode* expr, const char* error_type, GooNode* recover_block, uint32_t line, uint32_t column) {
    GooTryStmtNode* node = malloc(sizeof(GooTryStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_TRY_STMT, line, column);
    node->expr = expr;
    node->error_type = error_type ? strdup(error_type) : NULL;
    node->recover_block = recover_block;
    
    return node;
}

// Create a module declaration node
GooModuleDeclNode* goo_ast_create_module_decl_node(const char* name, GooNode* declarations, uint32_t line, uint32_t column) {
    GooModuleDeclNode* node = malloc(sizeof(GooModuleDeclNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_MODULE_DECL, line, column);
    node->name = strdup(name);
    node->declarations = declarations;
    
    return node;
}

// Create a type node
GooTypeNode* goo_ast_create_type_node(GooNodeType type_kind, GooNode* elem_type, bool is_capability, uint32_t line, uint32_t column) {
    GooTypeNode* node = malloc(sizeof(GooTypeNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_TYPE, line, column);
    node->type_kind = type_kind;
    node->elem_type = elem_type;
    node->is_capability = is_capability;
    
    return node;
}

// Create an allocator declaration node
GooAllocatorDeclNode* goo_ast_create_allocator_decl_node(const char* name, GooAllocatorType type, GooNode* options, uint32_t line, uint32_t column) {
    GooAllocatorDeclNode* node = malloc(sizeof(GooAllocatorDeclNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_ALLOCATOR_DECL, line, column);
    node->name = strdup(name);
    node->type = type;
    node->options = options;
    
    return node;
}

// Create an allocation expression node
GooAllocExprNode* goo_ast_create_alloc_expr_node(GooNode* type, GooNode* size, GooNode* allocator, uint32_t line, uint32_t column) {
    GooAllocExprNode* node = malloc(sizeof(GooAllocExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_ALLOC_EXPR, line, column);
    node->type = type;
    node->size = size;
    node->allocator = allocator;
    
    return node;
}

// Create a free expression node
GooFreeExprNode* goo_ast_create_free_expr_node(GooNode* expr, GooNode* allocator, uint32_t line, uint32_t column) {
    GooFreeExprNode* node = malloc(sizeof(GooFreeExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_FREE_EXPR, line, column);
    node->expr = expr;
    node->allocator = allocator;
    
    return node;
}

// Create a scope block node
GooScopeBlockNode* goo_ast_create_scope_block_node(GooNode* allocator, GooNode* body, uint32_t line, uint32_t column) {
    GooScopeBlockNode* node = malloc(sizeof(GooScopeBlockNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_SCOPE_BLOCK, line, column);
    node->allocator = allocator;
    node->body = body;
    
    return node;
}

// Create a range literal node
GooRangeLiteralNode* goo_ast_create_range_literal_node(int64_t start, int64_t end, uint32_t line, uint32_t column) {
    GooRangeLiteralNode* node = malloc(sizeof(GooRangeLiteralNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_RANGE_LITERAL, line, column);
    node->start = start;
    node->end = end;
    
    return node;
}

// Create an integer literal node
GooIntLiteralNode* goo_ast_create_int_literal_node(int64_t value, uint32_t line, uint32_t column) {
    GooIntLiteralNode* node = malloc(sizeof(GooIntLiteralNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_INT_LITERAL, line, column);
    node->value = value;
    
    return node;
}

// Create a float literal node
GooFloatLiteralNode* goo_ast_create_float_literal_node(double value, uint32_t line, uint32_t column) {
    GooFloatLiteralNode* node = malloc(sizeof(GooFloatLiteralNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_FLOAT_LITERAL, line, column);
    node->value = value;
    
    return node;
}

// Create a boolean literal node
GooBoolLiteralNode* goo_ast_create_bool_literal_node(bool value, uint32_t line, uint32_t column) {
    GooBoolLiteralNode* node = malloc(sizeof(GooBoolLiteralNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_BOOL_LITERAL, line, column);
    node->value = value;
    
    return node;
}

// Create a string literal node
GooStringLiteralNode* goo_ast_create_string_literal_node(const char* value, uint32_t line, uint32_t column) {
    GooStringLiteralNode* node = malloc(sizeof(GooStringLiteralNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_STRING_LITERAL, line, column);
    node->value = strdup(value);
    
    return node;
}

// Create a binary expression node
// This will be used for the range operator and other binary expressions
typedef struct {
    GooNode base;
    GooNode* left;
    GooNode* right;
    int operator;  // This can be an enum in the future
} GooBinaryExprNode;

GooBinaryExprNode* goo_ast_create_binary_expr_node(GooNode* left, int operator, GooNode* right, uint32_t line, uint32_t column) {
    GooBinaryExprNode* node = malloc(sizeof(GooBinaryExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_BINARY_EXPR, line, column);
    node->left = left;
    node->right = right;
    node->operator = operator;
    
    return node;
}

// Create a unary expression node
GooUnaryExprNode* goo_ast_create_unary_expr_node(int operator, GooNode* expr, uint32_t line, uint32_t column) {
    GooUnaryExprNode* node = malloc(sizeof(GooUnaryExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_UNARY_EXPR, line, column);
    node->expr = expr;
    node->operator = operator;
    
    return node;
}

// Create a function call expression node
GooCallExprNode* goo_ast_create_call_expr_node(GooNode* func, GooNode* args, uint32_t line, uint32_t column) {
    GooCallExprNode* node = malloc(sizeof(GooCallExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_CALL_EXPR, line, column);
    node->func = func;
    node->args = args;
    
    return node;
}

// Create a super expression node
GooSuperExprNode* goo_ast_create_super_expr_node(GooNode* expr, uint32_t line, uint32_t column) {
    GooSuperExprNode* node = malloc(sizeof(GooSuperExprNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_SUPER_EXPR, line, column);
    node->expr = expr;
    
    return node;
}

// Create a return statement node
GooReturnStmtNode* goo_ast_create_return_stmt_node(GooNode* expr, uint32_t line, uint32_t column) {
    GooReturnStmtNode* node = malloc(sizeof(GooReturnStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_RETURN_STMT, line, column);
    node->expr = expr;
    
    return node;
}

// Create a block statement node
GooBlockStmtNode* goo_ast_create_block_stmt_node(GooNode* statements, uint32_t line, uint32_t column) {
    GooBlockStmtNode* node = malloc(sizeof(GooBlockStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_BLOCK_STMT, line, column);
    node->statements = statements;
    
    return node;
}

// Create an if statement node
GooIfStmtNode* goo_ast_create_if_stmt_node(GooNode* condition, GooNode* then_block, GooNode* else_block, uint32_t line, uint32_t column) {
    GooIfStmtNode* node = malloc(sizeof(GooIfStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_IF_STMT, line, column);
    node->condition = condition;
    node->then_block = then_block;
    node->else_block = else_block;
    
    return node;
}

// Create a for statement node
GooForStmtNode* goo_ast_create_for_stmt_node(GooNode* condition, GooNode* init_expr, GooNode* update_expr, GooNode* body, bool is_range, uint32_t line, uint32_t column) {
    GooForStmtNode* node = malloc(sizeof(GooForStmtNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_FOR_STMT, line, column);
    node->condition = condition;
    node->init_expr = init_expr;
    node->update_expr = update_expr;
    node->body = body;
    node->is_range = is_range;
    
    return node;
}

// Create a parameter node
GooParamNode* goo_ast_create_param_node(const char* name, GooNode* type, bool is_capability, bool is_allocator, GooAllocatorType alloc_type, uint32_t line, uint32_t column) {
    GooParamNode* node = malloc(sizeof(GooParamNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_PARAM, line, column);
    node->name = strdup(name);
    node->type = type;
    node->is_capability = is_capability;
    node->is_allocator = is_allocator;
    node->alloc_type = alloc_type;
    
    return node;
}

// Create a comptime build declaration node
GooComptimeBuildNode* goo_ast_create_comptime_build_node(GooNode* block, uint32_t line, uint32_t column) {
    GooComptimeBuildNode* node = malloc(sizeof(GooComptimeBuildNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_COMPTIME_BUILD_DECL, line, column);
    node->block = block;
    
    return node;
}

// Create a comptime SIMD declaration node
GooComptimeSIMDNode* goo_ast_create_comptime_simd_node(GooNode* block, uint32_t line, uint32_t column) {
    GooComptimeSIMDNode* node = malloc(sizeof(GooComptimeSIMDNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_COMPTIME_SIMD_DECL, line, column);
    node->block = block;
    
    return node;
}

// Create a SIMD type declaration node
GooSIMDTypeNode* goo_ast_create_simd_type_node(
    const char* name, 
    GooVectorDataType data_type, 
    int width, 
    GooSIMDType simd_type, 
    bool is_safe, 
    size_t alignment,
    uint32_t line, 
    uint32_t column
) {
    GooSIMDTypeNode* node = malloc(sizeof(GooSIMDTypeNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_SIMD_TYPE_DECL, line, column);
    node->name = strdup(name);
    node->data_type = data_type;
    node->vector_width = width;
    node->simd_type = simd_type;
    node->is_safe = is_safe;
    node->alignment = alignment;
    
    return node;
}

// Create a SIMD operation declaration node
GooSIMDOpNode* goo_ast_create_simd_op_node(
    const char* name, 
    GooVectorOp op, 
    GooNode* vec_type, 
    bool is_masked, 
    bool is_fused, 
    uint32_t line, 
    uint32_t column
) {
    GooSIMDOpNode* node = malloc(sizeof(GooSIMDOpNode));
    if (!node) return NULL;
    
    init_node(&node->base, GOO_NODE_SIMD_OP_DECL, line, column);
    node->name = strdup(name);
    node->op = op;
    node->vec_type = vec_type;
    node->is_masked = is_masked;
    node->is_fused = is_fused;
    
    return node;
} 