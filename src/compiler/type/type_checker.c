/**
 * type_checker.c
 * 
 * Type checking implementation for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/type_checker.h"
#include "../include/ast.h"
#include "../include/error.h"

/**
 * Structure representing a type checking context
 */
struct GooTypeContext {
    GooSymbolTable* symbols;       /* Symbol table for name resolution */
    GooTypeTable* types;           /* Type table for type storage */
    GooAstNode* current_module;    /* Current module being type checked */
    int error_count;               /* Number of type errors encountered */
    bool in_loop;                  /* Whether we're inside a loop */
    bool in_function;              /* Whether we're inside a function */
};

/* Create a new type checking context */
GooTypeContext* goo_type_context_create(void) {
    GooTypeContext* ctx = (GooTypeContext*)malloc(sizeof(GooTypeContext));
    if (!ctx) {
        return NULL;
    }
    
    ctx->symbols = goo_symbol_table_create();
    if (!ctx->symbols) {
        free(ctx);
        return NULL;
    }
    
    ctx->types = goo_type_table_create();
    if (!ctx->types) {
        goo_symbol_table_destroy(ctx->symbols);
        free(ctx);
        return NULL;
    }
    
    ctx->current_module = NULL;
    ctx->error_count = 0;
    ctx->in_loop = false;
    ctx->in_function = false;
    
    /* Initialize built-in types */
    goo_type_context_add_builtin_type(ctx, "void", GOO_TYPE_VOID);
    goo_type_context_add_builtin_type(ctx, "bool", GOO_TYPE_BOOL);
    goo_type_context_add_builtin_type(ctx, "int", GOO_TYPE_INT);
    goo_type_context_add_builtin_type(ctx, "float", GOO_TYPE_FLOAT);
    goo_type_context_add_builtin_type(ctx, "string", GOO_TYPE_STRING);
    
    return ctx;
}

/* Free a type checking context */
void goo_type_context_free(GooTypeContext* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->symbols) {
        goo_symbol_table_destroy(ctx->symbols);
    }
    
    if (ctx->types) {
        goo_type_table_destroy(ctx->types);
    }
    
    free(ctx);
}

/* Get the number of type errors encountered */
int goo_type_context_get_error_count(GooTypeContext* ctx) {
    return ctx ? ctx->error_count : 0;
}

/* Set the current module being type checked */
void goo_type_context_set_module(GooTypeContext* ctx, GooAstNode* module) {
    if (ctx) {
        ctx->current_module = module;
    }
}

/* Add a built-in type to the type checker */
GooType* goo_type_context_add_builtin_type(GooTypeContext* ctx, const char* name, GooTypeKind kind) {
    if (!ctx || !name) {
        return NULL;
    }
    
    GooType* type = goo_type_table_create_type(ctx->types, kind);
    if (!type) {
        return NULL;
    }
    
    GooSymbol* symbol = goo_symbol_create(GOO_SYMBOL_TYPE, name);
    if (!symbol) {
        // No need to free type as it's managed by the type table
        return NULL;
    }
    
    symbol->type = type;
    
    // Add to global scope
    if (!goo_symbol_table_add(ctx->symbols, symbol)) {
        goo_symbol_destroy(symbol);
        return NULL;
    }
    
    return type;
}

/* Add a built-in function to the type checker */
bool goo_type_context_add_builtin_function(GooTypeContext* ctx, const char* name, 
                                         GooType* return_type, GooType** param_types, 
                                         int param_count) {
    if (!ctx || !name || !return_type) {
        return false;
    }
    
    GooType* func_type = goo_type_table_create_function_type(
        ctx->types, return_type, param_types, param_count);
    if (!func_type) {
        return false;
    }
    
    GooSymbol* symbol = goo_symbol_create(GOO_SYMBOL_FUNCTION, name);
    if (!symbol) {
        return false;
    }
    
    symbol->type = func_type;
    symbol->flags |= GOO_SYMBOL_FLAG_BUILTIN;
    
    // Add to global scope
    if (!goo_symbol_table_add(ctx->symbols, symbol)) {
        goo_symbol_destroy(symbol);
        return false;
    }
    
    return true;
}

/* Get the symbol table from the type checking context */
GooSymbolTable* goo_type_context_get_symbol_table(GooTypeContext* ctx) {
    return ctx ? ctx->symbols : NULL;
}

/* Get the type table from the type checking context */
GooTypeTable* goo_type_context_get_type_table(GooTypeContext* ctx) {
    return ctx ? ctx->types : NULL;
}

