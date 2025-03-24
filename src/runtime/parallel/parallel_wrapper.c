/**
 * parallel_wrapper.c
 * 
 * C wrapper for the Zig implementation of parallel execution primitives.
 * This file provides the C API for thread pools and parallel algorithms.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../include/parallel/parallel.h"
#include "../../include/memory.h"

// Forward declarations for Zig functions
extern bool memoryInit(void);
extern void memoryCleanup(void);
extern void* threadPoolCreate(size_t num_threads);
extern void threadPoolDestroy(void* pool);
extern void* taskCreate(void (*execute_fn)(void* data), void* data);
extern void taskDestroy(void* task);
extern void taskExecute(void* task);
extern bool threadPoolSubmit(void* pool, void* task);
extern void threadPoolWaitAll(void* pool);
extern void* parallelForCreate(void* pool, size_t start, size_t end, size_t step, 
                             void (*fn_ptr)(size_t idx, void* data), void* data);
extern void parallelForDestroy(void* parallel_for);
extern bool parallelForExecute(void* parallel_for);
extern void* parallelReduceCreate(void* pool, size_t start, size_t end, 
                                void* identity_value, 
                                void* (*mapper_fn)(size_t idx, void* data), 
                                void* (*reducer_fn)(void* a, void* b), 
                                void* data);
extern void parallelReduceDestroy(void* parallel_reduce);
extern bool parallelReduceExecute(void* parallel_reduce, void** result);

// C API implementation

typedef struct GooThreadPool {
    void* zig_pool;
} GooThreadPool;

typedef struct GooTask {
    void* zig_task;
} GooTask;

typedef struct GooParallelFor {
    void* zig_parallel_for;
} GooParallelFor;

typedef struct GooParallelReduce {
    void* zig_parallel_reduce;
} GooParallelReduce;

/**
 * Initialize the parallel execution module.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_parallel_init(void) {
    return memoryInit();
}

/**
 * Clean up the parallel execution module.
 */
void goo_parallel_cleanup(void) {
    memoryCleanup();
}

/**
 * Create a new thread pool.
 * 
 * @param num_threads Number of threads in the pool
 * @return A new thread pool, or NULL if creation failed
 */
GooThreadPool* goo_thread_pool_create(size_t num_threads) {
    void* zig_pool = threadPoolCreate(num_threads);
    if (zig_pool == NULL) {
        return NULL;
    }
    
    GooThreadPool* pool = goo_alloc(sizeof(GooThreadPool));
    if (pool == NULL) {
        threadPoolDestroy(zig_pool);
        return NULL;
    }
    
    pool->zig_pool = zig_pool;
    return pool;
}

/**
 * Destroy a thread pool.
 * 
 * @param pool The thread pool to destroy
 */
void goo_thread_pool_destroy(GooThreadPool* pool) {
    if (pool == NULL) {
        return;
    }
    
    threadPoolDestroy(pool->zig_pool);
    goo_free(pool);
}

/**
 * Create a new task.
 * 
 * @param execute_fn Function to execute
 * @param data Data to pass to the function
 * @return A new task, or NULL if creation failed
 */
GooTask* goo_task_create(void (*execute_fn)(void* data), void* data) {
    void* zig_task = taskCreate(execute_fn, data);
    if (zig_task == NULL) {
        return NULL;
    }
    
    GooTask* task = goo_alloc(sizeof(GooTask));
    if (task == NULL) {
        taskDestroy(zig_task);
        return NULL;
    }
    
    task->zig_task = zig_task;
    return task;
}

/**
 * Destroy a task.
 * 
 * @param task The task to destroy
 */
void goo_task_destroy(GooTask* task) {
    if (task == NULL) {
        return;
    }
    
    taskDestroy(task->zig_task);
    goo_free(task);
}

/**
 * Execute a task.
 * 
 * @param task The task to execute
 */
