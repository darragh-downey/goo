/**
 * @file reflection.h
 * @brief Reflection and meta-programming for the Goo programming language.
 *
 * This header provides functions for runtime type introspection and manipulation,
 * allowing programs to inspect and modify their structure at runtime.
 */

#ifndef GOO_REFLECTION_H
#define GOO_REFLECTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a reflection context.
 */
typedef struct GooReflectionContext GooReflectionContext;

/**
 * @brief Opaque type representing type information.
 */
typedef struct GooTypeInfo GooTypeInfo;

/**
 * @brief Opaque type representing a field in a struct.
 */
typedef struct GooFieldInfo GooFieldInfo;

/**
 * @brief Opaque type representing a method.
 */
typedef struct GooMethodInfo GooMethodInfo;

/**
 * @brief Opaque type representing a value.
 */
typedef struct GooValue GooValue;

/**
 * @brief Enumeration of base types.
 */
typedef enum GooBaseType {
    GOO_TYPE_VOID,
    GOO_TYPE_BOOL,
    GOO_TYPE_I8,
    GOO_TYPE_I16,
    GOO_TYPE_I32,
    GOO_TYPE_I64,
    GOO_TYPE_U8,
    GOO_TYPE_U16,
    GOO_TYPE_U32,
    GOO_TYPE_U64,
    GOO_TYPE_F32,
    GOO_TYPE_F64,
    GOO_TYPE_STRING,
    GOO_TYPE_STRUCT,
    GOO_TYPE_ENUM,
    GOO_TYPE_ARRAY,
    GOO_TYPE_SLICE,
    GOO_TYPE_POINTER,
    GOO_TYPE_FUNCTION,
    GOO_TYPE_TYPE,
    GOO_TYPE_CUSTOM
} GooBaseType;

/**
 * @brief Initialize the reflection subsystem.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_reflection_init(void);

/**
 * @brief Clean up the reflection subsystem.
 */
void goo_reflection_cleanup(void);

/**
 * @brief Create a new reflection context.
 *
 * @return A pointer to the new context, or NULL if creation fails.
 */
GooReflectionContext* goo_reflection_context_create(void);

/**
 * @brief Destroy a reflection context.
 *
 * @param context The context to destroy.
 */
void goo_reflection_context_destroy(GooReflectionContext* context);

/**
 * @brief Register a basic type with the reflection context.
 *
 * @param context The context to register with.
 * @param type The type to register.
 * @param name The name of the type.
 * @param size The size of the type in bytes.
 * @return A pointer to the type information, or NULL if registration fails.
 */
GooTypeInfo* goo_reflection_register_basic_type(GooReflectionContext* context, GooBaseType type, const char* name, size_t size);

/**
 * @brief Register a struct type with the reflection context.
 *
 * @param context The context to register with.
 * @param name The name of the struct.
 * @param size The size of the struct in bytes.
 * @return A pointer to the type information, or NULL if registration fails.
 */
GooTypeInfo* goo_reflection_register_struct(GooReflectionContext* context, const char* name, size_t size);

/**
 * @brief Add a field to a struct type.
 *
 * @param context The context to use.
 * @param struct_type The struct type to add the field to.
 * @param name The name of the field.
 * @param type The type of the field.
 * @param offset The offset of the field in bytes.
 * @return A pointer to the field information, or NULL if addition fails.
 */
GooFieldInfo* goo_reflection_add_field(GooReflectionContext* context, GooTypeInfo* struct_type, const char* name, GooTypeInfo* type, size_t offset);

/**
 * @brief Register an enum type with the reflection context.
 *
 * @param context The context to register with.
 * @param name The name of the enum.
 * @param underlying_type The underlying type of the enum.
 * @return A pointer to the type information, or NULL if registration fails.
 */
GooTypeInfo* goo_reflection_register_enum(GooReflectionContext* context, const char* name, GooTypeInfo* underlying_type);