/* Type check an AST node and return its type */
GooType* goo_type_check_node(GooTypeContext* ctx, GooAstNode* node) {
    if (!ctx || !node) {
        return NULL;
    }
    
    GooType* type = NULL;
    
    switch (node->kind) {
        case GOO_AST_INTEGER_LITERAL:
            type = goo_symbol_table_lookup_type(ctx->symbols, "int");
            break;
            
        case GOO_AST_FLOAT_LITERAL:
            type = goo_symbol_table_lookup_type(ctx->symbols, "float");
            break;
            
        case GOO_AST_STRING_LITERAL:
            type = goo_symbol_table_lookup_type(ctx->symbols, "string");
            break;
            
        case GOO_AST_BOOLEAN_LITERAL:
            type = goo_symbol_table_lookup_type(ctx->symbols, "bool");
            break;
            
        case GOO_AST_VARIABLE:
            {
                GooSymbol* symbol = goo_symbol_table_lookup(ctx->symbols, node->data.variable.name);
                if (!symbol) {
                    goo_report_error(node->line, node->column, 
                                    "Undefined variable '%s'", node->data.variable.name);
                    ctx->error_count++;
                    return NULL;
                }
                
                type = symbol->type;
            }
            break;
            
        case GOO_AST_BINARY:
            {
                GooType* left_type = goo_type_check_node(ctx, node->data.binary.left);
                GooType* right_type = goo_type_check_node(ctx, node->data.binary.right);
                
                if (!left_type || !right_type) {
                    return NULL;
                }
                
                // Type checking for binary operators should check operator/type compatibility
                // For now, we do a simplified version
                switch (node->data.binary.op) {
                    case '+':
                    case '-':
                    case '*':
                    case '/':
                        if (left_type->kind == GOO_TYPE_INT && right_type->kind == GOO_TYPE_INT) {
                            type = left_type;
                        } else if (left_type->kind == GOO_TYPE_FLOAT || right_type->kind == GOO_TYPE_FLOAT) {
                            type = goo_symbol_table_lookup_type(ctx->symbols, "float");
                        } else {
                            goo_report_error(node->line, node->column, 
                                           "Invalid operand types for arithmetic operation");
                            ctx->error_count++;
                            return NULL;
                        }
                        break;
                        
                    case '<':
                    case '>':
                    case GOO_TOKEN_LEQ:
                    case GOO_TOKEN_GEQ:
                    case GOO_TOKEN_EQ:
                    case GOO_TOKEN_NEQ:
                        type = goo_symbol_table_lookup_type(ctx->symbols, "bool");
                        break;
                        
                    default:
                        goo_report_error(node->line, node->column, "Unsupported binary operator");
                        ctx->error_count++;
                        return NULL;
                }
            }
            break;
            
        case GOO_AST_UNARY:
            {
                GooType* operand_type = goo_type_check_node(ctx, node->data.unary.operand);
                
                if (!operand_type) {
                    return NULL;
                }
                
                switch (node->data.unary.op) {
                    case '-':
                        if (operand_type->kind != GOO_TYPE_INT && operand_type->kind != GOO_TYPE_FLOAT) {
                            goo_report_error(node->line, node->column, 
                                           "Unary minus requires numeric operand");
                            ctx->error_count++;
                            return NULL;
                        }
                        type = operand_type;
                        break;
                        
                    case '!':
                        if (operand_type->kind != GOO_TYPE_BOOL) {
                            goo_report_error(node->line, node->column, 
                                           "Logical not requires boolean operand");
                            ctx->error_count++;
                            return NULL;
                        }
                        type = operand_type;
                        break;
                        
                    default:
                        goo_report_error(node->line, node->column, "Unsupported unary operator");
                        ctx->error_count++;
                        return NULL;
                }
            }
            break;
            
        case GOO_AST_ASSIGNMENT:
            {
                // Check that target is assignable (variable, array access, field access)
                if (node->data.assignment.target->kind != GOO_AST_VARIABLE &&
                    node->data.assignment.target->kind != GOO_AST_ARRAY_ACCESS &&
                    node->data.assignment.target->kind != GOO_AST_FIELD_ACCESS) {
                    goo_report_error(node->line, node->column, "Invalid assignment target");
                    ctx->error_count++;
                    return NULL;
                }
                
                GooType* target_type = goo_type_check_node(ctx, node->data.assignment.target);
                GooType* value_type = goo_type_check_node(ctx, node->data.assignment.value);
                
                if (!target_type || !value_type) {
                    return NULL;
                }
                
                // Check type compatibility
                if (target_type->kind != value_type->kind) {
                    // Simple type checking for now
                    goo_report_error(node->line, node->column, 
                                   "Type mismatch in assignment");
                    ctx->error_count++;
                    return NULL;
                }
                
                type = target_type;
            }
            break;
            
        case GOO_AST_FUNCTION_CALL:
            {
                GooSymbol* func_symbol = goo_symbol_table_lookup(
                    ctx->symbols, node->data.function_call.name);
                
                if (!func_symbol || func_symbol->kind != GOO_SYMBOL_FUNCTION) {
                    goo_report_error(node->line, node->column, 
                                   "Undefined function '%s'", node->data.function_call.name);
                    ctx->error_count++;
                    return NULL;
                }
                
                GooType* func_type = func_symbol->type;
                if (func_type->kind != GOO_TYPE_FUNCTION) {
                    goo_report_error(node->line, node->column, 
                                   "'%s' is not a function", node->data.function_call.name);
                    ctx->error_count++;
                    return NULL;
                }
                
                // Check parameter count
                if (func_type->data.function.param_count != node->data.function_call.arg_count) {
                    goo_report_error(node->line, node->column, 
                                   "Function '%s' expects %d arguments, got %d",
                                   node->data.function_call.name,
                                   func_type->data.function.param_count,
                                   node->data.function_call.arg_count);
                    ctx->error_count++;
                    return NULL;
                }
                
                // Check parameter types
                for (int i = 0; i < node->data.function_call.arg_count; i++) {
                    GooType* arg_type = goo_type_check_node(ctx, node->data.function_call.args[i]);
                    
                    if (!arg_type) {
                        return NULL;
                    }
                    
                    if (arg_type->kind != func_type->data.function.param_types[i]->kind) {
                        goo_report_error(node->line, node->column, 
                                       "Type mismatch in argument %d of call to '%s'",
                                       i + 1, node->data.function_call.name);
                        ctx->error_count++;
                        return NULL;
                    }
                }
                
                type = func_type->data.function.return_type;
            }
            break;
            
        // Implement other AST node types like GOO_AST_IF, GOO_AST_WHILE, etc.
        
        default:
            goo_report_error(node->line, node->column, "Unsupported AST node type for type checking");
            ctx->error_count++;
            return NULL;
    }
    
    // Store the type in the AST node for later use
    node->type = type;
    
    return type;
}

