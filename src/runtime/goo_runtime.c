/**
 * goo_runtime.c
 * 
 * Implementation of the unified Goo runtime system.
 * This file provides initialization and cleanup of all runtime components.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>

#include "../include/goo_runtime.h"
#include "../include/memory/scoped_alloc.h"
#include "../include/comptime/comptime.h"
#include "../include/meta/reflection.h"
#include "../include/parallel/parallel.h"
#include "../include/messaging/messaging.h"

// Forward declarations of subsystem initialization functions
extern bool goo_memory_init(void);
extern void goo_memory_cleanup(void);

// Thread-local storage for panic recovery
__thread jmp_buf* goo_recover_point = NULL;
__thread void* goo_panic_value = NULL;

// ===== Goroutine Implementation =====

// Thread pool configuration
#define GOO_DEFAULT_THREAD_POOL_SIZE 16
#define GOO_MAX_QUEUED_TASKS 1024

// Thread pool state
typedef struct {
    pthread_t threads[GOO_DEFAULT_THREAD_POOL_SIZE];
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    GooTask* task_queue[GOO_MAX_QUEUED_TASKS];
    int queue_head;
    int queue_tail;
    int queue_size;
    bool shutdown;
} GooThreadPool;

static GooThreadPool* global_thread_pool = NULL;

// Initialize the thread pool
bool goo_thread_pool_init(int thread_count) {
    if (global_thread_pool) {
        return true;  // Already initialized
    }
    
    global_thread_pool = (GooThreadPool*)malloc(sizeof(GooThreadPool));
    if (!global_thread_pool) {
        return false;
    }
    
    // Default to hardware concurrency if not specified
    if (thread_count <= 0) {
        thread_count = goo_runtime_get_num_cores();
    }
    
    // Initialize thread pool values
    global_thread_pool->num_threads = thread_count;
    global_thread_pool->threads = (pthread_t*)malloc(thread_count * sizeof(pthread_t));
    global_thread_pool->running = true;
    global_thread_pool->active_tasks = 0;
    
    if (!global_thread_pool->threads) {
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Initialize mutex and condition variables
    if (pthread_mutex_init(&global_thread_pool->mutex, NULL) != 0) {
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    if (pthread_cond_init(&global_thread_pool->cond, NULL) != 0) {
        pthread_mutex_destroy(&global_thread_pool->mutex);
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Initialize task queue
    global_thread_pool->queue = goo_task_queue_create(100);  // Default size
    if (!global_thread_pool->queue) {
        pthread_cond_destroy(&global_thread_pool->cond);
        pthread_mutex_destroy(&global_thread_pool->mutex);
        free(global_thread_pool->threads);
        free(global_thread_pool);
        global_thread_pool = NULL;
        return false;
    }
    
    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&global_thread_pool->threads[i], NULL, goo_worker_thread, NULL) != 0) {
            // Failed to create thread, clean up
            goo_thread_pool_cleanup();
            return false;
        }
    }
    
    return true;
}

// Cleanup the thread pool
void goo_thread_pool_cleanup() {
    if (!global_thread_pool) return;
    
    // Signal shutdown
    pthread_mutex_lock(&global_thread_pool->queue_mutex);
    global_thread_pool->shutdown = true;
    pthread_cond_broadcast(&global_thread_pool->queue_cond);
    pthread_mutex_unlock(&global_thread_pool->queue_mutex);
    
    // Wait for threads to exit
    for (int i = 0; i < GOO_DEFAULT_THREAD_POOL_SIZE; i++) {
        pthread_join(global_thread_pool->threads[i], NULL);
    }
    
    // Clean up remaining tasks
    while (global_thread_pool->queue_size > 0) {
        GooTask* task = global_thread_pool->task_queue[global_thread_pool->queue_head];
        global_thread_pool->queue_head = (global_thread_pool->queue_head + 1) % GOO_MAX_QUEUED_TASKS;
        global_thread_pool->queue_size--;
        free(task);
    }
    
    // Cleanup synchronization primitives
    pthread_mutex_destroy(&global_thread_pool->queue_mutex);
    pthread_cond_destroy(&global_thread_pool->queue_cond);
    
    // Free the thread pool
    free(global_thread_pool);
    global_thread_pool = NULL;
}

// Worker thread function
void* goo_worker_thread(void* arg) {
    while (true) {
        // Get a task from the queue
        pthread_mutex_lock(&global_thread_pool->queue_mutex);
        
        while (global_thread_pool->queue_size == 0 && !global_thread_pool->shutdown) {
            pthread_cond_wait(&global_thread_pool->queue_cond, &global_thread_pool->queue_mutex);
        }
        
        if (global_thread_pool->shutdown && global_thread_pool->queue_size == 0) {
            pthread_mutex_unlock(&global_thread_pool->queue_mutex);
            return NULL;
        }
        
        GooTask* task = global_thread_pool->task_queue[global_thread_pool->queue_head];
        global_thread_pool->queue_head = (global_thread_pool->queue_head + 1) % GOO_MAX_QUEUED_TASKS;
        global_thread_pool->queue_size--;
        
        pthread_mutex_unlock(&global_thread_pool->queue_mutex);
        
        // Execute the task with panic recovery
        jmp_buf recover_buf;
        jmp_buf* old_recover_point = goo_recover_point;
        
        if (setjmp(recover_buf) == 0) {
            goo_recover_point = &recover_buf;
            task->func(task->arg);
        } else {
            // Panic occurred, handle according to supervision policy
            if (task->supervisor) {
                goo_supervise_handle_error(task->supervisor, task, goo_panic_value);
            } else {
                fprintf(stderr, "Unhandled panic in goroutine: %p\n", goo_panic_value);
            }
        }
        
        goo_recover_point = old_recover_point;
        free(task);
    }
    
    return NULL;
}

// Schedule a task to run on the thread pool
bool goo_schedule_task(GooTask* task) {
    if (!global_thread_pool && !goo_thread_pool_init()) {
        return false;
    }
    
    pthread_mutex_lock(&global_thread_pool->queue_mutex);
    
    if (global_thread_pool->queue_size >= GOO_MAX_QUEUED_TASKS) {
        pthread_mutex_unlock(&global_thread_pool->queue_mutex);
        fprintf(stderr, "Task queue is full\n");
        return false;
    }
    
    global_thread_pool->task_queue[global_thread_pool->queue_tail] = task;
    global_thread_pool->queue_tail = (global_thread_pool->queue_tail + 1) % GOO_MAX_QUEUED_TASKS;
    global_thread_pool->queue_size++;
    
    pthread_cond_signal(&global_thread_pool->queue_cond);
    pthread_mutex_unlock(&global_thread_pool->queue_mutex);
    
    return true;
}

// Create and spawn a new goroutine
bool goo_goroutine_spawn(GooTaskFunc func, void* arg, GooSupervisor* supervisor) {
    GooTask* task = (GooTask*)malloc(sizeof(GooTask));
    if (!task) {
        perror("Failed to allocate memory for task");
        return false;
    }
    
    task->func = func;
    task->arg = arg;
    task->supervisor = supervisor;
    
    if (!goo_schedule_task(task)) {
        free(task);
        return false;
    }
    
    return true;
}

// ===== Channel Implementation =====

// Create a channel with specified element size, capacity, and pattern
GooChannel* goo_channel_create(size_t element_size, size_t capacity, GooChannelPattern pattern) {
    GooChannelOptions options = {
        .buffer_size = capacity,
        .is_blocking = true,
        .pattern = pattern,
        .timeout_ms = -1 // Infinite timeout by default
    };
    
    GooChannel* channel = (GooChannel*)malloc(sizeof(GooChannel));
    if (!channel) return NULL;
    
    memset(channel, 0, sizeof(GooChannel));
    channel->type = pattern;
    channel->elem_size = element_size;
    channel->buffer_size = capacity > 0 ? capacity : 1;
    channel->high_water_mark = capacity;
    channel->low_water_mark = capacity / 2;
    channel->timeout_ms = -1; // Infinite timeout
    
    // Initialize buffer for buffered channels
    if (capacity > 0) {
        channel->buffer = malloc(channel->buffer_size * channel->elem_size);
        if (!channel->buffer) {
            free(channel);
            return NULL;
        }
    }
    
    // Initialize synchronization primitives
    if (pthread_mutex_init(&channel->mutex, NULL) != 0) {
        if (channel->buffer) free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    if (pthread_cond_init(&channel->send_cond, NULL) != 0) {
        pthread_mutex_destroy(&channel->mutex);
        if (channel->buffer) free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    if (pthread_cond_init(&channel->recv_cond, NULL) != 0) {
        pthread_cond_destroy(&channel->send_cond);
        pthread_mutex_destroy(&channel->mutex);
        if (channel->buffer) free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    return channel;
}

// Close a channel
void goo_channel_close(GooChannel* channel) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    channel->closed = true;
    // Signal all waiters
    pthread_cond_broadcast(&channel->not_empty);
    pthread_cond_broadcast(&channel->not_full);
    pthread_mutex_unlock(&channel->mutex);
}

// Free a channel
void goo_channel_free(GooChannel* channel) {
    if (!channel) return;
    
    goo_channel_close(channel);
    
    pthread_mutex_destroy(&channel->mutex);
    pthread_cond_destroy(&channel->not_empty);
    pthread_cond_destroy(&channel->not_full);
    
    free(channel->buffer);
    free(channel);
}

// Send data to a channel
bool goo_channel_send(GooChannel* channel, void* data) {
    if (!channel || !data) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Wait until there's room in the channel or it's closed
    while (channel->count == channel->capacity && !channel->closed) {
        pthread_cond_wait(&channel->not_full, &channel->mutex);
    }
    
    // Check if the channel is closed
    if (channel->closed) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Copy data into the buffer
    void* dest = (char*)channel->buffer + (channel->tail * channel->element_size);
    memcpy(dest, data, channel->element_size);
    
    // Update indices
    channel->tail = (channel->tail + 1) % channel->capacity;
    channel->count++;
    
    // Signal that the channel is not empty
    pthread_cond_signal(&channel->not_empty);
    pthread_mutex_unlock(&channel->mutex);
    
    // For pub/sub channels, forward to all subscribers
    if (channel->type == GOO_CHANNEL_PUB && channel->subscribers) {
        for (int i = 0; i < channel->subscriber_count; i++) {
            goo_channel_send(channel->subscribers[i], data);
        }
    }
    
    return true;
}

// Receive data from a channel
bool goo_channel_recv(GooChannel* channel, void* data) {
    if (!channel || !data) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Wait until there's data or the channel is closed
    while (channel->count == 0 && !channel->closed) {
        pthread_cond_wait(&channel->not_empty, &channel->mutex);
    }
    
    // Check if the channel is empty and closed
    if (channel->count == 0 && channel->closed) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Copy data from the buffer
    void* src = (char*)channel->buffer + (channel->head * channel->element_size);
    memcpy(data, src, channel->element_size);
    
    // Update indices
    channel->head = (channel->head + 1) % channel->capacity;
    channel->count--;
    
    // Signal that the channel is not full
    pthread_cond_signal(&channel->not_full);
    pthread_mutex_unlock(&channel->mutex);
    
    return true;
}

// Try to send data to a channel (non-blocking)
bool goo_channel_try_send(GooChannel* channel, void* data) {
    if (!channel || !data) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // If channel is full or closed, return false
    if (channel->count == channel->capacity || channel->closed) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Copy data into the buffer
    void* dest = (char*)channel->buffer + (channel->tail * channel->element_size);
    memcpy(dest, data, channel->element_size);
    
    // Update indices
    channel->tail = (channel->tail + 1) % channel->capacity;
    channel->count++;
    
    // Signal that the channel is not empty
    pthread_cond_signal(&channel->not_empty);
    pthread_mutex_unlock(&channel->mutex);
    
    // For pub/sub channels, forward to all subscribers
    if (channel->type == GOO_CHANNEL_PUB && channel->subscribers) {
        for (int i = 0; i < channel->subscriber_count; i++) {
            goo_channel_try_send(channel->subscribers[i], data);
        }
    }
    
    return true;
}

// Try to receive data from a channel (non-blocking)
bool goo_channel_try_recv(GooChannel* channel, void* data) {
    if (!channel || !data) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // If channel is empty, return false
    if (channel->count == 0) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Copy data from the buffer
    void* src = (char*)channel->buffer + (channel->head * channel->element_size);
    memcpy(data, src, channel->element_size);
    
    // Update indices
    channel->head = (channel->head + 1) % channel->capacity;
    channel->count--;
    
    // Signal that the channel is not full
    pthread_cond_signal(&channel->not_full);
    pthread_mutex_unlock(&channel->mutex);
    
    return true;
}

// Subscribe to a pub channel
bool goo_channel_subscribe(GooChannel* pub, GooChannel* sub) {
    if (!pub || !sub || pub->type != GOO_CHANNEL_PUB) return false;
    
    pthread_mutex_lock(&pub->mutex);
    
    // Resize the subscribers array
    GooChannel** new_subscribers = (GooChannel**)realloc(
        pub->subscribers, (pub->subscriber_count + 1) * sizeof(GooChannel*));
        
    if (!new_subscribers) {
        pthread_mutex_unlock(&pub->mutex);
        return false;
    }
    
    pub->subscribers = new_subscribers;
    pub->subscribers[pub->subscriber_count++] = sub;
    
    pthread_mutex_unlock(&pub->mutex);
    return true;
}

// ===== Supervision System =====

// Create a new supervisor
GooSupervisor* goo_supervise_init() {
    GooSupervisor* supervisor = (GooSupervisor*)malloc(sizeof(GooSupervisor));
    if (!supervisor) {
        perror("Failed to allocate memory for supervisor");
        return NULL;
    }
    
    supervisor->children = NULL;
    supervisor->child_count = 0;
    supervisor->restart_policy = GOO_SUPERVISE_ONE_FOR_ONE;
    supervisor->max_restarts = 10;
    supervisor->time_window = 5; // 5 seconds
    supervisor->restart_count = 0;
    supervisor->last_restart_time = time(NULL);
    
    if (pthread_mutex_init(&supervisor->mutex, NULL) != 0) {
        perror("Failed to initialize supervisor mutex");
        free(supervisor);
        return NULL;
    }
    
    return supervisor;
}

// Free a supervisor
void goo_supervise_free(GooSupervisor* supervisor) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Free all children
    for (int i = 0; i < supervisor->child_count; i++) {
        free(supervisor->children[i]);
    }
    
    free(supervisor->children);
    pthread_mutex_unlock(&supervisor->mutex);
    
    pthread_mutex_destroy(&supervisor->mutex);
    free(supervisor);
}

// Register a child with a supervisor
bool goo_supervise_register(GooSupervisor* supervisor, GooTaskFunc func, void* arg) {
    if (!supervisor || !func) return false;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Create the child structure
    GooSuperviseChild* child = (GooSuperviseChild*)malloc(sizeof(GooSuperviseChild));
    if (!child) {
        pthread_mutex_unlock(&supervisor->mutex);
        return false;
    }
    
    child->func = func;
    child->arg = arg;
    child->failed = false;
    child->supervisor = supervisor;
    
    // Resize the children array
    GooSuperviseChild** new_children = (GooSuperviseChild**)realloc(
        supervisor->children, (supervisor->child_count + 1) * sizeof(GooSuperviseChild*));
        
    if (!new_children) {
        free(child);
        pthread_mutex_unlock(&supervisor->mutex);
        return false;
    }
    
    supervisor->children = new_children;
    supervisor->children[supervisor->child_count++] = child;
    
    pthread_mutex_unlock(&supervisor->mutex);
    
    // Create a task for the child
    GooTask* task = (GooTask*)malloc(sizeof(GooTask));
    if (!task) {
        perror("Failed to allocate memory for supervised task");
        return false;
    }
    
    task->func = func;
    task->arg = arg;
    task->supervisor = supervisor;
    
    // Schedule the task
    if (!goo_schedule_task(task)) {
        free(task);
        return false;
    }
    
    return true;
}

// Set the supervision policy for a supervisor
void goo_supervise_set_policy(GooSupervisor* supervisor, GooSupervisionPolicy policy, int max_restarts, int time_window) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    supervisor->policy = policy;
    supervisor->max_restarts = max_restarts;
    supervisor->restart_window = time_window;
    pthread_mutex_unlock(&supervisor->mutex);
}

// Start the supervision system
bool goo_supervise_start(GooSupervisor* supervisor) {
    if (!supervisor) return false;
    
    // The supervision system is already running when children are registered
    return true;
}

// Handle an error in a supervised task
void goo_supervise_handle_error(GooSupervisor* supervisor, GooTask* failed_task, void* error_info) {
    if (!supervisor || !failed_task) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Update restart statistics
    time_t now = time(NULL);
    if (now - supervisor->last_restart_time > supervisor->time_window) {
        // Reset the counter if the time window has passed
        supervisor->restart_count = 0;
        supervisor->last_restart_time = now;
    }
    
    supervisor->restart_count++;
    
    // Check if we've exceeded the maximum number of restarts
    if (supervisor->restart_count > supervisor->max_restarts) {
        fprintf(stderr, "Maximum restart count exceeded for supervisor\n");
        pthread_mutex_unlock(&supervisor->mutex);
        return;
    }
    
    // Find the failed child
    int failed_index = -1;
    for (int i = 0; i < supervisor->child_count; i++) {
        if (supervisor->children[i]->func == failed_task->func && 
            supervisor->children[i]->arg == failed_task->arg) {
            failed_index = i;
            supervisor->children[i]->failed = true;
            break;
        }
    }
    
    if (failed_index == -1) {
        // Child not found, can't restart
        pthread_mutex_unlock(&supervisor->mutex);
        return;
    }
    
    // Implement restart policy
    switch (supervisor->restart_policy) {
        case GOO_SUPERVISE_ONE_FOR_ONE:
            // Restart just the failed child
            goo_supervise_restart_child(supervisor, failed_index);
            break;
            
        case GOO_SUPERVISE_ONE_FOR_ALL:
            // Restart all children
            for (int i = 0; i < supervisor->child_count; i++) {
                goo_supervise_restart_child(supervisor, i);
            }
            break;
            
        case GOO_SUPERVISE_REST_FOR_ONE:
            // Restart the failed child and all children that were started after it
            for (int i = failed_index; i < supervisor->child_count; i++) {
                goo_supervise_restart_child(supervisor, i);
            }
            break;
            
        default:
            // Unknown policy, just restart the failed child
            goo_supervise_restart_child(supervisor, failed_index);
            break;
    }
    
    pthread_mutex_unlock(&supervisor->mutex);
}

// Restart a specific child
void goo_supervise_restart_child(GooSupervisor* supervisor, int child_index) {
    if (!supervisor || child_index < 0 || child_index >= supervisor->child_count) return;
    
    GooSuperviseChild* child = supervisor->children[child_index];
    
    // Create a new task for the child
    GooTask* task = (GooTask*)malloc(sizeof(GooTask));
    if (!task) {
        perror("Failed to allocate memory for restarting supervised task");
        return;
    }
    
    task->func = child->func;
    task->arg = child->arg;
    task->supervisor = supervisor;
    
    // Schedule the task
    if (!goo_schedule_task(task)) {
        free(task);
        return;
    }
    
    // Reset the failed flag
    child->failed = false;
}

// ===== Error Handling =====

// Trigger an unrecoverable runtime panic
void goo_panic(void* value, const char* message) {
    // Print error message
    fprintf(stderr, "PANIC: %s\n", message ? message : "Unknown error");
    
    // Log the error if logging is enabled
    if (goo_log_level > 0) {
        goo_log_error("PANIC: %s", message ? message : "Unknown error");
    }
    
    // Store error information in thread-local storage
    thread_error_value = value;
    
    // Call any registered panic handlers
    if (panic_handler) {
        panic_handler(value, message);
    }
    
    // Exit the program
    abort();
}

// Returns true if currently handling a panic
bool goo_is_panic() {
    return goo_panic_value != NULL;
}

// Get the current panic value
void* goo_get_panic_value() {
    return goo_panic_value;
}

// Clear the current panic
void goo_clear_panic() {
    goo_panic_value = NULL;
}

// ===== Parallel Execution =====

// Execute a function in parallel across multiple threads
bool goo_parallel_execute(GooParallelFunc func, void* arg, int num_threads) {
    if (!func) return false;
    
    // Use a reasonable default if num_threads is invalid
    if (num_threads <= 0) {
        num_threads = 4; // Default to 4 threads
    }
    
    // Create thread objects
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    if (!threads) {
        perror("Failed to allocate memory for parallel threads");
        return false;
    }
    
    // Create thread arguments
    GooParallelArg* thread_args = (GooParallelArg*)malloc(num_threads * sizeof(GooParallelArg));
    if (!thread_args) {
        perror("Failed to allocate memory for parallel thread arguments");
        free(threads);
        return false;
    }
    
    // Initialize and start threads
    for (int i = 0; i < num_threads; i++) {
        thread_args[i].func = func;
        thread_args[i].arg = arg;
        thread_args[i].thread_id = i;
        thread_args[i].num_threads = num_threads;
        
        if (pthread_create(&threads[i], NULL, goo_parallel_worker, &thread_args[i]) != 0) {
            perror("Failed to create parallel worker thread");
            
            // Clean up already created threads
            for (int j = 0; j < i; j++) {
                pthread_cancel(threads[j]);
                pthread_join(threads[j], NULL);
            }
            
            free(thread_args);
            free(threads);
            return false;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Clean up
    free(thread_args);
    free(threads);
    
    return true;
}

// Worker function for parallel execution
void* goo_parallel_worker(void* arg) {
    GooParallelArg* p_arg = (GooParallelArg*)arg;
    
    // Handle panics
    jmp_buf recover_buf;
    jmp_buf* old_recover_point = goo_recover_point;
    
    if (setjmp(recover_buf) == 0) {
        goo_recover_point = &recover_buf;
        p_arg->func(p_arg->thread_id, p_arg->num_threads, p_arg->arg);
    } else {
        // Panic occurred in parallel execution
        fprintf(stderr, "Panic in parallel worker thread %d: %p\n", 
                p_arg->thread_id, goo_get_panic_value());
    }
    
    goo_recover_point = old_recover_point;
    return NULL;
}

// ===== Runtime Initialization and Cleanup =====

// Runtime state
static bool is_initialized = false;
static const char* version = "0.1.0";

// Subsystem initialization order (from bottom up)
typedef enum {
    SUBSYS_MEMORY = 0,
    SUBSYS_SCOPED_ALLOC,
    SUBSYS_COMPTIME,
    SUBSYS_REFLECTION,
    SUBSYS_PARALLEL,
    SUBSYS_MESSAGING,
    SUBSYS_COUNT
} Subsystem;

// Track which subsystems are initialized
static bool subsys_initialized[SUBSYS_COUNT] = {false};

/**
 * Initialize the Goo runtime system.
 * This initializes all subsystems: memory, comptime, reflection, etc.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_runtime_init(void) {
    // Already initialized?
    if (is_initialized) {
        return true;
    }

    // Initialize memory management first
    if (!goo_memory_init()) {
        fprintf(stderr, "Failed to initialize memory management subsystem\n");
        return false;
    }
    subsys_initialized[SUBSYS_MEMORY] = true;

    // Initialize scope-based memory allocation
    if (!goo_scoped_alloc_init()) {
        fprintf(stderr, "Failed to initialize scope-based memory allocation\n");
        goto cleanup;
    }
    subsys_initialized[SUBSYS_SCOPED_ALLOC] = true;

    // Initialize compile-time evaluation
    if (!goo_comptime_init()) {
        fprintf(stderr, "Failed to initialize compile-time evaluation\n");
        goto cleanup;
    }
    subsys_initialized[SUBSYS_COMPTIME] = true;

    // Initialize reflection
    if (!goo_reflection_init()) {
        fprintf(stderr, "Failed to initialize reflection system\n");
        goto cleanup;
    }
    subsys_initialized[SUBSYS_REFLECTION] = true;

    // Initialize parallel execution
    if (!goo_parallel_init()) {
        fprintf(stderr, "Failed to initialize parallel execution system\n");
        goto cleanup;
    }
    subsys_initialized[SUBSYS_PARALLEL] = true;

    // Initialize messaging system
    if (!goo_messaging_init()) {
        fprintf(stderr, "Failed to initialize messaging system\n");
        goto cleanup;
    }
    subsys_initialized[SUBSYS_MESSAGING] = true;

    // All subsystems initialized successfully
    is_initialized = true;
    return true;

cleanup:
    // Cleanup in reverse order of initialization
    goo_runtime_cleanup();
    return false;
}

/**
 * Clean up the Goo runtime system.
 * This cleans up all subsystems in the appropriate order.
 */
