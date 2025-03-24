#include <stdio.h>
#include <stdlib.h>
#include "goo_memory.h"

/**
 * Minimal test file for Goo language
 * This file includes only the essential Goo headers
 * to test the basic memory functionality
 */
int main(int argc, char** argv) {
    printf("Goo Minimal Test Program\n");
    printf("Testing basic memory functionality...\n");
    
    // Test basic memory allocation
    void* mem = goo_runtime_alloc(1024);
    if (mem != NULL) {
        printf("Memory allocation successful: %p\n", mem);
        goo_runtime_free(mem);
        printf("Memory freed successfully\n");
    } else {
        printf("Memory allocation failed\n");
        return 1;
    }
    
    printf("Build system setup successful!\n");
    return 0;
} 