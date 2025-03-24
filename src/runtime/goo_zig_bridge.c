#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "goo_core.h"
#include "goo_runtime.h"
#include "goo_memory.h"
#include "goo_vectorization.h"
#include "goo_zig_runtime.h"

// Forward declarations for Zig functions (defined in goo_runtime_wrapper.zig)
extern bool goo_zig_memory_init(void);
extern void goo_zig_memory_cleanup(void);
extern void* goo_zig_alloc_aligned(size_t size, size_t alignment);
extern void* goo_zig_realloc_aligned(void* ptr, size_t old_size, size_t new_size, size_t alignment);
extern void goo_zig_free_aligned(void* ptr, size_t size, size_t alignment);
extern void goo_zig_copy_aligned(void* dest, const void* src, size_t size, size_t alignment);
extern void goo_zig_set_aligned(void* dest, uint8_t value, size_t size, size_t alignment);

extern bool goo_zig_vectorization_init(int simd_type);
extern void goo_zig_vectorization_cleanup(void);
extern int goo_zig_vectorization_detect_simd(void);
extern size_t goo_zig_vectorization_get_alignment(int simd_type);
extern bool goo_zig_vectorization_is_aligned(void* ptr, int simd_type);
extern size_t goo_zig_vectorization_get_width(int data_type, int simd_type);
extern bool goo_zig_vectorization_is_accelerated(int data_type, int op, int simd_type);
extern void* goo_zig_vectorization_create_mask(size_t size, int type_val);
extern void goo_zig_vectorization_free_mask(void* mask);

// Memory management bridge functions
static bool memory_initialized = false;

bool goo_memory_init(void) {
    if (memory_initialized) {
        return true;
    }
    
    memory_initialized = goo_zig_memory_init();
    return memory_initialized;
}

void goo_memory_cleanup(void) {
    if (memory_initialized) {
        goo_zig_memory_cleanup();
        memory_initialized = false;
    }
}

void* goo_alloc(size_t size) {
    return goo_zig_alloc_aligned(size, 8);
}

void* goo_realloc(void* ptr, size_t old_size, size_t new_size) {
    return goo_zig_realloc_aligned(ptr, old_size, new_size, 8);
}

void goo_free(void* ptr, size_t size) {
    goo_zig_free_aligned(ptr, size, 8);
}

void* goo_alloc_aligned(size_t size, size_t alignment) {
    return goo_zig_alloc_aligned(size, alignment);
}

void* goo_realloc_aligned(void* ptr, size_t old_size, size_t new_size, size_t alignment) {
    return goo_zig_realloc_aligned(ptr, old_size, new_size, alignment);
}

void goo_free_aligned(void* ptr, size_t size, size_t alignment) {
    goo_zig_free_aligned(ptr, size, alignment);
}

void goo_copy_mem(void* dest, const void* src, size_t size) {
    goo_zig_copy_aligned(dest, src, size, 1);
}

void goo_set_mem(void* dest, uint8_t value, size_t size) {
    goo_zig_set_aligned(dest, value, size, 1);
}

// Vectorization bridge functions
static bool vectorization_initialized = false;
static GooSIMDType active_simd_type = GOO_SIMD_NONE;

bool goo_vectorization_init(GooSIMDType simd_type) {
    if (vectorization_initialized) {
        return true;
    }
    
    if (simd_type == GOO_SIMD_AUTO) {
        simd_type = goo_vectorization_detect_simd();
    }
    
    int zig_simd_type = (int)simd_type;
    vectorization_initialized = goo_zig_vectorization_init(zig_simd_type);
    
    if (vectorization_initialized) {
        active_simd_type = simd_type;
    }
    
    return vectorization_initialized;
}

void goo_vectorization_cleanup(void) {
    if (vectorization_initialized) {
        goo_zig_vectorization_cleanup();
        vectorization_initialized = false;
        active_simd_type = GOO_SIMD_NONE;
    }
}

GooSIMDType goo_vectorization_detect_simd(void) {
    int simd_type = goo_zig_vectorization_detect_simd();
    return (GooSIMDType)simd_type;
}

GooSIMDType goo_vectorization_get_active_simd(void) {
    return active_simd_type;
}

size_t goo_vectorization_get_alignment(GooSIMDType simd_type) {
    return goo_zig_vectorization_get_alignment((int)simd_type);
}

bool goo_vectorization_is_aligned(void* ptr, GooSIMDType simd_type) {
    return goo_zig_vectorization_is_aligned(ptr, (int)simd_type);
}

size_t goo_vectorization_get_width(GooVectorDataType data_type, GooSIMDType simd_type) {
    return goo_zig_vectorization_get_width((int)data_type, (int)simd_type);
}

bool goo_vectorization_is_accelerated(GooVectorDataType data_type, GooVectorOp op, GooSIMDType simd_type) {
    return goo_zig_vectorization_is_accelerated((int)data_type, (int)op, (int)simd_type);
}

void* goo_vectorization_create_mask(size_t size, GooVectorDataType data_type) {
    return goo_zig_vectorization_create_mask(size, (int)data_type);
}

void goo_vectorization_free_mask(void* mask) {
    goo_zig_vectorization_free_mask(mask);
}

bool goo_vectorization_set_mask(void* mask, size_t* indices, size_t count) {
    // Implemented in C as a simple wrapper
    if (mask == NULL || (indices == NULL && count > 0)) {
        return false;
    }
    
    // Implementation depends on the mask structure, which is opaque
    // This is a stub - the actual implementation would set bits in the mask
    return true;
}

bool goo_vectorization_execute(GooVectorOp op, void* src1, void* src2, void* dst,
                              size_t elem_size, size_t length, GooVectorDataType data_type,
                              GooSIMDType simd_type, void* mask) {
    // Stub implementation - the actual execution depends on the Zig SIMD implementation
    // In a complete implementation, this would forward to the appropriate Zig function
    // based on the operation and data type
    
    if (simd_type == GOO_SIMD_AUTO) {
        simd_type = active_simd_type;
    }
    
    // Basic validation
    if (src1 == NULL || dst == NULL) {
        return false;
    }
    
    // Certain operations require src2 to be non-NULL
    if ((op >= GOO_VECTOR_OP_ADD && op <= GOO_VECTOR_OP_DIV) && src2 == NULL) {
        return false;
    }
    
    // For now, just use memcpy as a fallback for operations we don't handle
    switch (op) {
        case GOO_VECTOR_OP_LOAD:
            goo_copy_mem(dst, src1, elem_size * length);
            return true;
        case GOO_VECTOR_OP_STORE:
            goo_copy_mem(dst, src1, elem_size * length);
            return true;
        default:
            // This would be replaced with actual SIMD implementation calls
            return false;
    }
}

// Runtime integration
bool goo_zig_integration_init(void) {
    bool memory_ok = goo_memory_init();
    bool vector_ok = goo_vectorization_init(GOO_SIMD_AUTO);
    
    return memory_ok && vector_ok;
}

void goo_zig_integration_cleanup(void) {
    goo_vectorization_cleanup();
    goo_memory_cleanup();
} 