/**
 * goo_lang_memory.c
 * 
 * Language-specific memory operations for the Goo programming language.
 * This file provides memory management functions tailored for Goo language constructs.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "memory/memory.h"
#include "scope/scope.h"
#include "runtime.h"
#include "lang/goo_lang_memory.h"

/**
 * Allocate memory for a Goo string.
 * 
 * @param length The length of the string in bytes
 * @return A pointer to the allocated string memory
 */
char* goo_string_alloc(size_t length) {
    // Allocate memory for the string plus a null terminator
    char* str = (char*)goo_alloc(length + 1);
    if (!str) {
        goo_runtime_out_of_memory(length + 1);
        return NULL;
    }
    
    // Initialize to zeros
    memset(str, 0, length + 1);
    
    return str;
}

/**
 * Free a Goo string.
 * 
 * @param str The string to free
 * @param length The length of the string in bytes
 */
void goo_string_free(char* str, size_t length) {
    if (!str) return;
    
    // Free the string memory
    goo_free(str, length + 1);
}

/**
 * Create a new Goo string from a C string.
 * 
 * @param cstr The C string to copy
 * @return A new Goo string
 */
GooString* goo_string_create(const char* cstr) {
    if (!cstr) return NULL;
    
    size_t length = strlen(cstr);
    
    // Allocate the string structure
    GooString* str = (GooString*)goo_alloc(sizeof(GooString));
    if (!str) {
        goo_runtime_out_of_memory(sizeof(GooString));
        return NULL;
    }
    
    // Allocate the string data
    str->data = goo_string_alloc(length);
    if (!str->data) {
        goo_free(str, sizeof(GooString));
        return NULL;
    }
    
    // Copy the string data
    memcpy(str->data, cstr, length);
    str->length = length;
    
    return str;
}

/**
 * Destroy a Goo string.
 * 
 * @param str The string to destroy
 */
void goo_string_destroy(GooString* str) {
    if (!str) return;
    
    // Free the string data
    if (str->data) {
        goo_string_free(str->data, str->length);
    }
    
    // Free the string structure
    goo_free(str, sizeof(GooString));
}

/**
 * Allocate memory for a Goo array.
 * 
 * @param element_size The size of each array element
 * @param count The number of elements
 * @return A pointer to the allocated array
 */
GooArray* goo_array_create(size_t element_size, size_t count) {
    // Allocate the array structure
    GooArray* array = (GooArray*)goo_alloc(sizeof(GooArray));
    if (!array) {
        goo_runtime_out_of_memory(sizeof(GooArray));
        return NULL;
    }
    
    // Allocate the array data
    size_t total_size = element_size * count;
    array->data = goo_alloc(total_size);
    if (!array->data) {
        goo_free(array, sizeof(GooArray));
        goo_runtime_out_of_memory(total_size);
        return NULL;
    }
    
    // Initialize the array
    array->element_size = element_size;
    array->count = count;
    array->capacity = count;
    
    // Zero the memory
    memset(array->data, 0, total_size);
    
    return array;
}

/**
 * Resize a Goo array.
 * 
 * @param array The array to resize
 * @param new_count The new number of elements
 * @return true if the resize was successful, false otherwise
 */
bool goo_array_resize(GooArray* array, size_t new_count) {
    if (!array) return false;
    
    // If the new size is within the current capacity, just update the count
    if (new_count <= array->capacity) {
        // If growing, initialize the new elements to zero
        if (new_count > array->count) {
            size_t new_bytes = (new_count - array->count) * array->element_size;
            char* start = ((char*)array->data) + (array->count * array->element_size);
            memset(start, 0, new_bytes);
        }
        
        array->count = new_count;
        return true;
    }
    
    // Calculate the new capacity (double the current or the new count, whichever is larger)
    size_t new_capacity = array->capacity * 2;
    if (new_capacity < new_count) {
        new_capacity = new_count;
    }
    
    // Reallocate the array data
    size_t old_size = array->capacity * array->element_size;
    size_t new_size = new_capacity * array->element_size;
    void* new_data = goo_realloc(array->data, old_size, new_size);
    if (!new_data) {
        goo_runtime_out_of_memory(new_size);
        return false;
    }
    
    // Update the array
    array->data = new_data;
    array->capacity = new_capacity;
    array->count = new_count;
    
    // Initialize the new elements to zero
    size_t old_bytes = old_size;
    size_t new_bytes = new_size - old_bytes;
    if (new_bytes > 0) {
        char* start = ((char*)array->data) + old_bytes;
        memset(start, 0, new_bytes);
    }
    
    return true;
}

/**
 * Destroy a Goo array.
 * 
 * @param array The array to destroy
 */
void goo_array_destroy(GooArray* array) {
    if (!array) return;
    
    // Free the array data
    if (array->data) {
        goo_free(array->data, array->capacity * array->element_size);
    }
    
    // Free the array structure
    goo_free(array, sizeof(GooArray));
}

/**
 * Set an element in a Goo array.
 * 
 * @param array The array
 * @param index The index to set
 * @param value Pointer to the value to set
 * @return true if the set was successful, false otherwise
 */
bool goo_array_set(GooArray* array, size_t index, const void* value) {
    if (!array || !value) return false;
    
    // Check if the index is within bounds
    if (index >= array->count) {
        return false;
    }
    
    // Calculate the offset
    size_t offset = index * array->element_size;
    
    // Copy the value
    memcpy(((char*)array->data) + offset, value, array->element_size);
    
    return true;
}

/**
 * Get an element from a Goo array.
 * 
 * @param array The array
 * @param index The index to get
 * @param value Pointer to store the value
 * @return true if the get was successful, false otherwise
 */
bool goo_array_get(const GooArray* array, size_t index, void* value) {
    if (!array || !value) return false;
    
    // Check if the index is within bounds
    if (index >= array->count) {
        return false;
    }
    
    // Calculate the offset
    size_t offset = index * array->element_size;
    
    // Copy the value
    memcpy(value, ((const char*)array->data) + offset, array->element_size);
    
    return true;
}

/**
 * Get a pointer to an element in a Goo array.
 * 
 * @param array The array
 * @param index The index to get
 * @return A pointer to the element, or NULL if the index is out of bounds
 */
void* goo_array_get_ptr(const GooArray* array, size_t index) {
    if (!array) return NULL;
    
    // Check if the index is within bounds
    if (index >= array->count) {
        return NULL;
    }
    
    // Calculate the offset
    size_t offset = index * array->element_size;
    
    // Return a pointer to the element
    return ((char*)array->data) + offset;
} 