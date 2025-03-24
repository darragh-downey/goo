/**
 * interface.c
 * 
 * Implementation of the interface system for the Goo programming language.
 */

#include <stdlib.h>
#include <string.h>
#include "goo_interface.h"

GooInterfaceTable *goo_interface_table_init() {
    GooInterfaceTable *table = malloc(sizeof(GooInterfaceTable));
    if (!table) return NULL;
    
    table->interfaces = NULL;
    table->interface_count = 0;
    table->impl_table = NULL;
    table->impl_count = 0;
    
    return table;
}

void goo_interface_table_free(GooInterfaceTable *table) {
    if (!table) return;
    
    // Free interfaces
    for (int i = 0; i < table->interface_count; i++) {
        GooInterface *interface = table->interfaces[i];
        // Free methods
        for (int j = 0; j < interface->method_count; j++) {
            free(interface->methods[j]);
        }
        free(interface->methods);
        free(interface->extends);
        free(interface);
    }
    free(table->interfaces);
    
    // Free implementations
    for (int i = 0; i < table->impl_count; i++) {
        free(table->impl_table[i]->method_impls);
        free(table->impl_table[i]);
    }
    free(table->impl_table);
    
    free(table);
}

GooInterface *goo_interface_register(
    GooInterfaceTable *table,
    const char *name,
    GooInterfaceMethod **methods,
    int method_count,
    GooInterface **extends,
    int extends_count
) {
    if (!table) return NULL;
    
    // Check if interface already exists
    for (int i = 0; i < table->interface_count; i++) {
        if (strcmp(table->interfaces[i]->name, name) == 0) {
            return table->interfaces[i];
        }
    }
    
    // Create new interface
    GooInterface *interface = malloc(sizeof(GooInterface));
    if (!interface) return NULL;
    
    interface->name = strdup(name);
    interface->methods = methods;
    interface->method_count = method_count;
    interface->extends = extends;
    interface->extends_count = extends_count;
    
    // Add to table
    table->interface_count++;
    table->interfaces = realloc(table->interfaces, table->interface_count * sizeof(GooInterface*));
    if (!table->interfaces) {
        free(interface);
        return NULL;
    }
    
    table->interfaces[table->interface_count - 1] = interface;
    
    return interface;
}

GooInterfaceImpl *goo_interface_impl_register(
    GooInterfaceTable *table,
    GooInterface *interface,
    TypeEntry *type,
    void **method_impls
) {
    if (!table || !interface || !type) return NULL;
    
    // Check if implementation already exists
    for (int i = 0; i < table->impl_count; i++) {
        if (table->impl_table[i]->interface == interface && 
            table->impl_table[i]->type == type) {
            return table->impl_table[i];
        }
    }
    
    // Create new implementation
    GooInterfaceImpl *impl = malloc(sizeof(GooInterfaceImpl));
    if (!impl) return NULL;
    
    impl->interface = interface;
    impl->type = type;
    impl->method_impls = method_impls;
    
    // Add to table
    table->impl_count++;
    table->impl_table = realloc(table->impl_table, table->impl_count * sizeof(GooInterfaceImpl*));
    if (!table->impl_table) {
        free(impl);
        return NULL;
    }
    
    table->impl_table[table->impl_count - 1] = impl;
    
    return impl;
}

/**
 * Check if two type entries are equivalent for interface method compatibility
 */
static bool type_entries_compatible(TypeEntry *required, TypeEntry *provided) {
    if (!required || !provided) return false;
    
    // Exact same type is obviously compatible
    if (required == provided) return true;
    
    // If names match, consider them compatible
    // (This is a simplified check, in a real compiler we'd need more sophisticated type checking)
    if (strcmp(required->name, provided->name) == 0) return true;
    
    // Special case for interfaces - if provided type implements required interface
    if (required->kind == TYPE_INTERFACE) {
        // TODO: Check if provided implements required interface
        // This would need to be handled by the type system
    }
    
    return false;
}

/**
 * Check if a method signature matches the interface method requirements
 * @param required The interface method
 * @param provided The method from the implementing type
 * @return true if signatures are compatible, false otherwise
 */
static bool method_signatures_compatible(
    GooInterfaceMethod *required,
    MethodSignature *provided
) {
    if (!required || !provided) return false;
    
    // Check parameter count
    if (required->param_count != provided->param_count) {
        return false;
    }
    
    // Check return type compatibility
    if (!type_entries_compatible(required->return_type, provided->return_type)) {
        return false;
    }
    
    // Check parameter type compatibility
    for (int i = 0; i < required->param_count; i++) {
        if (!type_entries_compatible(required->param_types[i], provided->param_types[i])) {
            return false;
        }
    }
    
    return true;
}

