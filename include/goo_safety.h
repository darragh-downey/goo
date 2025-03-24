/**
 * @file goo_safety.h
 * @brief Header file for the Goo Safety System
 * 
 * This header defines interfaces and macros for enhancing type safety
 * and preventing race conditions in the Goo language implementation.
 */

#ifndef GOO_SAFETY_H
#define GOO_SAFETY_H

/**
 * Goo Language Safety System
 * Provides runtime safety checking and error handling
 */

#include <stddef.h>
#include <stdbool.h>
#include <setjmp.h>
#include "goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Safety and error handling context
typedef struct GooSafetyContext {
    jmp_buf error_jmp;           // Jump buffer for error recovery
    void* error_value;           // Error value to return
    const char* error_message;   // Error message
    bool has_error;              // Whether an error has occurred
    bool can_recover;            // Whether errors can be recovered
} GooSafetyContext;

// Error handling result codes
typedef enum {
    GOO_ERROR_NONE = 0,          // No error
    GOO_ERROR_MEMORY = 1,        // Memory allocation error
    GOO_ERROR_BOUNDS = 2,        // Array bounds error
    GOO_ERROR_NULL = 3,          // Null pointer error
    GOO_ERROR_TYPE = 4,          // Type error
    GOO_ERROR_DIVISION = 5,      // Division by zero
    GOO_ERROR_OVERFLOW = 6,      // Numeric overflow
    GOO_ERROR_IO = 7,            // I/O error
    GOO_ERROR_SYSTEM = 8,        // Operating system error
    GOO_ERROR_CAPABILITY = 9,    // Capability error
    GOO_ERROR_USER = 10,         // User-defined error
    GOO_ERROR_INTERNAL = 11      // Internal error
} GooErrorCode;

/**
 * Initialize the safety system
 */
bool goo_safety_init(void);

/**
 * Clean up the safety system
 */
void goo_safety_cleanup(void);

/**
 * Set up a safety context for the current thread
 */
GooSafetyContext* goo_safety_context_create(void);

/**
 * Clean up safety context for the current thread
 */
void goo_safety_context_destroy(GooSafetyContext* context);

/**
 * Start a protected section that can recover from errors
 * Returns true if entering normally, false if recovering from an error
 */
bool goo_safety_try(GooSafetyContext* context);

/**
 * Throw an error within a protected section
 */
void goo_safety_throw(GooSafetyContext* context, GooErrorCode code, const char* message);

/**
 * Check a condition and throw an error if it's false
 */
void goo_safety_assert(GooSafetyContext* context, bool condition, GooErrorCode code, const char* message);

/**
 * Get the current safety context
 */
GooSafetyContext* goo_safety_get_current_context(void);

/**
 * Set the current safety context
 */
void goo_safety_set_current_context(GooSafetyContext* context);

/**
 * Trigger a panic (unrecoverable error)
 */
void goo_panic(void* error_value, const char* message);

/**
 * Check if the current context allows recovery from errors
 */
bool goo_safety_can_recover(void);

/**
 * Perform a bounds check and throw an error if out of bounds
 */
void goo_safety_check_bounds(GooSafetyContext* context, size_t index, size_t length);

/**
 * Perform a null check and throw an error if null
 */
void goo_safety_check_null(GooSafetyContext* context, void* ptr, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* GOO_SAFETY_H */ 