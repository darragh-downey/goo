#ifndef GOO_TYPE_TABLE_SIMPLE_H
#define GOO_TYPE_TABLE_SIMPLE_H

#include <stdbool.h>

// Type kinds in the Goo language
typedef enum {
    GOO_TYPE_VOID,
    GOO_TYPE_BOOL,
    GOO_TYPE_INT,
    GOO_TYPE_FLOAT,
    GOO_TYPE_STRING,
    GOO_TYPE_RANGE,
    GOO_TYPE_ARRAY,
    GOO_TYPE_CHANNEL,
    GOO_TYPE_FUNCTION,
    GOO_TYPE_STRUCT,
    GOO_TYPE_INTERFACE,
    GOO_TYPE_ALLOCATOR,
    GOO_TYPE_USER_DEFINED
} GooTypeKind;

// Forward declaration for AST node
typedef struct GooNode GooNode;

// Type entry in the type table
typedef struct GooType {
    char* name;                // Type name
    GooTypeKind kind;          // Type kind
    void* llvm_type;           // LLVM type reference (void* to avoid dependency)
    struct GooType* element_type; // For arrays and channels, the element type
    struct GooField* fields;   // For structs, the fields
    struct GooType* next;      // Next type in the table
} GooType;

// Field in a struct type
typedef struct GooField {
    char* name;                // Field name
    struct GooType* type;      // Field type
    int offset;                // Field offset in the struct
    struct GooField* next;     // Next field in the struct
} GooField;

// Type table structure
typedef struct GooTypeTable {
    GooType* types;            // Linked list of types
    GooType* void_type;        // Pre-defined void type
    GooType* bool_type;        // Pre-defined bool type
    GooType* int_type;         // Pre-defined int type 
    GooType* float_type;       // Pre-defined float type
    GooType* string_type;      // Pre-defined string type
} GooTypeTable;

// Initialize a new type table
GooTypeTable* goo_type_table_init(void* llvm_context);

// Free a type table
void goo_type_table_free(GooTypeTable* table);

// Add a type to the type table
GooType* goo_type_table_add(GooTypeTable* table, const char* name, GooTypeKind kind, 
                         void* llvm_type, GooType* element_type);

// Add a field to a struct type
GooField* goo_type_add_field(GooType* struct_type, const char* name, GooType* type, int offset);

// Look up a type by name
GooType* goo_type_table_lookup(GooTypeTable* table, const char* name);

// Look up a type by AST type node
GooType* goo_type_table_lookup_node(GooTypeTable* table, GooNode* type_node);

// Convert an AST type node to an LLVM type
void* goo_convert_type_node(GooTypeTable* table, GooNode* type_node, void* context);

// Create a channel type with the specified element type
GooType* goo_type_table_create_channel(GooTypeTable* table, GooType* element_type, void* context);

// Create an array type with the specified element type
GooType* goo_type_table_create_array(GooTypeTable* table, GooType* element_type, int size, void* context);

// Create a function type
GooType* goo_type_table_create_function(GooTypeTable* table, GooType* return_type, 
                                     GooType** param_types, int param_count, 
                                     void* context);

// Create a struct type
GooType* goo_type_table_create_struct(GooTypeTable* table, const char* name, 
                                   void* context);

// Set the fields of a struct type
bool goo_type_set_struct_body(GooType* struct_type, GooField* fields, void* context);

#endif // GOO_TYPE_TABLE_SIMPLE_H 