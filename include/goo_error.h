#ifndef GOO_ERROR_H
#define GOO_ERROR_H

#include <stdbool.h>

/**
 * Goo Error Handling System
 * 
 * This header defines the error handling API for the Goo language,
 * including panic/recover mechanisms and error propagation.
 */

// Setup error recovery point
// Returns: true on initial setup, false if recovering from panic
bool goo_recover_setup(void);

// Finish a recovery block (clean up the recovery point)
void goo_recover_finish(void);

// Trigger a panic with a value and message
// value: The value to pass to recover
// message: The error message
void goo_panic(void* value, const char* message);

// Check if the current thread is in a panic state
// Returns: true if in a panic state, false otherwise
bool goo_is_panic(void);

// Get the value passed to panic
// Returns: The panic value, or NULL if not in a panic
void* goo_get_panic_value(void);

// Get the panic message
// Returns: The panic message, or NULL if not in a panic
const char* goo_get_panic_message(void);

// Clear the panic state
void goo_clear_panic(void);

// Runtime panic function (unrecoverable error if no recovery point)
// message: Error message
void goo_runtime_panic(const char* message);

#endif // GOO_ERROR_H 