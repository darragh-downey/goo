/**
 * runtime.h
 * 
 * Main runtime interface for the Goo programming language.
 * This file defines functions for initializing and controlling the runtime.
 */

#ifndef GOO_RUNTIME_H
#define GOO_RUNTIME_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the Goo runtime.
 * This initializes all runtime subsystems in the correct order.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_runtime_init(void);

/**
 * Clean up the Goo runtime.
 * This cleans up all runtime subsystems in the reverse order of initialization.
 */
void goo_runtime_cleanup(void);

/**
 * Check if the runtime is initialized.
 * 
 * @return true if the runtime is initialized, false otherwise
 */
bool goo_runtime_is_initialized(void);

/**
 * Report a runtime error and abort the program.
 * 
 * @param message The error message to display
 */
void goo_runtime_error(const char* message);

/**
 * Handle out of memory condition.
 * This is called when memory allocation fails.
 * 
 * @param size The size that failed to allocate
 */
void goo_runtime_out_of_memory(size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GOO_RUNTIME_H */ 