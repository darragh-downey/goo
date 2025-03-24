#include "goo_parallel.h"
#include "goo_work_distribution.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

// Thread pool implementation
typedef struct GooThreadPoolTask {
    void (*function)(uint64_t, void*);  // Task function
    void *context;                      // Task context
    uint64_t start;                     // Loop start (for parallel loops)
    uint64_t end;                       // Loop end (for parallel loops)
    uint64_t step;                      // Loop step (for parallel loops)
    int priority;                       // Task priority
    struct GooThreadPoolTask *next;     // Next task in queue
} GooThreadPoolTask;

typedef struct GooThreadPool {
    pthread_t *threads;                 // Array of thread handles
    int num_threads;                    // Number of threads in pool
    GooThreadPoolTask *task_queue;      // Task queue head
    GooThreadPoolTask *task_queue_tail; // Task queue tail
    pthread_mutex_t queue_mutex;        // Mutex for task queue
    pthread_cond_t queue_cond;          // Condition variable for task queue
    pthread_cond_t complete_cond;       // Condition variable for task completion
    int tasks_count;                    // Number of tasks in queue
    int working_count;                  // Number of threads working
    bool shutdown;                      // Shutdown flag
} GooThreadPool;

// Thread local storage for thread ID
static pthread_key_t thread_id_key;
static pthread_once_t thread_id_once = PTHREAD_ONCE_INIT;

// Thread pool management
static GooThreadPool *global_thread_pool = NULL;
static pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrier_cond = PTHREAD_COND_INITIALIZER;
static int barrier_count = 0;
static int barrier_total = 0;

// Initialize the thread ID key with robust error handling
static void init_thread_id_key(void) {
    int result = pthread_key_create(&thread_id_key, free);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to create thread ID key: %s\n", strerror(result));
        // Fall back to a degraded but functional mode
        thread_id_key = 0;
    }
}

// Worker thread function for the thread pool
static void *thread_pool_worker(void *arg) {
    GooThreadPool *pool = (GooThreadPool *)arg;
    
    // Set cancellation state to allow threads to be canceled but only at cancellation points
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    // Allocate and set thread ID
    int *id = (int*)malloc(sizeof(int));
    if (!id) {
        fprintf(stderr, "Error: Thread ID allocation failed\n");
        return NULL;
    }
    
    // Assign sequential thread ID
    static _Atomic int next_thread_id = 0;
    *id = atomic_fetch_add(&next_thread_id, 1);
    pthread_setspecific(thread_id_key, id);
    
    // Main worker loop
    while (!pool->shutdown) {
        GooThreadPoolTask *task = NULL;
        
        // CRITICAL SECTION: Protected by queue_mutex
        pthread_mutex_lock(&pool->queue_mutex);
        
        // Wait for work or shutdown signal
        while (!pool->shutdown && pool->task_queue == NULL) {
            // Use timed wait to periodically check for shutdown
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 1;  // 1 second timeout
            
            // Wait with timeout
            int wait_result = pthread_cond_timedwait(&pool->queue_cond, 
                                                  &pool->queue_mutex, &ts);
            
            // Check wait result
            if (wait_result != 0 && wait_result != ETIMEDOUT) {
                fprintf(stderr, "Error: Condition wait failed: %s\n", 
                        strerror(wait_result));
            }
        }
        
        // Check if we have a task or should exit
        if (pool->shutdown && pool->task_queue == NULL) {
            pthread_mutex_unlock(&pool->queue_mutex);
            break;  // Exit the worker thread
        }
        
        // Get task from queue
        if (pool->task_queue != NULL) {
            task = pool->task_queue;
            pool->task_queue = task->next;
            
            // Update queue state
            pool->tasks_count--;
            pool->working_count++;
            
            // If this was the last item, update the tail pointer
            if (pool->task_queue == NULL) {
                pool->task_queue_tail = NULL;
            }
        }
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Execute task outside of critical section if we have one
        if (task != NULL) {
            // Execute the task (loop body function)
            // Handle potential function call failure gracefully
            if (task->function != NULL) {
                // Cancellation point before task execution
                pthread_testcancel();
                
                // Execute task
                for (uint64_t i = task->start; i < task->end; i += task->step) {
                    task->function(i, task->context);
                }
            }
            
            // Mark task as complete
            pthread_mutex_lock(&pool->queue_mutex);
            pool->working_count--;
            
            // Signal completion if this was the last task
            if (pool->working_count == 0 && pool->task_queue == NULL) {
                pthread_cond_signal(&pool->complete_cond);
            }
            
            pthread_mutex_unlock(&pool->queue_mutex);
            
            // Free task memory
            free(task);
        }
    }
    
    return NULL;
}

