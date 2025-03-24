/**
 * @file goo_runtime.h
 * @brief Unified runtime initialization and cleanup for the Goo programming language.
 *
 * This header provides functions for initializing and cleaning up the Goo runtime
 * system, which manages all subsystems including memory management, scope-based
 * allocation, compile-time evaluation, and reflection.
 */

#ifndef GOO_RUNTIME_H
#define GOO_RUNTIME_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the Goo runtime.
 *
 * This function initializes all subsystems in the correct order:
 * 1. Memory management
 * 2. Scope-based allocation
 * 3. Compile-time evaluation
 * 4. Reflection
 * 5. Parallel execution
 * 6. Messaging
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_runtime_init(void);

/**
 * @brief Clean up the Goo runtime.
 *
 * This function cleans up all initialized subsystems in reverse order.
 * It's safe to call this function multiple times - subsequent calls will be no-ops.
 */
void goo_runtime_cleanup(void);

/**
 * @brief Check if the runtime is initialized.
 *
 * @return true if the runtime is initialized, false otherwise.
 */
bool goo_runtime_is_initialized(void);

/**
 * @brief Get the current version of the Goo runtime.
 *
 * @return A string containing the version information.
 */
const char* goo_runtime_version(void);

#ifdef __cplusplus
}
#endif

#endif /* GOO_RUNTIME_H */ 