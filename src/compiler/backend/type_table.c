#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <llvm-c/Core.h>
#include "type_table.h"
#include "ast.h"

// Initialize a new type table with built-in types
GooTypeTable* goo_type_table_init(LLVMContextRef llvm_context) {
    GooTypeTable* table = (GooTypeTable*)malloc(sizeof(GooTypeTable));
    if (!table) return NULL;

    table->types = NULL;

    // Create built-in types
    // void type
    table->void_type = goo_type_table_add(table, "void", GOO_TYPE_VOID, 
        LLVMVoidTypeInContext(llvm_context), NULL);

    // bool type
    table->bool_type = goo_type_table_add(table, "bool", GOO_TYPE_BOOL, 
        LLVMInt1TypeInContext(llvm_context), NULL);

    // int type
    table->int_type = goo_type_table_add(table, "int", GOO_TYPE_INT, 
        LLVMInt32TypeInContext(llvm_context), NULL);

    // float type
    table->float_type = goo_type_table_add(table, "float", GOO_TYPE_FLOAT,
        LLVMDoubleTypeInContext(llvm_context), NULL);

    // string type (represented as char* in LLVM)
    table->string_type = goo_type_table_add(table, "string", GOO_TYPE_STRING,
        LLVMPointerType(LLVMInt8TypeInContext(llvm_context), 0), NULL);

    return table;
}

