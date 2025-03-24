/**
 * type_checker.c
 * 
 * Type checking implementation for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "../include/goo.h"
#include "../include/ast.h"
#include "../include/type_checker.h"

// Forward declarations for internal functions
typedef struct GooSymbolTable GooSymbolTable;
typedef struct GooTypeTable GooTypeTable;

// Placeholder function declarations until we implement actual symbol_table.h and type_table.h
GooSymbolTable* goo_symbol_table_create(void);
void goo_symbol_table_free(GooSymbolTable* table);
void goo_symbol_table_push_scope(GooSymbolTable* table);
void goo_symbol_table_pop_scope(GooSymbolTable* table);
GooSymbol* goo_symbol_create(const char* name, GooType* type, int symbol_kind);
void goo_symbol_table_add(GooSymbolTable* table, GooSymbol* symbol);

GooTypeTable* goo_type_table_create(void);
void goo_type_table_free(GooTypeTable* table);
void goo_type_table_add_primitives(GooTypeTable* table);
GooType* goo_type_table_get_bool(GooTypeTable* table);
GooType* goo_type_table_get_void(GooTypeTable* table);
GooType* goo_type_table_get_any(GooTypeTable* table);
GooType* goo_type_table_add_function(GooTypeTable* table, GooType* return_type, GooType** param_types, int param_count);

// Symbol kinds
#define GOO_SYMBOL_VARIABLE 1
#define GOO_SYMBOL_FUNCTION 2
#define GOO_SYMBOL_PARAMETER 3

// Placeholder for a symbol structure
typedef struct GooSymbol {
    const char* name;
    GooType* type;
    int kind;
} GooSymbol;

/**
 * GooTypeContext - Maintains state for the type checking process
 */
struct GooTypeContext {
    GooSymbolTable* symbols;      // Symbol table
    GooTypeTable* types;          // Type table
    GooAstNode* current_function; // Current function being type-checked
    GooType* current_return_type; // Return type of current function
    int error_count;              // Number of type errors encountered
    bool in_loop;                 // Whether we're in a loop context
    bool in_defer;                // Whether we're in a defer block
    GooAstNode* current_module;   // Current module being processed
};

// Create a new type checking context
GooTypeContext* goo_type_context_create(void) {
    GooTypeContext* ctx = (GooTypeContext*)malloc(sizeof(GooTypeContext));
    if (!ctx) {
        return NULL;
    }
    
    ctx->symbols = goo_symbol_table_create();
    ctx->types = goo_type_table_create();
    ctx->current_function = NULL;
    ctx->current_return_type = NULL;
    ctx->error_count = 0;
    ctx->in_loop = false;
    ctx->in_defer = false;
    ctx->current_module = NULL;
    
    // Initialize built-in types
    goo_type_table_add_primitives(ctx->types);
    
    return ctx;
}

// Free the type checking context
void goo_type_context_free(GooTypeContext* ctx) {
    if (!ctx) {
        return;
    }
    
    goo_symbol_table_free(ctx->symbols);
    goo_type_table_free(ctx->types);
    free(ctx);
}

// Report a type error
static void report_type_error(GooTypeContext* ctx, GooAstNode* node, const char* format, ...) {
    ctx->error_count++;
    
    va_list args;
    va_start(args, format);
    
    fprintf(stderr, "Type error at %s:%d:%d: ", 
            node->location.filename, 
            node->location.line,
            node->location.column);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    
    va_end(args);
}

// Check if two types are compatible
static bool are_types_compatible(GooTypeContext* ctx, GooType* left, GooType* right) {
    // Handle NULL types (error recovery)
    if (!left || !right) {
        return false;
    }
    
    // Same type is always compatible
    if (left == right) {
        return true;
    }
    
    // Check specific compatibility rules based on type kind
    switch (left->kind) {
        case GOO_TYPE_PRIMITIVE:
            if (right->kind == GOO_TYPE_PRIMITIVE) {
                // Handle primitive type compatibility (e.g., numeric conversions)
                return left->primitive.kind == right->primitive.kind;
            }
            break;
            
        case GOO_TYPE_STRUCT:
            // Structural typing rules would go here
            break;
            
        case GOO_TYPE_INTERFACE:
            // Check if right implements the interface
            return goo_type_implements_interface(ctx->types, right, left);
            
        case GOO_TYPE_FUNCTION:
            // Function type compatibility
            if (right->kind != GOO_TYPE_FUNCTION) {
                return false;
            }
            // Check return type and parameters
            if (!are_types_compatible(ctx, left->function.return_type, right->function.return_type)) {
                return false;
            }
            // Check parameter count
            if (left->function.param_count != right->function.param_count) {
                return false;
            }
            // Check each parameter
            for (int i = 0; i < left->function.param_count; i++) {
                if (!are_types_compatible(ctx, left->function.param_types[i], right->function.param_types[i])) {
                    return false;
                }
            }
            return true;
            
        case GOO_TYPE_ARRAY:
            // Array type compatibility
            if (right->kind != GOO_TYPE_ARRAY) {
                return false;
            }
            // Check element type
            return are_types_compatible(ctx, left->array.element_type, right->array.element_type);
            
        case GOO_TYPE_POINTER:
            // Pointer type compatibility
            if (right->kind != GOO_TYPE_POINTER) {
                return false;
            }
            // Check pointed type
            return are_types_compatible(ctx, left->pointer.pointed_type, right->pointer.pointed_type);
            
        default:
            break;
    }
    
    return false;
}