/**
 * @brief Add a constant to an enum type.
 *
 * @param context The context to use.
 * @param enum_type The enum type to add the constant to.
 * @param name The name of the constant.
 * @param value The value of the constant.
 * @return true if addition succeeds, false otherwise.
 */
bool goo_reflection_add_enum_constant(GooReflectionContext* context, GooTypeInfo* enum_type, const char* name, int64_t value);

/**
 * @brief Register an array type with the reflection context.
 *
 * @param context The context to register with.
 * @param element_type The type of the array elements.
 * @param length The length of the array.
 * @return A pointer to the type information, or NULL if registration fails.
 */
GooTypeInfo* goo_reflection_register_array(GooReflectionContext* context, GooTypeInfo* element_type, size_t length);

/**
 * @brief Register a pointer type with the reflection context.
 *
 * @param context The context to register with.
 * @param pointee_type The type the pointer points to.
 * @return A pointer to the type information, or NULL if registration fails.
 */
GooTypeInfo* goo_reflection_register_pointer(GooReflectionContext* context, GooTypeInfo* pointee_type);

/**
 * @brief Get type information by name.
 *
 * @param context The context to use.
 * @param name The name of the type.
 * @return A pointer to the type information, or NULL if not found.
 */
GooTypeInfo* goo_reflection_get_type_info(GooReflectionContext* context, const char* name);

/**
 * @brief Get the name of a type.
 *
 * @param context The context to use.
 * @param type The type to get the name of.
 * @return The name of the type, or NULL if the type is invalid.
 */
const char* goo_reflection_get_type_name(GooReflectionContext* context, GooTypeInfo* type);

/**
 * @brief Get the size of a type.
 *
 * @param context The context to use.
 * @param type The type to get the size of.
 * @return The size of the type in bytes, or 0 if the type is invalid.
 */
size_t goo_reflection_get_type_size(GooReflectionContext* context, GooTypeInfo* type);

/**
 * @brief Get the base type of a type.
 *
 * @param context The context to use.
 * @param type The type to get the base type of.
 * @return The base type, or GOO_TYPE_VOID if the type is invalid.
 */
GooBaseType goo_reflection_get_base_type(GooReflectionContext* context, GooTypeInfo* type);

/**
 * @brief Get the number of fields in a struct type.
 *
 * @param context The context to use.
 * @param struct_type The struct type to get the number of fields of.
 * @return The number of fields, or 0 if the type is invalid or not a struct.
 */
size_t goo_reflection_get_field_count(GooReflectionContext* context, GooTypeInfo* struct_type);

/**
 * @brief Get field information by index.
 *
 * @param context The context to use.
 * @param struct_type The struct type to get the field from.
 * @param index The index of the field.
 * @return A pointer to the field information, or NULL if not found.
 */
GooFieldInfo* goo_reflection_get_field(GooReflectionContext* context, GooTypeInfo* struct_type, size_t index);

/**
 * @brief Get field information by name.
 *
 * @param context The context to use.
 * @param struct_type The struct type to get the field from.
 * @param name The name of the field.
 * @return A pointer to the field information, or NULL if not found.
 */
GooFieldInfo* goo_reflection_get_field_by_name(GooReflectionContext* context, GooTypeInfo* struct_type, const char* name);

/**
 * @brief Get the name of a field.
 *
 * @param context The context to use.
 * @param field The field to get the name of.
 * @return The name of the field, or NULL if the field is invalid.
 */
const char* goo_reflection_get_field_name(GooReflectionContext* context, GooFieldInfo* field);

/**
 * @brief Get the type of a field.
 *
 * @param context The context to use.
 * @param field The field to get the type of.
 * @return A pointer to the type information, or NULL if the field is invalid.
 */
GooTypeInfo* goo_reflection_get_field_type(GooReflectionContext* context, GooFieldInfo* field);

/**
 * @brief Get the offset of a field.
 *
 * @param context The context to use.
 * @param field The field to get the offset of.
 * @return The offset of the field in bytes, or 0 if the field is invalid.
 */
