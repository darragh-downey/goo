/**
 * @file scoped_alloc.h
 * @brief Scope-based memory allocation for the Goo programming language.
 *
 * This header provides functions for managing memory allocations within scopes.
 * Memory allocated within a scope is automatically freed when the scope is exited,
 * reducing the risk of memory leaks and simplifying memory management.
 */

#ifndef GOO_SCOPED_ALLOC_H
#define GOO_SCOPED_ALLOC_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a scoped allocator.
 */
typedef struct GooScopedAllocator GooScopedAllocator;

/**
 * @brief Opaque type representing a scope stack.
 */
typedef struct GooScopeStack GooScopeStack;

/**
 * @brief Opaque type representing a single scope.
 */
typedef struct GooScope GooScope;

/**
 * @brief Initialize the scope-based allocation subsystem.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_scoped_alloc_init(void);

/**
 * @brief Clean up the scope-based allocation subsystem.
 */
void goo_scoped_alloc_cleanup(void);

/**
 * @brief Create a new scoped allocator.
 *
 * @return A pointer to the new allocator, or NULL if creation fails.
 */
GooScopedAllocator* goo_scoped_alloc_create(void);

/**
 * @brief Destroy a scoped allocator.
 *
 * @param allocator The allocator to destroy.
 */
void goo_scoped_alloc_destroy(GooScopedAllocator* allocator);

/**
 * @brief Enter a new scope.
 *
 * @param allocator The allocator to use.
 */
void goo_scoped_alloc_enter_scope(GooScopedAllocator* allocator);

/**
 * @brief Exit the current scope, freeing all memory allocated within it.
 *
 * @param allocator The allocator to use.
 */
void goo_scoped_alloc_exit_scope(GooScopedAllocator* allocator);

/**
 * @brief Exit all scopes, freeing all memory allocated by the allocator.
 *
 * @param allocator The allocator to use.
 */
void goo_scoped_alloc_exit_all_scopes(GooScopedAllocator* allocator);

/**
 * @brief Allocate memory within the current scope.
 *
 * @param allocator The allocator to use.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void* goo_scoped_alloc_malloc(GooScopedAllocator* allocator, size_t size);

/**
 * @brief Allocate zeroed memory within the current scope.
 *
 * @param allocator The allocator to use.
 * @param size The number of bytes to allocate.
 * @return A pointer to the allocated memory, or NULL if allocation fails.
 */
void* goo_scoped_alloc_calloc(GooScopedAllocator* allocator, size_t num, size_t size);

/**
 * @brief Reallocate memory within the current scope.
 *
 * @param allocator The allocator to use.
 * @param ptr The pointer to reallocate.
 * @param size The new size.
 * @return A pointer to the reallocated memory, or NULL if reallocation fails.
 */
void* goo_scoped_alloc_realloc(GooScopedAllocator* allocator, void* ptr, size_t size);

/**
 * @brief Free memory allocated within the current scope.
 *
 * @param allocator The allocator to use.
 * @param ptr The pointer to free.
 */
void goo_scoped_alloc_free(GooScopedAllocator* allocator, void* ptr);

/**
 * @brief Create a new scope stack.
 *
 * @return A pointer to the new scope stack, or NULL if creation fails.
 */
GooScopeStack* goo_scope_stack_create(void);

/**
 * @brief Destroy a scope stack.
 *
 * @param stack The stack to destroy.
 */
void goo_scope_stack_destroy(GooScopeStack* stack);

/**
 * @brief Push a new scope onto the stack.
 *
 * @param stack The stack to push onto.
 */
void goo_scope_stack_push(GooScopeStack* stack);

/**
 * @brief Pop a scope from the stack, freeing all memory allocated within it.
 *
 * @param stack The stack to pop from.
 */
void goo_scope_stack_pop(GooScopeStack* stack);

/**
 * @brief Get the current scope from the stack.
 *
 * @param stack The stack to get the current scope from.
 * @return A pointer to the current scope, or NULL if the stack is empty.
 */
GooScope* goo_scope_stack_current(GooScopeStack* stack);

/**
 * @brief Execute a function within a new scope.
 *
 * @param allocator The allocator to use.
 * @param func The function to execute.
 * @param arg An argument to pass to the function.
 * @return The return value of the function.
 */
typedef void* (*GooScopedFunc)(void* arg);
void* goo_scoped_alloc_with_scope(GooScopedAllocator* allocator, GooScopedFunc func, void* arg);

/**
 * @brief Get the global scoped allocator.
 *
 * @return A pointer to the global scoped allocator.
 */
GooScopedAllocator* goo_get_global_scoped_allocator(void);

/**
 * @brief Get the current scope from the global allocator.
 *
 * @return A pointer to the current scope, or NULL if no scope has been entered.
 */
GooScope* goo_get_current_scope(void);

/**
 * @brief Convenience macro for entering a scope.
 */
#define GOO_BEGIN_SCOPE() \
    goo_scoped_alloc_enter_scope(goo_get_global_scoped_allocator()); \
    do {

/**
 * @brief Convenience macro for exiting a scope.
 */
#define GOO_END_SCOPE() \
    } while (0); \
    goo_scoped_alloc_exit_scope(goo_get_global_scoped_allocator())

#ifdef __cplusplus
}
#endif

#endif /* GOO_SCOPED_ALLOC_H */ 