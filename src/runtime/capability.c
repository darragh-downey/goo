#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include "../include/goo_runtime.h"

// Thread-local storage for capabilities
static pthread_key_t capability_key;
static pthread_once_t capability_key_once = PTHREAD_ONCE_INIT;

// Capability entry structure
typedef struct CapabilityEntry {
    int type;
    void* data;
    struct CapabilityEntry* next;
} CapabilityEntry;

// Capability set structure
struct GooCapabilitySet {
    CapabilityEntry* entries;
    pthread_mutex_t mutex;
};

// Initialize thread-local storage for capabilities
static void init_capability_key(void) {
    pthread_key_create(&capability_key, NULL);
}

// Create a new capability set
GooCapabilitySet* goo_capability_set_create(void) {
    GooCapabilitySet* caps = malloc(sizeof(GooCapabilitySet));
    if (!caps) return NULL;
    
    caps->entries = NULL;
    
    // Initialize the mutex
    if (pthread_mutex_init(&caps->mutex, NULL) != 0) {
        free(caps);
        return NULL;
    }
    
    return caps;
}

// Clone an existing capability set
GooCapabilitySet* goo_capability_set_clone(GooCapabilitySet* caps) {
    if (!caps) return NULL;
    
    GooCapabilitySet* new_caps = goo_capability_set_create();
    if (!new_caps) return NULL;
    
    pthread_mutex_lock(&caps->mutex);
    
    // Clone all entries
    CapabilityEntry* entry = caps->entries;
    while (entry) {
        // Clone the data if needed (implementation-dependent)
        void* cloned_data = entry->data;
        
        // Add to the new set
        goo_capability_add(new_caps, entry->type, cloned_data);
        
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&caps->mutex);
    
    return new_caps;
}

// Destroy a capability set
void goo_capability_set_destroy(GooCapabilitySet* caps) {
    if (!caps) return;
    
    pthread_mutex_lock(&caps->mutex);
    
    // Free all entries
    CapabilityEntry* entry = caps->entries;
    while (entry) {
        CapabilityEntry* next = entry->next;
        
        // Free the data if needed (implementation-dependent)
        // Note: We don't free data here as it might be shared
        
        free(entry);
        entry = next;
    }
    
    pthread_mutex_unlock(&caps->mutex);
    
    // Destroy the mutex
    pthread_mutex_destroy(&caps->mutex);
    
    free(caps);
}

// Add a capability to a set
bool goo_capability_add(GooCapabilitySet* caps, int capability_type, void* data) {
    if (!caps) return false;
    
    // Create a new entry
    CapabilityEntry* entry = malloc(sizeof(CapabilityEntry));
    if (!entry) return false;
    
    entry->type = capability_type;
    entry->data = data;
    
    pthread_mutex_lock(&caps->mutex);
    
    // Add to the front of the list
    entry->next = caps->entries;
    caps->entries = entry;
    
    pthread_mutex_unlock(&caps->mutex);
    
    return true;
}

// Remove a capability from a set
bool goo_capability_remove(GooCapabilitySet* caps, int capability_type) {
    if (!caps) return false;
    
    pthread_mutex_lock(&caps->mutex);
    
    CapabilityEntry* prev = NULL;
    CapabilityEntry* entry = caps->entries;
    
    while (entry) {
        if (entry->type == capability_type) {
            // Remove this entry
            if (prev) {
                prev->next = entry->next;
            } else {
                caps->entries = entry->next;
            }
            
            // Free the entry
            free(entry);
            
            pthread_mutex_unlock(&caps->mutex);
            return true;
        }
        
        prev = entry;
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&caps->mutex);
    
    // Capability not found
    return false;
}

// Check if a capability is in a set
bool goo_capability_check(GooCapabilitySet* caps, int capability_type) {
    if (!caps) return false;
    
    pthread_mutex_lock(&caps->mutex);
    
    CapabilityEntry* entry = caps->entries;
    while (entry) {
        if (entry->type == capability_type) {
            pthread_mutex_unlock(&caps->mutex);
            return true;
        }
        
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&caps->mutex);
    
    // Capability not found
    return false;
}

// Get capability data if present
void* goo_capability_get_data(GooCapabilitySet* caps, int capability_type) {
    if (!caps) return NULL;
    
    pthread_mutex_lock(&caps->mutex);
    
    CapabilityEntry* entry = caps->entries;
    while (entry) {
        if (entry->type == capability_type) {
            void* data = entry->data;
            pthread_mutex_unlock(&caps->mutex);
            return data;
        }
        
        entry = entry->next;
    }
    
    pthread_mutex_unlock(&caps->mutex);
    
    // Capability not found
    return NULL;
}

// Get the current capability set for this thread
GooCapabilitySet* goo_runtime_get_current_caps(void) {
    pthread_once(&capability_key_once, init_capability_key);
    return (GooCapabilitySet*)pthread_getspecific(capability_key);
}

// Set the current capability set for this thread
void goo_runtime_set_current_caps(GooCapabilitySet* caps) {
    pthread_once(&capability_key_once, init_capability_key);
    pthread_setspecific(capability_key, caps);
}

// Create a capability set with all available capabilities
GooCapabilitySet* goo_capability_set_create_all(void) {
    GooCapabilitySet* caps = goo_capability_set_create();
    if (!caps) return NULL;
    
    // Add all standard capabilities
    // Note: These capability types should be defined in a header file
    goo_capability_add(caps, 1, NULL); // File I/O
    goo_capability_add(caps, 2, NULL); // Network
    goo_capability_add(caps, 3, NULL); // Process
    goo_capability_add(caps, 4, NULL); // Memory
    
    return caps;
}

// Initialize the capability system
bool goo_capability_system_init(void) {
    // Create a default capability set with all capabilities
    GooCapabilitySet* default_caps = goo_capability_set_create_all();
    if (!default_caps) return false;
    
    // Set as the default for the main thread
    goo_runtime_set_current_caps(default_caps);
    
    return true;
}

// Shutdown the capability system
void goo_capability_system_shutdown(void) {
    // Free the current capability set
    GooCapabilitySet* caps = goo_runtime_get_current_caps();
    if (caps) {
        goo_capability_set_destroy(caps);
        goo_runtime_set_current_caps(NULL);
    }
} 