/**
 * goo_type_system.c
 * 
 * Enhanced type system implementation for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "goo_type_system.h"
#include "../../../include/goo_diagnostics.h"

// Helper function to allocate memory with error checking
static void* safe_malloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Helper function to duplicate a string with error checking
static char* safe_strdup(const char* str) {
    if (!str) return NULL;
    
    char* dup = strdup(str);
    if (!dup) {
        fprintf(stderr, "Error: String duplication failed\n");
        exit(EXIT_FAILURE);
    }
    return dup;
}

// Create a new type system context
GooTypeContext* goo_type_system_create(GooDiagnosticContext* diagnostics) {
    GooTypeContext* ctx = (GooTypeContext*)safe_malloc(sizeof(GooTypeContext));
    
    // Initialize the type table
    ctx->type_table = goo_type_table_create();
    if (!ctx->type_table) {
        free(ctx);
        return NULL;
    }
    
    // Initialize type variables array
    ctx->type_vars = NULL;
    ctx->type_var_count = 0;
    ctx->next_type_var_id = 1;  // Start IDs at 1
    
    // Initialize lifetimes array
    ctx->lifetimes = NULL;
    ctx->lifetime_count = 0;
    ctx->current_scope_level = 0;
    
    // Initialize 'self' type
    ctx->self_type = NULL;
    
    // Store diagnostics context
    ctx->diagnostics = diagnostics;
    
    // Initialize unsafe block flag
    ctx->in_unsafe_block = false;
    
    return ctx;
}

// Destroy a type system context
void goo_type_system_destroy(GooTypeContext* ctx) {
    if (!ctx) return;
    
    // Destroy the type table
    if (ctx->type_table) {
        goo_type_table_destroy(ctx->type_table);
    }
    
    // Free type variables
    for (int i = 0; i < ctx->type_var_count; i++) {
        GooTypeVar* var = ctx->type_vars[i];
        
        // Free constraints
        GooTypeConstraint* constraint = var->constraints;
        while (constraint) {
            GooTypeConstraint* next = constraint->next;
            free(constraint);
            constraint = next;
        }
        
        free(var);
    }
    free(ctx->type_vars);
    
    // Free lifetimes
    for (int i = 0; i < ctx->lifetime_count; i++) {
        free(ctx->lifetimes[i]->name);
        free(ctx->lifetimes[i]);
    }
    free(ctx->lifetimes);
    
    // Free the context itself
    free(ctx);
}

// Create a type variable
GooTypeVar* goo_type_system_create_type_var(GooTypeContext* ctx) {
    GooTypeVar* var = (GooTypeVar*)safe_malloc(sizeof(GooTypeVar));
    var->id = ctx->next_type_var_id++;
    var->resolved_type = NULL;
    var->constraints = NULL;
    
    // Add to the context's type variables array
    ctx->type_vars = (GooTypeVar**)realloc(ctx->type_vars, sizeof(GooTypeVar*) * (ctx->type_var_count + 1));
    if (!ctx->type_vars) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    ctx->type_vars[ctx->type_var_count++] = var;
    
    return var;
}

// Create an integer type
GooType* goo_type_system_create_int_type(GooTypeContext* ctx, GooIntWidth width, bool is_signed) {
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_INT;
    type->int_info.width = width;
    type->int_info.is_signed = is_signed;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    return type;
}

// Create a float type
GooType* goo_type_system_create_float_type(GooTypeContext* ctx, GooFloatPrecision precision) {
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_FLOAT;
    type->float_info.precision = precision;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    return type;
}

// Create a boolean type
GooType* goo_type_system_create_bool_type(GooTypeContext* ctx) {
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_BOOL;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    return type;
}

// Create a char type
GooType* goo_type_system_create_char_type(GooTypeContext* ctx) {
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_CHAR;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    return type;
}

// Create a string type
GooType* goo_type_system_create_string_type(GooTypeContext* ctx) {
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_STRING;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    return type;
}

// Create an array type
GooType* goo_type_system_create_array_type(GooTypeContext* ctx, GooType* element_type, size_t size) {
    if (!element_type) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_ARRAY;
    type->array_info.element_type = element_type;
    type->array_info.size = size;
    
    // Set metadata
    type->is_capability = element_type->is_capability;
    type->is_copyable = element_type->is_copyable && size > 0;
    type->is_sized = size > 0 && element_type->is_sized;
    type->is_thread_safe = element_type->is_thread_safe;
    
    return type;
}

// Create a slice type
GooType* goo_type_system_create_slice_type(GooTypeContext* ctx, GooType* element_type) {
    if (!element_type) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_SLICE;
    type->slice_info.element_type = element_type;
    
    // Set metadata
    type->is_capability = element_type->is_capability;
    type->is_copyable = false;  // Slices are not trivially copyable (they're borrowed views)
    type->is_sized = true;      // Slice metadata has a fixed size
    type->is_thread_safe = element_type->is_thread_safe;
    
    return type;
}

// Create a tuple type
GooType* goo_type_system_create_tuple_type(GooTypeContext* ctx, GooType** element_types, int element_count) {
    if (!element_types || element_count <= 0) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_TUPLE;
    
    // Copy the element types
    type->tuple_info.element_types = (GooType**)safe_malloc(sizeof(GooType*) * element_count);
    for (int i = 0; i < element_count; i++) {
        type->tuple_info.element_types[i] = element_types[i];
    }
    type->tuple_info.element_count = element_count;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    // Check if any element affects the tuple's properties
    for (int i = 0; i < element_count; i++) {
        GooType* elem = element_types[i];
        if (elem->is_capability) type->is_capability = true;
        if (!elem->is_copyable) type->is_copyable = false;
        if (!elem->is_sized) type->is_sized = false;
        if (!elem->is_thread_safe) type->is_thread_safe = false;
    }
    
    return type;
}

// Create a function type
GooType* goo_type_system_create_function_type(GooTypeContext* ctx, GooType* return_type, 
                                              GooType** param_types, int param_count,
                                              bool is_unsafe, bool is_kernel) {
    if (!return_type) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_FUNCTION;
    type->function_info.return_type = return_type;
    type->function_info.is_unsafe = is_unsafe;
    type->function_info.is_kernel = is_kernel;
    type->function_info.param_lifetimes = NULL;  // No lifetimes by default
    
    // Copy parameter types if any
    if (param_count > 0 && param_types) {
        type->function_info.param_types = (GooType**)safe_malloc(sizeof(GooType*) * param_count);
        for (int i = 0; i < param_count; i++) {
            type->function_info.param_types[i] = param_types[i];
        }
    } else {
        type->function_info.param_types = NULL;
    }
    type->function_info.param_count = param_count;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;  // Function types are references, so they're copyable
    type->is_sized = true;     // Function pointers have a fixed size
    type->is_thread_safe = true;
    
    return type;
}

// Create a reference type
GooType* goo_type_system_create_ref_type(GooTypeContext* ctx, GooType* referenced_type,
                                         GooLifetime* lifetime, bool is_mutable) {
    if (!referenced_type) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = is_mutable ? GOO_TYPE_MUT_REF : GOO_TYPE_REF;
    type->ref_info.referenced_type = referenced_type;
    type->ref_info.lifetime = lifetime;
    type->ref_info.region = NULL;  // No region by default
    
    // Set metadata
    type->is_capability = referenced_type->is_capability;
    type->is_copyable = true;  // References are copyable
    type->is_sized = true;     // References have a fixed size
    type->is_thread_safe = is_mutable ? false : referenced_type->is_thread_safe;
    
    return type;
}

// Create a channel type
GooType* goo_type_system_create_channel_type(GooTypeContext* ctx, GooType* element_type,
                                            int buffer_size, bool is_distributed) {
    if (!element_type) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_CHANNEL;
    type->channel_info.element_type = element_type;
    type->channel_info.buffer_size = buffer_size;
    type->channel_info.is_distributed = is_distributed;
    
    // Set metadata
    type->is_capability = true;  // Channels are inherently capability types
    type->is_copyable = false;   // Channels are not trivially copyable
    type->is_sized = true;       // Channels have a fixed size (they're reference types)
    type->is_thread_safe = true; // Channels are designed for concurrent use
    
    return type;
}

// Create a struct type
GooType* goo_type_system_create_struct_type(GooTypeContext* ctx, const char* name,
                                           const char** field_names, GooType** field_types,
                                           int field_count) {
    if (!name || (field_count > 0 && (!field_names || !field_types))) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_STRUCT;
    type->struct_info.name = safe_strdup(name);
    type->struct_info.impl_count = 0;
    type->struct_info.impls = NULL;
    
    // Copy field information
    if (field_count > 0) {
        type->struct_info.field_names = (char**)safe_malloc(sizeof(char*) * field_count);
        type->struct_info.field_types = (GooType**)safe_malloc(sizeof(GooType*) * field_count);
        
        for (int i = 0; i < field_count; i++) {
            type->struct_info.field_names[i] = safe_strdup(field_names[i]);
            type->struct_info.field_types[i] = field_types[i];
        }
    } else {
        type->struct_info.field_names = NULL;
        type->struct_info.field_types = NULL;
    }
    type->struct_info.field_count = field_count;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = true;
    type->is_sized = true;
    type->is_thread_safe = true;
    
    // Check if any field affects the struct's properties
    for (int i = 0; i < field_count; i++) {
        GooType* field_type = field_types[i];
        if (field_type->is_capability) type->is_capability = true;
        if (!field_type->is_copyable) type->is_copyable = false;
        if (!field_type->is_sized) type->is_sized = false;
        if (!field_type->is_thread_safe) type->is_thread_safe = false;
    }
    
    return type;
}

// Create an enum type
GooType* goo_type_system_create_enum_type(GooTypeContext* ctx, const char* name,
                                         const char** variant_names, GooType** variant_types,
                                         int variant_count) {
    if (!name || !variant_names || variant_count <= 0) return NULL;
    
    GooType* type = (GooType*)safe_malloc(sizeof(GooType));
    type->kind = GOO_TYPE_ENUM;
    type->enum_info.name = safe_strdup(name);
    
    // Copy variant information
    type->enum_info.variant_names = (char**)safe_malloc(sizeof(char*) * variant_count);
    if (variant_types) {
        type->enum_info.variant_types = (GooType**)safe_malloc(sizeof(GooType*) * variant_count);
        for (int i = 0; i < variant_count; i++) {
            type->enum_info.variant_names[i] = safe_strdup(variant_names[i]);
            type->enum_info.variant_types[i] = variant_types[i];
        }
    } else {
        type->enum_info.variant_types = NULL;
        for (int i = 0; i < variant_count; i++) {
            type->enum_info.variant_names[i] = safe_strdup(variant_names[i]);
        }
    }
    type->enum_info.variant_count = variant_count;
    
    // Set metadata
    type->is_capability = false;
    type->is_copyable = variant_types == NULL;  // Simple enums are copyable
    type->is_sized = true;
    type->is_thread_safe = true;
    
    // Check if any variant affects the enum's properties
    if (variant_types) {
        for (int i = 0; i < variant_count; i++) {
            GooType* variant_type = variant_types[i];
            if (variant_type) {
                if (variant_type->is_capability) type->is_capability = true;
                if (!variant_type->is_sized) type->is_sized = false;
                if (!variant_type->is_thread_safe) type->is_thread_safe = false;
            }
        }
    }
    
    return type;
}

// Resolve a type variable to its concrete type
GooType* goo_type_system_resolve_type_var(GooTypeContext* ctx, GooTypeVar* var) {
    if (!var) return NULL;
    return var->resolved_type;
}

// Utility function to compare if two types are equal
bool goo_type_system_types_equal(GooTypeContext* ctx, GooType* a, GooType* b) {
    if (!a || !b) return a == b;
    
    // Handle type variables
    if (a->kind == GOO_TYPE_VAR) {
        GooType* resolved_a = goo_type_system_resolve_type_var(ctx, a->type_var);
        if (resolved_a) return goo_type_system_types_equal(ctx, resolved_a, b);
    }
    
    if (b->kind == GOO_TYPE_VAR) {
        GooType* resolved_b = goo_type_system_resolve_type_var(ctx, b->type_var);
        if (resolved_b) return goo_type_system_types_equal(ctx, a, resolved_b);
    }
    
    // If both are type variables and unresolved, compare the variables
    if (a->kind == GOO_TYPE_VAR && b->kind == GOO_TYPE_VAR) {
        return a->type_var->id == b->type_var->id;
    }
    
    // Different kinds means different types
    if (a->kind != b->kind) return false;
    
    // Compare based on type kind
    switch (a->kind) {
        case GOO_TYPE_VOID:
        case GOO_TYPE_UNIT:
        case GOO_TYPE_BOOL:
        case GOO_TYPE_CHAR:
        case GOO_TYPE_STRING:
        case GOO_TYPE_NEVER:
        case GOO_TYPE_UNKNOWN:
        case GOO_TYPE_ANY:
            return true;  // These types have no parameters to compare
            
        case GOO_TYPE_INT:
            return a->int_info.width == b->int_info.width && 
                   a->int_info.is_signed == b->int_info.is_signed;
                   
        case GOO_TYPE_FLOAT:
            return a->float_info.precision == b->float_info.precision;
            
        case GOO_TYPE_ARRAY:
            return a->array_info.size == b->array_info.size &&
                   goo_type_system_types_equal(ctx, a->array_info.element_type, b->array_info.element_type);
                   
        case GOO_TYPE_SLICE:
            return goo_type_system_types_equal(ctx, a->slice_info.element_type, b->slice_info.element_type);
            
        case GOO_TYPE_TUPLE:
            if (a->tuple_info.element_count != b->tuple_info.element_count) return false;
            for (int i = 0; i < a->tuple_info.element_count; i++) {
                if (!goo_type_system_types_equal(ctx, a->tuple_info.element_types[i], b->tuple_info.element_types[i])) {
                    return false;
                }
            }
            return true;
            
        case GOO_TYPE_STRUCT:
            // Compare by name for now (nominal typing)
            return strcmp(a->struct_info.name, b->struct_info.name) == 0;
            
        case GOO_TYPE_ENUM:
            // Compare by name for now (nominal typing)
            return strcmp(a->enum_info.name, b->enum_info.name) == 0;
            
        case GOO_TYPE_FUNCTION:
            if (!goo_type_system_types_equal(ctx, a->function_info.return_type, b->function_info.return_type)) {
                return false;
            }
            if (a->function_info.param_count != b->function_info.param_count) {
                return false;
            }
            for (int i = 0; i < a->function_info.param_count; i++) {
                if (!goo_type_system_types_equal(ctx, a->function_info.param_types[i], b->function_info.param_types[i])) {
                    return false;
                }
            }
            return true;
            
        case GOO_TYPE_REF:
        case GOO_TYPE_MUT_REF:
            return goo_type_system_types_equal(ctx, a->ref_info.referenced_type, b->ref_info.referenced_type);
            
        case GOO_TYPE_CHANNEL:
            return goo_type_system_types_equal(ctx, a->channel_info.element_type, b->channel_info.element_type);
            
        default:
            // For other complex types, we need more specific comparison logic
            return false;
    }
}

// Create a lifetime
GooLifetime* goo_type_system_create_lifetime(GooTypeContext* ctx, const char* name, bool is_static) {
    GooLifetime* lifetime = (GooLifetime*)safe_malloc(sizeof(GooLifetime));
    lifetime->name = name ? safe_strdup(name) : NULL;
    lifetime->scope_level = is_static ? 0 : ctx->current_scope_level;
    lifetime->is_static = is_static;
    lifetime->is_anonymous = name == NULL;
    
    // Add to the context's lifetimes array
    ctx->lifetimes = (GooLifetime**)realloc(ctx->lifetimes, sizeof(GooLifetime*) * (ctx->lifetime_count + 1));
    if (!ctx->lifetimes) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    ctx->lifetimes[ctx->lifetime_count++] = lifetime;
    
    return lifetime;
}

// Check if lifetime a outlives lifetime b
bool goo_type_system_lifetime_outlives(GooTypeContext* ctx, GooLifetime* a, GooLifetime* b) {
    if (!a || !b) return false;
    
    // Static lifetimes outlive everything
    if (a->is_static) return true;
    if (b->is_static) return false;
    
    // A lifetime at a lower or equal scope level outlives one at a higher level
    return a->scope_level <= b->scope_level;
}

// Enter a new scope
void goo_type_system_enter_scope(GooTypeContext* ctx) {
    ctx->current_scope_level++;
}

// Exit the current scope
void goo_type_system_exit_scope(GooTypeContext* ctx) {
    if (ctx->current_scope_level > 0) {
        ctx->current_scope_level--;
    }
} 