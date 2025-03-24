#ifndef GOO_PARALLEL_H
#define GOO_PARALLEL_H

/**
 * Goo Language Parallel Execution Support
 * Provides parallel execution primitives for the Goo language
 */

#include <stddef.h>
#include <stdbool.h>
#include "../goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Scheduling policies for parallel execution
typedef enum {
    GOO_SCHEDULE_STATIC,     // Static workload division
    GOO_SCHEDULE_DYNAMIC,    // Dynamic workload division
    GOO_SCHEDULE_GUIDED,     // Guided auto workload division
    GOO_SCHEDULE_AUTO,       // Automatic selection based on workload
    GOO_SCHEDULE_RUNTIME     // Determined at runtime
} GooSchedulingPolicy;

// Parallel for-loop configuration
typedef struct {
    int num_threads;           // Number of threads to use (0 = auto)
    GooSchedulingPolicy schedule;  // Scheduling policy
    int chunk_size;            // Chunk size for workload division (0 = auto)
    bool ordered;              // Whether execution must preserve ordering
    bool nowait;               // Whether to skip implicit barrier at end
} GooParallelForConfig;

// Parallel section configuration
typedef struct {
    int num_threads;           // Number of threads to use (0 = auto)
    bool is_master;            // Whether only the master thread executes
    bool is_single;            // Whether only one thread executes
} GooParallelSectionConfig;

// Function pointers for parallel work
typedef void (*GooParallelWorkFn)(int thread_id, int start, int end, void* data);
typedef void (*GooParallelTaskFn)(void* data);

/**
 * Initialize the parallel execution system
 */
bool goo_parallel_init(void);

/**
 * Clean up the parallel execution system
 */
void goo_parallel_cleanup(void);

/**
 * Set the default number of threads for parallel sections
 */
void goo_parallel_set_num_threads(int num_threads);

/**
 * Get the default number of threads for parallel sections
 */
int goo_parallel_get_num_threads(void);

/**
 * Get the maximum number of threads available
 */
int goo_parallel_get_max_threads(void);

/**
 * Get the current thread ID within a parallel section
 */
int goo_parallel_get_thread_id(void);

/**
 * Execute a loop in parallel
 */
bool goo_parallel_for(int start, int end, int step, 
                     GooParallelWorkFn work_fn, 
                     void* data,
                     const GooParallelForConfig* config);

/**
 * Execute a section in parallel
 */
bool goo_parallel_section(GooParallelTaskFn* tasks, 
                         int num_tasks,
                         const GooParallelSectionConfig* config);

/**
 * Create a parallel barrier
 */
bool goo_parallel_barrier(void);

/**
 * Execute a critical section (mutex-protected)
 */
bool goo_parallel_critical(GooParallelTaskFn task, void* data);

/**
 * Check if currently executing in a parallel section
 */
bool goo_parallel_in_parallel(void);

/**
 * Get the current nesting level of parallel sections
 */
int goo_parallel_get_level(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_PARALLEL_H 