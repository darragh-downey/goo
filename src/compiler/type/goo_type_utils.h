/**
 * goo_type_utils.h
 * 
 * Utility functions for the Goo type system
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_UTILS_H
#define GOO_TYPE_UTILS_H

#include <stddef.h>
#include "goo_type_system.h"

/**
 * Convert a type to its string representation.
 * 
 * @param ctx The type context
 * @param type The type to convert
 * @param buffer The buffer to store the result
 * @param buffer_size The size of the buffer
 * @return The buffer containing the string representation of the type
 */
const char* goo_type_system_type_to_string(GooTypeContext* ctx, GooType* type, char* buffer, size_t buffer_size);

/**
 * Check if a type is a numeric type (integer or float).
 * 
 * @param ctx The type context
 * @param type The type to check
 * @return True if the type is numeric, false otherwise
 */
bool goo_type_system_is_numeric(GooTypeContext* ctx, GooType* type);

/**
 * Check if a type is an integer type.
 * 
 * @param ctx The type context
 * @param type The type to check
 * @return True if the type is an integer, false otherwise
 */
bool goo_type_system_is_integer(GooTypeContext* ctx, GooType* type);

/**
 * Check if a type is a floating point type.
 * 
 * @param ctx The type context
 * @param type The type to check
 * @return True if the type is a float, false otherwise
 */
bool goo_type_system_is_float(GooTypeContext* ctx, GooType* type);

/**
 * Check if a type is a signed integer type.
 * 
 * @param ctx The type context
 * @param type The type to check
 * @return True if the type is a signed integer, false otherwise
 */
bool goo_type_system_is_signed_integer(GooTypeContext* ctx, GooType* type);

#endif /* GOO_TYPE_UTILS_H */ 