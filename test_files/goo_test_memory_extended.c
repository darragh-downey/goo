#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goo_memory.h"
#include "goo/runtime/memory.h"
#include "goo/core/types.h"

/**
 * Extended test file for Goo language memory management
 * This file tests both legacy and modern memory APIs
 */

// Custom out-of-memory handler
void out_of_memory_handler(void) {
    fprintf(stderr, "ERROR: Out of memory detected!\n");
}

// Function to test various memory operations
void test_memory_operations(void) {
    printf("Testing memory operations...\n");
    
    // Test basic allocation
    void* mem1 = goo_memory_alloc(1024);
    if (mem1 != NULL) {
        printf("Memory allocation successful: %p\n", mem1);
        
        // Test memory set
        goo_memory_set(mem1, 0xAA, 1024);
        unsigned char* mem_check = (unsigned char*)mem1;
        if (mem_check[0] == 0xAA && mem_check[512] == 0xAA && mem_check[1023] == 0xAA) {
            printf("Memory set operation successful\n");
        } else {
            printf("Memory set operation failed\n");
        }
        
        // Test memory copy
        void* mem2 = goo_memory_alloc(1024);
        if (mem2 != NULL) {
            goo_memory_copy(mem2, mem1, 1024);
            mem_check = (unsigned char*)mem2;
            if (mem_check[0] == 0xAA && mem_check[512] == 0xAA && mem_check[1023] == 0xAA) {
                printf("Memory copy operation successful\n");
            } else {
                printf("Memory copy operation failed\n");
            }
            
            goo_memory_free(mem2, 1024);
        }
        
        // Test reallocation
        void* mem3 = goo_memory_realloc(mem1, 1024, 2048);
        if (mem3 != NULL) {
            printf("Memory reallocation successful: %p\n", mem3);
            mem_check = (unsigned char*)mem3;
            if (mem_check[0] == 0xAA && mem_check[512] == 0xAA) {
                printf("Reallocation preserved memory contents\n");
            } else {
                printf("Reallocation failed to preserve memory contents\n");
            }
            
            goo_memory_free(mem3, 2048);
        } else {
            goo_memory_free(mem1, 1024);
            printf("Memory reallocation failed\n");
        }
    } else {
        printf("Memory allocation failed\n");
    }
}

// Function to test legacy memory API
void test_legacy_memory_api(void) {
    printf("Testing legacy memory API...\n");
    
    // Test basic allocation
    void* mem = goo_runtime_alloc(1024);
    if (mem != NULL) {
        printf("Legacy memory allocation successful: %p\n", mem);
        
        // Test reallocation
        void* mem_realloc = goo_runtime_realloc(mem, 2048);
        if (mem_realloc != NULL) {
            printf("Legacy memory reallocation successful: %p\n", mem_realloc);
            goo_runtime_free(mem_realloc);
        } else {
            goo_runtime_free(mem);
            printf("Legacy memory reallocation failed\n");
        }
    } else {
        printf("Legacy memory allocation failed\n");
    }
    
    // Test even older legacy aliases
    void* mem_old = goo_alloc(512);
    if (mem_old != NULL) {
        printf("Legacy alias allocation successful: %p\n", mem_old);
        
        void* mem_old_realloc = goo_realloc(mem_old, 1024);
        if (mem_old_realloc != NULL) {
            printf("Legacy alias reallocation successful: %p\n", mem_old_realloc);
            goo_free(mem_old_realloc);
        } else {
            goo_free(mem_old);
            printf("Legacy alias reallocation failed\n");
        }
    } else {
        printf("Legacy alias allocation failed\n");
    }
}

int main(int argc, char** argv) {
    printf("Goo Extended Memory Test Program\n");
    
    // Initialize memory subsystem
    if (!goo_memory_init()) {
        printf("Failed to initialize memory subsystem\n");
        return 1;
    }
    
    // Set out-of-memory handler
    goo_set_out_of_mem_handler(out_of_memory_handler);
    
    // Run memory tests
    test_memory_operations();
    test_legacy_memory_api();
    
    // Clean up memory subsystem
    goo_memory_cleanup();
    
    printf("Memory tests completed successfully!\n");
    return 0;
} 