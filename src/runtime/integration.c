#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include "../include/goo_runtime.h"
#include "../include/goo_memory.h"
#include "../include/goo_capability.h"
#include "../include/goo_error.h"

#include "goo_core.h"
#include "goo_vectorization.h"
#include "goo_zig_runtime.h"

// Runtime integration status
static struct {
    bool initialized;
    bool subsystems_initialized;
    pthread_mutex_t init_mutex;
    GooArenaAllocator* global_arena;
    GooPoolAllocator** type_pools;
    int type_pool_count;
} runtime_integration = {
    .initialized = false,
    .subsystems_initialized = false,
    .type_pool_count = 0
};

// Initialize runtime integration
bool goo_runtime_integration_init(void) {
    // Initialize the mutex
    if (pthread_mutex_init(&runtime_integration.init_mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize runtime integration mutex\n");
        return false;
    }
    
    pthread_mutex_lock(&runtime_integration.init_mutex);
    
    // Check if already initialized
    if (runtime_integration.initialized) {
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return true;
    }
    
    // Mark as initialized
    runtime_integration.initialized = true;
    
    // Create global arena allocator for runtime use
    runtime_integration.global_arena = goo_arena_create(1024 * 1024); // 1MB pages
    if (!runtime_integration.global_arena) {
        fprintf(stderr, "Failed to create global arena allocator\n");
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return false;
    }
    
    // Initialize type pools array
    runtime_integration.type_pools = NULL;
    runtime_integration.type_pool_count = 0;
    
    pthread_mutex_unlock(&runtime_integration.init_mutex);
    return true;
}

// Initialize runtime subsystems
bool goo_runtime_subsystems_init(void) {
    if (!runtime_integration.initialized) {
        fprintf(stderr, "Runtime integration not initialized\n");
        return false;
    }
    
    pthread_mutex_lock(&runtime_integration.init_mutex);
    
    // Check if already initialized
    if (runtime_integration.subsystems_initialized) {
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return true;
    }
    
    // Initialize capability system
    if (!goo_capability_system_init()) {
        fprintf(stderr, "Failed to initialize capability system\n");
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return false;
    }
    
    // Mark subsystems as initialized
    runtime_integration.subsystems_initialized = true;
    
    pthread_mutex_unlock(&runtime_integration.init_mutex);
    return true;
}

// Get or create a type-specific memory pool
GooPoolAllocator* goo_runtime_get_type_pool(size_t obj_size, const char* type_name) {
    if (!runtime_integration.initialized) {
        fprintf(stderr, "Runtime integration not initialized\n");
        return NULL;
    }
    
    pthread_mutex_lock(&runtime_integration.init_mutex);
    
    // Search for existing pool of this size
    for (int i = 0; i < runtime_integration.type_pool_count; i++) {
        GooPoolAllocator* pool = runtime_integration.type_pools[i];
        if (goo_pool_get_obj_size(pool) == obj_size) {
            pthread_mutex_unlock(&runtime_integration.init_mutex);
            return pool;
        }
    }
    
    // Create a new pool for this type
    size_t initial_capacity = 32;  // Start with 32 objects
    float growth_factor = 2.0f;   // Double when full
    
    GooPoolAllocator* pool = goo_pool_create(obj_size, initial_capacity, growth_factor);
    if (!pool) {
        fprintf(stderr, "Failed to create pool for type %s\n", type_name ? type_name : "unknown");
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return NULL;
    }
    
    // Add the pool to our list
    int new_count = runtime_integration.type_pool_count + 1;
    GooPoolAllocator** new_pools = realloc(runtime_integration.type_pools, 
                                         new_count * sizeof(GooPoolAllocator*));
    if (!new_pools) {
        fprintf(stderr, "Failed to expand type pools array\n");
        goo_pool_destroy(pool);
        pthread_mutex_unlock(&runtime_integration.init_mutex);
        return NULL;
    }
    
    runtime_integration.type_pools = new_pools;
    runtime_integration.type_pools[runtime_integration.type_pool_count] = pool;
    runtime_integration.type_pool_count = new_count;
    
    pthread_mutex_unlock(&runtime_integration.init_mutex);
    return pool;
}

// Allocate memory with type information
void* goo_runtime_typed_alloc(size_t size, const char* type_name) {
    if (!runtime_integration.initialized) {
        fprintf(stderr, "Runtime integration not initialized\n");
        return NULL;
    }
    
    // For small objects of known type, use a type-specific pool
    if (size <= 1024 && type_name != NULL) {
        GooPoolAllocator* pool = goo_runtime_get_type_pool(size, type_name);
        if (pool) {
            return goo_pool_alloc(pool);
        }
    }
    
    // For larger objects or when pool allocation fails, use the thread-local allocator
    GooCustomAllocator* current = goo_get_current_allocator();
    if (current) {
        return goo_custom_alloc(current, size, 8);  // Default 8-byte alignment
    }
    
    // Fallback to global arena
    return goo_arena_alloc(runtime_integration.global_arena, size, 8);
}

// Free memory with type information
void goo_runtime_typed_free(void* ptr, const char* type_name, size_t size) {
    if (!runtime_integration.initialized || !ptr) {
        return;
    }
    
    // For small objects of known type, try to return to type-specific pool
    // Note: In a full implementation, we'd store allocator information with the object
    if (size <= 1024 && type_name != NULL) {
        for (int i = 0; i < runtime_integration.type_pool_count; i++) {
            GooPoolAllocator* pool = runtime_integration.type_pools[i];
            if (goo_pool_get_obj_size(pool) == size) {
                // This assumes ptr came from this pool - in real implementation,
                // we'd need to track which pool each object came from
                goo_pool_free(pool, ptr);
                return;
            }
        }
    }
    
    // If we can't return to a pool, try the current thread allocator
    GooCustomAllocator* current = goo_get_current_allocator();
    if (current) {
        goo_custom_free(current, ptr);
    }
    
    // Note: Arena allocations can't be individually freed
    // They'll be freed when the arena is reset or destroyed
}

// Connect error handling with runtime
void goo_runtime_handle_error(void* error_value, const char* error_message) {
    // Log the error
    if (error_message) {
        fprintf(stderr, "Runtime error: %s\n", error_message);
    } else {
        fprintf(stderr, "Runtime error: <no message>\n");
    }
    
    // Check if error can be recovered based on capabilities
    GooCapabilitySet* caps = goo_runtime_get_current_caps();
    if (caps && goo_capability_check(caps, GOO_CAP_ERROR_HANDLING)) {
        // Trigger a panic that can be recovered
        goo_panic(error_value, error_message);
    } else {
        // Unrecoverable error - terminate
        goo_runtime_panic(error_message ? error_message : "Unrecoverable error");
    }
}

// Connect capability system with memory allocators
void* goo_runtime_capability_checked_alloc(int capability_type, size_t size, size_t alignment) {
    // Check if current context has the capability
    GooCapabilitySet* caps = goo_runtime_get_current_caps();
    if (!caps || !goo_capability_check(caps, capability_type)) {
        // No permission - trigger a panic
        char error_msg[100];
        snprintf(error_msg, sizeof(error_msg), "Missing capability for memory allocation: %d", capability_type);
        goo_panic(NULL, error_msg);
        return NULL;
    }
    
    // Perform the allocation
    GooCustomAllocator* current = goo_get_current_allocator();
    if (current) {
        return goo_custom_alloc(current, size, alignment);
    }
    
    // Fallback to global arena
    return goo_arena_alloc(runtime_integration.global_arena, size, alignment);
}

// Cleanup and shutdown runtime integration
void goo_runtime_integration_shutdown(void) {
    if (!runtime_integration.initialized) {
        return;
    }
    
    pthread_mutex_lock(&runtime_integration.init_mutex);
    
    // Clean up subsystems if initialized
    if (runtime_integration.subsystems_initialized) {
        goo_capability_system_shutdown();
        runtime_integration.subsystems_initialized = false;
    }
    
    // Free type pools
    for (int i = 0; i < runtime_integration.type_pool_count; i++) {
        goo_pool_destroy(runtime_integration.type_pools[i]);
    }
    free(runtime_integration.type_pools);
    runtime_integration.type_pools = NULL;
    runtime_integration.type_pool_count = 0;
    
    // Destroy global arena
    if (runtime_integration.global_arena) {
        goo_arena_destroy(runtime_integration.global_arena);
        runtime_integration.global_arena = NULL;
    }
    
    // Mark as uninitialized
    runtime_integration.initialized = false;
    
    pthread_mutex_unlock(&runtime_integration.init_mutex);
    pthread_mutex_destroy(&runtime_integration.init_mutex);
}

// Register runtime integration with runtime
bool goo_runtime_register_integration(void) {
    // Initialize integration
    if (!goo_runtime_integration_init()) {
        return false;
    }
    
    // Initialize subsystems
    if (!goo_runtime_subsystems_init()) {
        goo_runtime_integration_shutdown();
        return false;
    }
    
    return true;
}

// Initialize Zig runtime components from C
bool goo_initialize_zig_runtime(void) {
    printf("Initializing Goo Zig runtime components...\n");
    
    // Initialize memory management
    if (!goo_zig_memory_init()) {
        fprintf(stderr, "Failed to initialize Zig memory system\n");
        return false;
    }
    
    // Initialize vectorization with auto-detection
    if (!goo_zig_vectorization_init(GOO_SIMD_AUTO)) {
        fprintf(stderr, "Failed to initialize Zig vectorization system\n");
        goo_zig_memory_cleanup();
        return false;
    }
    
    // Detect available SIMD capabilities
    GooSIMDType detected_simd = goo_zig_detect_simd();
    printf("Detected SIMD support: %s\n", 
           detected_simd == GOO_SIMD_SCALAR ? "Scalar only" :
           detected_simd == GOO_SIMD_SSE2 ? "SSE2" :
           detected_simd == GOO_SIMD_SSE4 ? "SSE4" :
           detected_simd == GOO_SIMD_AVX ? "AVX" :
           detected_simd == GOO_SIMD_AVX2 ? "AVX2" :
           detected_simd == GOO_SIMD_AVX512 ? "AVX512" :
           detected_simd == GOO_SIMD_NEON ? "NEON" :
           "Unknown");
    
    return true;
}

// Perform a vector operation using Zig SIMD implementation
bool goo_perform_vector_operation(GooVectorOp op, 
                                void* src1, 
                                void* src2, 
                                void* dst, 
                                size_t elem_size, 
                                size_t length, 
                                GooVectorDataType data_type) {
    // Use auto-detection for SIMD type
    GooSIMDType detected_simd = goo_zig_detect_simd();
    
    // Check if the vector operation can be accelerated
    if (!goo_zig_vector_is_accelerated(data_type, op, detected_simd)) {
        fprintf(stderr, "Vector operation cannot be accelerated with available SIMD\n");
        return false;
    }
    
    // Check alignment
    size_t required_alignment = goo_zig_get_alignment_for_simd(detected_simd);
    
    if (!goo_zig_vector_is_aligned(src1, detected_simd)) {
        fprintf(stderr, "Source 1 buffer not properly aligned for SIMD\n");
        return false;
    }
    
    if (src2 && !goo_zig_vector_is_aligned(src2, detected_simd)) {
        fprintf(stderr, "Source 2 buffer not properly aligned for SIMD\n");
        return false;
    }
    
    if (!goo_zig_vector_is_aligned(dst, detected_simd)) {
        fprintf(stderr, "Destination buffer not properly aligned for SIMD\n");
        return false;
    }
    
    // Execute the vector operation
    return goo_zig_vector_execute(op, src1, src2, dst, elem_size, length, data_type, detected_simd, NULL);
}

// Allocate memory with proper alignment for SIMD operations
void* goo_allocate_simd_buffer(size_t size) {
    // Get required alignment for current SIMD capabilities
    GooSIMDType detected_simd = goo_zig_detect_simd();
    size_t alignment = goo_zig_get_alignment_for_simd(detected_simd);
    
    // Allocate aligned memory using Zig allocator
    return goo_zig_alloc_aligned(size, alignment);
}

// Free memory allocated with goo_allocate_simd_buffer
void goo_free_simd_buffer(void* ptr, size_t size) {
    // Get required alignment for current SIMD capabilities
    GooSIMDType detected_simd = goo_zig_detect_simd();
    size_t alignment = goo_zig_get_alignment_for_simd(detected_simd);
    
    // Free aligned memory using Zig allocator
    goo_zig_free_aligned(ptr, size, alignment);
}

// Clean up Zig runtime components
void goo_cleanup_zig_runtime(void) {
    goo_zig_vectorization_cleanup();
    goo_zig_memory_cleanup();
    printf("Goo Zig runtime components cleaned up\n");
}

// Test function to validate Zig integration
bool goo_test_zig_integration(void) {
    printf("Testing Goo Zig integration...\n");
    
    // Allocate test buffers
    const size_t elem_count = 16;
    const size_t elem_size = sizeof(int32_t);
    const size_t buffer_size = elem_count * elem_size;
    
    int32_t* src1 = (int32_t*)goo_allocate_simd_buffer(buffer_size);
    int32_t* src2 = (int32_t*)goo_allocate_simd_buffer(buffer_size);
    int32_t* dst = (int32_t*)goo_allocate_simd_buffer(buffer_size);
    
    if (!src1 || !src2 || !dst) {
        fprintf(stderr, "Failed to allocate test buffers\n");
        goto cleanup;
    }
    
    // Initialize test data
    for (size_t i = 0; i < elem_count; i++) {
        src1[i] = (int32_t)i;
        src2[i] = 10;
    }
    
    // Perform vector addition
    bool result = goo_perform_vector_operation(
        GOO_VECTOR_OP_ADD,
        src1,
        src2,
        dst,
        elem_size,
        elem_count,
        GOO_VECTOR_DATA_INT32
    );
    
    if (!result) {
        fprintf(stderr, "Vector operation failed\n");
        goto cleanup;
    }
    
    // Verify results
    bool verify_success = true;
    for (size_t i = 0; i < elem_count; i++) {
        if (dst[i] != src1[i] + src2[i]) {
            fprintf(stderr, "Verification failed at index %zu: %d + %d != %d\n", 
                    i, src1[i], src2[i], dst[i]);
            verify_success = false;
            break;
        }
    }
    
    if (verify_success) {
        printf("Vector operation successfully verified\n");
    }
    
cleanup:
    // Free test buffers
    if (src1) goo_free_simd_buffer(src1, buffer_size);
    if (src2) goo_free_simd_buffer(src2, buffer_size);
    if (dst) goo_free_simd_buffer(dst, buffer_size);
    
    return verify_success;
}

// Entry point for testing the Zig integration from C
int goo_run_zig_tests(void) {
    if (!goo_initialize_zig_runtime()) {
        return EXIT_FAILURE;
    }
    
    bool test_result = goo_test_zig_integration();
    
    goo_cleanup_zig_runtime();
    
    return test_result ? EXIT_SUCCESS : EXIT_FAILURE;
} 