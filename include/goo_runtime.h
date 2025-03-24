#ifndef GOO_RUNTIME_H
#define GOO_RUNTIME_H

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// Include these headers only if they are needed
#include "goo_memory.h"
#include "goo_error.h"
#include "goo_integration.h"

// Include goo_core.h last to minimize include conflicts
#include "goo_core.h"

// Forward declarations
typedef struct GooSupervisor GooSupervisor;
typedef struct GooTask GooTask;
typedef struct GooSuperviseChild GooSuperviseChild;

// Forward declarations to avoid circular includes
typedef struct GooRuntimeContext GooRuntimeContext;
typedef struct GooCodegenContext GooCodegenContext;
typedef struct GooExecutionContext GooExecutionContext;

// ===== Task and Goroutine Definitions =====

// Function type for goroutines and supervised tasks
typedef void (*GooTaskFunc)(void*);

// Function type for parallel execution
typedef void (*GooParallelFunc)(int, int, void*);

// Task structure for the thread pool
typedef struct GooTask {
    GooTaskFunc func;
    void* arg;
    GooSupervisor* supervisor;
} GooTask;

// Arguments for parallel execution
typedef struct GooParallelArg {
    GooParallelFunc func;
    void* arg;
    int thread_id;
    int num_threads;
} GooParallelArg;

// ===== Channel Definitions =====

// Channel pattern type (moved here to prevent include conflicts)
typedef enum GooChannelPattern {
    GOO_CHANNEL_SYNC,
    GOO_CHANNEL_BUFFERED,
    GOO_CHANNEL_ASYNC,
    GOO_CHANNEL_RENDEZVOUS,
    GOO_CHANNEL_DISTRIBUTED
} GooChannelPattern;

// Internal channel implementation
typedef struct GooChannel {
    void* buffer;
    size_t element_size;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    GooChannelPattern type;
    GooChannelPattern impl_type;
    
    // For distributed channels
    char* endpoint;
    GooChannel** subscribers;
    int subscriber_count;
} GooChannel;

// ===== Supervisor Definitions =====

// Supervision policy type (moved here to prevent include conflicts)
typedef enum GooSupervisionPolicy {
    GOO_SUPERVISION_ONE_FOR_ONE,
    GOO_SUPERVISION_ONE_FOR_ALL,
    GOO_SUPERVISION_REST_FOR_ONE
} GooSupervisionPolicy;

typedef struct GooSuperviseChild {
    GooTaskFunc func;
    void* arg;
    bool failed;
    GooSupervisor* supervisor;
} GooSuperviseChild;

typedef struct GooSupervisor {
    GooSuperviseChild** children;
    int child_count;
    GooSupervisionPolicy restart_policy;
    int max_restarts;
    int time_window;
    int restart_count;
    time_t last_restart_time;
    pthread_mutex_t mutex;
} GooSupervisor;

// ===== Thread Pool Functions =====

bool goo_thread_pool_init(int thread_count);
void goo_thread_pool_cleanup(void);

bool goo_schedule_task(GooTask* task);

// ===== Goroutine Functions =====

bool goo_goroutine_spawn(GooTaskFunc func, void* arg, GooSupervisor* supervisor);

// ===== Channel Functions =====

GooChannel* goo_channel_create(size_t element_size, size_t capacity, GooChannelPattern type);
void goo_channel_close(GooChannel* channel);
void goo_channel_free(GooChannel* channel);
bool goo_channel_send(GooChannel* channel, void* data);
bool goo_channel_recv(GooChannel* channel, void* data);
bool goo_channel_try_send(GooChannel* channel, void* data);
bool goo_channel_try_recv(GooChannel* channel, void* data);
bool goo_channel_subscribe(GooChannel* pub, GooChannel* sub);

// ===== Supervision Functions =====

GooSupervisor* goo_supervise_create(void);
void goo_supervise_free(GooSupervisor* supervisor);
bool goo_supervise_register(GooSupervisor* supervisor, GooTaskFunc func, void* arg);
void goo_supervise_set_policy(GooSupervisor* supervisor, GooSupervisionPolicy policy, int max_restarts, int time_window);
bool goo_supervise_start(GooSupervisor* supervisor);
void goo_supervise_handle_error(GooSupervisor* supervisor, GooTask* failed_task, void* error_info);
void goo_supervise_restart_child(GooSupervisor* supervisor, int child_index);

// ===== Parallel Execution Functions =====

bool goo_parallel_execute(GooParallelFunc func, void* arg, int num_threads);

// ===== Runtime Context =====

// Moved compiler mode enums here to prevent include conflicts
typedef enum GooCompilationMode {
    GOO_MODE_DEVELOPMENT,
    GOO_MODE_PRODUCTION,
    GOO_MODE_DEBUG,
    GOO_MODE_PROFILE
} GooCompilationMode;

typedef enum GooContextMode {
    GOO_CONTEXT_SAFE,
    GOO_CONTEXT_UNSAFE
} GooContextMode;

struct GooRuntimeContext {
    GooCompilationMode mode;            // Current compilation mode
    GooContextMode context_mode;        // Current context mode (safe, unsafe, etc.)
    struct GooAllocator* allocator;     // Current allocator
    void* goroutine_scheduler;          // Scheduler for goroutines
    void* channel_manager;              // Manager for channels
    void* supervision_system;           // Fault tolerance supervision
    void* capability_manager;           // Manager for capabilities
    void* simd_ctx;                     // Compile-time SIMD context
    bool is_initializing;               // Whether runtime is initializing
    bool is_shutting_down;              // Whether runtime is shutting down
};

// ===== Extended API Functions =====

// Channel creation with additional options
GooChannel* goo_channel_create_with_endpoint(size_t element_size, size_t capacity, 
                                            GooChannelPattern channel_type,
                                            const char* endpoint);

// Send/receive with timeout
bool goo_channel_send_timeout(GooChannel* channel, void* data, int timeout_ms);
bool goo_channel_receive_timeout(GooChannel* channel, void* data_out, int timeout_ms);

// Goroutine with capabilities
bool goo_goroutine_spawn_with_caps(GooTaskFunc func, void* arg, void* capabilities);

// Memory allocation functions
void goo_runtime_set_thread_allocator(struct GooAllocator* allocator);
void* goo_runtime_alloc(size_t size);
void goo_runtime_free(void* ptr);

// Runtime initialization and shutdown
bool goo_runtime_init(void);
void goo_runtime_shutdown(void);
bool goo_runtime_is_shutting_down(void);

#endif // GOO_RUNTIME_H