// Type check a binary expression
static GooType* type_check_binary_expr(GooTypeContext* ctx, GooAstNode* node) {
    if (!node || node->kind != GOO_AST_BINARY_EXPR) {
        return NULL;
    }
    
    // Type check left and right operands
    GooType* left_type = goo_type_check_node(ctx, node->binary.left);
    GooType* right_type = goo_type_check_node(ctx, node->binary.right);
    
    if (!left_type || !right_type) {
        return NULL;  // Error recovery
    }
    
    // Check operator-specific type rules
    switch (node->binary.op) {
        case GOO_OP_ADD:
        case GOO_OP_SUB:
        case GOO_OP_MUL:
        case GOO_OP_DIV:
            // Numeric operators
            if (left_type->kind != GOO_TYPE_PRIMITIVE || right_type->kind != GOO_TYPE_PRIMITIVE) {
                report_type_error(ctx, node, "Binary operator '%s' requires numeric operands", 
                                 goo_op_to_string(node->binary.op));
                return NULL;
            }
            
            if (!goo_is_numeric_type(left_type) || !goo_is_numeric_type(right_type)) {
                report_type_error(ctx, node, "Binary operator '%s' requires numeric operands", 
                                 goo_op_to_string(node->binary.op));
                return NULL;
            }
            
            // Type promotion rules
            return goo_promote_numeric_types(ctx->types, left_type, right_type);
            
        case GOO_OP_EQ:
        case GOO_OP_NEQ:
            // Equality operators (more permissive)
            if (!are_types_compatible(ctx, left_type, right_type) && 
                !are_types_compatible(ctx, right_type, left_type)) {
                report_type_error(ctx, node, "Cannot compare values of incompatible types");
                return NULL;
            }
            // Equality operators return boolean
            return goo_type_table_get_bool(ctx->types);
            
        case GOO_OP_LT:
        case GOO_OP_GT:
        case GOO_OP_LTE:
        case GOO_OP_GTE:
            // Comparison operators
            if (!goo_is_comparable_type(left_type) || !goo_is_comparable_type(right_type)) {
                report_type_error(ctx, node, "Binary operator '%s' requires comparable operands", 
                                 goo_op_to_string(node->binary.op));
                return NULL;
            }
            
            if (!are_types_compatible(ctx, left_type, right_type)) {
                report_type_error(ctx, node, "Cannot compare values of incompatible types");
                return NULL;
            }
            
            // Comparison operators return boolean
            return goo_type_table_get_bool(ctx->types);
            
        default:
            report_type_error(ctx, node, "Unsupported binary operator '%s'", 
                             goo_op_to_string(node->binary.op));
            return NULL;
    }
}

// Type check a variable declaration
static GooType* type_check_var_decl(GooTypeContext* ctx, GooAstNode* node) {
    if (!node || node->kind != GOO_AST_VAR_DECL) {
        return NULL;
    }
    
    // Get or resolve the declared type
    GooType* declared_type = NULL;
    if (node->var_decl.type_node) {
        declared_type = goo_resolve_type_node(ctx, node->var_decl.type_node);
        if (!declared_type) {
            report_type_error(ctx, node, "Unknown type in variable declaration");
            return NULL;
        }
    }
    
    // Type check initializer if present
    GooType* init_type = NULL;
    if (node->var_decl.init) {
        init_type = goo_type_check_node(ctx, node->var_decl.init);
        if (!init_type) {
            return NULL;  // Error recovery
        }
    }
    
    // Infer type if not explicitly declared
    if (!declared_type) {
        if (!init_type) {
            report_type_error(ctx, node, "Cannot infer type for variable without initializer");
            return NULL;
        }
        declared_type = init_type;
    } else if (init_type) {
        // Check type compatibility for explicit declaration with initializer
        if (!are_types_compatible(ctx, declared_type, init_type)) {
            report_type_error(ctx, node, "Initializer type does not match declared variable type");
            return NULL;
        }
    }
    
    // Add variable to symbol table
    GooSymbol* symbol = goo_symbol_create(node->var_decl.name, declared_type, GOO_SYMBOL_VARIABLE);
    goo_symbol_table_add(ctx->symbols, symbol);
    
    // Store the type with the node
    node->type = declared_type;
    
    return declared_type;
}

