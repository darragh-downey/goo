#ifndef GOO_INSPECTOR_H
#define GOO_INSPECTOR_H

#include <stdbool.h>
#include <stdint.h>
#include "goo_runtime.h"
#include "goo_channels.h"
#include "goo_channels_advanced.h"
#include "goo_supervision.h"

// Inspector handle
typedef struct GooInspector GooInspector;

// Inspector severity levels for events and logs
typedef enum {
    GOO_INSPECT_DEBUG = 0,
    GOO_INSPECT_INFO = 1,
    GOO_INSPECT_WARNING = 2,
    GOO_INSPECT_ERROR = 3,
    GOO_INSPECT_CRITICAL = 4
} GooInspectSeverity;

// Inspector callback types
typedef void (*GooInspectorCallback)(void* context, GooInspectSeverity severity, const char* component, const char* message);
typedef void (*GooChannelEventCallback)(void* context, GooChannel* channel, const char* event, size_t data_size);
typedef void (*GooSupervisorEventCallback)(void* context, GooSupervisor* supervisor, const char* event, void* details);
typedef void (*GooMemoryEventCallback)(void* context, void* ptr, size_t size, const char* operation);
typedef void (*GooThreadEventCallback)(void* context, uint64_t thread_id, const char* event);

// Inspector configuration
typedef struct {
    bool enable_channel_tracing;     // Trace channel operations
    bool enable_supervision_tracing; // Trace supervisor events
    bool enable_memory_tracing;      // Trace memory allocations/frees
    bool enable_thread_tracing;      // Trace thread creation/termination
    bool enable_call_tracing;        // Trace function calls
    bool collect_statistics;         // Collect runtime statistics
    int sampling_rate;               // 1-100, percentage of events to sample
    int log_level;                   // Minimum severity level to log
} GooInspectorConfig;

// Runtime statistics
typedef struct {
    // Thread statistics
    int active_threads;
    int total_threads_created;
    int peak_threads;
    
    // Memory statistics
    size_t current_allocated_bytes;
    size_t peak_allocated_bytes;
    uint64_t total_allocations;
    uint64_t total_frees;
    
    // Channel statistics
    int active_channels;
    uint64_t messages_sent;
    uint64_t messages_received;
    uint64_t channel_operations;
    
    // Supervision statistics
    int active_supervisors;
    int supervised_tasks;
    int task_restarts;
    int supervisor_restarts;
    
    // Performance statistics
    double cpu_usage_percent;
    double memory_usage_percent;
    uint64_t runtime_ms;
} GooRuntimeStats;

// Channel snapshot
typedef struct {
    GooChannelType type;
    size_t element_size;
    size_t capacity;
    size_t current_size;
    uint64_t total_sends;
    uint64_t total_receives;
    uint64_t total_timeouts;
    double avg_wait_time_us;
} GooChannelSnapshot;

// Supervisor snapshot
typedef struct {
    char* name;
    int child_count;
    int restart_policy;
    int restart_count;
    int max_restarts;
    time_t last_restart_time;
    bool is_started;
    bool* child_failed_status;
} GooSupervisorSnapshot;

// Thread snapshot
typedef struct {
    uint64_t thread_id;
    char* name;
    bool is_worker;
    uint64_t cpu_time_us;
    uint64_t tasks_processed;
    void* current_task;
} GooThreadSnapshot;

// === Inspector API ===

// Create a new inspector
GooInspector* goo_inspector_create(GooInspectorConfig* config);

// Destroy an inspector
void goo_inspector_destroy(GooInspector* inspector);

// Enable or disable the inspector
void goo_inspector_enable(GooInspector* inspector, bool enable);

// Set the global inspector instance
void goo_inspector_set_global(GooInspector* inspector);

// Get the global inspector instance
GooInspector* goo_inspector_get_global(void);

// Set inspector callbacks
void goo_inspector_set_log_callback(GooInspector* inspector, GooInspectorCallback callback, void* context);
void goo_inspector_set_channel_callback(GooInspector* inspector, GooChannelEventCallback callback, void* context);
void goo_inspector_set_supervisor_callback(GooInspector* inspector, GooSupervisorEventCallback callback, void* context);
void goo_inspector_set_memory_callback(GooInspector* inspector, GooMemoryEventCallback callback, void* context);
void goo_inspector_set_thread_callback(GooInspector* inspector, GooThreadEventCallback callback, void* context);

// Get runtime statistics
GooRuntimeStats* goo_inspector_get_stats(GooInspector* inspector);

// Snapshot API
GooChannelSnapshot* goo_inspector_snapshot_channel(GooInspector* inspector, GooChannel* channel);
GooSupervisorSnapshot* goo_inspector_snapshot_supervisor(GooInspector* inspector, GooSupervisor* supervisor);
GooThreadSnapshot** goo_inspector_snapshot_threads(GooInspector* inspector, int* count);
void goo_inspector_free_snapshot(void* snapshot);

// Tracing helpers
void goo_inspector_trace_message(GooInspector* inspector, GooInspectSeverity severity, 
                                const char* component, const char* format, ...);

// Profiling control
void goo_inspector_start_profiling(GooInspector* inspector, const char* profile_name);
void goo_inspector_stop_profiling(GooInspector* inspector);
void goo_inspector_mark_event(GooInspector* inspector, const char* event_name);

// Function call tracing
#define GOO_TRACE_FUNCTION_ENTRY(inspector) \
    goo_inspector_trace_message(inspector, GOO_INSPECT_DEBUG, __FILE__, "Entering %s", __func__)

#define GOO_TRACE_FUNCTION_EXIT(inspector) \
    goo_inspector_trace_message(inspector, GOO_INSPECT_DEBUG, __FILE__, "Exiting %s", __func__)

// Breakpoint for debugger integration
void goo_inspector_breakpoint(GooInspector* inspector, const char* reason);

// Visual tracing output (for message flows)
void goo_inspector_visualize_message_flow(GooInspector* inspector, const char* output_file);
void goo_inspector_visualize_supervision_tree(GooInspector* inspector, const char* output_file);

#endif // GOO_INSPECTOR_H 