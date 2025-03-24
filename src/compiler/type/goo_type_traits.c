/**
 * goo_type_traits.c
 * 
 * Trait system implementation for the Goo programming language's type system
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

// Create a trait
GooTrait* goo_type_system_create_trait(GooTypeContext* ctx, const char* name,
                                       const char** method_names, GooType** method_types,
                                       int method_count) {
    if (!name || (method_count > 0 && (!method_names || !method_types))) return NULL;
    
    GooTrait* trait = (GooTrait*)safe_malloc(sizeof(GooTrait));
    trait->name = safe_strdup(name);
    
    // Initialize type parameters
    trait->type_params = NULL;
    trait->type_param_count = 0;
    
    // Initialize super traits
    trait->super_traits = NULL;
    trait->super_trait_count = 0;
    
    // Copy method information
    if (method_count > 0) {
        trait->method_names = (char**)safe_malloc(sizeof(char*) * method_count);
        trait->method_types = (GooType**)safe_malloc(sizeof(GooType*) * method_count);
        
        for (int i = 0; i < method_count; i++) {
            trait->method_names[i] = safe_strdup(method_names[i]);
            trait->method_types[i] = method_types[i];
        }
    } else {
        trait->method_names = NULL;
        trait->method_types = NULL;
    }
    trait->method_count = method_count;
    
    return trait;
}

// Add a type parameter to a trait
void goo_type_system_add_trait_type_param(GooTypeContext* ctx, GooTrait* trait, GooTypeVar* param) {
    if (!trait || !param) return;
    
    trait->type_params = (GooTypeVar**)realloc(trait->type_params, 
                                              sizeof(GooTypeVar*) * (trait->type_param_count + 1));
    if (!trait->type_params) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    trait->type_params[trait->type_param_count++] = param;
}

// Add a super trait
void goo_type_system_add_super_trait(GooTypeContext* ctx, GooTrait* trait, GooTrait* super_trait) {
    if (!trait || !super_trait) return;
    
    trait->super_traits = (GooTrait**)realloc(trait->super_traits, 
                                            sizeof(GooTrait*) * (trait->super_trait_count + 1));
    if (!trait->super_traits) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    trait->super_traits[trait->super_trait_count++] = super_trait;
}

// Create a trait implementation
GooTypeImpl* goo_type_system_create_impl(GooTypeContext* ctx, GooType* type, GooTrait* trait,
                                         GooType** type_args, int type_arg_count) {
    if (!type || !trait) return NULL;
    
    GooTypeImpl* impl = (GooTypeImpl*)safe_malloc(sizeof(GooTypeImpl));
    impl->type = type;
    impl->trait = trait;
    
    // Copy type arguments if provided
    if (type_arg_count > 0 && type_args) {
        impl->type_args = (GooType**)safe_malloc(sizeof(GooType*) * type_arg_count);
        for (int i = 0; i < type_arg_count; i++) {
            impl->type_args[i] = type_args[i];
        }
    } else {
        impl->type_args = NULL;
    }
    impl->type_arg_count = type_arg_count;
    
    // Initialize method implementations
    impl->method_impls = NULL;
    impl->method_count = 0;
    
    // Add implementation to the type if it's a struct
    if (type->kind == GOO_TYPE_STRUCT) {
        type->struct_info.impls = (GooTypeImpl**)realloc(type->struct_info.impls, 
                                                        sizeof(GooTypeImpl*) * (type->struct_info.impl_count + 1));
        if (!type->struct_info.impls) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        
        type->struct_info.impls[type->struct_info.impl_count++] = impl;
    }
    
    return impl;
}

// Add a method implementation to a trait implementation
void goo_type_system_add_method_impl(GooTypeContext* ctx, GooTypeImpl* impl, const char* method_name) {
    if (!impl || !method_name) return;
    
    impl->method_impls = (char**)realloc(impl->method_impls, 
                                        sizeof(char*) * (impl->method_count + 1));
    if (!impl->method_impls) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    
    impl->method_impls[impl->method_count++] = safe_strdup(method_name);
}

// Check if a type implements a trait
bool goo_type_system_type_implements_trait(GooTypeContext* ctx, GooType* type, GooTrait* trait,
                                          GooType*** out_type_args) {
    if (!type || !trait) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_type_implements_trait(ctx, resolved, trait, out_type_args);
        }
        
        // Check type variable constraints
        GooTypeConstraint* constraint = type->type_var->constraints;
        while (constraint) {
            if (constraint->kind == GOO_CONSTRAINT_TRAIT && constraint->trait == trait) {
                return true;
            }
            constraint = constraint->next;
        }
        
        return false;
    }
    
    // For now, only structs can implement traits
    if (type->kind != GOO_TYPE_STRUCT) return false;
    
    // Check direct implementations
    for (int i = 0; i < type->struct_info.impl_count; i++) {
        GooTypeImpl* impl = type->struct_info.impls[i];
        
        if (impl->trait == trait) {
            // If requested, return the type arguments
            if (out_type_args && impl->type_arg_count > 0) {
                *out_type_args = impl->type_args;
            }
            return true;
        }
        
        // Check super traits
        for (int j = 0; j < impl->trait->super_trait_count; j++) {
            if (impl->trait->super_traits[j] == trait) {
                // If requested, return the type arguments
                if (out_type_args && impl->type_arg_count > 0) {
                    *out_type_args = impl->type_args;
                }
                return true;
            }
        }
    }
    
    return false;
}

// Add a trait constraint to a type variable
void goo_type_system_add_trait_constraint(GooTypeContext* ctx, GooTypeVar* var, GooTrait* trait) {
    if (!var || !trait) return;
    
    GooTypeConstraint* constraint = (GooTypeConstraint*)safe_malloc(sizeof(GooTypeConstraint));
    constraint->kind = GOO_CONSTRAINT_TRAIT;
    constraint->trait = trait;
    constraint->next = NULL;
    
    // Add to the constraint list
    if (!var->constraints) {
        var->constraints = constraint;
    } else {
        GooTypeConstraint* last = var->constraints;
        while (last->next) {
            last = last->next;
        }
        last->next = constraint;
    }
}

// Add a subtype constraint to a type variable
void goo_type_system_add_subtype_constraint(GooTypeContext* ctx, GooTypeVar* var, GooType* super_type) {
    if (!var || !super_type) return;
    
    GooTypeConstraint* constraint = (GooTypeConstraint*)safe_malloc(sizeof(GooTypeConstraint));
    constraint->kind = GOO_CONSTRAINT_SUBTYPE;
    constraint->subtype_of = super_type;
    constraint->next = NULL;
    
    // Add to the constraint list
    if (!var->constraints) {
        var->constraints = constraint;
    } else {
        GooTypeConstraint* last = var->constraints;
        while (last->next) {
            last = last->next;
        }
        last->next = constraint;
    }
}

// Add an equality constraint to a type variable
void goo_type_system_add_equality_constraint(GooTypeContext* ctx, GooTypeVar* var, GooType* type) {
    if (!var || !type) return;
    
    GooTypeConstraint* constraint = (GooTypeConstraint*)safe_malloc(sizeof(GooTypeConstraint));
    constraint->kind = GOO_CONSTRAINT_EQUALITY;
    constraint->equal_to = type;
    constraint->next = NULL;
    
    // Add to the constraint list
    if (!var->constraints) {
        var->constraints = constraint;
    } else {
        GooTypeConstraint* last = var->constraints;
        while (last->next) {
            last = last->next;
        }
        last->next = constraint;
    }
}

// Check if a type is a subtype of another
bool goo_type_system_is_subtype(GooTypeContext* ctx, GooType* sub, GooType* super) {
    if (!sub || !super) return false;
    
    // Any type is a subtype of itself
    if (goo_type_system_types_equal(ctx, sub, super)) return true;
    
    // Any type is a subtype of Any
    if (super->kind == GOO_TYPE_ANY) return true;
    
    // Never is a subtype of any type
    if (sub->kind == GOO_TYPE_NEVER) return true;
    
    // Handle type variables
    if (sub->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, sub->type_var);
        if (resolved) {
            return goo_type_system_is_subtype(ctx, resolved, super);
        }
        
        // Check type variable constraints
        GooTypeConstraint* constraint = sub->type_var->constraints;
        while (constraint) {
            if (constraint->kind == GOO_CONSTRAINT_SUBTYPE && 
                goo_type_system_types_equal(ctx, constraint->subtype_of, super)) {
                return true;
            }
            constraint = constraint->next;
        }
        
        return false;
    }
    
    if (super->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, super->type_var);
        if (resolved) {
            return goo_type_system_is_subtype(ctx, sub, resolved);
        }
        
        return false;
    }
    
    // Handle trait objects
    if (super->kind == GOO_TYPE_TRAIT_OBJECT) {
        GooTrait* trait = super->trait_object_info.trait;
        return goo_type_system_type_implements_trait(ctx, sub, trait, NULL);
    }
    
    // Handle specific subtyping relationships
    switch (sub->kind) {
        case GOO_TYPE_INT:
            // Handle integer subtyping rules (e.g., i32 -> i64)
            if (super->kind == GOO_TYPE_INT && super->int_info.is_signed == sub->int_info.is_signed) {
                return sub->int_info.width <= super->int_info.width;
            }
            return false;
            
        case GOO_TYPE_FLOAT:
            // Handle float subtyping rules (e.g., f32 -> f64)
            if (super->kind == GOO_TYPE_FLOAT) {
                return sub->float_info.precision <= super->float_info.precision;
            }
            return false;
            
        case GOO_TYPE_ARRAY:
            // Arrays are invariant for now
            if (super->kind == GOO_TYPE_ARRAY) {
                return sub->array_info.size == super->array_info.size &&
                       goo_type_system_types_equal(ctx, sub->array_info.element_type, super->array_info.element_type);
            }
            return false;
            
        case GOO_TYPE_SLICE:
            // Slices are covariant in their element type
            if (super->kind == GOO_TYPE_SLICE) {
                return goo_type_system_is_subtype(ctx, sub->slice_info.element_type, super->slice_info.element_type);
            }
            return false;
            
        case GOO_TYPE_REF:
            // Immutable references are covariant in their referenced type
            if (super->kind == GOO_TYPE_REF) {
                return goo_type_system_is_subtype(ctx, sub->ref_info.referenced_type, super->ref_info.referenced_type);
            }
            return false;
            
        case GOO_TYPE_MUT_REF:
            // Mutable references are invariant
            if (super->kind == GOO_TYPE_MUT_REF) {
                return goo_type_system_types_equal(ctx, sub->ref_info.referenced_type, super->ref_info.referenced_type);
            }
            return false;
            
        case GOO_TYPE_FUNCTION:
            // Function subtyping: contravariant in parameters, covariant in return type
            if (super->kind == GOO_TYPE_FUNCTION) {
                // Parameters must be contravariant
                if (sub->function_info.param_count != super->function_info.param_count) {
                    return false;
                }
                
                for (int i = 0; i < sub->function_info.param_count; i++) {
                    if (!goo_type_system_is_subtype(ctx, super->function_info.param_types[i], sub->function_info.param_types[i])) {
                        return false;
                    }
                }
                
                // Return type must be covariant
                return goo_type_system_is_subtype(ctx, sub->function_info.return_type, super->function_info.return_type);
            }
            return false;
            
        default:
            // Default is nominal typing
            return false;
    }
}

// Unify two types (for type inference)
bool goo_type_system_unify(GooTypeContext* ctx, GooType* type1, GooType* type2) {
    // Resolve type variables
    if (type1->kind == GOO_TYPE_VAR) {
        GooType* resolved1 = goo_type_system_resolve_type_var(ctx, type1->type_var);
        if (resolved1) {
            return goo_type_system_unify(ctx, resolved1, type2);
        }
    }
    
    if (type2->kind == GOO_TYPE_VAR) {
        GooType* resolved2 = goo_type_system_resolve_type_var(ctx, type2->type_var);
        if (resolved2) {
            return goo_type_system_unify(ctx, type1, resolved2);
        }
    }
    
    // If both are unresolved type variables, bind one to the other
    if (type1->kind == GOO_TYPE_VAR && type2->kind == GOO_TYPE_VAR) {
        // For simplicity, just bind the first to the second
        type1->type_var->resolved_type = type2;
        return true;
    }
    
    // If one is an unresolved type variable, bind it to the other type
    if (type1->kind == GOO_TYPE_VAR) {
        // Check constraints
        GooTypeConstraint* constraint = type1->type_var->constraints;
        while (constraint) {
            switch (constraint->kind) {
                case GOO_CONSTRAINT_SUBTYPE:
                    if (!goo_type_system_is_subtype(ctx, type2, constraint->subtype_of)) {
                        return false;
                    }
                    break;
                    
                case GOO_CONSTRAINT_TRAIT:
                    if (!goo_type_system_type_implements_trait(ctx, type2, constraint->trait, NULL)) {
                        return false;
                    }
                    break;
                    
                case GOO_CONSTRAINT_EQUALITY:
                    if (!goo_type_system_types_equal(ctx, type2, constraint->equal_to)) {
                        return false;
                    }
                    break;
                    
                default:
                    // Skip other constraint types for now
                    break;
            }
            
            constraint = constraint->next;
        }
        
        type1->type_var->resolved_type = type2;
        return true;
    }
    
    if (type2->kind == GOO_TYPE_VAR) {
        // Check constraints
        GooTypeConstraint* constraint = type2->type_var->constraints;
        while (constraint) {
            switch (constraint->kind) {
                case GOO_CONSTRAINT_SUBTYPE:
                    if (!goo_type_system_is_subtype(ctx, type1, constraint->subtype_of)) {
                        return false;
                    }
                    break;
                    
                case GOO_CONSTRAINT_TRAIT:
                    if (!goo_type_system_type_implements_trait(ctx, type1, constraint->trait, NULL)) {
                        return false;
                    }
                    break;
                    
                case GOO_CONSTRAINT_EQUALITY:
                    if (!goo_type_system_types_equal(ctx, type1, constraint->equal_to)) {
                        return false;
                    }
                    break;
                    
                default:
                    // Skip other constraint types for now
                    break;
            }
            
            constraint = constraint->next;
        }
        
        type2->type_var->resolved_type = type1;
        return true;
    }
    
    // Otherwise, types must be structurally compatible
    if (type1->kind != type2->kind) {
        return false;
    }
    
    // Compare based on type kind
    switch (type1->kind) {
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
            return type1->int_info.width == type2->int_info.width && 
                   type1->int_info.is_signed == type2->int_info.is_signed;
                   
        case GOO_TYPE_FLOAT:
            return type1->float_info.precision == type2->float_info.precision;
            
        case GOO_TYPE_ARRAY:
            return type1->array_info.size == type2->array_info.size &&
                   goo_type_system_unify(ctx, type1->array_info.element_type, type2->array_info.element_type);
                   
        case GOO_TYPE_SLICE:
            return goo_type_system_unify(ctx, type1->slice_info.element_type, type2->slice_info.element_type);
            
        case GOO_TYPE_TUPLE:
            if (type1->tuple_info.element_count != type2->tuple_info.element_count) return false;
            for (int i = 0; i < type1->tuple_info.element_count; i++) {
                if (!goo_type_system_unify(ctx, type1->tuple_info.element_types[i], type2->tuple_info.element_types[i])) {
                    return false;
                }
            }
            return true;
            
        case GOO_TYPE_STRUCT:
            // For structs, nominal typing is used
            return strcmp(type1->struct_info.name, type2->struct_info.name) == 0;
            
        case GOO_TYPE_ENUM:
            // For enums, nominal typing is used
            return strcmp(type1->enum_info.name, type2->enum_info.name) == 0;
            
        case GOO_TYPE_FUNCTION:
            if (!goo_type_system_unify(ctx, type1->function_info.return_type, type2->function_info.return_type)) {
                return false;
            }
            if (type1->function_info.param_count != type2->function_info.param_count) {
                return false;
            }
            for (int i = 0; i < type1->function_info.param_count; i++) {
                if (!goo_type_system_unify(ctx, type1->function_info.param_types[i], type2->function_info.param_types[i])) {
                    return false;
                }
            }
            return true;
            
        case GOO_TYPE_REF:
        case GOO_TYPE_MUT_REF:
            return goo_type_system_unify(ctx, type1->ref_info.referenced_type, type2->ref_info.referenced_type);
            
        case GOO_TYPE_CHANNEL:
            return goo_type_system_unify(ctx, type1->channel_info.element_type, type2->channel_info.element_type);
            
        default:
            // For other complex types, we need more specific unification logic
            return false;
    }
}

// Resolve all type variables in a type recursively
GooType* goo_type_system_resolve_type(GooTypeContext* ctx, GooType* type) {
    if (!type) return NULL;
    
    // Handle type variable
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_resolve_type(ctx, resolved);
        }
        return type;
    }
    
    // Handle composite types
    switch (type->kind) {
        case GOO_TYPE_ARRAY:
            type->array_info.element_type = goo_type_system_resolve_type(ctx, type->array_info.element_type);
            break;
            
        case GOO_TYPE_SLICE:
            type->slice_info.element_type = goo_type_system_resolve_type(ctx, type->slice_info.element_type);
            break;
            
        case GOO_TYPE_TUPLE:
            for (int i = 0; i < type->tuple_info.element_count; i++) {
                type->tuple_info.element_types[i] = goo_type_system_resolve_type(ctx, type->tuple_info.element_types[i]);
            }
            break;
            
        case GOO_TYPE_STRUCT:
            for (int i = 0; i < type->struct_info.field_count; i++) {
                type->struct_info.field_types[i] = goo_type_system_resolve_type(ctx, type->struct_info.field_types[i]);
            }
            break;
            
        case GOO_TYPE_ENUM:
            if (type->enum_info.variant_types) {
                for (int i = 0; i < type->enum_info.variant_count; i++) {
                    if (type->enum_info.variant_types[i]) {
                        type->enum_info.variant_types[i] = goo_type_system_resolve_type(ctx, type->enum_info.variant_types[i]);
                    }
                }
            }
            break;
            
        case GOO_TYPE_FUNCTION:
            type->function_info.return_type = goo_type_system_resolve_type(ctx, type->function_info.return_type);
            for (int i = 0; i < type->function_info.param_count; i++) {
                type->function_info.param_types[i] = goo_type_system_resolve_type(ctx, type->function_info.param_types[i]);
            }
            break;
            
        case GOO_TYPE_REF:
        case GOO_TYPE_MUT_REF:
            type->ref_info.referenced_type = goo_type_system_resolve_type(ctx, type->ref_info.referenced_type);
            break;
            
        case GOO_TYPE_CHANNEL:
            type->channel_info.element_type = goo_type_system_resolve_type(ctx, type->channel_info.element_type);
            break;
            
        default:
            // Other types don't contain nested types
            break;
    }
    
    return type;
} 