void goo_runtime_cleanup(void) {
    // Only cleanup if initialized
    if (!is_initialized) {
        return;
    }

    // Cleanup in reverse order of initialization
    if (subsys_initialized[SUBSYS_MESSAGING]) {
        goo_messaging_cleanup();
        subsys_initialized[SUBSYS_MESSAGING] = false;
    }

    if (subsys_initialized[SUBSYS_PARALLEL]) {
        goo_parallel_cleanup();
        subsys_initialized[SUBSYS_PARALLEL] = false;
    }

    if (subsys_initialized[SUBSYS_REFLECTION]) {
        goo_reflection_cleanup();
        subsys_initialized[SUBSYS_REFLECTION] = false;
    }

    if (subsys_initialized[SUBSYS_COMPTIME]) {
        goo_comptime_cleanup();
        subsys_initialized[SUBSYS_COMPTIME] = false;
    }

    if (subsys_initialized[SUBSYS_SCOPED_ALLOC]) {
        goo_scoped_alloc_cleanup();
        subsys_initialized[SUBSYS_SCOPED_ALLOC] = false;
    }

    if (subsys_initialized[SUBSYS_MEMORY]) {
        goo_memory_cleanup();
        subsys_initialized[SUBSYS_MEMORY] = false;
    }

    is_initialized = false;
}

/**
 * Check if the Goo runtime system is initialized.
 * 
 * @return true if the runtime is initialized, false otherwise
 */
bool goo_runtime_is_initialized(void) {
    return is_initialized;
}

/**
 * Get the current version of the Goo runtime.
 * 
 * @return A string containing the version number
 */
const char* goo_runtime_version(void) {
    return version;
} 