// Initialize thread pool with robust error handling
static bool init_thread_pool(int num_threads) {
    if (global_thread_pool != NULL) {
        return true;
    }
    
    // Initialize thread-local storage
    pthread_once(&thread_id_once, init_thread_id_key);
    
    // Create thread pool with null pointer check
    global_thread_pool = (GooThreadPool*)calloc(1, sizeof(GooThreadPool));
    if (global_thread_pool == NULL) {
        fprintf(stderr, "Error: Failed to allocate thread pool\n");
        return false;
    }
    
    // Determine number of threads with safe defaults
    if (num_threads <= 0) {
        long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
        if (nprocs <= 0) {
            fprintf(stderr, "Warning: Could not determine processor count, defaulting to 2 threads\n");
            num_threads = 2;
        } else {
            num_threads = (int)nprocs;
        }
    }
    
    // Initialize thread pool
    global_thread_pool->num_threads = num_threads;
    global_thread_pool->threads = (pthread_t*)calloc(num_threads, sizeof(pthread_t));
    if (global_thread_pool->threads == NULL) {
        fprintf(stderr, "Error: Failed to allocate thread array\n");
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Initialize mutex and condition variables with error checking
    int result;
    
    // Initialize mutex
    result = pthread_mutex_init(&global_thread_pool->queue_mutex, NULL);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to initialize queue mutex: %s\n", strerror(result));
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Initialize condition variables
    result = pthread_cond_init(&global_thread_pool->queue_cond, NULL);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to initialize queue condition: %s\n", strerror(result));
        pthread_mutex_destroy(&global_thread_pool->queue_mutex);
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    result = pthread_cond_init(&global_thread_pool->complete_cond, NULL);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to initialize completion condition: %s\n", strerror(result));
        pthread_cond_destroy(&global_thread_pool->queue_cond);
        pthread_mutex_destroy(&global_thread_pool->queue_mutex);
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        result = pthread_create(&global_thread_pool->threads[i], NULL, 
                              thread_pool_worker, global_thread_pool);
        if (result != 0) {
            fprintf(stderr, "Error: Failed to create worker thread %d: %s\n", 
                    i, strerror(result));
            
            // Clean up threads created so far
            global_thread_pool->shutdown = true;
            pthread_cond_broadcast(&global_thread_pool->queue_cond);
            
            // Wait for all created threads to exit
            for (int j = 0; j < i; j++) {
                pthread_join(global_thread_pool->threads[j], NULL);
            }
            
            // Clean up resources
            pthread_cond_destroy(&global_thread_pool->complete_cond);
            pthread_cond_destroy(&global_thread_pool->queue_cond);
            pthread_mutex_destroy(&global_thread_pool->queue_mutex);
            free(global_thread_pool->threads);
            free(global_thread_pool);
            global_thread_pool = NULL;
            return false;
        }
    }
    
    // Initialize barrier count
    barrier_total = num_threads;
    
    return true;
}

// Cleanup thread pool with robust error handling
void goo_parallel_cleanup(void) {
    if (global_thread_pool == NULL) {
        return;
    }
    
    // Signal threads to exit
    pthread_mutex_lock(&global_thread_pool->queue_mutex);
    global_thread_pool->shutdown = true;
    pthread_cond_broadcast(&global_thread_pool->queue_cond);
    pthread_mutex_unlock(&global_thread_pool->queue_mutex);
    
    // Wait for all threads to exit
    for (int i = 0; i < global_thread_pool->num_threads; i++) {
        int join_result = pthread_join(global_thread_pool->threads[i], NULL);
        if (join_result != 0) {
            fprintf(stderr, "Warning: Failed to join thread %d: %s\n", 
                    i, strerror(join_result));
            
            // Try to cancel the thread as a fallback
            pthread_cancel(global_thread_pool->threads[i]);
            join_result = pthread_join(global_thread_pool->threads[i], NULL);
            if (join_result != 0) {
                fprintf(stderr, "Error: Failed to join thread after cancellation: %s\n",
                        strerror(join_result));
            }
        }
    }
    
    // Free task queue with null pointer protection
    GooThreadPoolTask *task = global_thread_pool->task_queue;
    while (task != NULL) {
        GooThreadPoolTask *next = task->next;
        free(task);
        task = next;
    }
    
    // Free thread pool resources
    pthread_cond_destroy(&global_thread_pool->complete_cond);
    pthread_cond_destroy(&global_thread_pool->queue_cond);
    pthread_mutex_destroy(&global_thread_pool->queue_mutex);
    
    free(global_thread_pool->threads);
    free(global_thread_pool);
    global_thread_pool = NULL;
    
    // Clean up barrier
    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&barrier_cond);
}

