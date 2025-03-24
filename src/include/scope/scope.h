/**
 * scope.h
 * 
 * Scope-based resource management system for Goo.
 * This provides automatic cleanup of resources when execution exits a scope.
 */

#ifndef GOO_SCOPE_H
#define GOO_SCOPE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the scope system.
 * This must be called before using any other scope functions.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_scope_init(void);

/**
 * Clean up the scope system.
 * This should be called when the program exits.
 */
void goo_scope_cleanup(void);

/**
 * Enter a new scope.
 * Resources registered in this scope will be cleaned up when the scope is exited.
 * 
 * @return true if the scope was entered successfully, false otherwise
 */
bool goo_scope_enter(void);

/**
 * Exit the current scope and clean up resources.
 * This will clean up all resources registered in the current scope and
 * return to the parent scope.
 */
void goo_scope_exit(void);

/**
 * Register a cleanup function for the current scope.
 * This function will be called when the scope is exited.
 * 
 * @param data User data to pass to the cleanup function
 * @param cleanup_fn Function to call when the scope exits
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_cleanup(void* data, void (*cleanup_fn)(void*));

#ifdef __cplusplus
}
#endif

#endif /* GOO_SCOPE_H */ 