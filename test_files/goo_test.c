#include <stdio.h>
#include "goo/runtime/memory.h"
#include "goo/core/types.h"

int main() {
    // Initialize the Goo runtime
    if (!goo_memory_init()) {
        fprintf(stderr, "Failed to initialize memory subsystem\n");
        return 1;
    }

    // Test memory allocation
    const size_t msg_size = 100;
    char* msg = (char*)goo_memory_alloc(msg_size);
    if (msg == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        goo_memory_cleanup();
        return 1;
    }

    // Use the allocated memory
    sprintf(msg, "Hello, Goo!");
    printf("Test message: %s\n", msg);

    // Test zero-initialized memory
    char* zeroed = (char*)goo_memory_alloc_zeroed(msg_size);
    if (zeroed == NULL) {
        fprintf(stderr, "Zeroed memory allocation failed\n");
        goo_memory_free(msg, msg_size);
        goo_memory_cleanup();
        return 1;
    }
    
    // Check that memory is zeroed
    for (size_t i = 0; i < msg_size; i++) {
        if (zeroed[i] != 0) {
            fprintf(stderr, "Zeroed memory check failed at position %zu\n", i);
            goo_memory_free(msg, msg_size);
            goo_memory_free(zeroed, msg_size);
            goo_memory_cleanup();
            return 1;
        }
    }
    printf("Zeroed memory check passed\n");
    
    // Test memory reallocation
    char* bigger = (char*)goo_memory_realloc(msg, msg_size, msg_size * 2);
    if (bigger == NULL) {
        fprintf(stderr, "Memory reallocation failed\n");
        goo_memory_free(msg, msg_size);
        goo_memory_free(zeroed, msg_size);
        goo_memory_cleanup();
        return 1;
    }
    
    // Check that content is preserved
    printf("Reallocated message: %s\n", bigger);
    
    // Clean up
    goo_memory_free(bigger, msg_size * 2);
    goo_memory_free(zeroed, msg_size);
    goo_memory_cleanup();
    
    printf("Memory test successful\n");
    return 0;
} 