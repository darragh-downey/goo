/**
 * reflection_wrapper.c
 * 
 * C wrapper for Goo's reflection and meta-programming functionality.
 * This file provides a C API for the Zig implementation of the reflection system.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Forward declarations for Zig functions
extern bool reflectionInit(void);
extern void reflectionCleanup(void);
extern void* reflectionContextCreate(void);
extern void reflectionContextDestroy(void* context);
extern bool reflectionRegisterBasicType(void* context, uint32_t kind, const char* name, size_t size, size_t alignment, size_t type_id);
extern bool reflectionRegisterIntType(void* context, const char* name, size_t size, size_t alignment, uint16_t bits, bool is_signed, size_t type_id);
extern bool reflectionRegisterFloatType(void* context, const char* name, size_t size, size_t alignment, uint16_t bits, size_t type_id);
extern void* reflectionCreateValue(void* context, const char* type_name);
extern void reflectionDestroyValue(void* value);

// Opaque types for the C interface
typedef struct GooReflectionContext GooReflectionContext;
typedef struct GooTypeInfo GooTypeInfo;
typedef struct GooValue GooValue;

// Type kinds
typedef enum {
    GOO_TYPE_VOID,
    GOO_TYPE_BOOL,
    GOO_TYPE_INT,
    GOO_TYPE_FLOAT,
    GOO_TYPE_POINTER,
    GOO_TYPE_ARRAY,
    GOO_TYPE_SLICE,
    GOO_TYPE_STRUCT,
    GOO_TYPE_ENUM,
    GOO_TYPE_UNION,
    GOO_TYPE_FUNCTION,
    GOO_TYPE_OPTIONAL,
    GOO_TYPE_ERROR_UNION,
    GOO_TYPE_ERROR_SET,
    GOO_TYPE_VECTOR,
    GOO_TYPE_OPAQUE,
    // Goo-specific types
    GOO_TYPE_ANY,
    GOO_TYPE_DYNAMIC,
    GOO_TYPE_INTERFACE,
    GOO_TYPE_NULL,
    GOO_TYPE_UNDEFINED
} GooTypeKind;

// Value kinds
typedef enum {
    GOO_VALUE_VOID,
    GOO_VALUE_BOOL,
    GOO_VALUE_INT,
    GOO_VALUE_FLOAT,
    GOO_VALUE_POINTER,
    GOO_VALUE_ARRAY,
    GOO_VALUE_SLICE,
    GOO_VALUE_STRUCT,
    GOO_VALUE_ENUM,
    GOO_VALUE_UNION,
    GOO_VALUE_FUNCTION,
    GOO_VALUE_OPTIONAL,
    GOO_VALUE_ERROR,
    GOO_VALUE_ERROR_UNION,
    GOO_VALUE_VECTOR,
    // Goo-specific values
    GOO_VALUE_ANY,
    GOO_VALUE_DYNAMIC,
    GOO_VALUE_NULL,
    GOO_VALUE_UNDEFINED
} GooValueKind;

/**
 * Initialize the reflection system.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_reflection_init(void) {
    return reflectionInit();
}

/**
 * Clean up the reflection system.
 */
void goo_reflection_cleanup(void) {
    reflectionCleanup();
}

/**
 * Create a new reflection context.
 * 
 * @return A new reflection context, or NULL if creation failed
 */
GooReflectionContext* goo_reflection_context_create(void) {
    return (GooReflectionContext*)reflectionContextCreate();
}

/**
 * Destroy a reflection context.
 * 
 * @param context The context to destroy
 */
void goo_reflection_context_destroy(GooReflectionContext* context) {
    reflectionContextDestroy(context);
}

/**
 * Register a basic type with the reflection context.
 * 
 * @param context The reflection context
 * @param kind The kind of the type
 * @param name The name of the type
 * @param size The size of the type in bytes
 * @param alignment The alignment of the type in bytes
 * @param type_id The unique ID of the type
 * @return true if registration was successful, false otherwise
 */
bool goo_reflection_register_basic_type(
    GooReflectionContext* context,
    GooTypeKind kind,
    const char* name,
    size_t size,
    size_t alignment,
    size_t type_id
) {
    return reflectionRegisterBasicType(context, (uint32_t)kind, name, size, alignment, type_id);
}

