/**
 * runtime.c
 * 
 * Main runtime initialization and cleanup for the Goo programming language.
 * This file coordinates the initialization of all runtime subsystems.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "../include/goo_runtime.h"
#include "../include/goo_memory.h"
#include "../include/goo_capability.h"
#include "../include/goo_error.h"
#include "../include/goo_integration.h"
#include "memory/memory.h"
#include "scope/scope.h"
#include "runtime.h"

// Track initialization state
static bool runtime_initialized = false;

// Runtime state structure
static struct {
    bool initialized;        // Whether runtime is initialized
    int log_level;           // Log level (0 for none, higher for more verbose)
    pthread_mutex_t mutex;   // Mutex for thread safety
    bool shutting_down;      // Whether runtime is shutting down
    
    // Thread pool state (to be implemented)
    int thread_pool_size;
    void* thread_pool;
    
    // Capability system state
    void* capability_system;
    
    // Thread-local storage for memory allocators
    pthread_key_t allocator_key;
} runtime_state = {
    .initialized = false,
    .log_level = 0,
    .shutting_down = false,
    .thread_pool_size = 0,
    .thread_pool = NULL,
    .capability_system = NULL
};

// Forward declarations for internal functions
extern bool goo_thread_pool_init(int thread_count);
extern void goo_thread_pool_cleanup(void);
extern bool goo_capability_system_init(void);
extern void goo_capability_system_shutdown(void);

// Signal handler for graceful shutdown
static void runtime_signal_handler(int sig) {
    if (runtime_state.log_level > 0) {
        fprintf(stderr, "Goo runtime received signal %d, initiating shutdown...\n", sig);
    }
    
    // Mark as shutting down
    runtime_state.shutting_down = true;
    
    // If it's a termination signal and we're not shutting down properly, force exit
    if ((sig == SIGINT || sig == SIGTERM) && !runtime_state.initialized) {
        exit(1);
    }
}

/**
 * Initialize the Goo runtime.
 * This initializes all runtime subsystems in the correct order.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_runtime_init(void) {
    if (runtime_initialized) {
        return true;
    }
    
    // Initialize memory subsystem first
    if (!goo_memory_init()) {
        fprintf(stderr, "Failed to initialize memory subsystem\n");
        return false;
    }
    
    // Initialize scope system
    if (!goo_scope_init()) {
        fprintf(stderr, "Failed to initialize scope subsystem\n");
        goo_memory_cleanup();
        return false;
    }
    
    // Initialize any other subsystems here
    
    runtime_initialized = true;
    return true;
}

/**
 * Clean up the Goo runtime.
 * This cleans up all runtime subsystems in the reverse order of initialization.
 */
void goo_runtime_cleanup(void) {
    if (!runtime_initialized) {
        return;
    }
    
    // Clean up other subsystems here
    
    // Clean up scope system
    goo_scope_cleanup();
    
    // Clean up memory subsystem last
    goo_memory_cleanup();
    
    runtime_initialized = false;
}

/**
 * Check if the runtime is initialized.
 * 
 * @return true if the runtime is initialized, false otherwise
 */
bool goo_runtime_is_initialized(void) {
    return runtime_initialized;
}

/**
 * Report a runtime error and abort the program.
 * 
 * @param message The error message to display
 */
void goo_runtime_error(const char* message) {
    fprintf(stderr, "Runtime error: %s\n", message ? message : "Unknown error");
    abort();
}

/**
 * Handle out of memory condition.
 * This is called when memory allocation fails.
 * 
 * @param size The size that failed to allocate
 */
void goo_runtime_out_of_memory(size_t size) {
    fprintf(stderr, "Runtime error: Out of memory (requested %zu bytes)\n", size);
    abort();
}

// Shutdown runtime
void goo_runtime_shutdown(void) {
    // Check if initialized
    if (!runtime_state.initialized) {
        return;
    }
    
    // Mark as shutting down
    runtime_state.shutting_down = true;
    
    // Lock mutex
    pthread_mutex_lock(&runtime_state.mutex);
    
    if (runtime_state.log_level > 0) {
        printf("Shutting down Goo runtime...\n");
    }
    
    // Shutdown thread pool (to be implemented)
    // TODO: Implement thread pool shutdown
    
    // Clean up thread-local storage
    pthread_key_delete(runtime_state.allocator_key);
    
    // Shutdown integration layer (which will shut down capability system and other subsystems)
    goo_runtime_integration_shutdown();
    
    // Mark as not initialized
    runtime_state.initialized = false;
    
    // Unlock and destroy mutex
    pthread_mutex_unlock(&runtime_state.mutex);
    pthread_mutex_destroy(&runtime_state.mutex);
    
    if (runtime_state.log_level > 0) {
        printf("Goo runtime shutdown complete\n");
    }
}

// Check if runtime is shutting down
bool goo_runtime_is_shutting_down(void) {
    return runtime_state.shutting_down;
}

// Runtime panic function (wrapper around error handling)
void goo_runtime_panic(const char* message) {
    // Just delegate to error subsystem
    if (message) {
        fprintf(stderr, "RUNTIME PANIC: %s\n", message);
    } else {
        fprintf(stderr, "RUNTIME PANIC: <no message>\n");
    }
    
    // If we're shutting down, just exit
    if (runtime_state.shutting_down) {
        exit(1);
    }
    
    // Otherwise, try to panic properly
    goo_panic(NULL, message);
    
    // If we get here, there was no recovery point
    abort();
}

// Set current thread's allocator
void goo_runtime_set_thread_allocator(GooCustomAllocator* allocator) {
    if (!runtime_state.initialized) {
        return;
    }
    
    pthread_setspecific(runtime_state.allocator_key, allocator);
}

// Get current thread's allocator
GooCustomAllocator* goo_runtime_get_thread_allocator(void) {
    if (!runtime_state.initialized) {
        return NULL;
    }
    
    return (GooCustomAllocator*)pthread_getspecific(runtime_state.allocator_key);
}

// Perform a runtime allocation with the current thread allocator
void* goo_runtime_alloc(size_t size) {
    if (!runtime_state.initialized) {
        return malloc(size);  // Fallback to standard malloc
    }
    
    // Get current thread's allocator
    GooCustomAllocator* allocator = goo_runtime_get_thread_allocator();
    if (allocator) {
        return goo_custom_alloc(allocator, size, 8);  // Default 8-byte alignment
    }
    
    // Fall back to typed allocation with no type info
    return goo_runtime_typed_alloc(size, NULL);
}

// Perform a runtime free with the current thread allocator
void goo_runtime_free(void* ptr) {
    if (!runtime_state.initialized || !ptr) {
        free(ptr);  // Fallback to standard free
        return;
    }
    
    // Get current thread's allocator
    GooCustomAllocator* allocator = goo_runtime_get_thread_allocator();
    if (allocator) {
        goo_custom_free(allocator, ptr);
        return;
    }
    
    // Fall back to typed free with no type info
    // Note: This may not actually free arena allocations
    goo_runtime_typed_free(ptr, NULL, 0);
} 