/**
 * goo_interface.h
 * 
 * Interface system for the Goo programming language.
 * Provides support for both implicit (Go-style) and explicit (Rust-style) interface implementation.
 */

#ifndef GOO_INTERFACE_H
#define GOO_INTERFACE_H

#include <stdbool.h>
#include <stddef.h>
#include "symbol_table.h"
#include "type_table.h"

/**
 * Interface method descriptor
 */
typedef struct GooInterfaceMethod {
    const char *name;                    // Method name
    int param_count;                     // Number of parameters
    TypeEntry *return_type;              // Return type
    TypeEntry **param_types;             // Parameter types
    void *default_impl;                  // Default implementation (function pointer)
    bool has_default_impl;               // Whether it has a default implementation
} GooInterfaceMethod;

/**
 * Interface descriptor
 */
typedef struct GooInterface {
    const char *name;                    // Interface name
    GooInterfaceMethod **methods;        // Array of methods
    int method_count;                    // Number of methods
    struct GooInterface **extends;       // Interfaces this interface extends
    int extends_count;                   // Number of extended interfaces
} GooInterface;

/**
 * Explicit implementation of an interface for a type
 */
typedef struct GooInterfaceImpl {
    GooInterface *interface;             // Interface being implemented
    TypeEntry *type;                     // Type implementing the interface
    void **method_impls;                 // Array of method implementations (function pointers)
} GooInterfaceImpl;

/**
 * Interface table for tracking all interfaces and implementations
 */
typedef struct GooInterfaceTable {
    GooInterface **interfaces;           // Array of all interfaces
    int interface_count;                 // Number of interfaces
    GooInterfaceImpl **impl_table;       // Array of explicit implementations
    int impl_count;                      // Number of implementations
} GooInterfaceTable;

/**
 * Initialize the interface table
 * @return A new interface table
 */
GooInterfaceTable *goo_interface_table_init();

/**
 * Free the interface table
 * @param table The interface table to free
 */
void goo_interface_table_free(GooInterfaceTable *table);

/**
 * Register a new interface
 * @param table The interface table
 * @param name The interface name
 * @param methods Array of methods
 * @param method_count Number of methods
 * @param extends Array of interfaces this interface extends
 * @param extends_count Number of extended interfaces
 * @return The new interface
 */
GooInterface *goo_interface_register(
    GooInterfaceTable *table,
    const char *name,
    GooInterfaceMethod **methods,
    int method_count,
    GooInterface **extends,
    int extends_count
);

/**
 * Register an explicit interface implementation
 * @param table The interface table
 * @param interface The interface being implemented
 * @param type The type implementing the interface
 * @param method_impls Array of method implementations (function pointers)
 * @return The new implementation
 */
GooInterfaceImpl *goo_interface_impl_register(
    GooInterfaceTable *table,
    GooInterface *interface,
    TypeEntry *type,
    void **method_impls
);

/**
 * Check if a type implicitly satisfies an interface
 * @param interface The interface to check
 * @param type The type to check
 * @param symbol_table The symbol table for looking up methods
 * @return True if the type satisfies the interface, false otherwise
 */
bool goo_interface_check_implicit(
    GooInterface *interface,
    TypeEntry *type,
    SymbolTable *symbol_table
);

/**
 * Resolve a method call for an interface
 * @param interface The interface
 * @param type The concrete type
 * @param method_name The method name
 * @param table The interface table
 * @param symbol_table The symbol table
 * @return Function pointer to the method implementation, or NULL if not found
 */
void *goo_interface_resolve_method(
    GooInterface *interface,
    TypeEntry *type,
    const char *method_name,
    GooInterfaceTable *table,
    SymbolTable *symbol_table
);

/**
 * Get an interface by name
 * @param table The interface table
 * @param name The interface name
 * @return The interface, or NULL if not found
 */
GooInterface *goo_interface_get_by_name(
    GooInterfaceTable *table,
    const char *name
);

/**
 * Compile-time check that a type satisfies an interface
 * This performs a complete static analysis at compile time to ensure
 * the type fully implements all required methods of the interface
 * 
 * @param interface The interface to check against
 * @param type The type to check
 * @param table The interface table
 * @param symbol_table The symbol table
 * @param error_msg Buffer to store error message if check fails
 * @param error_msg_size Size of error message buffer
 * @return True if the type satisfies the interface, false otherwise with error_msg set
 */
bool goo_interface_check_compile_time(
    GooInterface *interface,
    TypeEntry *type,
    GooInterfaceTable *table,
    SymbolTable *symbol_table,
    char *error_msg,
    size_t error_msg_size
);

/**
 * Generate method resolution stubs for default interface methods
 * This creates function stubs that handle calling the default implementations
 * when a type doesn't provide its own implementation of an optional method
 * 
 * @param interface The interface with default methods
 * @param type The type implementing the interface
 * @param table The interface table
 * @return Array of function pointers to generated stubs, or NULL on error
 */
void **goo_interface_generate_default_stubs(
    GooInterface *interface,
    TypeEntry *type,
    GooInterfaceTable *table
);

/**
 * Verify all explicit interface implementations at compile time
 * This checks that all registered implementations correctly satisfy
 * their interfaces and reports any errors
 * 
 * @param table The interface table
 * @param symbol_table The symbol table
 * @param error_msg Buffer to store error message if check fails
 * @param error_msg_size Size of error message buffer
 * @return True if all implementations are valid, false otherwise with error_msg set
 */
bool goo_interface_verify_all_implementations(
    GooInterfaceTable *table,
    SymbolTable *symbol_table,
    char *error_msg,
    size_t error_msg_size
);

#endif /* GOO_INTERFACE_H */ 