/**
 * Register an integer type with the reflection context.
 * 
 * @param context The reflection context
 * @param name The name of the type
 * @param size The size of the type in bytes
 * @param alignment The alignment of the type in bytes
 * @param bits The number of bits in the integer
 * @param is_signed Whether the integer is signed
 * @param type_id The unique ID of the type
 * @return true if registration was successful, false otherwise
 */
bool goo_reflection_register_int_type(
    GooReflectionContext* context,
    const char* name,
    size_t size,
    size_t alignment,
    uint16_t bits,
    bool is_signed,
    size_t type_id
) {
    return reflectionRegisterIntType(context, name, size, alignment, bits, is_signed, type_id);
}

/**
 * Register a floating-point type with the reflection context.
 * 
 * @param context The reflection context
 * @param name The name of the type
 * @param size The size of the type in bytes
 * @param alignment The alignment of the type in bytes
 * @param bits The number of bits in the floating-point number
 * @param type_id The unique ID of the type
 * @return true if registration was successful, false otherwise
 */
bool goo_reflection_register_float_type(
    GooReflectionContext* context,
    const char* name,
    size_t size,
    size_t alignment,
    uint16_t bits,
    size_t type_id
) {
    return reflectionRegisterFloatType(context, name, size, alignment, bits, type_id);
}

/**
 * Create a value from a type.
 * 
 * @param context The reflection context
 * @param type_name The name of the type
 * @return A new value, or NULL if creation failed
 */
GooValue* goo_reflection_create_value(
    GooReflectionContext* context,
    const char* type_name
) {
    return (GooValue*)reflectionCreateValue(context, type_name);
}

/**
 * Destroy a value.
 * 
 * @param value The value to destroy
 */
void goo_reflection_destroy_value(GooValue* value) {
    reflectionDestroyValue(value);
}

/**
 * Convert a boolean value to a string representation.
 * 
 * @param value The boolean value
 * @param buffer The buffer to write the string to
 * @param buffer_size The size of the buffer
 * @return true if successful, false otherwise
 */
bool goo_reflection_bool_to_string(bool value, char* buffer, size_t buffer_size) {
    if (buffer_size < 6) {
        return false;
    }
    
    if (value) {
        strcpy(buffer, "true");
    } else {
        strcpy(buffer, "false");
    }
    
    return true;
}

/**
 * Convert an integer value to a string representation.
 * 
 * @param value The integer value
 * @param buffer The buffer to write the string to
 * @param buffer_size The size of the buffer
 * @return true if successful, false otherwise
 */
bool goo_reflection_int_to_string(int64_t value, char* buffer, size_t buffer_size) {
    if (buffer_size < 22) {  // Max length of int64_t as string plus null terminator
        return false;
    }
    
    snprintf(buffer, buffer_size, "%lld", (long long)value);
    return true;
}

/**
 * Convert a floating-point value to a string representation.
 * 
 * @param value The floating-point value
 * @param buffer The buffer to write the string to
 * @param buffer_size The size of the buffer
 * @return true if successful, false otherwise
 */
bool goo_reflection_float_to_string(double value, char* buffer, size_t buffer_size) {
    if (buffer_size < 32) {  // Should be enough for most double values
        return false;
    }
    
    snprintf(buffer, buffer_size, "%g", value);
    return true;
}

/**
 * Register a struct type with the reflection context.
 * 
 * @param context The reflection context
 * @param name The name of the type
 * @param size The size of the type in bytes
 * @param alignment The alignment of the type in bytes
 * @param type_id The unique ID of the type
 * @return A new type info object, or NULL if creation failed
 */
GooTypeInfo* goo_reflection_register_struct_type(
    GooReflectionContext* context,
    const char* name,
    size_t size,
    size_t alignment,
    size_t type_id
) {
    // This would need additional implementation in the Zig code
    // For now, return NULL
    return NULL;
}

/**
 * Add a field to a struct type.
 * 
 * @param context The reflection context
 * @param struct_type The struct type
 * @param field_name The name of the field
 * @param field_type The type of the field
 * @param offset The offset of the field in bytes
 * @return true if successful, false otherwise
 */
bool goo_reflection_add_struct_field(
    GooReflectionContext* context,
    GooTypeInfo* struct_type,
    const char* field_name,
    GooTypeInfo* field_type,
    size_t offset
) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get the type name from a type info object.
 * 
 * @param type_info The type info object
 * @param buffer The buffer to write the name to
 * @param buffer_size The size of the buffer
 * @return true if successful, false otherwise
 */
