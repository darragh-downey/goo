/**
 * goo_work_distribution.h
 * 
 * Header for advanced work distribution algorithms for Goo's parallel execution system.
 * Provides APIs for work stealing and guided scheduling to optimize parallel workloads.
 */

#ifndef GOO_WORK_DISTRIBUTION_H
#define GOO_WORK_DISTRIBUTION_H

/**
 * Goo Language Work Distribution System
 * Provides task scheduling and distribution for parallel processing
 */

#include <stddef.h>
#include <stdbool.h>
#include "../../../include/goo_core.h"
#include "../../../include/goo_concurrency.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations for work distribution types
typedef struct GooWorkerPool GooWorkerPool;
typedef struct GooTask GooTask;
typedef struct GooWorkItem GooWorkItem;

// Worker pool options
typedef struct {
    unsigned int num_workers;     // Number of worker threads (0 = auto)
    bool dynamic_scaling;         // Whether to dynamically scale worker count
    unsigned int queue_size;      // Size of work queue (0 = unlimited)
    GooSchedulingPolicy policy;   // Scheduling policy
} GooWorkerPoolOptions;

// Function pointer typedefs
typedef void (*GooTaskFunc)(void* data, size_t size, void* result);
typedef void (*GooTaskCompleteCallback)(void* result, void* user_data);

/**
 * Initialize the work distribution system
 */
bool goo_work_distribution_init(void);

/**
 * Clean up the work distribution system
 */
void goo_work_distribution_cleanup(void);

/**
 * Create a worker pool
 */
GooWorkerPool* goo_worker_pool_create(const GooWorkerPoolOptions* options);

/**
 * Destroy a worker pool
 */
void goo_worker_pool_destroy(GooWorkerPool* pool);

/**
 * Create a new task
 */
GooTask* goo_task_create(GooTaskFunc func, void* data, size_t data_size, GooTaskCompleteCallback callback, void* user_data);

/**
 * Destroy a task
 */
void goo_task_destroy(GooTask* task);

/**
 * Submit a task to a worker pool
 */
bool goo_worker_pool_submit(GooWorkerPool* pool, GooTask* task);

/**
 * Wait for all tasks in a worker pool to complete
 */
bool goo_worker_pool_wait_all(GooWorkerPool* pool, int timeout_ms);

/**
 * Get number of workers in a pool
 */
unsigned int goo_worker_pool_get_size(const GooWorkerPool* pool);

/**
 * Set number of workers in a pool
 */
bool goo_worker_pool_set_size(GooWorkerPool* pool, unsigned int num_workers);

/**
 * Get number of queued tasks in a pool
 */
unsigned int goo_worker_pool_get_queue_size(const GooWorkerPool* pool);

/**
 * Pause workers in a pool
 */
bool goo_worker_pool_pause(GooWorkerPool* pool);

/**
 * Resume workers in a pool
 */
bool goo_worker_pool_resume(GooWorkerPool* pool);

#ifdef __cplusplus
}
#endif

#endif /* GOO_WORK_DISTRIBUTION_H */ 