/* Type check a module */
bool goo_type_check_module(GooTypeContext* ctx, GooAstNode* module) {
    if (!ctx || !module || module->kind != GOO_AST_MODULE) {
        return false;
    }
    
    ctx->current_module = module;
    
    // Create a new scope for the module
    goo_symbol_table_enter_scope(ctx->symbols);
    
    // First pass: gather all declarations to allow forward references
    for (int i = 0; i < module->data.module.declaration_count; i++) {
        GooAstNode* decl = module->data.module.declarations[i];
        
        if (decl->kind == GOO_AST_FUNCTION_DECLARATION) {
            // Add function declaration to symbol table
            const char* name = decl->data.function_declaration.name;
            GooType* return_type = goo_resolve_type_node(ctx, decl->data.function_declaration.return_type);
            
            if (!return_type) {
                ctx->error_count++;
                continue;
            }
            
            int param_count = decl->data.function_declaration.param_count;
            GooType** param_types = malloc(param_count * sizeof(GooType*));
            
            if (!param_types) {
                goo_report_error(decl->line, decl->column, "Out of memory");
                ctx->error_count++;
                continue;
            }
            
            for (int j = 0; j < param_count; j++) {
                param_types[j] = goo_resolve_type_node(
                    ctx, decl->data.function_declaration.params[j]->data.parameter.type);
                
                if (!param_types[j]) {
                    ctx->error_count++;
                    break;
                }
            }
            
            GooType* func_type = goo_type_table_create_function_type(
                ctx->types, return_type, param_types, param_count);
            
            free(param_types);
            
            if (!func_type) {
                goo_report_error(decl->line, decl->column, "Failed to create function type");
                ctx->error_count++;
                continue;
            }
            
            GooSymbol* symbol = goo_symbol_create(GOO_SYMBOL_FUNCTION, name);
            if (!symbol) {
                goo_report_error(decl->line, decl->column, "Out of memory");
                ctx->error_count++;
                continue;
            }
            
            symbol->type = func_type;
            symbol->node = decl;
            
            if (!goo_symbol_table_add(ctx->symbols, symbol)) {
                goo_report_error(decl->line, decl->column, 
                               "Duplicate declaration of '%s'", name);
                goo_symbol_destroy(symbol);
                ctx->error_count++;
                continue;
            }
        }
        // Add similar handling for other declaration types
    }
    
    // Second pass: type check all declarations
    for (int i = 0; i < module->data.module.declaration_count; i++) {
        GooAstNode* decl = module->data.module.declarations[i];
        
        if (decl->kind == GOO_AST_FUNCTION_DECLARATION) {
            // Type check function body
            goo_symbol_table_enter_scope(ctx->symbols);
            
            // Add parameters to scope
            for (int j = 0; j < decl->data.function_declaration.param_count; j++) {
                GooAstNode* param = decl->data.function_declaration.params[j];
                const char* param_name = param->data.parameter.name;
                GooType* param_type = goo_resolve_type_node(ctx, param->data.parameter.type);
                
                if (!param_type) {
                    ctx->error_count++;
                    continue;
                }
                
                GooSymbol* param_symbol = goo_symbol_create(GOO_SYMBOL_VARIABLE, param_name);
                if (!param_symbol) {
                    goo_report_error(param->line, param->column, "Out of memory");
                    ctx->error_count++;
                    continue;
                }
                
                param_symbol->type = param_type;
                
                if (!goo_symbol_table_add(ctx->symbols, param_symbol)) {
                    goo_report_error(param->line, param->column, 
                                   "Duplicate parameter name '%s'", param_name);
                    goo_symbol_destroy(param_symbol);
                    ctx->error_count++;
                    continue;
                }
            }
            
            // Save current state
            bool prev_in_function = ctx->in_function;
            ctx->in_function = true;
            
            // Type check function body
            GooType* body_type = goo_type_check_node(ctx, decl->data.function_declaration.body);
            
            // Restore state
            ctx->in_function = prev_in_function;
            
            // Leave the function scope
            goo_symbol_table_leave_scope(ctx->symbols);
            
            if (!body_type) {
                continue;
            }
            
            // Check return type compatibility
            GooSymbol* func_symbol = goo_symbol_table_lookup(
                ctx->symbols, decl->data.function_declaration.name);
            
            if (!func_symbol) {
                goo_report_error(decl->line, decl->column, 
                               "Internal error: function symbol not found");
                ctx->error_count++;
                continue;
            }
            
            GooType* return_type = func_symbol->type->data.function.return_type;
            
            // Simple type compatibility check for now
            if (body_type->kind != return_type->kind && 
                !(return_type->kind == GOO_TYPE_VOID && body_type->kind == GOO_TYPE_UNIT)) {
                goo_report_error(decl->line, decl->column, 
                               "Function body type does not match declared return type");
                ctx->error_count++;
            }
        }
        // Add similar handling for other declaration types
    }
    
    // Leave the module scope
    goo_symbol_table_leave_scope(ctx->symbols);
    
    return ctx->error_count == 0;
}