bool goo_interface_check_implicit(
    GooInterface *interface,
    TypeEntry *type,
    SymbolTable *symbol_table
) {
    if (!interface || !type || !symbol_table) return false;
    
    // Check each method of the interface
    for (int i = 0; i < interface->method_count; i++) {
        GooInterfaceMethod *method = interface->methods[i];
        
        // Skip methods with default implementations
        if (method->has_default_impl) continue;
        
        // Build method name (Type.Method)
        char method_name[256];
        snprintf(method_name, sizeof(method_name), "%s.%s", type->name, method->name);
        
        // Check if method exists in symbol table
        SymbolEntry *entry = symbol_table_lookup(symbol_table, method_name);
        if (!entry) {
            return false;
        }
        
        // Check method signature matches
        if (!method_signatures_compatible(method, entry->method_signature)) {
            return false;
        }
    }
    
    // Check extended interfaces
    for (int i = 0; i < interface->extends_count; i++) {
        if (!goo_interface_check_implicit(interface->extends[i], type, symbol_table)) {
            return false;
        }
    }
    
    return true;
}

void *goo_interface_resolve_method(
    GooInterface *interface,
    TypeEntry *type,
    const char *method_name,
    GooInterfaceTable *table,
    SymbolTable *symbol_table
) {
    if (!interface || !type || !method_name || !table || !symbol_table) return NULL;
    
    // Try to find explicit implementation first
    for (int i = 0; i < table->impl_count; i++) {
        GooInterfaceImpl *impl = table->impl_table[i];
        if (impl->interface == interface && impl->type == type) {
            // Find the method
            for (int j = 0; j < interface->method_count; j++) {
                if (strcmp(interface->methods[j]->name, method_name) == 0) {
                    return impl->method_impls[j];
                }
            }
        }
    }
    
    // Try to find implicit implementation
    // Build method name (Type.Method)
    char full_method_name[256];
    snprintf(full_method_name, sizeof(full_method_name), "%s.%s", type->name, method_name);
    
    // Check if method exists in symbol table
    SymbolEntry *entry = symbol_table_lookup(symbol_table, full_method_name);
    if (entry) {
        return entry->function_ptr;
    }
    
    // Try to find default implementation
    for (int i = 0; i < interface->method_count; i++) {
        if (strcmp(interface->methods[i]->name, method_name) == 0) {
            if (interface->methods[i]->has_default_impl) {
                return interface->methods[i]->default_impl;
            }
            break;
        }
    }
    
    // Check extended interfaces
    for (int i = 0; i < interface->extends_count; i++) {
        void *method = goo_interface_resolve_method(
            interface->extends[i], type, method_name, table, symbol_table);
        if (method) return method;
    }
    
    // Method not found
    return NULL;
}

GooInterface *goo_interface_get_by_name(
    GooInterfaceTable *table,
    const char *name
) {
    if (!table || !name) return NULL;
    
    for (int i = 0; i < table->interface_count; i++) {
        if (strcmp(table->interfaces[i]->name, name) == 0) {
            return table->interfaces[i];
        }
    }
    
    return NULL;
}