void goo_task_execute(GooTask* task) {
    if (task == NULL) {
        return;
    }
    
    taskExecute(task->zig_task);
}

/**
 * Submit a task to a thread pool.
 * 
 * @param pool The thread pool
 * @param task The task to submit
 * @return true if the task was submitted successfully, false otherwise
 */
bool goo_thread_pool_submit(GooThreadPool* pool, GooTask* task) {
    if (pool == NULL || task == NULL) {
        return false;
    }
    
    return threadPoolSubmit(pool->zig_pool, task->zig_task);
}

/**
 * Wait for all tasks in a thread pool to complete.
 * 
 * @param pool The thread pool
 */
void goo_thread_pool_wait_all(GooThreadPool* pool) {
    if (pool == NULL) {
        return;
    }
    
    threadPoolWaitAll(pool->zig_pool);
}

/**
 * Create a parallel for loop.
 * 
 * @param pool The thread pool to use
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param step Step size
 * @param fn_ptr Function to execute for each index
 * @param data Data to pass to the function
 * @return A new parallel for loop, or NULL if creation failed
 */
GooParallelFor* goo_parallel_for_create(GooThreadPool* pool, size_t start, size_t end, size_t step, 
                                      void (*fn_ptr)(size_t idx, void* data), void* data) {
    if (pool == NULL) {
        return NULL;
    }
    
    void* zig_parallel_for = parallelForCreate(pool->zig_pool, start, end, step, fn_ptr, data);
    if (zig_parallel_for == NULL) {
        return NULL;
    }
    
    GooParallelFor* parallel_for = goo_alloc(sizeof(GooParallelFor));
    if (parallel_for == NULL) {
        parallelForDestroy(zig_parallel_for);
        return NULL;
    }
    
    parallel_for->zig_parallel_for = zig_parallel_for;
    return parallel_for;
}

/**
 * Destroy a parallel for loop.
 * 
 * @param parallel_for The parallel for loop to destroy
 */
void goo_parallel_for_destroy(GooParallelFor* parallel_for) {
    if (parallel_for == NULL) {
        return;
    }
    
    parallelForDestroy(parallel_for->zig_parallel_for);
    goo_free(parallel_for);
}

/**
 * Execute a parallel for loop.
 * 
 * @param parallel_for The parallel for loop to execute
 * @return true if execution was successful, false otherwise
 */
bool goo_parallel_for_execute(GooParallelFor* parallel_for) {
    if (parallel_for == NULL) {
        return false;
    }
    
    return parallelForExecute(parallel_for->zig_parallel_for);
}

/**
 * Create a parallel reduce operation.
 * 
 * @param pool The thread pool to use
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param identity_value Identity value for the reduction
 * @param mapper_fn Function to map each index to a value
 * @param reducer_fn Function to reduce two values
 * @param data Data to pass to the functions
 * @return A new parallel reduce operation, or NULL if creation failed
 */
GooParallelReduce* goo_parallel_reduce_create(GooThreadPool* pool, size_t start, size_t end, 
                                            void* identity_value, 
                                            void* (*mapper_fn)(size_t idx, void* data), 
                                            void* (*reducer_fn)(void* a, void* b), 
                                            void* data) {
    if (pool == NULL) {
        return NULL;
    }
    
    void* zig_parallel_reduce = parallelReduceCreate(pool->zig_pool, start, end, 
                                                  identity_value, mapper_fn, reducer_fn, data);
    if (zig_parallel_reduce == NULL) {
        return NULL;
    }
    
    GooParallelReduce* parallel_reduce = goo_alloc(sizeof(GooParallelReduce));
    if (parallel_reduce == NULL) {
        parallelReduceDestroy(zig_parallel_reduce);
        return NULL;
    }
    
    parallel_reduce->zig_parallel_reduce = zig_parallel_reduce;
    return parallel_reduce;
}

