#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "type_table_simple.h"

// Initialize a new type table
GooTypeTable* goo_type_table_init(void* llvm_context) {
    GooTypeTable* table = (GooTypeTable*)malloc(sizeof(GooTypeTable));
    if (!table) {
        fprintf(stderr, "Failed to allocate type table\n");
        return NULL;
    }
    
    // Initialize with empty data
    memset(table, 0, sizeof(GooTypeTable));
    
    // Create basic types (stub implementation)
    table->void_type = goo_type_table_add(table, "void", GOO_TYPE_VOID, NULL, NULL);
    table->bool_type = goo_type_table_add(table, "bool", GOO_TYPE_BOOL, NULL, NULL);
    table->int_type = goo_type_table_add(table, "int", GOO_TYPE_INT, NULL, NULL);
    table->float_type = goo_type_table_add(table, "float", GOO_TYPE_FLOAT, NULL, NULL);
    table->string_type = goo_type_table_add(table, "string", GOO_TYPE_STRING, NULL, NULL);
    
    return table;
}

// Free a type table
void goo_type_table_free(GooTypeTable* table) {
    if (!table) return;
    
    // Free all types
    GooType* type = table->types;
    while (type) {
        GooType* next = type->next;
        
        // Free fields for struct types
        if (type->kind == GOO_TYPE_STRUCT) {
            GooField* field = type->fields;
            while (field) {
                GooField* next_field = field->next;
                free(field->name);
                free(field);
                field = next_field;
            }
        }
        
        free(type->name);
        free(type);
        type = next;
    }
    
    free(table);
}

// Add a type to the type table
GooType* goo_type_table_add(GooTypeTable* table, const char* name, GooTypeKind kind, 
                         void* llvm_type, GooType* element_type) {
    if (!table) return NULL;
    
    // Allocate new type
    GooType* type = (GooType*)malloc(sizeof(GooType));
    if (!type) {
        fprintf(stderr, "Failed to allocate type\n");
        return NULL;
    }
    
    // Initialize type
    memset(type, 0, sizeof(GooType));
    type->name = name ? strdup(name) : NULL;
    type->kind = kind;
    type->llvm_type = llvm_type;
    type->element_type = element_type;
    
    // Add to linked list
    type->next = table->types;
    table->types = type;
    
    return type;
}

// Add a field to a struct type
GooField* goo_type_add_field(GooType* struct_type, const char* name, GooType* type, int offset) {
    if (!struct_type || struct_type->kind != GOO_TYPE_STRUCT) return NULL;
    
    // Allocate new field
    GooField* field = (GooField*)malloc(sizeof(GooField));
    if (!field) {
        fprintf(stderr, "Failed to allocate field\n");
        return NULL;
    }
    
    // Initialize field
    field->name = strdup(name);
    field->type = type;
    field->offset = offset;
    
    // Add to linked list
    field->next = struct_type->fields;
    struct_type->fields = field;
    
    return field;
}

// Look up a type by name
GooType* goo_type_table_lookup(GooTypeTable* table, const char* name) {
    if (!table || !name) return NULL;
    
    GooType* type = table->types;
    while (type) {
        if (type->name && strcmp(type->name, name) == 0) {
            return type;
        }
        type = type->next;
    }
    
    return NULL;
}

// Look up a type by AST type node - stub for now
GooType* goo_type_table_lookup_node(GooTypeTable* table, GooNode* type_node) {
    // This is just a stub implementation - in a real implementation, 
    // we would inspect the AST node and determine its type
    return table->void_type; // Return void type as default
}

// Convert an AST type node to an LLVM type - stub for now
void* goo_convert_type_node(GooTypeTable* table, GooNode* type_node, void* context) {
    // Stub implementation
    return NULL;
}

// Create a channel type with the specified element type
GooType* goo_type_table_create_channel(GooTypeTable* table, GooType* element_type, void* context) {
    if (!table || !element_type) return NULL;
    
    char type_name[256];
    snprintf(type_name, sizeof(type_name), "chan[%s]", element_type->name ? element_type->name : "unknown");
    
    return goo_type_table_add(table, type_name, GOO_TYPE_CHANNEL, NULL, element_type);
}

// Create an array type with the specified element type
GooType* goo_type_table_create_array(GooTypeTable* table, GooType* element_type, int size, void* context) {
    if (!table || !element_type) return NULL;
    
    char type_name[256];
    snprintf(type_name, sizeof(type_name), "[%d]%s", size, element_type->name ? element_type->name : "unknown");
    
    return goo_type_table_add(table, type_name, GOO_TYPE_ARRAY, NULL, element_type);
}

// Create a function type - stub for now
GooType* goo_type_table_create_function(GooTypeTable* table, GooType* return_type, 
                                     GooType** param_types, int param_count, 
                                     void* context) {
    if (!table || !return_type) return NULL;
    
    // Create a simple function type name
    char type_name[256];
    snprintf(type_name, sizeof(type_name), "func(...)");
    
    return goo_type_table_add(table, type_name, GOO_TYPE_FUNCTION, NULL, NULL);
}

// Create a struct type
GooType* goo_type_table_create_struct(GooTypeTable* table, const char* name, 
                                   void* context) {
    if (!table || !name) return NULL;
    
    return goo_type_table_add(table, name, GOO_TYPE_STRUCT, NULL, NULL);
}

// Set the fields of a struct type - stub for now
bool goo_type_set_struct_body(GooType* struct_type, GooField* fields, void* context) {
    if (!struct_type || struct_type->kind != GOO_TYPE_STRUCT) return false;
    
    // In a real implementation, we would use the LLVM API to set the struct body
    // For this stub, we just set the fields pointer
    struct_type->fields = fields;
    
    return true;
} 