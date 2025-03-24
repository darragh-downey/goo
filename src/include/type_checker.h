/**
 * type_checker.h
 * 
 * Type checking interface for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_CHECKER_H
#define GOO_TYPE_CHECKER_H

#include <stdbool.h>
#include "../include/ast.h"
#include "symbol_table.h"
#include "type_table.h"

// Forward declarations
typedef struct GooTypeContext GooTypeContext;
typedef struct GooType GooType;
typedef struct GooAst GooAst;
typedef struct GooAstNode GooAstNode;

/**
 * Create a new type checking context
 * 
 * @return A new type checking context or NULL on failure
 */
GooTypeContext* goo_type_context_create(void);

/**
 * Free a type checking context
 * 
 * @param ctx The context to free
 */
void goo_type_context_free(GooTypeContext* ctx);

/**
 * Type check a single AST node
 * 
 * @param ctx The type checking context
 * @param node The AST node to type check
 * @return The type of the node or NULL on error
 */
GooType* goo_type_check_node(GooTypeContext* ctx, GooAstNode* node);

/**
 * Type check a module
 * 
 * @param ctx The type checking context
 * @param module The module AST node to type check
 * @return true if type checking succeeded, false otherwise
 */
bool goo_type_check_module(GooTypeContext* ctx, GooAstNode* module);

/**
 * Type check an entire AST
 * 
 * @param ast The AST to type check
 * @return true if type checking succeeded, false otherwise
 */
bool goo_type_check(GooAst* ast);

/**
 * Resolve a type from a type node
 * 
 * @param ctx The type checking context
 * @param type_node The type node to resolve
 * @return The resolved type or NULL on error
 */
GooType* goo_resolve_type_node(GooTypeContext* ctx, GooAstNode* type_node);

/**
 * Get the number of type errors encountered
 * 
 * @param ctx The type checking context
 * @return The number of errors
 */
int goo_type_context_get_error_count(GooTypeContext* ctx);

/**
 * Set the current module being type checked
 * 
 * @param ctx The type checking context
 * @param module The module AST node
 */
void goo_type_context_set_module(GooTypeContext* ctx, GooAstNode* module);

/**
 * Add a built-in type to the type checker
 * 
 * @param ctx The type checking context
 * @param name The type name
 * @param kind The type kind
 * @return The created type
 */
GooType* goo_type_context_add_builtin_type(GooTypeContext* ctx, const char* name, GooTypeKind kind);

/**
 * Add a built-in function to the type checker
 * 
 * @param ctx The type checking context
 * @param name The function name
 * @param return_type The return type
 * @param param_types The parameter types
 * @param param_count The number of parameters
 * @return true if successful, false otherwise
 */
bool goo_type_context_add_builtin_function(GooTypeContext* ctx, const char* name, 
                                         GooType* return_type, GooType** param_types, 
                                         int param_count);

/**
 * Get the symbol table from the type checking context
 * 
 * @param ctx The type checking context
 * @return The symbol table
 */
GooSymbolTable* goo_type_context_get_symbol_table(GooTypeContext* ctx);

/**
 * Get the type table from the type checking context
 * 
 * @param ctx The type checking context
 * @return The type table
 */
GooTypeTable* goo_type_context_get_type_table(GooTypeContext* ctx);

#endif /* GOO_TYPE_CHECKER_H */ 