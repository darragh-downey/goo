/**
 * goo_lang_memory.h
 * 
 * Language-specific memory operations for the Goo programming language.
 * This file defines structures and functions for memory management of language types.
 */

#ifndef GOO_LANG_MEMORY_H
#define GOO_LANG_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Goo string structure.
 */
typedef struct {
    char* data;     // String data (null-terminated)
    size_t length;  // String length (excluding null terminator)
} GooString;

/**
 * Goo array structure.
 */
typedef struct {
    void* data;         // Array data
    size_t element_size; // Size of each element
    size_t count;       // Number of elements
    size_t capacity;    // Capacity of the array
} GooArray;

/**
 * Allocate memory for a Goo string.
 * 
 * @param length The length of the string in bytes
 * @return A pointer to the allocated string memory
 */
char* goo_string_alloc(size_t length);

/**
 * Free a Goo string.
 * 
 * @param str The string to free
 * @param length The length of the string in bytes
 */
void goo_string_free(char* str, size_t length);

/**
 * Create a new Goo string from a C string.
 * 
 * @param cstr The C string to copy
 * @return A new Goo string
 */
GooString* goo_string_create(const char* cstr);

/**
 * Destroy a Goo string.
 * 
 * @param str The string to destroy
 */
void goo_string_destroy(GooString* str);

/**
 * Allocate memory for a Goo array.
 * 
 * @param element_size The size of each array element
 * @param count The number of elements
 * @return A pointer to the allocated array
 */
GooArray* goo_array_create(size_t element_size, size_t count);

/**
 * Resize a Goo array.
 * 
 * @param array The array to resize
 * @param new_count The new number of elements
 * @return true if the resize was successful, false otherwise
 */
bool goo_array_resize(GooArray* array, size_t new_count);

/**
 * Destroy a Goo array.
 * 
 * @param array The array to destroy
 */
void goo_array_destroy(GooArray* array);

/**
 * Set an element in a Goo array.
 * 
 * @param array The array
 * @param index The index to set
 * @param value Pointer to the value to set
 * @return true if the set was successful, false otherwise
 */
bool goo_array_set(GooArray* array, size_t index, const void* value);

/**
 * Get an element from a Goo array.
 * 
 * @param array The array
 * @param index The index to get
 * @param value Pointer to store the value
 * @return true if the get was successful, false otherwise
 */
bool goo_array_get(const GooArray* array, size_t index, void* value);

/**
 * Get a pointer to an element in a Goo array.
 * 
 * @param array The array
 * @param index The index to get
 * @return A pointer to the element, or NULL if the index is out of bounds
 */
void* goo_array_get_ptr(const GooArray* array, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* GOO_LANG_MEMORY_H */ 