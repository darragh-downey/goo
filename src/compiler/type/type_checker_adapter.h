/**
 * type_checker_adapter.h
 *
 * Header for integrating diagnostics with the type checker
 *
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_CHECKER_ADAPTER_H
#define GOO_TYPE_CHECKER_ADAPTER_H

#include <stdbool.h>

// Forward declarations for type system types
typedef struct GooTypeContext GooTypeContext;
typedef struct GooType GooType;

// Forward declarations for node and diagnostics structures
typedef struct GooAstNode GooAstNode;
typedef struct GooDiagContext GooDiagContext;

// Type checker context
typedef struct GooTypeCheckerContext GooTypeCheckerContext;

/**
 * Create a new type checker context
 * 
 * @param type_ctx Type system context to use
 * @return Newly created type checker context
 */
GooTypeCheckerContext* goo_typechecker_create_context(GooTypeContext* type_ctx);

/**
 * Free a type checker context
 * 
 * @param ctx Context to free
 */
void goo_typechecker_free_context(GooTypeCheckerContext* ctx);

/**
 * Register a function to convert types to strings
 * 
 * @param ctx Type checker context
 * @param func Function pointer to type conversion function
 */
void goo_typechecker_register_type_to_string(GooTypeCheckerContext* ctx, 
                                           char* (*func)(const GooType*));

/**
 * Report a general type error
 * 
 * @param ctx Type checker context
 * @param node AST node where the error occurred
 * @param code Error code
 * @param message Error message
 */
void goo_typechecker_report_error(GooTypeCheckerContext* ctx, 
                                const GooAstNode* node,
                                const char* code, 
                                const char* message);

/**
 * Report a type mismatch error
 * 
 * @param ctx Type checker context
 * @param node AST node where the error occurred
 * @param expected Expected type
 * @param found Actual type found
 */
void goo_typechecker_report_type_mismatch(GooTypeCheckerContext* ctx, 
                                        const GooAstNode* node,
                                        const GooType* expected, 
                                        const GooType* found);

/**
 * Add a note to the current error
 * 
 * @param ctx Type checker context
 * @param message Note message
 */
void goo_typechecker_add_note(GooTypeCheckerContext* ctx, const char* message);

/**
 * Add a suggestion to the current error
 * 
 * @param ctx Type checker context
 * @param message Suggestion message
 */
void goo_typechecker_add_suggestion(GooTypeCheckerContext* ctx, const char* message);

/**
 * Get the current error count
 * 
 * @param ctx Type checker context
 * @return Number of errors reported so far
 */
int goo_typechecker_get_error_count(GooTypeCheckerContext* ctx);

/**
 * Check if error limit has been reached
 * 
 * @param ctx Type checker context
 * @return true if error limit has been reached, false otherwise
 */
bool goo_typechecker_error_limit_reached(GooTypeCheckerContext* ctx);

/**
 * Print diagnostic summary
 * 
 * @param ctx Type checker context
 */
void goo_typechecker_print_diagnostics(GooTypeCheckerContext* ctx);

#endif /* GOO_TYPE_CHECKER_ADAPTER_H */ 