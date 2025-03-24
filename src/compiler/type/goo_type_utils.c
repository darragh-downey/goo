/**
 * goo_type_utils.c
 * 
 * Utility functions for the Goo type system
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

// Helper function to safely concatenate to a string buffer
static void safe_strcat(char* buffer, size_t buffer_size, const char* str) {
    size_t current_len = strlen(buffer);
    size_t str_len = strlen(str);
    
    if (current_len + str_len < buffer_size - 1) {
        strcat(buffer, str);
    }
}

// Helper function for numeric conversions
static bool can_convert_numeric(GooType* from, GooType* to) {
    // Integer to integer
    if (from->kind == GOO_TYPE_INT && to->kind == GOO_TYPE_INT) {
        // Unsigned to signed of same or larger width is safe
        if (!from->int_info.is_signed && to->int_info.is_signed && 
            from->int_info.width <= to->int_info.width) {
            return true;
        }
        
        // Signed to unsigned is unsafe (requires runtime check)
        if (from->int_info.is_signed && !to->int_info.is_signed) {
            return false;
        }
        
        // Same signedness, smaller to larger width is safe
        if (from->int_info.is_signed == to->int_info.is_signed && 
            from->int_info.width <= to->int_info.width) {
            return true;
        }
        
        // Other conversions might lose precision
        return false;
    }
    
    // Float to float
    if (from->kind == GOO_TYPE_FLOAT && to->kind == GOO_TYPE_FLOAT) {
        // f32 to f64 is safe
        if (from->float_info.precision == GOO_FLOAT_32 && 
            to->float_info.precision == GOO_FLOAT_64) {
            return true;
        }
        
        // f64 to f32 might lose precision
        return false;
    }
    
    // Integer to float
    if (from->kind == GOO_TYPE_INT && to->kind == GOO_TYPE_FLOAT) {
        // All integers can be converted to float, but might lose precision
        // Small integers can be exactly represented in f64
        if (to->float_info.precision == GOO_FLOAT_64 && 
            from->int_info.width <= GOO_INT_32) {
            return true;
        }
        
        return false;
    }
    
    // Float to integer requires runtime check
    if (from->kind == GOO_TYPE_FLOAT && to->kind == GOO_TYPE_INT) {
        return false;
    }
    
    return false;
}

// Check if one type can be converted to another
bool goo_type_system_can_convert(GooTypeContext* ctx, GooType* from, GooType* to) {
    if (!from || !to) return false;
    
    // Same types can always be converted
    if (goo_type_system_types_equal(ctx, from, to)) return true;
    
    // Numeric conversions
    if ((from->kind == GOO_TYPE_INT || from->kind == GOO_TYPE_FLOAT) &&
        (to->kind == GOO_TYPE_INT || to->kind == GOO_TYPE_FLOAT)) {
        return can_convert_numeric(from, to);
    }
    
    // References can be converted to raw pointers in unsafe context
    if ((from->kind == GOO_TYPE_REF || from->kind == GOO_TYPE_MUT_REF) && 
        ctx->in_unsafe_block) {
        // Allow conversion to raw pointers in unsafe context
        // This is a placeholder for raw pointer types
        return false;
    }
    
    // Owned types can be dereferenced
    if (from->kind == GOO_TYPE_OWNED && 
        goo_type_system_types_equal(ctx, from->ref_info.referenced_type, to)) {
        return true;
    }
    
    // Trait object conversions
    if (to->kind == GOO_TYPE_TRAIT_OBJECT) {
        // Check if from implements the trait
        return goo_type_system_type_implements_trait(ctx, from, to->trait_object_info.trait, NULL);
    }
    
    return false;
}

// Convert a type to its string representation
const char* goo_type_system_type_to_string(GooTypeContext* ctx, GooType* type, char* buffer, size_t buffer_size) {
    if (!type || !buffer || buffer_size == 0) return NULL;
    
    // Clear the buffer
    buffer[0] = '\0';
    
    // Handle different type kinds
    switch (type->kind) {
        case GOO_TYPE_VOID:
            safe_strcat(buffer, buffer_size, "void");
            break;
            
        case GOO_TYPE_UNIT:
            safe_strcat(buffer, buffer_size, "()");
            break;
            
        case GOO_TYPE_BOOL:
            safe_strcat(buffer, buffer_size, "bool");
            break;
            
        case GOO_TYPE_INT:
            if (type->int_info.is_signed) {
                switch (type->int_info.width) {
                    case GOO_INT_8:
                        safe_strcat(buffer, buffer_size, "i8");
                        break;
                    case GOO_INT_16:
                        safe_strcat(buffer, buffer_size, "i16");
                        break;
                    case GOO_INT_32:
                        safe_strcat(buffer, buffer_size, "i32");
                        break;
                    case GOO_INT_64:
                        safe_strcat(buffer, buffer_size, "i64");
                        break;
                    case GOO_INT_128:
                        safe_strcat(buffer, buffer_size, "i128");
                        break;
                    case GOO_INT_SIZE:
                        safe_strcat(buffer, buffer_size, "isize");
                        break;
                }
            } else {
                switch (type->int_info.width) {
                    case GOO_INT_8:
                        safe_strcat(buffer, buffer_size, "u8");
                        break;
                    case GOO_INT_16:
                        safe_strcat(buffer, buffer_size, "u16");
                        break;
                    case GOO_INT_32:
                        safe_strcat(buffer, buffer_size, "u32");
                        break;
                    case GOO_INT_64:
                        safe_strcat(buffer, buffer_size, "u64");
                        break;
                    case GOO_INT_128:
                        safe_strcat(buffer, buffer_size, "u128");
                        break;
                    case GOO_INT_SIZE:
                        safe_strcat(buffer, buffer_size, "usize");
                        break;
                }
            }
            break;
            
        case GOO_TYPE_FLOAT:
            switch (type->float_info.precision) {
                case GOO_FLOAT_32:
                    safe_strcat(buffer, buffer_size, "f32");
                    break;
                case GOO_FLOAT_64:
                    safe_strcat(buffer, buffer_size, "f64");
                    break;
            }
            break;
            
        case GOO_TYPE_CHAR:
            safe_strcat(buffer, buffer_size, "char");
            break;
            
        case GOO_TYPE_STRING:
            safe_strcat(buffer, buffer_size, "string");
            break;
            
        case GOO_TYPE_ARRAY:
            {
                char element_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->array_info.element_type, element_str, sizeof(element_str));
                
                char array_str[300];
                if (type->array_info.size > 0) {
                    snprintf(array_str, sizeof(array_str), "[%s; %zu]", element_str, type->array_info.size);
                } else {
                    snprintf(array_str, sizeof(array_str), "[%s]", element_str);
                }
                
                safe_strcat(buffer, buffer_size, array_str);
            }
            break;
            
        case GOO_TYPE_SLICE:
            {
                char element_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->slice_info.element_type, element_str, sizeof(element_str));
                
                char slice_str[300];
                snprintf(slice_str, sizeof(slice_str), "&[%s]", element_str);
                
                safe_strcat(buffer, buffer_size, slice_str);
            }
            break;
            
        case GOO_TYPE_TUPLE:
            {
                safe_strcat(buffer, buffer_size, "(");
                
                for (int i = 0; i < type->tuple_info.element_count; i++) {
                    if (i > 0) {
                        safe_strcat(buffer, buffer_size, ", ");
                    }
                    
                    char element_str[256] = {0};
                    goo_type_system_type_to_string(ctx, type->tuple_info.element_types[i], element_str, sizeof(element_str));
                    safe_strcat(buffer, buffer_size, element_str);
                }
                
                safe_strcat(buffer, buffer_size, ")");
            }
            break;
            
        case GOO_TYPE_STRUCT:
            safe_strcat(buffer, buffer_size, type->struct_info.name);
            break;
            
        case GOO_TYPE_ENUM:
            safe_strcat(buffer, buffer_size, type->enum_info.name);
            break;
            
        case GOO_TYPE_FUNCTION:
            {
                safe_strcat(buffer, buffer_size, "fn(");
                
                for (int i = 0; i < type->function_info.param_count; i++) {
                    if (i > 0) {
                        safe_strcat(buffer, buffer_size, ", ");
                    }
                    
                    char param_str[256] = {0};
                    goo_type_system_type_to_string(ctx, type->function_info.param_types[i], param_str, sizeof(param_str));
                    safe_strcat(buffer, buffer_size, param_str);
                }
                
                safe_strcat(buffer, buffer_size, ") -> ");
                
                char return_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->function_info.return_type, return_str, sizeof(return_str));
                safe_strcat(buffer, buffer_size, return_str);
            }
            break;
            
        case GOO_TYPE_REF:
            {
                char ref_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->ref_info.referenced_type, ref_str, sizeof(ref_str));
                
                char full_str[300];
                snprintf(full_str, sizeof(full_str), "&%s", ref_str);
                
                safe_strcat(buffer, buffer_size, full_str);
            }
            break;
            
        case GOO_TYPE_MUT_REF:
            {
                char ref_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->ref_info.referenced_type, ref_str, sizeof(ref_str));
                
                char full_str[300];
                snprintf(full_str, sizeof(full_str), "&mut %s", ref_str);
                
                safe_strcat(buffer, buffer_size, full_str);
            }
            break;
            
        case GOO_TYPE_CHANNEL:
            {
                char element_str[256] = {0};
                goo_type_system_type_to_string(ctx, type->channel_info.element_type, element_str, sizeof(element_str));
                
                char chan_str[300];
                snprintf(chan_str, sizeof(chan_str), "chan<%s>", element_str);
                
                safe_strcat(buffer, buffer_size, chan_str);
            }
            break;
            
        case GOO_TYPE_TRAIT_OBJECT:
            {
                safe_strcat(buffer, buffer_size, "dyn ");
                safe_strcat(buffer, buffer_size, type->trait_object_info.trait->name);
            }
            break;
            
        case GOO_TYPE_ERROR:
            safe_strcat(buffer, buffer_size, "Error");
            break;
            
        case GOO_TYPE_NEVER:
            safe_strcat(buffer, buffer_size, "!");
            break;
            
        case GOO_TYPE_UNKNOWN:
            safe_strcat(buffer, buffer_size, "?");
            break;
            
        case GOO_TYPE_ANY:
            safe_strcat(buffer, buffer_size, "any");
            break;
            
        case GOO_TYPE_VAR:
            {
                // Handle type variables
                if (type->type_var->resolved_type) {
                    // Use the resolved type
                    return goo_type_system_type_to_string(ctx, type->type_var->resolved_type, buffer, buffer_size);
                } else {
                    // Use a placeholder with the variable ID
                    char var_str[64];
                    snprintf(var_str, sizeof(var_str), "T%u", type->type_var->id);
                    safe_strcat(buffer, buffer_size, var_str);
                }
            }
            break;
            
        default:
            safe_strcat(buffer, buffer_size, "<unknown type>");
            break;
    }
    
    return buffer;
}

// Check if a type is a numeric type
bool goo_type_system_is_numeric(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_numeric(ctx, resolved);
        }
        return false;
    }
    
    return (type->kind == GOO_TYPE_INT || type->kind == GOO_TYPE_FLOAT);
}

// Check if a type is an integer type
bool goo_type_system_is_integer(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_integer(ctx, resolved);
        }
        return false;
    }
    
    return (type->kind == GOO_TYPE_INT);
}

// Check if a type is a floating point type
bool goo_type_system_is_float(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_float(ctx, resolved);
        }
        return false;
    }
    
    return (type->kind == GOO_TYPE_FLOAT);
}

// Check if a type is a signed integer type
bool goo_type_system_is_signed_integer(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_signed_integer(ctx, resolved);
        }
        return false;
    }
    
    return (type->kind == GOO_TYPE_INT && type->int_info.is_signed);
}

// Check if a type is a valid array element type
bool goo_type_system_is_valid_array_element(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_valid_array_element(ctx, resolved);
        }
        return true;  // Assume unresolved type variables could be valid
    }
    
    // Most types can be array elements, except a few special ones
    return type->kind != GOO_TYPE_VOID && 
           type->kind != GOO_TYPE_NEVER &&
           type->kind != GOO_TYPE_UNKNOWN &&
           type->kind != GOO_TYPE_ANY;
}

// Check if a type is sized (has a known size at compile time)
bool goo_type_system_is_sized(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_sized(ctx, resolved);
        }
        return false;  // Assume unresolved type variables are not sized
    }
    
    return type->is_sized;
}

// Check if a type is copyable
bool goo_type_system_is_copyable(GooTypeContext* ctx, GooType* type) {
    if (!type) return false;
    
    // Handle type variables
    if (type->kind == GOO_TYPE_VAR) {
        GooType* resolved = goo_type_system_resolve_type_var(ctx, type->type_var);
        if (resolved) {
            return goo_type_system_is_copyable(ctx, resolved);
        }
        return false;  // Assume unresolved type variables are not copyable
    }
    
    return type->is_copyable;
} 