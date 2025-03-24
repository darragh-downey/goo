/**
 * @file goo_type_safety.c
 * @brief Implementation of type safety functions for the Goo language runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../../../include/goo/runtime/safety/goo_type_safety.h"

/* FNV-1a hash function implementation for string hashing */
uint32_t goo_hash_string(const char* str) {
    const uint32_t FNV_PRIME = 16777619;
    const uint32_t FNV_OFFSET_BASIS = 2166136261;
    
    if (!str) return 0;
    
    uint32_t hash = FNV_OFFSET_BASIS;
    
    for (const unsigned char* p = (const unsigned char*)str; *p; ++p) {
        hash ^= *p;
        hash *= FNV_PRIME;
    }
    
    return hash;
}

/* Create a type signature for a type */
GooTypeSignature goo_type_signature(const char* type_name, size_t type_size) {
    GooTypeSignature sig;
    sig.type_id = goo_hash_string(type_name);
    sig.type_name = type_name;
    sig.type_size = type_size;
    return sig;
}

/* Check if a pointer has the expected type */
bool goo_check_type(void* data, GooTypeSignature expected_type) {
    if (!data) return false;
    
    /* Get the header which is placed before the actual data */
    GooTypeHeader* header = (GooTypeHeader*)data - 1;
    return header->type_id == expected_type.type_id;
}

/* Safe memory allocation with type tracking */
void* goo_safe_malloc_with_type(size_t count, size_t size, const char* type_name) {
    /* Check for integer overflow in multiplication */
    if (count > 0 && size > SIZE_MAX / count) {
        fprintf(stderr, "Integer overflow in allocation: %zu * %zu\n", count, size);
        return NULL;
    }
    
    /* Calculate total size including the header */
    size_t total_size = count * size + sizeof(GooTypeHeader);
    
    /* Allocate memory for the data plus the header */
    GooTypeHeader* header = (GooTypeHeader*)malloc(total_size);
    if (!header) {
        fprintf(stderr, "Memory allocation failed for %zu bytes\n", total_size);
        return NULL;
    }
    
    /* Initialize the header */
    header->type_id = goo_hash_string(type_name);
    header->type_name = type_name;
    header->size = size;
    header->count = count;
    
    /* Return a pointer to the data area (after the header) */
    return (void*)(header + 1);
}

/* Free memory allocated with goo_safe_malloc_with_type */
void goo_safe_free(void* ptr) {
    if (!ptr) return;
    
    /* Get the header which is placed before the actual data */
    GooTypeHeader* header = (GooTypeHeader*)ptr - 1;
    free(header);
}

/* Create a bounds-checked buffer */
GooSafeBuffer* goo_safe_buffer_create(size_t size, const char* type_name, size_t type_size) {
    if (size == 0) return NULL;
    
    GooSafeBuffer* buffer = (GooSafeBuffer*)malloc(sizeof(GooSafeBuffer));
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed for GooSafeBuffer\n");
        return NULL;
    }
    
    buffer->data = malloc(size);
    if (!buffer->data) {
        fprintf(stderr, "Memory allocation failed for buffer data (%zu bytes)\n", size);
        free(buffer);
        return NULL;
    }
    
    buffer->size = size;
    buffer->type = goo_type_signature(type_name, type_size);
    
    /* Initialize the buffer with zeros */
    memset(buffer->data, 0, size);
    
    return buffer;
}

/* Get a pointer to data in a bounds-checked buffer */
void* goo_safe_buffer_get(GooSafeBuffer* buffer, size_t offset, size_t size) {
    if (!buffer || !buffer->data) return NULL;
    
    /* Check for buffer overflow */
    if (offset + size > buffer->size) {
        fprintf(stderr, "Buffer bounds error: offset %zu + size %zu > buffer size %zu\n", 
                offset, size, buffer->size);
        return NULL;
    }
    
    return (uint8_t*)buffer->data + offset;
}

/* Free a bounds-checked buffer */
void goo_safe_buffer_free(GooSafeBuffer* buffer) {
    if (!buffer) return;
    
    if (buffer->data) {
        free(buffer->data);
    }
    
    free(buffer);
}

/* Check if a pointer is aligned to a specific boundary */
bool goo_is_aligned(const void* ptr, size_t alignment) {
    if (!ptr || alignment == 0) return false;
    
    /* Check if alignment is a power of 2 */
    if ((alignment & (alignment - 1)) != 0) {
        fprintf(stderr, "Alignment must be a power of 2: %zu\n", alignment);
        return false;
    }
    
    /* Check if the pointer is aligned */
    return (((uintptr_t)ptr) & (alignment - 1)) == 0;
} 