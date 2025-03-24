#ifndef GOO_MEMORY_STATS_H
#define GOO_MEMORY_STATS_H

/**
 * Memory Statistics Tracking for Goo
 * Internal header for memory statistics functionality
 */

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize memory statistics tracking
 * 
 * @return true if initialization was successful
 */
bool goo_memory_stats_init(void);

/**
 * Clean up memory statistics tracking
 */
void goo_memory_stats_cleanup(void);

/**
 * Track memory allocation
 * 
 * @param size Size of allocation in bytes
 */
void goo_memory_stats_track_alloc(size_t size);

/**
 * Track memory deallocation
 * 
 * @param size Size of allocation in bytes being freed
 */
void goo_memory_stats_track_free(size_t size);

/**
 * Track memory reallocation
 * 
 * @param old_size Original size in bytes
 * @param new_size New size in bytes
 */
void goo_memory_stats_track_realloc(size_t old_size, size_t new_size);

// Public API is exposed through goo_memory.h

#ifdef __cplusplus
}
#endif

#endif // GOO_MEMORY_STATS_H 