/**
 * @file parallel.h
 * @brief Parallel execution primitives for the Goo programming language.
 *
 * This header provides functions for multithreaded execution, including thread
 * pools, tasks, parallel for loops, and parallel reduce operations.
 */

#ifndef GOO_PARALLEL_H
#define GOO_PARALLEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a thread pool.
 */
typedef struct GooThreadPool GooThreadPool;

/**
 * @brief Opaque type representing a task.
 */
typedef struct GooTask GooTask;

/**
 * @brief Opaque type representing a parallel for loop.
 */
typedef struct GooParallelFor GooParallelFor;

/**
 * @brief Opaque type representing a parallel reduce operation.
 */
typedef struct GooParallelReduce GooParallelReduce;

/**
 * @brief Function type for task execution.
 */
typedef void (*GooTaskFunc)(void* arg);

/**
 * @brief Function type for parallel for loop body.
 * @param index The current index.
 * @param context User-provided context.
 */
typedef void (*GooParallelForFunc)(size_t index, void* context);

/**
 * @brief Function type for parallel reduce operations.
 * @param a First value.
 * @param b Second value.
 * @param context User-provided context.
 * @return The result of reducing a and b.
 */
typedef void* (*GooParallelReduceFunc)(void* a, void* b, void* context);

/**
 * @brief Initialize the parallel execution subsystem.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_parallel_init(void);

/**
 * @brief Clean up the parallel execution subsystem.
 */
void goo_parallel_cleanup(void);

/**
 * @brief Create a new thread pool.
 *
 * @param num_threads The number of threads in the pool. If 0, uses the number of available CPU cores.
 * @return A pointer to the new thread pool, or NULL if creation fails.
 */
GooThreadPool* goo_thread_pool_create(size_t num_threads);

/**
 * @brief Destroy a thread pool.
 *
 * @param pool The thread pool to destroy.
 */
void goo_thread_pool_destroy(GooThreadPool* pool);

/**
 * @brief Get the number of threads in a thread pool.
 *
 * @param pool The thread pool.
 * @return The number of threads in the pool.
 */
size_t goo_thread_pool_get_num_threads(GooThreadPool* pool);

/**
 * @brief Create a new task.
 *
 * @param pool The thread pool to use.
 * @param func The function to execute.
 * @param arg The argument to pass to the function.
 * @return A pointer to the new task, or NULL if creation fails.
 */
GooTask* goo_task_create(GooThreadPool* pool, GooTaskFunc func, void* arg);

/**
 * @brief Destroy a task.
 *
 * @param task The task to destroy.
 */
void goo_task_destroy(GooTask* task);

/**
 * @brief Execute a task asynchronously.
 *
 * @param task The task to execute.
 * @return true if execution started successfully, false otherwise.
 */
bool goo_task_execute(GooTask* task);

/**
 * @brief Wait for a task to complete.
 *
 * @param task The task to wait for.
 * @return true if the task completed successfully, false otherwise.
 */
bool goo_task_wait(GooTask* task);

/**
 * @brief Check if a task has completed.
 *
 * @param task The task to check.
 * @return true if the task has completed, false otherwise.
 */
bool goo_task_is_done(GooTask* task);

/**
 * @brief Create a new parallel for loop.
 *
 * @param pool The thread pool to use.
 * @param start The start index (inclusive).
 * @param end The end index (exclusive).
 * @param func The function to execute for each index.
 * @param context User-provided context passed to func.
 * @return A pointer to the new parallel for loop, or NULL if creation fails.
 */
GooParallelFor* goo_parallel_for_create(GooThreadPool* pool, size_t start, size_t end, GooParallelForFunc func, void* context);

/**
 * @brief Destroy a parallel for loop.
 *
 * @param parallel_for The parallel for loop to destroy.
 */
void goo_parallel_for_destroy(GooParallelFor* parallel_for);

/**
 * @brief Execute a parallel for loop.
 *
 * @param parallel_for The parallel for loop to execute.
 * @return true if execution succeeds, false otherwise.
 */
bool goo_parallel_for_execute(GooParallelFor* parallel_for);

/**
 * @brief Wait for a parallel for loop to complete.
 *
 * @param parallel_for The parallel for loop to wait for.
 * @return true if the loop completed successfully, false otherwise.
 */
bool goo_parallel_for_wait(GooParallelFor* parallel_for);

/**
 * @brief Create a new parallel reduce operation.
 *
 * @param pool The thread pool to use.
 * @param start The start index (inclusive).
 * @param end The end index (exclusive).
 * @param init The initial value for reduction.
 * @param map_func The function to execute for each index to produce a value.
 * @param reduce_func The function to reduce two values into one.
 * @param context User-provided context passed to both functions.
 * @return A pointer to the new parallel reduce operation, or NULL if creation fails.
 */
GooParallelReduce* goo_parallel_reduce_create(
    GooThreadPool* pool,
    size_t start,
    size_t end,
    void* init,
    GooParallelForFunc map_func,
    GooParallelReduceFunc reduce_func,
    void* context
);

/**
 * @brief Destroy a parallel reduce operation.
 *
 * @param parallel_reduce The parallel reduce operation to destroy.
 */
void goo_parallel_reduce_destroy(GooParallelReduce* parallel_reduce);

/**
 * @brief Execute a parallel reduce operation.
 *
 * @param parallel_reduce The parallel reduce operation to execute.
 * @return true if execution succeeds, false otherwise.
 */
bool goo_parallel_reduce_execute(GooParallelReduce* parallel_reduce);

/**
 * @brief Wait for a parallel reduce operation to complete.
 *
 * @param parallel_reduce The parallel reduce operation to wait for.
 * @param result Pointer to store the result of the reduction.
 * @return true if the operation completed successfully, false otherwise.
 */
bool goo_parallel_reduce_wait(GooParallelReduce* parallel_reduce, void** result);

/**
 * @brief Get the global thread pool.
 *
 * @return A pointer to the global thread pool.
 */
GooThreadPool* goo_get_global_thread_pool(void);

/**
 * @brief Convenience function to execute a parallel for loop.
 *
 * @param start The start index (inclusive).
 * @param end The end index (exclusive).
 * @param func The function to execute for each index.
 * @param context User-provided context passed to func.
 * @return true if execution succeeds, false otherwise.
 */
bool goo_parallel_for(size_t start, size_t end, GooParallelForFunc func, void* context);

/**
 * @brief Convenience function to execute a parallel reduce operation.
 *
 * @param start The start index (inclusive).
 * @param end The end index (exclusive).
 * @param init The initial value for reduction.
 * @param map_func The function to execute for each index to produce a value.
 * @param reduce_func The function to reduce two values into one.
 * @param context User-provided context passed to both functions.
 * @param result Pointer to store the result of the reduction.
 * @return true if execution succeeds, false otherwise.
 */
bool goo_parallel_reduce(
    size_t start,
    size_t end,
    void* init,
    GooParallelForFunc map_func,
    GooParallelReduceFunc reduce_func,
    void* context,
    void** result
);

#ifdef __cplusplus
}
#endif

#endif /* GOO_PARALLEL_H */ 