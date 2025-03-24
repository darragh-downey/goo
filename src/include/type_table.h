/**
 * type_table.h
 * 
 * Type management interface for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_TABLE_H
#define GOO_TYPE_TABLE_H

#include <stdbool.h>

// Type kinds
typedef enum {
    GOO_TYPE_UNKNOWN,
    GOO_TYPE_VOID,
    GOO_TYPE_UNIT,        // Unit type (e.g., result of expressions with no value)
    GOO_TYPE_BOOL,
    GOO_TYPE_INT,
    GOO_TYPE_FLOAT,
    GOO_TYPE_STRING,
    GOO_TYPE_ARRAY,
    GOO_TYPE_FUNCTION,
    GOO_TYPE_STRUCT,
    GOO_TYPE_INTERFACE,
    GOO_TYPE_ENUM,
    GOO_TYPE_ALIAS
} GooTypeKind;

// Forward declarations
typedef struct GooType GooType;
typedef struct GooTypeTable GooTypeTable;

// Type table operations
/**
 * Create a new type table
 * 
 * @return A new type table or NULL on failure
 */
GooTypeTable* goo_type_table_create(void);

/**
 * Destroy a type table and all its types
 * 
 * @param table The type table to destroy
 */
void goo_type_table_destroy(GooTypeTable* table);

/**
 * Create a primitive type
 * 
 * @param table The type table
 * @param kind The type kind
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_type(GooTypeTable* table, GooTypeKind kind);

/**
 * Create an array type
 * 
 * @param table The type table
 * @param element_type The element type
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_array_type(GooTypeTable* table, GooType* element_type);

/**
 * Create a function type
 * 
 * @param table The type table
 * @param return_type The return type
 * @param param_types The parameter types
 * @param param_count The number of parameters
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_function_type(GooTypeTable* table, GooType* return_type,
                                           GooType** param_types, int param_count);

/**
 * Create a struct type
 * 
 * @param table The type table
 * @param field_names Array of field names
 * @param field_types Array of field types
 * @param field_count Number of fields
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_struct_type(GooTypeTable* table,
                                         const char** field_names,
                                         GooType** field_types,
                                         int field_count);

/**
 * Create an interface type
 * 
 * @param table The type table
 * @param method_names Array of method names
 * @param method_types Array of method types (function types)
 * @param method_count Number of methods
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_interface_type(GooTypeTable* table,
                                            const char** method_names,
                                            GooType** method_types,
                                            int method_count);

/**
 * Create an enum type
 * 
 * @param table The type table
 * @param variant_names Array of variant names
 * @param variant_types Array of variant types (NULL for simple enums)
 * @param variant_count Number of variants
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_enum_type(GooTypeTable* table,
                                       const char** variant_names,
                                       GooType** variant_types,
                                       int variant_count);

/**
 * Create a type alias
 * 
 * @param table The type table
 * @param target_type The target type
 * @return The created type or NULL on failure
 */
GooType* goo_type_table_create_alias_type(GooTypeTable* table, GooType* target_type);

/**
 * Check if two types are equal
 * 
 * @param a First type
 * @param b Second type
 * @return True if the types are equal, false otherwise
 */
bool goo_type_equal(GooType* a, GooType* b);

/**
 * Check if a type implements an interface
 * 
 * @param struct_type The struct type
 * @param interface_type The interface type
 * @return True if the struct implements the interface, false otherwise
 */
bool goo_type_implements_interface(GooType* struct_type, GooType* interface_type);

/**
 * Check if a type is numeric
 * 
 * @param type The type to check
 * @return True if the type is numeric, false otherwise
 */
bool goo_type_is_numeric(GooType* type);

/**
 * Check if a type is a valid array element type
 * 
 * @param type The type to check
 * @return True if the type can be used as an array element, false otherwise
 */
bool goo_type_is_valid_array_element(GooType* type);

/**
 * Get the string representation of a type
 * 
 * @param type The type
 * @param buffer Buffer to store the string
 * @param size Buffer size
 * @return The number of characters written or that would have been written if the buffer is too small
 */
int goo_type_to_string(GooType* type, char* buffer, int size);

/**
 * Get the kind of a type
 * 
 * @param type The type
 * @return The type kind
 */
GooTypeKind goo_type_get_kind(GooType* type);

/**
 * Get the element type of an array type
 * 
 * @param type The array type
 * @return The element type or NULL if not an array type
 */
GooType* goo_type_get_array_element(GooType* type);

/**
 * Get the return type of a function type
 * 
 * @param type The function type
 * @return The return type or NULL if not a function type
 */
GooType* goo_type_get_function_return(GooType* type);

/**
 * Get the parameter types of a function type
 * 
 * @param type The function type
 * @param count Pointer to store the parameter count
 * @return Array of parameter types or NULL if not a function type
 */
GooType** goo_type_get_function_params(GooType* type, int* count);

/**
 * Get the field type of a struct type by name
 * 
 * @param type The struct type
 * @param field_name The field name
 * @return The field type or NULL if not found
 */
GooType* goo_type_get_struct_field(GooType* type, const char* field_name);

/**
 * Get all field names and types of a struct type
 * 
 * @param type The struct type
 * @param field_names Pointer to store the field names
 * @param field_types Pointer to store the field types
 * @param count Pointer to store the field count
 * @return True if successful, false otherwise
 */
bool goo_type_get_struct_fields(GooType* type, const char*** field_names,
                              GooType*** field_types, int* count);

/**
 * Get the method type of an interface type by name
 * 
 * @param type The interface type
 * @param method_name The method name
 * @return The method type or NULL if not found
 */
GooType* goo_type_get_interface_method(GooType* type, const char* method_name);

/**
 * Get the target type of an alias type
 * 
 * @param type The alias type
 * @return The target type or NULL if not an alias type
 */
GooType* goo_type_get_alias_target(GooType* type);

#endif /* GOO_TYPE_TABLE_H */ 