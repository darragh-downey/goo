/**
 * @file goo_type_safety.h
 * @brief Type safety implementation for the Goo language runtime
 *
 * This header defines structures and functions for enhancing type safety
 * in the Goo codebase. It implements runtime type checking, bounds checking,
 * and safe memory allocation with type tracking.
 */

#ifndef GOO_TYPE_SAFETY_H
#define GOO_TYPE_SAFETY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Type header structure placed before allocated memory
 */
typedef struct {
    uint32_t type_id;        /**< Unique identifier for the type */
    const char* type_name;   /**< Human-readable type name */
    size_t size;             /**< Size of a single element */
    size_t count;            /**< Number of elements */
} GooTypeHeader;

/**
 * @brief Type signature structure for type checking
 */
typedef struct {
    uint32_t type_id;        /**< Unique identifier for the type */
    const char* type_name;   /**< Human-readable type name */
    size_t type_size;        /**< Size of the type in bytes */
} GooTypeSignature;

/**
 * @brief Bounds-checked memory wrapper
 */
typedef struct {
    void* data;              /**< Pointer to the data */
    size_t size;             /**< Size of the buffer in bytes */
    GooTypeSignature type;   /**< Type signature of the buffer */
} GooSafeBuffer;

/**
 * @brief Generate a hash from a string (for type IDs)
 * 
 * @param str The string to hash
 * @return uint32_t The hash value
 */
uint32_t goo_hash_string(const char* str);

/**
 * @brief Get type signature for a type
 * 
 * @param type_name The name of the type
 * @param type_size The size of the type
 * @return GooTypeSignature The type signature
 */
GooTypeSignature goo_type_signature(const char* type_name, size_t type_size);

/**
 * @brief Check if a pointer has the expected type
 * 
 * @param data Pointer to check
 * @param expected_type Expected type signature
 * @return bool true if the type matches, false otherwise
 */
bool goo_check_type(void* data, GooTypeSignature expected_type);

/**
 * @brief Safe memory allocation with type tracking
 * 
 * @param count Number of elements to allocate
 * @param size Size of each element
 * @param type_name Name of the type
 * @return void* Pointer to the allocated memory (after the header)
 */
void* goo_safe_malloc_with_type(size_t count, size_t size, const char* type_name);

/**
 * @brief Free memory allocated with goo_safe_malloc_with_type
 * 
 * @param ptr Pointer to the memory to free
 */
void goo_safe_free(void* ptr);

/**
 * @brief Create a bounds-checked buffer
 * 
 * @param size Size of the buffer in bytes
 * @param type_name Name of the type stored in the buffer
 * @param type_size Size of the type stored in the buffer
 * @return GooSafeBuffer* Pointer to the created buffer or NULL on error
 */
GooSafeBuffer* goo_safe_buffer_create(size_t size, const char* type_name, size_t type_size);

/**
 * @brief Get a pointer to data in a bounds-checked buffer
 * 
 * @param buffer The buffer to access
 * @param offset Offset into the buffer in bytes
 * @param size Size of the data to access in bytes
 * @return void* Pointer to the data or NULL if out of bounds
 */
void* goo_safe_buffer_get(GooSafeBuffer* buffer, size_t offset, size_t size);

/**
 * @brief Free a bounds-checked buffer
 * 
 * @param buffer The buffer to free
 */
void goo_safe_buffer_free(GooSafeBuffer* buffer);

/**
 * @brief Check if a pointer is aligned to a specific boundary
 * 
 * @param ptr The pointer to check
 * @param alignment The required alignment
 * @return bool true if aligned, false otherwise
 */
bool goo_is_aligned(const void* ptr, size_t alignment);

/* Macro for safe memory allocation with type tracking */
#define GOO_SAFE_MALLOC(count, type) \
    ((type*)goo_safe_malloc_with_type((count), sizeof(type), #type))

/* Macro for bounds-checked array access */
#define GOO_ARRAY_GET(array, index, size) \
    (((index) < (size)) ? (array)[(index)] : \
        (fprintf(stderr, "Array bounds error: %zu >= %zu at %s:%d\n", \
            (size_t)(index), (size_t)(size), __FILE__, __LINE__), abort(), (array)[0]))

/* Macro for bounds-checked array access (non-aborting version) */
#define GOO_ARRAY_GET_SAFE(array, index, size, default_value) \
    (((index) < (size)) ? (array)[(index)] : (default_value))

/* Static assertions for type properties */
#define GOO_STATIC_ASSERT_TYPE_SIZE(type, size) \
    _Static_assert(sizeof(type) == (size), "Size of " #type " must be " #size " bytes")

#define GOO_STATIC_ASSERT_TYPE_ALIGNMENT(type, alignment) \
    _Static_assert(_Alignof(type) == (alignment), "Alignment of " #type " must be " #alignment " bytes")

/* Runtime invariant checking */
#ifdef GOO_DEBUG
    #define GOO_CHECK_INVARIANT(cond, message) \
        do { \
            if (!(cond)) { \
                fprintf(stderr, "Invariant violation at %s:%d: %s\n", \
                    __FILE__, __LINE__, (message)); \
                abort(); \
            } \
        } while (0)
#else
    #define GOO_CHECK_INVARIANT(cond, message) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GOO_TYPE_SAFETY_H */ 