// Free all fields in a struct type
static void free_fields(GooField* field) {
    GooField* current = field;
    GooField* next;

    while (current) {
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
}

// Free a type table
void goo_type_table_free(GooTypeTable* table) {
    if (!table) return;

    // Free all types
    GooType* current = table->types;
    GooType* next;

    while (current) {
        next = current->next;
        free(current->name);
        
        // Free fields if it's a struct
        if (current->fields) {
            free_fields(current->fields);
        }
        
        free(current);
        current = next;
    }

    // Free the table itself
    free(table);
}

// Add a type to the type table
GooType* goo_type_table_add(GooTypeTable* table, const char* name, GooTypeKind kind, 
                         LLVMTypeRef llvm_type, GooType* element_type) {
    if (!table || !name) return NULL;

    // Check if the type already exists
    GooType* existing = goo_type_table_lookup(table, name);
    if (existing) return existing;

    // Create new type
    GooType* type = (GooType*)malloc(sizeof(GooType));
    if (!type) return NULL;

    type->name = strdup(name);
    if (!type->name) {
        free(type);
        return NULL;
    }

    type->kind = kind;
    type->llvm_type = llvm_type;
    type->element_type = element_type;
    type->fields = NULL;

    // Add to the beginning of the linked list
    type->next = table->types;
    table->types = type;

    return type;
}

// Add a field to a struct type
GooField* goo_type_add_field(GooType* struct_type, const char* name, GooType* type, int offset) {
    if (!struct_type || !name || !type) return NULL;

    // Create new field
    GooField* field = (GooField*)malloc(sizeof(GooField));
    if (!field) return NULL;

    field->name = strdup(name);
    if (!field->name) {
        free(field);
        return NULL;
    }

    field->type = type;
    field->offset = offset;

    // Add to the beginning of the linked list
    field->next = struct_type->fields;
    struct_type->fields = field;

    return field;
}

// Look up a type by name
GooType* goo_type_table_lookup(GooTypeTable* table, const char* name) {
    if (!table || !name) return NULL;

    GooType* current = table->types;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

// Look up a type by AST type node
GooType* goo_type_table_lookup_node(GooTypeTable* table, GooNode* type_node) {
    if (!table || !type_node) return NULL;

    // Handle identifier types
    if (type_node->type == GOO_NODE_IDENTIFIER) {
        // Get the name from the identifier node
        char* name = ((GooIdentifierNode*)type_node)->name;
        return goo_type_table_lookup(table, name);
    }
    
    // For now, we only handle identifier types
    // More complex types will be handled later
    return NULL;
}

// Convert an AST type node to an LLVM type
LLVMTypeRef goo_convert_type_node(GooTypeTable* table, GooNode* type_node, LLVMContextRef context) {
    if (!table || !type_node || !context) return NULL;

    // Look up the type in the type table
    GooType* type = goo_type_table_lookup_node(table, type_node);
    if (type) {
        return type->llvm_type;
    }

    // Handle different AST type nodes
    switch (type_node->type) {
        case GOO_NODE_IDENTIFIER: {
            char* name = ((GooIdentifierNode*)type_node)->name;
            
            // Handle built-in types
            if (strcmp(name, "void") == 0) {
                return LLVMVoidTypeInContext(context);
            } else if (strcmp(name, "bool") == 0) {
                return LLVMInt1TypeInContext(context);
            } else if (strcmp(name, "int") == 0) {
                return LLVMInt32TypeInContext(context);
            } else if (strcmp(name, "int8") == 0) {
                return LLVMInt8TypeInContext(context);
            } else if (strcmp(name, "int16") == 0) {
                return LLVMInt16TypeInContext(context);
            } else if (strcmp(name, "int32") == 0) {
                return LLVMInt32TypeInContext(context);
            } else if (strcmp(name, "int64") == 0) {
                return LLVMInt64TypeInContext(context);
            } else if (strcmp(name, "uint") == 0) {
                return LLVMInt32TypeInContext(context);
            } else if (strcmp(name, "uint8") == 0) {
                return LLVMInt8TypeInContext(context);
            } else if (strcmp(name, "uint16") == 0) {
                return LLVMInt16TypeInContext(context);
            } else if (strcmp(name, "uint32") == 0) {
                return LLVMInt32TypeInContext(context);
            } else if (strcmp(name, "uint64") == 0) {
                return LLVMInt64TypeInContext(context);
            } else if (strcmp(name, "float") == 0 || strcmp(name, "float64") == 0) {
                return LLVMDoubleTypeInContext(context);
            } else if (strcmp(name, "float32") == 0) {
                return LLVMFloatTypeInContext(context);
            } else if (strcmp(name, "string") == 0) {
                return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
            }
            
            // Check for user-defined types (not found in our lookup)
            fprintf(stderr, "Warning: Unresolved type '%s', defaulting to int\n", name);
            return LLVMInt32TypeInContext(context);
        }
        
        case GOO_NODE_TYPE_EXPR: {
            GooTypeNode* type_expr = (GooTypeNode*)type_node;
            
            // Handle array types - []T
            if (type_expr->type_kind == GOO_NODE_TYPE_EXPR && type_expr->elem_type) {
                // Get the element type
                LLVMTypeRef elem_llvm_type = goo_convert_type_node(table, type_expr->elem_type, context);
                if (!elem_llvm_type) {
                    fprintf(stderr, "Failed to resolve element type for array\n");
                    return LLVMInt32TypeInContext(context);
                }
                
                // Create a pointer type for the array (slice representation)
                return LLVMPointerType(elem_llvm_type, 0);
            }
            
            // Handle capability types
            if (type_expr->is_capability) {
                // For capability types, we retain the original type but add metadata for safety checks
                LLVMTypeRef base_type = goo_convert_type_node(table, type_expr->elem_type, context);
                
                // For now, capability types have the same LLVM representation
                return base_type;
            }
            
            fprintf(stderr, "Warning: Unresolved type expression, defaulting to int\n");
            return LLVMInt32TypeInContext(context);
        }
        
        case GOO_NODE_CAP_TYPE_EXPR: {
            // Capability types are used for safety systems
            // Get the base type and add capability information
            GooTypeNode* cap_type = (GooTypeNode*)type_node;
            LLVMTypeRef base_type = goo_convert_type_node(table, cap_type->elem_type, context);
            
            // For now, capability types have the same LLVM representation
            return base_type;
        }
        
        case GOO_NODE_CHANNEL_DECL: {
            // Channel types
            GooChannelDeclNode* channel_decl = (GooChannelDeclNode*)type_node;
            
            // Get the element type
            LLVMTypeRef elem_llvm_type = goo_convert_type_node(table, channel_decl->element_type, context);
            if (!elem_llvm_type) {
                fprintf(stderr, "Failed to resolve element type for channel\n");
                return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
            }
            
            // Channels are represented as opaque pointers for now
            return LLVMPointerType(LLVMInt8TypeInContext(context), 0);
        }
        
        // We could add more cases for other node types here
        
        default:
            fprintf(stderr, "Warning: Unsupported type node kind: %d, defaulting to int\n", type_node->type);
            return LLVMInt32TypeInContext(context);
    }
}

// Create a channel type with the specified element type
GooType* goo_type_table_create_channel(GooTypeTable* table, GooType* element_type, LLVMContextRef context) {
    if (!table || !element_type || !context) return NULL;

    // Generate a name for the channel type
    char name[256];
    snprintf(name, sizeof(name), "channel<%s>", element_type->name);

    // Check if this channel type already exists
    GooType* existing = goo_type_table_lookup(table, name);
    if (existing) return existing;

    // Channel is represented as opaque pointer in LLVM
    LLVMTypeRef llvm_type = LLVMPointerType(LLVMInt8TypeInContext(context), 0);

    // Create and add the channel type
    return goo_type_table_add(table, name, GOO_TYPE_CHANNEL, llvm_type, element_type);
}

// Create an array type with the specified element type and size
GooType* goo_type_table_create_array(GooTypeTable* table, GooType* element_type, int size, LLVMContextRef context) {
    if (!table || !element_type || !context) return NULL;

    // Generate a name for the array type
    char name[256];
    if (size > 0) {
        // Fixed-size array
        snprintf(name, sizeof(name), "[%d]%s", size, element_type->name);
    } else {
        // Slice (dynamic array)
        snprintf(name, sizeof(name), "[]%s", element_type->name);
    }

    // Check if this array type already exists
    GooType* existing = goo_type_table_lookup(table, name);
    if (existing) return existing;

    // Create the LLVM type
    LLVMTypeRef llvm_type;
    if (size > 0) {
        // Fixed-size array
        llvm_type = LLVMArrayType(element_type->llvm_type, size);
    } else {
        // Slice is represented as a structure with ptr, len, cap
        // { data: *element_type, len: int, cap: int }
        LLVMTypeRef struct_elements[3];
        struct_elements[0] = LLVMPointerType(element_type->llvm_type, 0);  // data pointer
        struct_elements[1] = LLVMInt64TypeInContext(context);              // len
        struct_elements[2] = LLVMInt64TypeInContext(context);              // cap
        
        llvm_type = LLVMStructTypeInContext(context, struct_elements, 3, false);
    }

    // Create and add the array type
    return goo_type_table_add(table, name, GOO_TYPE_ARRAY, llvm_type, element_type);
}

// Create a function type
GooType* goo_type_table_create_function(GooTypeTable* table, GooType* return_type, 
                                     GooType** param_types, int param_count, 
                                     LLVMContextRef context) {
    if (!table || !return_type || !context) return NULL;

    // Generate a name for the function type
    char name[512] = "func(";
    int name_index = 5; // Length of "func("
    
    // Add parameter types to the name
    for (int i = 0; i < param_count; i++) {
        if (!param_types[i]) continue;
        
        int remaining = sizeof(name) - name_index - 1;
        int written = snprintf(name + name_index, remaining, "%s%s", 
                             i > 0 ? ", " : "", 
                             param_types[i]->name);
        if (written < 0 || written >= remaining) {
            // Name would be too long, truncate
            name_index = sizeof(name) - 5;
            strcpy(name + name_index, "...)");
            break;
        }
        name_index += written;
    }
    
    // Add return type to the name
    int remaining = sizeof(name) - name_index - 1;
    snprintf(name + name_index, remaining, ") %s", return_type->name);
    
    // Check if this function type already exists
    GooType* existing = goo_type_table_lookup(table, name);
    if (existing) return existing;
    
    // Create LLVM function type
    LLVMTypeRef* llvm_param_types = (LLVMTypeRef*)malloc(param_count * sizeof(LLVMTypeRef));
    if (!llvm_param_types) {
        fprintf(stderr, "Failed to allocate memory for function parameter types\n");
        return NULL;
    }
    
    for (int i = 0; i < param_count; i++) {
        llvm_param_types[i] = param_types[i]->llvm_type;
    }
    
    LLVMTypeRef llvm_type = LLVMFunctionType(
        return_type->llvm_type,
        llvm_param_types,
        param_count,
        false
    );
    
    free(llvm_param_types);
    
    // Create and add the function type
    return goo_type_table_add(table, name, GOO_TYPE_FUNCTION, llvm_type, NULL);
}

// Create a struct type
GooType* goo_type_table_create_struct(GooTypeTable* table, const char* name, 
                                   LLVMContextRef context) {
    if (!table || !name || !context) return NULL;
    
    // Check if the struct type already exists
    GooType* existing = goo_type_table_lookup(table, name);
    if (existing) return existing;
    
    // Create a new opaque struct type
    LLVMTypeRef llvm_type = LLVMStructCreateNamed(context, name);
    
    // Add the type to the table
    return goo_type_table_add(table, name, GOO_TYPE_STRUCT, llvm_type, NULL);
}

// Set the fields of a struct type
bool goo_type_set_struct_body(GooType* struct_type, GooField* fields, LLVMContextRef context) {
    if (!struct_type || !context || struct_type->kind != GOO_TYPE_STRUCT) return false;
    
    // Count the number of fields
    int field_count = 0;
    GooField* field = fields;
    while (field) {
        field_count++;
        field = field->next;
    }
    
    // Create array of field types
    LLVMTypeRef* field_types = (LLVMTypeRef*)malloc(field_count * sizeof(LLVMTypeRef));
    if (!field_types) {
        fprintf(stderr, "Failed to allocate memory for struct field types\n");
        return false;
    }
    
    field = fields;
    for (int i = 0; i < field_count; i++) {
        if (!field) {
            free(field_types);
            return false;
        }
        field_types[i] = field->type->llvm_type;
        field = field->next;
    }
    
    // Set the struct body
    LLVMStructSetBody(struct_type->llvm_type, field_types, field_count, false);
    
    free(field_types);
    return true;
} 