// Initialize the parallel execution system
bool goo_parallel_init(int num_threads) {
    return init_thread_pool(num_threads);
}

// Execute a parallel for loop (blocking version)
bool goo_parallel_for(size_t start, size_t end, size_t step,
                      goo_loop_body_t body, void *context,
                      GooScheduleType schedule, int chunk_size, int num_threads) {
    // If not initialized, initialize with auto thread count
    if (global_thread_pool == NULL) {
        if (!goo_parallel_init(0)) {
            fprintf(stderr, "Error: Failed to initialize parallel subsystem\n");
            return false;
        }
    }
    
    // Validate parameters
    if (body == NULL) {
        fprintf(stderr, "Error: Null function pointer provided to goo_parallel_for\n");
        return false;
    }
    
    if (step == 0) {
        fprintf(stderr, "Error: Step size cannot be zero in goo_parallel_for\n");
        return false;
    }
    
    // Avoid overflow in loop calculations
    if (start > end) {
        fprintf(stderr, "Warning: Empty loop range (start > end) in goo_parallel_for\n");
        return true; // Nothing to do but not an error
    }
    
    // Calculate iterations to prevent overflow
    size_t max_iterations;
    if (__builtin_sub_overflow(end, start, &max_iterations) || 
        __builtin_add_overflow(max_iterations, step - 1, &max_iterations) ||
        __builtin_div_overflow(max_iterations, step, &max_iterations)) {
        fprintf(stderr, "Error: Loop bounds would cause integer overflow\n");
        return false;
    }
    
    // Use default chunk size if not specified or invalid
    if (chunk_size <= 0) {
        // Simple adaptive heuristic for chunk size
        if (max_iterations < 100) {
            // For small workloads, use larger relative chunks
            chunk_size = (int)(max_iterations / 4);
        } else if (max_iterations < 10000) {
            // For medium workloads, target 8 chunks per thread
            int target_threads = global_thread_pool->num_threads;
            chunk_size = (int)(max_iterations / (target_threads * 8));
        } else {
            // For large workloads, aim for shorter chunks
            int target_threads = global_thread_pool->num_threads;
            chunk_size = (int)(max_iterations / (target_threads * 16));
        }
        
        // Ensure we don't get chunks that are too small
        if (chunk_size < 1) chunk_size = 1;
    }
    
    // Set up the work distribution
    if (!goo_work_distribution_init(start, end, step, max_iterations, 
                                  schedule, chunk_size, global_thread_pool->num_threads)) {
        fprintf(stderr, "Error: Failed to initialize work distribution\n");
        return false;
    }
    
    // Create tasks and submit to thread pool
    int task_count = (schedule == GOO_SCHEDULE_STATIC) ? global_thread_pool->num_threads : 
                                                       (int)((max_iterations + chunk_size - 1) / chunk_size);
                                                       
    pthread_mutex_lock(&global_thread_pool->queue_mutex);
    
    // Create task objects for each chunk
    for (int i = 0; i < task_count; i++) {
        GooThreadPoolTask *task = (GooThreadPoolTask*)malloc(sizeof(GooThreadPoolTask));
        if (!task) {
            fprintf(stderr, "Error: Failed to allocate task for parallel loop\n");
            pthread_mutex_unlock(&global_thread_pool->queue_mutex);
            return false;
        }
        
        // Set up task parameters
        task->function = body;
        task->context = context;
        task->start = 0;  // Will be determined by work distribution
        task->end = 0;    // Will be determined by work distribution
        task->step = step;
        task->priority = 0;
        task->next = NULL;
        
        // Add to task queue
        if (global_thread_pool->task_queue == NULL) {
            global_thread_pool->task_queue = task;
            global_thread_pool->task_queue_tail = task;
        } else {
            global_thread_pool->task_queue_tail->next = task;
            global_thread_pool->task_queue_tail = task;
        }
        
        global_thread_pool->tasks_count++;
    }
    
    // Notify all threads that work is available
    pthread_cond_broadcast(&global_thread_pool->queue_cond);
    
    // Wait for all tasks to complete
    while (global_thread_pool->tasks_count > 0 || global_thread_pool->working_count > 0) {
        pthread_cond_wait(&global_thread_pool->complete_cond, &global_thread_pool->queue_mutex);
    }
    
    pthread_mutex_unlock(&global_thread_pool->queue_mutex);
    
    // Clean up the work distribution
    goo_work_distribution_cleanup();
    
    return true;
}