// Type check a function declaration
static GooType* type_check_function_decl(GooTypeContext* ctx, GooAstNode* node) {
    if (!node || node->kind != GOO_AST_FUNCTION) {
        return NULL;
    }
    
    // Create a new scope for the function
    goo_symbol_table_push_scope(ctx->symbols);
    
    // Save previous function context
    GooAstNode* prev_function = ctx->current_function;
    GooType* prev_return_type = ctx->current_return_type;
    
    // Resolve return type
    GooType* return_type = NULL;
    if (node->function.return_type_node) {
        return_type = goo_resolve_type_node(ctx, node->function.return_type_node);
        if (!return_type) {
            report_type_error(ctx, node, "Unknown return type in function declaration");
            // Continue for error recovery
            return_type = goo_type_table_get_void(ctx->types);
        }
    } else {
        // Default to void return type
        return_type = goo_type_table_get_void(ctx->types);
    }
    
    // Set current function context
    ctx->current_function = node;
    ctx->current_return_type = return_type;
    
    // Process parameters
    GooType** param_types = NULL;
    int param_count = 0;
    
    if (node->function.params) {
        param_count = node->function.params->list.count;
        param_types = (GooType**)malloc(sizeof(GooType*) * param_count);
        
        for (int i = 0; i < param_count; i++) {
            GooAstNode* param = node->function.params->list.nodes[i];
            if (param->kind != GOO_AST_PARAM) {
                report_type_error(ctx, param, "Expected parameter declaration");
                continue;
            }
            
            GooType* param_type = goo_resolve_type_node(ctx, param->param.type_node);
            if (!param_type) {
                report_type_error(ctx, param, "Unknown parameter type");
                param_type = goo_type_table_get_any(ctx->types); // Use 'any' for error recovery
            }
            
            param_types[i] = param_type;
            
            // Add parameter to symbol table
            GooSymbol* symbol = goo_symbol_create(param->param.name, param_type, GOO_SYMBOL_PARAMETER);
            goo_symbol_table_add(ctx->symbols, symbol);
        }
    }
    
    // Create function type
    GooType* func_type = goo_type_table_add_function(ctx->types, return_type, param_types, param_count);
    
    // Add function to symbol table (in outer scope)
    goo_symbol_table_pop_scope(ctx->symbols);
    GooSymbol* func_symbol = goo_symbol_create(node->function.name, func_type, GOO_SYMBOL_FUNCTION);
    goo_symbol_table_add(ctx->symbols, func_symbol);
    
    // Push scope again for function body
    goo_symbol_table_push_scope(ctx->symbols);
    
    // Type check function body
    if (node->function.body) {
        goo_type_check_node(ctx, node->function.body);
    }
    
    // Pop function scope
    goo_symbol_table_pop_scope(ctx->symbols);
    
    // Restore previous function context
    ctx->current_function = prev_function;
    ctx->current_return_type = prev_return_type;
    
    // Store type with node
    node->type = func_type;
    
    return func_type;
}

// Main entry point for type checking a node
GooType* goo_type_check_node(GooTypeContext* ctx, GooAstNode* node) {
    if (!node) {
        return NULL;
    }
    
    // If node already has type, return it
    if (node->type) {
        return node->type;
    }
    
    // Type check based on node kind
    switch (node->kind) {
        case GOO_AST_BINARY_EXPR:
            return type_check_binary_expr(ctx, node);
            
        case GOO_AST_VAR_DECL:
            return type_check_var_decl(ctx, node);
            
        case GOO_AST_FUNCTION:
            return type_check_function_decl(ctx, node);
            
        // Add cases for other node types
        
        default:
            fprintf(stderr, "Warning: No type checking implemented for AST node kind %d\n", node->kind);
            return NULL;
    }
}

// Main entry point for type checking a module
bool goo_type_check_module(GooTypeContext* ctx, GooAstNode* module) {
    if (!ctx || !module || module->kind != GOO_AST_MODULE) {
        return false;
    }
    
    // Set current module
    ctx->current_module = module;
    
    // Create a new scope for the module
    goo_symbol_table_push_scope(ctx->symbols);
    
    // Type check all declarations in the module
    for (int i = 0; i < module->module.declarations->list.count; i++) {
        GooAstNode* decl = module->module.declarations->list.nodes[i];
        goo_type_check_node(ctx, decl);
    }
    
    // Pop module scope
    goo_symbol_table_pop_scope(ctx->symbols);
    
    // Return success if no errors were encountered
    return ctx->error_count == 0;
}

// Type check an entire AST
bool goo_type_check(GooAst* ast) {
    if (!ast || !ast->root) {
        return false;
    }
    
    // Create type checking context
    GooTypeContext* ctx = goo_type_context_create();
    if (!ctx) {
        return false;
    }
    
    // Perform type checking
    bool success = goo_type_check_module(ctx, ast->root);
    
    // Clean up
    goo_type_context_free(ctx);
    
    return success;
} 