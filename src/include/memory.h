/**
 * memory.h
 * 
 * Memory management functions for the Goo programming language.
 * This file defines the public API for memory allocation in Goo programs.
 */

#ifndef GOO_MEMORY_H
#define GOO_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the memory subsystem.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_memory_init(void);

/**
 * Clean up the memory subsystem.
 */
void goo_memory_cleanup(void);

/**
 * Allocate memory using the Goo allocator.
 * 
 * @param size The number of bytes to allocate
 * @return A pointer to the allocated memory, or NULL if allocation failed
 */
void* goo_alloc(size_t size);

/**
 * Free memory allocated with goo_alloc.
 * 
 * @param ptr The pointer to the memory to free
 * @param size The size of the memory block
 */
void goo_free(void* ptr, size_t size);

/**
 * Reallocate memory using the Goo allocator.
 * 
 * @param ptr The pointer to the memory to reallocate
 * @param old_size The old size of the memory block
 * @param new_size The new size of the memory block
 * @return A pointer to the reallocated memory, or NULL if reallocation failed
 */
void* goo_realloc(void* ptr, size_t old_size, size_t new_size);

/**
 * Allocate aligned memory.
 * 
 * @param size The number of bytes to allocate
 * @param alignment The alignment of the allocated memory
 * @return A pointer to the allocated memory, or NULL if allocation failed
 */
void* goo_alloc_aligned(size_t size, size_t alignment);

/**
 * Free aligned memory.
 * 
 * @param ptr The pointer to the memory to free
 * @param size The size of the memory block
 * @param alignment The alignment of the memory block
 */
void goo_free_aligned(void* ptr, size_t size, size_t alignment);

/**
 * Reallocate aligned memory.
 * 
 * @param ptr The pointer to the memory to reallocate
 * @param old_size The old size of the memory block
 * @param new_size The new size of the memory block
 * @param alignment The alignment of the memory block
 * @return A pointer to the reallocated memory, or NULL if reallocation failed
 */
void* goo_realloc_aligned(void* ptr, size_t old_size, size_t new_size, size_t alignment);

/**
 * Allocate and zero memory.
 * 
 * @param count The number of elements to allocate
 * @param size The size of each element
 * @return A pointer to the allocated memory, or NULL if allocation failed
 */
void* goo_calloc(size_t count, size_t size);

/**
 * Create a duplicate of the given string using the Goo allocator.
 * 
 * @param str The string to duplicate
 * @return A pointer to the duplicated string, or NULL if allocation failed
 */
char* goo_strdup(const char* str);

/**
 * Allocate memory with an error handler.
 * This function will panic if allocation fails.
 * 
 * @param size The number of bytes to allocate
 * @return A pointer to the allocated memory
 */
void* goo_alloc_or_panic(size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GOO_MEMORY_H */ 