/**
 * type_error_adapter.h
 * 
 * Adapter for integrating the diagnostics system with the type checker
 * without creating type conflicts
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_ERROR_ADAPTER_H
#define GOO_TYPE_ERROR_ADAPTER_H

#include <stdbool.h>
#include "type_error_codes.h"

// Forward declarations for types to avoid including headers that cause conflicts
typedef struct GooDiagnosticContext GooDiagnosticContext;
typedef struct GooTypeContext GooTypeContext;
typedef struct GooType GooType;

// Function pointer for the type to string conversion
typedef void (*TypeToStringFunc)(GooTypeContext*, GooType*, char*, size_t);

// Function to register the type to string converter
void goo_type_register_to_string_func(TypeToStringFunc func);

// Function to initialize a diagnostics context for type checking
GooDiagnosticContext* goo_type_init_diagnostics(void);

// Function to create and emit a type error
// Takes a void* for the node to avoid type conflicts with different AST implementations
void goo_type_report_error(GooTypeContext* ctx, void* node,
                           const char* error_code, const char* message);

// Function to report a type mismatch with expected and found types
void goo_type_report_mismatch(GooTypeContext* ctx, void* node,
                              GooType* expected, GooType* found);

// Function to add a note to the most recent diagnostic
void goo_type_add_note(GooTypeContext* ctx, void* node, const char* message);

// Function to add a suggestion to the most recent diagnostic
void goo_type_add_suggestion(GooTypeContext* ctx, void* node,
                             const char* message, const char* replacement);

// Function to check if we should abort due to too many errors
bool goo_type_should_abort(GooTypeContext* ctx);

// Function to get the current error count
int goo_type_error_count(GooTypeContext* ctx);

// Function to print all diagnostics
void goo_type_print_diagnostics(GooTypeContext* ctx);

#endif /* GOO_TYPE_ERROR_ADAPTER_H */ 