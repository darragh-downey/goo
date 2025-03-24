#ifndef GOO_TYPE_SAFETY_H
#define GOO_TYPE_SAFETY_H

/**
 * Goo Language Type Safety System
 * Provides runtime type checking and safety features
 */

#include <stddef.h>
#include <stdbool.h>
#include "goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Type safety flags
typedef enum {
    GOO_TYPE_SAFE_DEFAULT = 0,      // Standard safety checks
    GOO_TYPE_SAFE_NONE = 1,         // No safety checks (unsafe mode)
    GOO_TYPE_SAFE_STRICT = 2,       // Extra rigorous safety checks
    GOO_TYPE_SAFE_RUNTIME = 3       // Runtime safety checks only
} GooTypeSafetyMode;

// Type metadata structure
typedef struct {
    const char* name;               // Type name
    size_t size;                    // Size of type in bytes
    bool is_pointer;                // Whether type is a pointer
    bool is_reference;              // Whether type is a reference
    bool is_value;                  // Whether type is a value type
    bool is_nullable;               // Whether type can be null
    GooTypeSafetyMode safety_mode;  // Safety mode for this type
} GooTypeInfo;

/**
 * Initialize type safety system
 */
bool goo_type_safety_init(void);

/**
 * Clean up type safety system
 */
void goo_type_safety_cleanup(void);

/**
 * Register a new type with the type system
 */
bool goo_register_type(const GooTypeInfo* type_info);

/**
 * Validate a type cast
 */
bool goo_validate_cast(void* ptr, const char* from_type, const char* to_type);

/**
 * Check if a pointer is valid for the given type
 */
bool goo_check_pointer(void* ptr, const char* type_name);

/**
 * Perform a safe pointer cast
 */
void* goo_safe_cast(void* ptr, const char* from_type, const char* to_type);

/**
 * Check if a value is within the valid range for its type
 */
bool goo_check_range(void* value, const char* type_name);

/**
 * Set the current safety mode for this thread
 */
void goo_set_safety_mode(GooTypeSafetyMode mode);

/**
 * Get the current safety mode for this thread
 */
GooTypeSafetyMode goo_get_safety_mode(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_TYPE_SAFETY_H 