// Parallel barrier synchronization
void goo_parallel_barrier(void) {
    int result;
    
    // Acquire the barrier mutex
    result = pthread_mutex_lock(&barrier_mutex);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to lock barrier mutex: %s\n", strerror(result));
        return;
    }
    
    // Increment barrier count atomically
    barrier_count++;
    
    // Check if all threads have arrived
    if (barrier_count < barrier_total) {
        // Not all threads have arrived, wait with timeout to prevent deadlocks
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 60; // 1 minute timeout
        
        result = pthread_cond_timedwait(&barrier_cond, &barrier_mutex, &timeout);
        if (result == ETIMEDOUT) {
            fprintf(stderr, "Warning: Barrier timeout after 60 seconds. Forcing continuation.\n");
            // Reset the barrier in case of deadlock
            barrier_count = 0;
            pthread_cond_broadcast(&barrier_cond);
        } else if (result != 0) {
            fprintf(stderr, "Error: Barrier wait failed: %s\n", strerror(result));
        }
    } else {
        // All threads have arrived, reset and wake everyone
        barrier_count = 0;
        
        // Broadcast to wake up all waiting threads
        result = pthread_cond_broadcast(&barrier_cond);
        if (result != 0) {
            fprintf(stderr, "Error: Failed to broadcast barrier condition: %s\n", 
                   strerror(result));
        }
    }
    
    // Release the barrier mutex
    result = pthread_mutex_unlock(&barrier_mutex);
    if (result != 0) {
        fprintf(stderr, "Error: Failed to unlock barrier mutex: %s\n", strerror(result));
    }
}

// Placeholder stubs for remaining functions with proper handling of unused parameters
bool goo_parallel_task(void (*task_func)(void*), void *context, int priority) {
    (void)task_func;  // Mark as used
    (void)context;    // Mark as used
    (void)priority;   // Mark as used
    return false;
}

bool goo_parallel_foreach(void *items, size_t count, size_t item_size,
                         void (*body)(void*, void*), void *context,
                         GooScheduleType schedule, int chunk_size, int num_threads) {
    (void)items;      // Mark as used
    (void)count;      // Mark as used
    (void)item_size;  // Mark as used
    (void)body;       // Mark as used
    (void)context;    // Mark as used
    (void)schedule;   // Mark as used
    (void)chunk_size; // Mark as used
    (void)num_threads;// Mark as used
    return false;
}

bool goo_parallel_begin(int num_threads, GooSharedVar *shared_vars, int var_count) {
    (void)num_threads;  // Mark as used
    (void)shared_vars;  // Mark as used
    (void)var_count;    // Mark as used
    return false;
}

// Forward to the vectorization implementation
bool goo_vector_execute(GooVector *vec_op) {
    // This will be implemented in goo_vectorization.c
    (void)vec_op;  // Mark as used
    return false;
}

void goo_parallel_set_threads(int num_threads) {
    (void)num_threads;  // Mark as used
}

// Get thread number
int goo_parallel_get_thread_num(void) {
    int *id = (int*)pthread_getspecific(thread_id_key);
    return id ? *id : 0;
}

// Get number of threads
int goo_parallel_get_num_threads(void) {
    return global_thread_pool ? global_thread_pool->num_threads : 1;
}

// Wait for all tasks to complete
bool goo_parallel_taskwait(void) {
    // Not yet implemented
    return false;
}

void goo_parallel_end(void) {
} 