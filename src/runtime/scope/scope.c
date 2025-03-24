/**
 * scope.c
 * 
 * Implementation of the scope-based resource management system for Goo.
 * This provides automatic cleanup of resources when execution exits a scope.
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "memory/memory.h"
#include "scope/scope.h"

// Maximum number of scopes that can be nested
#define MAX_SCOPE_DEPTH 128

// Cleanup entry structure
typedef struct GooCleanupEntry {
    void* data;                  // User data for cleanup
    void (*cleanup_fn)(void*);   // Cleanup function
    struct GooCleanupEntry* next; // Next entry in the list
} GooCleanupEntry;

// Scope structure
typedef struct GooScope {
    GooCleanupEntry* cleanup_list; // List of cleanup entries
    struct GooScope* parent;       // Parent scope
} GooScope;

// Thread-local current scope
static pthread_key_t current_scope_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;
static bool system_initialized = false;

// Forward declarations
static void create_tls_key(void);
static void destroy_tls_data(void* data);
static GooScope* get_current_scope(void);

/**
 * Initialize the scope system.
 */
bool goo_scope_init(void) {
    if (system_initialized) return true;
    
    // Initialize the thread-local storage key
    pthread_once(&key_once, create_tls_key);
    
    system_initialized = true;
    return true;
}

/**
 * Clean up the scope system.
 */
void goo_scope_cleanup(void) {
    if (!system_initialized) return;
    
    // Clean up the current scope
    GooScope* scope = get_current_scope();
    if (scope) {
        goo_scope_exit();
    }
    
    system_initialized = false;
}

/**
 * Enter a new scope.
 */
bool goo_scope_enter(void) {
    if (!system_initialized) {
        if (!goo_scope_init()) {
            return false;
        }
    }
    
    // Get the current scope
    GooScope* current = get_current_scope();
    
    // Create a new scope
    GooScope* scope = (GooScope*)malloc(sizeof(GooScope));
    if (!scope) return false;
    
    // Initialize the new scope
    scope->cleanup_list = NULL;
    scope->parent = current;
    
    // Set as current scope
    pthread_setspecific(current_scope_key, scope);
    
    return true;
}

/**
 * Exit the current scope and clean up resources.
 */
void goo_scope_exit(void) {
    // Get the current scope
    GooScope* scope = get_current_scope();
    if (!scope) return;
    
    // Set the parent as current
    pthread_setspecific(current_scope_key, scope->parent);
    
    // Clean up resources in reverse order of registration
    GooCleanupEntry* entry = scope->cleanup_list;
    while (entry) {
        GooCleanupEntry* next = entry->next;
        
        // Call the cleanup function
        if (entry->cleanup_fn && entry->data) {
            entry->cleanup_fn(entry->data);
        }
        
        // Free the entry
        free(entry);
        
        entry = next;
    }
    
    // Free the scope itself
    free(scope);
}

/**
 * Register a cleanup function for the current scope.
 */
bool goo_scope_register_cleanup(void* data, void (*cleanup_fn)(void*)) {
    if (!cleanup_fn) return false;
    
    // Get the current scope
    GooScope* scope = get_current_scope();
    if (!scope) {
        // No scope active, create one
        if (!goo_scope_enter()) {
            return false;
        }
        scope = get_current_scope();
    }
    
    // Create a new cleanup entry
    GooCleanupEntry* entry = (GooCleanupEntry*)malloc(sizeof(GooCleanupEntry));
    if (!entry) return false;
    
    // Initialize the entry
    entry->data = data;
    entry->cleanup_fn = cleanup_fn;
    
    // Add to the front of the list (will be cleaned up in reverse order)
    entry->next = scope->cleanup_list;
    scope->cleanup_list = entry;
    
    return true;
}

/**
 * Create the thread-local storage key.
 */
static void create_tls_key(void) {
    pthread_key_create(&current_scope_key, destroy_tls_data);
}

/**
 * Destructor for thread-local storage data.
 */
static void destroy_tls_data(void* data) {
    if (!data) return;
    
    // Clean up all scopes in the thread
    GooScope* scope = (GooScope*)data;
    while (scope) {
        GooScope* parent = scope->parent;
        
        // Clean up resources in the scope
        GooCleanupEntry* entry = scope->cleanup_list;
        while (entry) {
            GooCleanupEntry* next = entry->next;
            
            // Call the cleanup function
            if (entry->cleanup_fn && entry->data) {
                entry->cleanup_fn(entry->data);
            }
            
            // Free the entry
            free(entry);
            
            entry = next;
        }
        
        // Free the scope
        free(scope);
        
        scope = parent;
    }
}

/**
 * Get the current scope.
 */
static GooScope* get_current_scope(void) {
    if (!system_initialized) return NULL;
    return (GooScope*)pthread_getspecific(current_scope_key);
} 