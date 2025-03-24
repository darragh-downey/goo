/**
 * scoped_alloc_wrapper.c
 * 
 * C wrapper for the Zig implementation of scope-based memory allocation.
 * This file provides the C API for memory allocators that automatically
 * free memory when leaving a scope.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../include/memory.h"

// Forward declarations for Zig functions
extern bool scopedAllocInit(void);
extern void scopedAllocCleanup(void);
extern void* scopedAllocCreate(void);
extern void scopedAllocDestroy(void* scope);
extern void scopedAllocEnter(void* scope);
extern void scopedAllocExit(void* scope, bool free_all);
extern void scopedAllocFreeAll(void* scope);
extern void* scopedAllocMalloc(void* scope, size_t size);
extern void scopedAllocFree(void* scope, void* ptr, size_t size);
extern void* scopedAllocRealloc(void* scope, void* ptr, size_t old_size, size_t new_size);
extern void* scopeStackCreate(void);
extern void scopeStackDestroy(void* stack);
extern void* scopeStackPush(void* stack);
extern bool scopeStackPop(void* stack, bool free_all);
extern void* scopeStackGetCurrent(void* stack);
extern void scopeStackFreeAll(void* stack);

// Opaque types for C interface
typedef struct GooScopedAllocator GooScopedAllocator;
typedef struct GooScopeStack GooScopeStack;

/**
 * Initialize the scope-based memory allocation subsystem.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_scoped_alloc_init(void) {
    return scopedAllocInit();
}

/**
 * Clean up the scope-based memory allocation subsystem.
 */
void goo_scoped_alloc_cleanup(void) {
    scopedAllocCleanup();
}

/**
 * Create a new scoped allocator.
 * 
 * @return A new scoped allocator, or NULL if creation failed
 */
GooScopedAllocator* goo_scoped_alloc_create(void) {
    return (GooScopedAllocator*)scopedAllocCreate();
}

/**
 * Destroy a scoped allocator.
 * 
 * @param scope The scoped allocator to destroy
 */
void goo_scoped_alloc_destroy(GooScopedAllocator* scope) {
    scopedAllocDestroy(scope);
}

/**
 * Enter a scope, making it active.
 * 
 * @param scope The scoped allocator to enter
 */
void goo_scoped_alloc_enter(GooScopedAllocator* scope) {
    scopedAllocEnter(scope);
}

/**
 * Exit a scope, optionally freeing all allocations.
 * 
 * @param scope The scoped allocator to exit
 * @param free_all Whether to free all allocations made in this scope
 */
void goo_scoped_alloc_exit(GooScopedAllocator* scope, bool free_all) {
    scopedAllocExit(scope, free_all);
}

/**
 * Free all allocations made in a scope.
 * 
 * @param scope The scoped allocator
 */
void goo_scoped_alloc_free_all(GooScopedAllocator* scope) {
    scopedAllocFreeAll(scope);
}

/**
 * Allocate memory in a scope.
 * 
 * @param scope The scoped allocator
 * @param size The size of the memory to allocate
 * @return A pointer to the allocated memory, or NULL if allocation failed
 */
void* goo_scoped_alloc_malloc(GooScopedAllocator* scope, size_t size) {
    return scopedAllocMalloc(scope, size);
}

/**
 * Free memory allocated in a scope.
 * 
 * @param scope The scoped allocator
 * @param ptr The pointer to the memory to free
 * @param size The size of the memory to free
 */
void goo_scoped_alloc_free(GooScopedAllocator* scope, void* ptr, size_t size) {
    scopedAllocFree(scope, ptr, size);
}

/**
 * Reallocate memory in a scope.
 * 
 * @param scope The scoped allocator
 * @param ptr The pointer to the memory to reallocate
 * @param old_size The old size of the memory
 * @param new_size The new size of the memory
 * @return A pointer to the reallocated memory, or NULL if reallocation failed
 */
void* goo_scoped_alloc_realloc(GooScopedAllocator* scope, void* ptr, size_t old_size, size_t new_size) {
    return scopedAllocRealloc(scope, ptr, old_size, new_size);
}

/**
 * Create a new scope stack.
 * 
 * @return A new scope stack, or NULL if creation failed
 */
GooScopeStack* goo_scope_stack_create(void) {
    return (GooScopeStack*)scopeStackCreate();
}

/**
 * Destroy a scope stack.
 * 
 * @param stack The scope stack to destroy
 */
void goo_scope_stack_destroy(GooScopeStack* stack) {
    scopeStackDestroy(stack);
}

/**
 * Push a new scope onto the stack.
 * 
 * @param stack The scope stack
 * @return The new scoped allocator, or NULL if creation failed
 */
GooScopedAllocator* goo_scope_stack_push(GooScopeStack* stack) {
    return (GooScopedAllocator*)scopeStackPush(stack);
}

/**
 * Pop the current scope from the stack, optionally freeing all allocations.
 * 
 * @param stack The scope stack
 * @param free_all Whether to free all allocations made in the current scope
 * @return true if successful, false if the stack is empty
 */
bool goo_scope_stack_pop(GooScopeStack* stack, bool free_all) {
    return scopeStackPop(stack, free_all);
}

/**
 * Get the current scope from the stack.
 * 
 * @param stack The scope stack
 * @return The current scoped allocator, or NULL if the stack is empty
 */
GooScopedAllocator* goo_scope_stack_get_current(GooScopeStack* stack) {
    return (GooScopedAllocator*)scopeStackGetCurrent(stack);
}

/**
 * Free all allocations in all scopes in the stack.
 * 
 * @param stack The scope stack
 */
void goo_scope_stack_free_all(GooScopeStack* stack) {
    scopeStackFreeAll(stack);
}

/**
 * Execute a function within a new scope, automatically cleaning up memory when done.
 * 
 * @param func The function to execute
 * @param data The data to pass to the function
 * @param free_on_exit Whether to free all allocations when the function returns
 * @return true if the function was executed successfully, false otherwise
 */
bool goo_with_scope(bool (*func)(GooScopedAllocator* scope, void* data), void* data, bool free_on_exit) {
    GooScopedAllocator* scope = goo_scoped_alloc_create();
    if (!scope) {
        return false;
    }
    
    bool result = func(scope, data);
    
    if (free_on_exit) {
        goo_scoped_alloc_free_all(scope);
    }
    
    goo_scoped_alloc_destroy(scope);
    
    return result;
} 