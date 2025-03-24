#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#include "goo_memory.h"

// Memory statistics for tracking allocations
static struct {
    size_t total_allocated;    // Current total bytes allocated
    size_t peak_allocated;     // Peak allocation in bytes
    size_t allocation_count;   // Number of active allocations
    bool tracking_enabled;     // Whether tracking is enabled
    pthread_mutex_t lock;      // Lock for thread-safe stats updates
} memory_stats = {0, 0, 0, true, PTHREAD_MUTEX_INITIALIZER};

// Initialize memory statistics tracking
bool goo_memory_stats_init(void) {
    memset(&memory_stats, 0, sizeof(memory_stats));
    memory_stats.tracking_enabled = true;
    
    // Initialize mutex with default attributes
    pthread_mutexattr_t attr;
    if (pthread_mutexattr_init(&attr) != 0) {
        return false;
    }
    
    if (pthread_mutex_init(&memory_stats.lock, &attr) != 0) {
        pthread_mutexattr_destroy(&attr);
        return false;
    }
    
    pthread_mutexattr_destroy(&attr);
    return true;
}

// Clean up memory statistics tracking
void goo_memory_stats_cleanup(void) {
    pthread_mutex_destroy(&memory_stats.lock);
}

// Track memory allocation
void goo_memory_stats_track_alloc(size_t size) {
    if (!memory_stats.tracking_enabled) {
        return;
    }
    
    pthread_mutex_lock(&memory_stats.lock);
    
    memory_stats.total_allocated += size;
    memory_stats.allocation_count++;
    
    if (memory_stats.total_allocated > memory_stats.peak_allocated) {
        memory_stats.peak_allocated = memory_stats.total_allocated;
    }
    
    pthread_mutex_unlock(&memory_stats.lock);
}

// Track memory deallocation
void goo_memory_stats_track_free(size_t size) {
    if (!memory_stats.tracking_enabled) {
        return;
    }
    
    pthread_mutex_lock(&memory_stats.lock);
    
    if (memory_stats.total_allocated >= size) {
        memory_stats.total_allocated -= size;
    } else {
        // Should never happen in correct code, but guard against underflow
        memory_stats.total_allocated = 0;
    }
    
    if (memory_stats.allocation_count > 0) {
        memory_stats.allocation_count--;
    }
    
    pthread_mutex_unlock(&memory_stats.lock);
}

// Get current memory statistics
bool goo_memory_get_stats(size_t* total_allocated, size_t* peak_allocated, size_t* allocation_count) {
    if (!memory_stats.tracking_enabled) {
        return false;
    }
    
    pthread_mutex_lock(&memory_stats.lock);
    
    if (total_allocated) {
        *total_allocated = memory_stats.total_allocated;
    }
    
    if (peak_allocated) {
        *peak_allocated = memory_stats.peak_allocated;
    }
    
    if (allocation_count) {
        *allocation_count = memory_stats.allocation_count;
    }
    
    pthread_mutex_unlock(&memory_stats.lock);
    return true;
}

// Reset memory statistics
bool goo_memory_reset_stats(void) {
    if (!memory_stats.tracking_enabled) {
        return false;
    }
    
    pthread_mutex_lock(&memory_stats.lock);
    
    memory_stats.total_allocated = 0;
    memory_stats.peak_allocated = 0;
    memory_stats.allocation_count = 0;
    
    pthread_mutex_unlock(&memory_stats.lock);
    return true;
}

// Enable or disable memory tracking
bool goo_memory_set_tracking(bool enable) {
    pthread_mutex_lock(&memory_stats.lock);
    bool previous = memory_stats.tracking_enabled;
    memory_stats.tracking_enabled = enable;
    pthread_mutex_unlock(&memory_stats.lock);
    
    return previous;
}

// Track memory reallocation
void goo_memory_stats_track_realloc(size_t old_size, size_t new_size) {
    if (!memory_stats.tracking_enabled) {
        return;
    }
    
    if (old_size == new_size) {
        return; // No change in allocation size
    }
    
    pthread_mutex_lock(&memory_stats.lock);
    
    if (new_size > old_size) {
        // Growing allocation
        size_t growth = new_size - old_size;
        memory_stats.total_allocated += growth;
        
        if (memory_stats.total_allocated > memory_stats.peak_allocated) {
            memory_stats.peak_allocated = memory_stats.total_allocated;
        }
    } else {
        // Shrinking allocation
        size_t reduction = old_size - new_size;
        
        if (memory_stats.total_allocated >= reduction) {
            memory_stats.total_allocated -= reduction;
        } else {
            // Should never happen in correct code, but guard against underflow
            memory_stats.total_allocated = 0;
        }
    }
    
    pthread_mutex_unlock(&memory_stats.lock);
} 