size_t goo_reflection_get_field_offset(GooReflectionContext* context, GooFieldInfo* field);

/**
 * @brief Create a new value.
 *
 * @param context The context to use.
 * @param type The type of the value.
 * @return A pointer to the new value, or NULL if creation fails.
 */
GooValue* goo_reflection_create_value(GooReflectionContext* context, GooTypeInfo* type);

/**
 * @brief Destroy a value.
 *
 * @param context The context to use.
 * @param value The value to destroy.
 */
void goo_reflection_destroy_value(GooReflectionContext* context, GooValue* value);

/**
 * @brief Set the value of a boolean.
 *
 * @param context The context to use.
 * @param value The value to set.
 * @param b The boolean value.
 * @return true if setting succeeds, false otherwise.
 */
bool goo_reflection_set_bool(GooReflectionContext* context, GooValue* value, bool b);

/**
 * @brief Set the value of an integer.
 *
 * @param context The context to use.
 * @param value The value to set.
 * @param i The integer value.
 * @return true if setting succeeds, false otherwise.
 */
bool goo_reflection_set_int(GooReflectionContext* context, GooValue* value, int64_t i);

/**
 * @brief Set the value of a float.
 *
 * @param context The context to use.
 * @param value The value to set.
 * @param f The float value.
 * @return true if setting succeeds, false otherwise.
 */
bool goo_reflection_set_float(GooReflectionContext* context, GooValue* value, double f);

/**
 * @brief Set the value of a string.
 *
 * @param context The context to use.
 * @param value The value to set.
 * @param s The string value.
 * @return true if setting succeeds, false otherwise.
 */
bool goo_reflection_set_string(GooReflectionContext* context, GooValue* value, const char* s);

/**
 * @brief Get the value of a boolean.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_b Pointer to store the boolean value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_reflection_get_bool(GooReflectionContext* context, GooValue* value, bool* out_b);

/**
 * @brief Get the value of an integer.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_i Pointer to store the integer value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_reflection_get_int(GooReflectionContext* context, GooValue* value, int64_t* out_i);

/**
 * @brief Get the value of a float.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_f Pointer to store the float value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_reflection_get_float(GooReflectionContext* context, GooValue* value, double* out_f);

/**
 * @brief Get the value of a string.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_s Pointer to store the string value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_reflection_get_string(GooReflectionContext* context, GooValue* value, const char** out_s);

/**
 * @brief Set a field value in a struct.
 *
 * @param context The context to use.
 * @param struct_value The struct value to set the field in.
 * @param field_name The name of the field.
 * @param field_value The value to set.
 * @return true if setting succeeds, false otherwise.
 */
bool goo_reflection_set_field(GooReflectionContext* context, GooValue* struct_value, const char* field_name, GooValue* field_value);

/**
 * @brief Get a field value from a struct.
 *
 * @param context The context to use.
 * @param struct_value The struct value to get the field from.
 * @param field_name The name of the field.
 * @param out_field_value Pointer to store the field value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_reflection_get_field_value(GooReflectionContext* context, GooValue* struct_value, const char* field_name, GooValue** out_field_value);

/**
 * @brief Check if a type has a method.
 *
 * @param context The context to use.
 * @param type The type to check.
 * @param method_name The name of the method.
 * @return true if the type has the method, false otherwise.
 */
bool goo_reflection_has_method(GooReflectionContext* context, GooTypeInfo* type, const char* method_name);

/**
 * @brief Call a method on a value.
 *
 * @param context The context to use.
 * @param value The value to call the method on.
 * @param method_name The name of the method.
 * @param args Array of argument values.
 * @param arg_count Number of arguments.
 * @param out_return_value Pointer to store the return value, or NULL if not needed.
 * @return true if calling succeeds, false otherwise.
 */
bool goo_reflection_call_method(GooReflectionContext* context, GooValue* value, const char* method_name, GooValue** args, size_t arg_count, GooValue** out_return_value);

#ifdef __cplusplus
}
#endif

#endif /* GOO_REFLECTION_H */ 