bool goo_reflection_get_type_name(
    GooTypeInfo* type_info,
    char* buffer,
    size_t buffer_size
) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get the type kind from a type info object.
 * 
 * @param type_info The type info object
 * @return The kind of the type
 */
GooTypeKind goo_reflection_get_type_kind(GooTypeInfo* type_info) {
    // This would need additional implementation in the Zig code
    // For now, return void
    return GOO_TYPE_VOID;
}

/**
 * Get the value kind from a value object.
 * 
 * @param value The value object
 * @return The kind of the value
 */
GooValueKind goo_reflection_get_value_kind(GooValue* value) {
    // This would need additional implementation in the Zig code
    // For now, return void
    return GOO_VALUE_VOID;
}

/**
 * Set a boolean value.
 * 
 * @param value The value object
 * @param bool_value The boolean value to set
 * @return true if successful, false otherwise
 */
bool goo_reflection_set_bool_value(GooValue* value, bool bool_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Set an integer value.
 * 
 * @param value The value object
 * @param int_value The integer value to set
 * @return true if successful, false otherwise
 */
bool goo_reflection_set_int_value(GooValue* value, int64_t int_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Set a floating-point value.
 * 
 * @param value The value object
 * @param float_value The floating-point value to set
 * @return true if successful, false otherwise
 */
bool goo_reflection_set_float_value(GooValue* value, double float_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get a boolean value.
 * 
 * @param value The value object
 * @param out_value Pointer to store the result
 * @return true if successful, false otherwise
 */
bool goo_reflection_get_bool_value(GooValue* value, bool* out_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get an integer value.
 * 
 * @param value The value object
 * @param out_value Pointer to store the result
 * @return true if successful, false otherwise
 */
bool goo_reflection_get_int_value(GooValue* value, int64_t* out_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get a floating-point value.
 * 
 * @param value The value object
 * @param out_value Pointer to store the result
 * @return true if successful, false otherwise
 */
bool goo_reflection_get_float_value(GooValue* value, double* out_value) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Evaluate an expression at compile time.
 * 
 * This function is used by the compiler to evaluate expressions during compilation.
 * It is not typically called directly by user code.
 * 
 * @param context The reflection context
 * @param expression The expression to evaluate
 * @return The result of the evaluation, or NULL if evaluation failed
 */
GooValue* goo_reflection_eval_expression(
    GooReflectionContext* context,
    const char* expression
) {
    // This would require a full expression parser and evaluator
    // For now, return NULL
    return NULL;
}

/**
 * Get type information at runtime.
 * 
 * This function allows runtime inspection of types.
 * 
 * @param context The reflection context
 * @param type_name The name of the type
 * @return Type information, or NULL if not found
 */
GooTypeInfo* goo_reflection_get_type_info(
    GooReflectionContext* context,
    const char* type_name
) {
    // This would need additional implementation in the Zig code
    // For now, return NULL
    return NULL;
}

/**
 * Check if a type has a specific method.
 * 
 * @param context The reflection context
 * @param type_info The type info object
 * @param method_name The name of the method
 * @return true if the type has the method, false otherwise
 */
bool goo_reflection_has_method(
    GooReflectionContext* context,
    GooTypeInfo* type_info,
    const char* method_name
) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Check if a type implements an interface.
 * 
 * @param context The reflection context
 * @param type_info The type info object
 * @param interface_type_info The interface type info object
 * @return true if the type implements the interface, false otherwise
 */
bool goo_reflection_implements_interface(
    GooReflectionContext* context,
    GooTypeInfo* type_info,
    GooTypeInfo* interface_type_info
) {
    // This would need additional implementation in the Zig code
    // For now, return false
    return false;
}

/**
 * Get the fields of a struct type.
 * 
 * @param context The reflection context
 * @param type_info The type info object
 * @param out_field_count Pointer to store the number of fields
 * @return An array of field names, or NULL if not a struct type
 */
const char** goo_reflection_get_struct_fields(
    GooReflectionContext* context,
    GooTypeInfo* type_info,
    size_t* out_field_count
) {
    // This would need additional implementation in the Zig code
    // For now, return NULL
    *out_field_count = 0;
    return NULL;
}

/**
 * Free an array of strings.
 * 
 * @param strings The array of strings
 * @param count The number of strings
 */
void goo_reflection_free_strings(const char** strings, size_t count) {
    if (strings) {
        for (size_t i = 0; i < count; i++) {
            if (strings[i]) {
                free((void*)strings[i]);
            }
        }
        free(strings);
    }
} 