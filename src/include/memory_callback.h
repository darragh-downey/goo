/**
 * memory_callback.h
 * 
 * Memory callback functions for automatic resource management in Goo.
 * This file defines functions for registering cleanup callbacks for automatic memory management.
 */

#ifndef GOO_MEMORY_CALLBACK_H
#define GOO_MEMORY_CALLBACK_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Memory cleanup callback for regular allocations.
 * This function is called when a scope exits and registered memory needs to be freed.
 * 
 * @param data Pointer to the cleanup data
 */
void goo_memory_cleanup_callback(void* data);

/**
 * Register memory for automatic cleanup when the current scope exits.
 * 
 * @param ptr Pointer to the memory to free
 * @param size Size of the memory allocation
 * @param alignment Alignment of the memory (0 for default)
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_memory_cleanup(void* ptr, size_t size, size_t alignment);

/**
 * Register a resource with custom cleanup function for automatic cleanup when scope exits.
 * 
 * @param resource Pointer to the resource to clean up
 * @param cleanup_fn Function to call to clean up the resource
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_resource_cleanup(void* resource, void (*cleanup_fn)(void*));

/**
 * Register a cleanup function for the current scope.
 * This is the core function for the scope-based resource management system.
 * 
 * @param data User data to pass to the cleanup function
 * @param cleanup_fn Function to call when the scope exits
 * @return true if registration was successful, false otherwise
 */
bool goo_scope_register_cleanup(void* data, void (*cleanup_fn)(void*));

#ifdef __cplusplus
}
#endif

#endif /* GOO_MEMORY_CALLBACK_H */ 