/**
 * Destroy a parallel reduce operation.
 * 
 * @param parallel_reduce The parallel reduce operation to destroy
 */
void goo_parallel_reduce_destroy(GooParallelReduce* parallel_reduce) {
    if (parallel_reduce == NULL) {
        return;
    }
    
    parallelReduceDestroy(parallel_reduce->zig_parallel_reduce);
    goo_free(parallel_reduce);
}

/**
 * Execute a parallel reduce operation.
 * 
 * @param parallel_reduce The parallel reduce operation to execute
 * @param result Pointer to store the result
 * @return true if execution was successful, false otherwise
 */
bool goo_parallel_reduce_execute(GooParallelReduce* parallel_reduce, void** result) {
    if (parallel_reduce == NULL || result == NULL) {
        return false;
    }
    
    return parallelReduceExecute(parallel_reduce->zig_parallel_reduce, result);
}

/**
 * Execute a parallel for loop with automatic thread pool creation and cleanup.
 * 
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param step Step size
 * @param fn_ptr Function to execute for each index
 * @param data Data to pass to the function
 * @param num_threads Number of threads to use (0 for default)
 * @return true if execution was successful, false otherwise
 */
bool goo_parallel_for(size_t start, size_t end, size_t step, void (*fn_ptr)(size_t idx, void* data), 
                    void* data, size_t num_threads) {
    // Use a reasonable default number of threads if not specified
    if (num_threads == 0) {
        // Simple heuristic: use number of logical processors or 4, whichever is greater
        num_threads = 4; // TODO: Get actual number of processors
    }
    
    // Create a thread pool
    GooThreadPool* pool = goo_thread_pool_create(num_threads);
    if (pool == NULL) {
        return false;
    }
    
    // Create a parallel for loop
    GooParallelFor* parallel_for = goo_parallel_for_create(pool, start, end, step, fn_ptr, data);
    if (parallel_for == NULL) {
        goo_thread_pool_destroy(pool);
        return false;
    }
    
    // Execute the parallel for loop
    bool success = goo_parallel_for_execute(parallel_for);
    
    // Clean up
    goo_parallel_for_destroy(parallel_for);
    goo_thread_pool_destroy(pool);
    
    return success;
}

/**
 * Execute a parallel reduce operation with automatic thread pool creation and cleanup.
 * 
 * @param start Start index (inclusive)
 * @param end End index (exclusive)
 * @param identity_value Identity value for the reduction
 * @param mapper_fn Function to map each index to a value
 * @param reducer_fn Function to reduce two values
 * @param data Data to pass to the functions
 * @param result Pointer to store the result
 * @param num_threads Number of threads to use (0 for default)
 * @return true if execution was successful, false otherwise
 */
bool goo_parallel_reduce(size_t start, size_t end, void* identity_value, 
                        void* (*mapper_fn)(size_t idx, void* data), 
                        void* (*reducer_fn)(void* a, void* b), 
                        void* data, void** result, size_t num_threads) {
    // Use a reasonable default number of threads if not specified
    if (num_threads == 0) {
        // Simple heuristic: use number of logical processors or 4, whichever is greater
        num_threads = 4; // TODO: Get actual number of processors
    }
    
    // Create a thread pool
    GooThreadPool* pool = goo_thread_pool_create(num_threads);
    if (pool == NULL) {
        return false;
    }
    
    // Create a parallel reduce operation
    GooParallelReduce* parallel_reduce = goo_parallel_reduce_create(pool, start, end, 
                                                                 identity_value, mapper_fn, reducer_fn, data);
    if (parallel_reduce == NULL) {
        goo_thread_pool_destroy(pool);
        return false;
    }
    
    // Execute the parallel reduce operation
    bool success = goo_parallel_reduce_execute(parallel_reduce, result);
    
    // Clean up
    goo_parallel_reduce_destroy(parallel_reduce);
    goo_thread_pool_destroy(pool);
    
    return success;
} 