/* Type check an entire AST */
bool goo_type_check(GooAst* ast) {
    if (!ast) {
        return false;
    }
    
    GooTypeContext* ctx = goo_type_context_create();
    if (!ctx) {
        return false;
    }
    
    bool result = true;
    
    // Type check each module in the AST
    for (int i = 0; i < ast->module_count; i++) {
        if (!goo_type_check_module(ctx, ast->modules[i])) {
            result = false;
        }
    }
    
    int error_count = goo_type_context_get_error_count(ctx);
    if (error_count > 0) {
        fprintf(stderr, "Type checking failed with %d error(s)\n", error_count);
        result = false;
    }
    
    goo_type_context_free(ctx);
    
    return result;
}

/* Resolve a type from a type node */
GooType* goo_resolve_type_node(GooTypeContext* ctx, GooAstNode* type_node) {
    if (!ctx || !type_node) {
        return NULL;
    }
    
    switch (type_node->kind) {
        case GOO_AST_TYPE_NAME:
            {
                const char* type_name = type_node->data.type_name.name;
                GooSymbol* type_symbol = goo_symbol_table_lookup_type_symbol(ctx->symbols, type_name);
                
                if (!type_symbol) {
                    goo_report_error(type_node->line, type_node->column, 
                                   "Undefined type '%s'", type_name);
                    ctx->error_count++;
                    return NULL;
                }
                
                return type_symbol->type;
            }
            
        case GOO_AST_ARRAY_TYPE:
            {
                GooType* element_type = goo_resolve_type_node(ctx, type_node->data.array_type.element_type);
                
                if (!element_type) {
                    return NULL;
                }
                
                return goo_type_table_create_array_type(ctx->types, element_type);
            }
            
        case GOO_AST_FUNCTION_TYPE:
            {
                GooType* return_type = goo_resolve_type_node(ctx, type_node->data.function_type.return_type);
                
                if (!return_type) {
                    return NULL;
                }
                
                int param_count = type_node->data.function_type.param_count;
                GooType** param_types = malloc(param_count * sizeof(GooType*));
                
                if (!param_types) {
                    goo_report_error(type_node->line, type_node->column, "Out of memory");
                    ctx->error_count++;
                    return NULL;
                }
                
                for (int i = 0; i < param_count; i++) {
                    param_types[i] = goo_resolve_type_node(ctx, type_node->data.function_type.param_types[i]);
                    
                    if (!param_types[i]) {
                        free(param_types);
                        return NULL;
                    }
                }
                
                GooType* function_type = goo_type_table_create_function_type(
                    ctx->types, return_type, param_types, param_count);
                
                free(param_types);
                
                return function_type;
            }
            
        default:
            goo_report_error(type_node->line, type_node->column, 
                           "Invalid type expression");
            ctx->error_count++;
            return NULL;
    }
}