bool goo_interface_check_compile_time(
    GooInterface *interface,
    TypeEntry *type,
    GooInterfaceTable *table,
    SymbolTable *symbol_table,
    char *error_msg,
    size_t error_msg_size
) {
    if (!interface || !type || !table || !symbol_table) {
        if (error_msg) {
            snprintf(error_msg, error_msg_size, "Invalid parameters for interface check");
        }
        return false;
    }
    
    // First check for explicit implementation
    for (int i = 0; i < table->impl_count; i++) {
        GooInterfaceImpl *impl = table->impl_table[i];
        if (impl->interface == interface && impl->type == type) {
            // Found explicit implementation, verify each method
            for (int j = 0; j < interface->method_count; j++) {
                GooInterfaceMethod *method = interface->methods[j];
                
                // If no implementation is provided for a required method
                if (!method->has_default_impl && !impl->method_impls[j]) {
                    if (error_msg) {
                        snprintf(error_msg, error_msg_size, 
                                "Type %s missing implementation for required method %s of interface %s",
                                type->name, method->name, interface->name);
                    }
                    return false;
                }
            }
            
            // All methods are implemented
            return true;
        }
    }
    
    // No explicit implementation found, check for implicit implementation
    for (int i = 0; i < interface->method_count; i++) {
        GooInterfaceMethod *method = interface->methods[i];
        
        // Skip methods with default implementations
        if (method->has_default_impl) continue;
        
        // Build method name (Type.Method)
        char method_name[256];
        snprintf(method_name, sizeof(method_name), "%s.%s", type->name, method->name);
        
        // Check if method exists in symbol table
        SymbolEntry *entry = symbol_table_lookup(symbol_table, method_name);
        if (!entry) {
            if (error_msg) {
                snprintf(error_msg, error_msg_size, 
                        "Type %s missing required method %s for interface %s",
                        type->name, method->name, interface->name);
            }
            return false;
        }
        
        // Check method signature matches
        if (!method_signatures_compatible(method, entry->method_signature)) {
            if (error_msg) {
                snprintf(error_msg, error_msg_size, 
                        "Method %s of type %s has incompatible signature for interface %s",
                        method->name, type->name, interface->name);
            }
            return false;
        }
    }
    
    // Check extended interfaces
    for (int i = 0; i < interface->extends_count; i++) {
        if (!goo_interface_check_compile_time(
                interface->extends[i], type, table, symbol_table, error_msg, error_msg_size)) {
            return false;
        }
    }
    
    return true;
}

void **goo_interface_generate_default_stubs(
    GooInterface *interface,
    TypeEntry *type,
    GooInterfaceTable *table
) {
    if (!interface || !type || !table) return NULL;
    
    // Allocate method implementations array
    void **method_impls = calloc(interface->method_count, sizeof(void*));
    if (!method_impls) return NULL;
    
    // Check for existing explicit implementation
    for (int i = 0; i < table->impl_count; i++) {
        GooInterfaceImpl *impl = table->impl_table[i];
        if (impl->interface == interface && impl->type == type) {
            // Already has an implementation, just return that
            free(method_impls);
            return impl->method_impls;
        }
    }
    
    // Fill in implicit implementations and default stubs
    for (int i = 0; i < interface->method_count; i++) {
        GooInterfaceMethod *method = interface->methods[i];
        
        // Build method name (Type.Method)
        char method_name[256];
        snprintf(method_name, sizeof(method_name), "%s.%s", type->name, method->name);
        
        // Check symbol table for implicit implementation
        SymbolEntry *entry = symbol_table_lookup(symbol_table, method_name);
        if (entry) {
            method_impls[i] = entry->function_ptr;
        } else if (method->has_default_impl) {
            // Use default implementation
            method_impls[i] = method->default_impl;
        } else {
            // Missing required method
            free(method_impls);
            return NULL;
        }
    }
    
    // Handle extended interfaces
    for (int i = 0; i < interface->extends_count; i++) {
        GooInterface *extended = interface->extends[i];
        
        // Generate stubs for extended interface
        void **extended_impls = goo_interface_generate_default_stubs(extended, type, table);
        if (!extended_impls) {
            free(method_impls);
            return NULL;
        }
        
        // Copy implementations from extended interface where needed
        for (int j = 0; j < extended->method_count; j++) {
            GooInterfaceMethod *extended_method = extended->methods[j];
            
            // Find corresponding method in main interface
            for (int k = 0; k < interface->method_count; k++) {
                if (strcmp(interface->methods[k]->name, extended_method->name) == 0) {
                    // If not already implemented, use the implementation from extended interface
                    if (!method_impls[k]) {
                        method_impls[k] = extended_impls[j];
                    }
                    break;
                }
            }
        }
        
        // Free extended implementations array (we've copied what we need)
        free(extended_impls);
    }
    
    return method_impls;
}

bool goo_interface_verify_all_implementations(
    GooInterfaceTable *table,
    SymbolTable *symbol_table,
    char *error_msg,
    size_t error_msg_size
) {
    if (!table || !symbol_table) {
        if (error_msg) {
            snprintf(error_msg, error_msg_size, "Invalid parameters for interface verification");
        }
        return false;
    }
    
    // Check each explicit implementation
    for (int i = 0; i < table->impl_count; i++) {
        GooInterfaceImpl *impl = table->impl_table[i];
        
        // Verify the implementation
        if (!goo_interface_check_compile_time(
                impl->interface, impl->type, table, symbol_table, error_msg, error_msg_size)) {
            return false;
        }
    }
    
    // All implementations are valid
    return true;
} 