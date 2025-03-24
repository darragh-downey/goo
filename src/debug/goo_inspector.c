#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>
#include <sys/time.h>

#include "goo_inspector.h"

// Max buffer size for trace messages
#define GOO_INSPECTOR_MAX_MSG_LEN 1024

// Max events to store in circular buffer
#define GOO_INSPECTOR_MAX_EVENTS 1000

// Event types for internal tracking
typedef enum {
    GOO_EVENT_LOG = 0,
    GOO_EVENT_CHANNEL = 1,
    GOO_EVENT_SUPERVISOR = 2,
    GOO_EVENT_MEMORY = 3,
    GOO_EVENT_THREAD = 4,
    GOO_EVENT_CUSTOM = 5
} GooEventType;

// Event structure for internal storage
typedef struct {
    GooEventType type;
    struct timeval timestamp;
    GooInspectSeverity severity;
    char* component;
    char* message;
    void* subject;  // Channel, supervisor, etc.
    size_t data_size;
    uint64_t thread_id;
} GooInspectorEvent;

// Inspector structure
struct GooInspector {
    GooInspectorConfig config;
    bool enabled;
    pthread_mutex_t mutex;
    
    // Callbacks
    GooInspectorCallback log_callback;
    void* log_context;
    GooChannelEventCallback channel_callback;
    void* channel_context;
    GooSupervisorEventCallback supervisor_callback;
    void* supervisor_context;
    GooMemoryEventCallback memory_callback;
    void* memory_context;
    GooThreadEventCallback thread_callback;
    void* thread_context;
    
    // Event circular buffer
    GooInspectorEvent* events;
    int event_head;
    int event_count;
    
    // Runtime statistics
    GooRuntimeStats stats;
    
    // Profiling
    struct timeval profile_start;
    char* profile_name;
    bool profiling_active;
};

// Global inspector instance
static GooInspector* g_inspector = NULL;

// Forward declarations of internal functions
static void goo_inspector_record_event(GooInspector* inspector, GooEventType type, 
                                     GooInspectSeverity severity, const char* component, 
                                     const char* message, void* subject, size_t data_size);
static void goo_inspector_update_stats(GooInspector* inspector, GooEventType type, 
                                     void* subject, const char* event);
static uint64_t goo_get_thread_id();
static char* goo_duplicate_string(const char* str);

// Create a new inspector
GooInspector* goo_inspector_create(GooInspectorConfig* config) {
    GooInspector* inspector = (GooInspector*)malloc(sizeof(GooInspector));
    if (!inspector) return NULL;
    
    // Initialize with defaults if no config provided
    if (config) {
        memcpy(&inspector->config, config, sizeof(GooInspectorConfig));
    } else {
        inspector->config.enable_channel_tracing = true;
        inspector->config.enable_supervision_tracing = true;
        inspector->config.enable_memory_tracing = false; // Off by default due to overhead
        inspector->config.enable_thread_tracing = true;
        inspector->config.enable_call_tracing = false;   // Off by default due to overhead
        inspector->config.collect_statistics = true;
        inspector->config.sampling_rate = 10;  // 10% sampling rate by default
        inspector->config.log_level = GOO_INSPECT_INFO;
    }
    
    inspector->enabled = true;
    pthread_mutex_init(&inspector->mutex, NULL);
    
    // Initialize callbacks
    inspector->log_callback = NULL;
    inspector->log_context = NULL;
    inspector->channel_callback = NULL;
    inspector->channel_context = NULL;
    inspector->supervisor_callback = NULL;
    inspector->supervisor_context = NULL;
    inspector->memory_callback = NULL;
    inspector->memory_context = NULL;
    inspector->thread_callback = NULL;
    inspector->thread_context = NULL;
    
    // Initialize event buffer
    inspector->events = (GooInspectorEvent*)calloc(GOO_INSPECTOR_MAX_EVENTS, sizeof(GooInspectorEvent));
    if (!inspector->events) {
        free(inspector);
        return NULL;
    }
    inspector->event_head = 0;
    inspector->event_count = 0;
    
    // Initialize statistics
    memset(&inspector->stats, 0, sizeof(GooRuntimeStats));
    inspector->stats.runtime_ms = 0;
    
    // Initialize profiling
    inspector->profile_name = NULL;
    inspector->profiling_active = false;
    
    // If this is the first inspector, make it global
    if (!g_inspector) {
        g_inspector = inspector;
    }
    
    return inspector;
}

// Destroy an inspector
void goo_inspector_destroy(GooInspector* inspector) {
    if (!inspector) return;
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Free event buffer contents
    if (inspector->events) {
        for (int i = 0; i < GOO_INSPECTOR_MAX_EVENTS; i++) {
            if (inspector->events[i].component) free(inspector->events[i].component);
            if (inspector->events[i].message) free(inspector->events[i].message);
        }
        free(inspector->events);
    }
    
    // Free profiling data
    if (inspector->profile_name) {
        free(inspector->profile_name);
    }
    
    pthread_mutex_unlock(&inspector->mutex);
    pthread_mutex_destroy(&inspector->mutex);
    
    // Update global reference
    if (g_inspector == inspector) {
        g_inspector = NULL;
    }
    
    free(inspector);
}

// Enable or disable the inspector
void goo_inspector_enable(GooInspector* inspector, bool enable) {
    if (!inspector) return;
    inspector->enabled = enable;
}

// Set the global inspector instance
void goo_inspector_set_global(GooInspector* inspector) {
    g_inspector = inspector;
}

// Get the global inspector instance
GooInspector* goo_inspector_get_global(void) {
    return g_inspector;
}

// Set inspector callbacks
void goo_inspector_set_log_callback(GooInspector* inspector, GooInspectorCallback callback, void* context) {
    if (!inspector) return;
    inspector->log_callback = callback;
    inspector->log_context = context;
}

void goo_inspector_set_channel_callback(GooInspector* inspector, GooChannelEventCallback callback, void* context) {
    if (!inspector) return;
    inspector->channel_callback = callback;
    inspector->channel_context = context;
}

void goo_inspector_set_supervisor_callback(GooInspector* inspector, GooSupervisorEventCallback callback, void* context) {
    if (!inspector) return;
    inspector->supervisor_callback = callback;
    inspector->supervisor_context = context;
}

void goo_inspector_set_memory_callback(GooInspector* inspector, GooMemoryEventCallback callback, void* context) {
    if (!inspector) return;
    inspector->memory_callback = callback;
    inspector->memory_context = context;
}

void goo_inspector_set_thread_callback(GooInspector* inspector, GooThreadEventCallback callback, void* context) {
    if (!inspector) return;
    inspector->thread_callback = callback;
    inspector->thread_context = context;
}

// Get runtime statistics
GooRuntimeStats* goo_inspector_get_stats(GooInspector* inspector) {
    if (!inspector) return NULL;
    
    // Return a copy of the stats to prevent race conditions
    GooRuntimeStats* stats = (GooRuntimeStats*)malloc(sizeof(GooRuntimeStats));
    if (!stats) return NULL;
    
    pthread_mutex_lock(&inspector->mutex);
    memcpy(stats, &inspector->stats, sizeof(GooRuntimeStats));
    pthread_mutex_unlock(&inspector->mutex);
    
    return stats;
}

// Channel snapshot API
GooChannelSnapshot* goo_inspector_snapshot_channel(GooInspector* inspector, GooChannel* channel) {
    if (!inspector || !channel) return NULL;
    if (!inspector->enabled) return NULL;
    
    GooChannelSnapshot* snapshot = (GooChannelSnapshot*)malloc(sizeof(GooChannelSnapshot));
    if (!snapshot) return NULL;
    
    // Fill in basic info - in a real implementation, we'd need access to channel internals
    // This is a simplified version
    snapshot->type = GOO_CHANNEL_STANDARD; // Placeholder
    snapshot->element_size = 0;            // Placeholder
    snapshot->capacity = 0;                // Placeholder
    snapshot->current_size = 0;            // Placeholder
    snapshot->total_sends = 0;             // Placeholder
    snapshot->total_receives = 0;          // Placeholder
    snapshot->total_timeouts = 0;          // Placeholder
    snapshot->avg_wait_time_us = 0.0;      // Placeholder
    
    return snapshot;
}

// Supervisor snapshot API
GooSupervisorSnapshot* goo_inspector_snapshot_supervisor(GooInspector* inspector, GooSupervisor* supervisor) {
    if (!inspector || !supervisor) return NULL;
    if (!inspector->enabled) return NULL;
    
    GooSupervisorSnapshot* snapshot = (GooSupervisorSnapshot*)malloc(sizeof(GooSupervisorSnapshot));
    if (!snapshot) return NULL;
    
    // Fill in basic info - in a real implementation, we'd need access to supervisor internals
    // This is a simplified version
    snapshot->name = goo_duplicate_string("unknown"); // Placeholder
    snapshot->child_count = 0;                       // Placeholder
    snapshot->restart_policy = 0;                    // Placeholder
    snapshot->restart_count = 0;                     // Placeholder
    snapshot->max_restarts = 0;                      // Placeholder
    snapshot->last_restart_time = 0;                 // Placeholder
    snapshot->is_started = false;                    // Placeholder
    snapshot->child_failed_status = NULL;            // Placeholder
    
    return snapshot;
}

// Thread snapshot API
GooThreadSnapshot** goo_inspector_snapshot_threads(GooInspector* inspector, int* count) {
    if (!inspector || !count) return NULL;
    if (!inspector->enabled) return NULL;
    
    // Set count to active threads
    *count = inspector->stats.active_threads;
    if (*count <= 0) return NULL;
    
    // Allocate array of thread snapshots
    GooThreadSnapshot** snapshots = (GooThreadSnapshot**)malloc(*count * sizeof(GooThreadSnapshot*));
    if (!snapshots) return NULL;
    
    // In a real implementation, we'd fill these with actual thread data
    // This is a simplified version
    for (int i = 0; i < *count; i++) {
        snapshots[i] = (GooThreadSnapshot*)malloc(sizeof(GooThreadSnapshot));
        if (!snapshots[i]) {
            // Free previously allocated snapshots
            for (int j = 0; j < i; j++) {
                free(snapshots[j]);
            }
            free(snapshots);
            return NULL;
        }
        
        snapshots[i]->thread_id = i + 1;                // Placeholder
        snapshots[i]->name = goo_duplicate_string("thread"); // Placeholder
        snapshots[i]->is_worker = true;                // Placeholder
        snapshots[i]->cpu_time_us = 0;                 // Placeholder
        snapshots[i]->tasks_processed = 0;             // Placeholder
        snapshots[i]->current_task = NULL;             // Placeholder
    }
    
    return snapshots;
}

// Free a snapshot
void goo_inspector_free_snapshot(void* snapshot) {
    if (!snapshot) return;
    
    // The proper implementation would determine snapshot type and free accordingly
    // For this simplified implementation, we'll just free the pointer
    free(snapshot);
}

// Tracing helpers
void goo_inspector_trace_message(GooInspector* inspector, GooInspectSeverity severity, 
                               const char* component, const char* format, ...) {
    if (!inspector || !inspector->enabled) return;
    if (severity < inspector->config.log_level) return;
    
    // Check sampling rate
    if (inspector->config.sampling_rate < 100) {
        int random_val = rand() % 100;
        if (random_val >= inspector->config.sampling_rate) return;
    }
    
    // Format the message
    char message[GOO_INSPECTOR_MAX_MSG_LEN];
    va_list args;
    va_start(args, format);
    vsnprintf(message, GOO_INSPECTOR_MAX_MSG_LEN, format, args);
    va_end(args);
    
    // Record the event
    goo_inspector_record_event(inspector, GOO_EVENT_LOG, severity, component, message, NULL, 0);
    
    // Call the callback if registered
    if (inspector->log_callback) {
        inspector->log_callback(inspector->log_context, severity, component, message);
    }
}

// Profiling control
void goo_inspector_start_profiling(GooInspector* inspector, const char* profile_name) {
    if (!inspector || !inspector->enabled) return;
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Record the start time
    gettimeofday(&inspector->profile_start, NULL);
    
    // Store the profile name
    if (inspector->profile_name) {
        free(inspector->profile_name);
    }
    inspector->profile_name = goo_duplicate_string(profile_name);
    inspector->profiling_active = true;
    
    // Log the profiling start
    char message[GOO_INSPECTOR_MAX_MSG_LEN];
    snprintf(message, GOO_INSPECTOR_MAX_MSG_LEN, "Starting profiling session: %s", profile_name);
    goo_inspector_record_event(inspector, GOO_EVENT_CUSTOM, GOO_INSPECT_INFO, "profiler", message, NULL, 0);
    
    pthread_mutex_unlock(&inspector->mutex);
}

void goo_inspector_stop_profiling(GooInspector* inspector) {
    if (!inspector || !inspector->enabled || !inspector->profiling_active) return;
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Get the end time and calculate duration
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    
    uint64_t start_us = (uint64_t)inspector->profile_start.tv_sec * 1000000 + inspector->profile_start.tv_usec;
    uint64_t end_us = (uint64_t)end_time.tv_sec * 1000000 + end_time.tv_usec;
    uint64_t duration_us = end_us - start_us;
    
    // Log the profiling end
    char message[GOO_INSPECTOR_MAX_MSG_LEN];
    snprintf(message, GOO_INSPECTOR_MAX_MSG_LEN, 
             "Profiling session completed: %s (duration: %.3f ms)", 
             inspector->profile_name, duration_us / 1000.0);
    
    goo_inspector_record_event(inspector, GOO_EVENT_CUSTOM, GOO_INSPECT_INFO, "profiler", message, NULL, 0);
    
    inspector->profiling_active = false;
    
    pthread_mutex_unlock(&inspector->mutex);
}

void goo_inspector_mark_event(GooInspector* inspector, const char* event_name) {
    if (!inspector || !inspector->enabled) return;
    
    // Log the event marker
    char message[GOO_INSPECTOR_MAX_MSG_LEN];
    snprintf(message, GOO_INSPECTOR_MAX_MSG_LEN, "Event marker: %s", event_name);
    
    goo_inspector_record_event(inspector, GOO_EVENT_CUSTOM, GOO_INSPECT_INFO, "marker", message, NULL, 0);
}

// Breakpoint for debugger integration
void goo_inspector_breakpoint(GooInspector* inspector, const char* reason) {
    if (!inspector || !inspector->enabled) return;
    
    // Log the breakpoint
    char message[GOO_INSPECTOR_MAX_MSG_LEN];
    snprintf(message, GOO_INSPECTOR_MAX_MSG_LEN, "Breakpoint: %s", reason);
    
    goo_inspector_record_event(inspector, GOO_EVENT_CUSTOM, GOO_INSPECT_INFO, "debugger", message, NULL, 0);
    
    // In a real implementation, this would integrate with a debugger
    // For now, just log the breakpoint
    fprintf(stderr, "BREAKPOINT: %s\n", reason);
}

// Visual tracing output (for message flows)
void goo_inspector_visualize_message_flow(GooInspector* inspector, const char* output_file) {
    if (!inspector || !inspector->enabled || !output_file) return;
    
    // In a real implementation, this would generate a visualization file (e.g., DOT graph)
    // For this simplified implementation, we'll just create a text file with basic info
    FILE* file = fopen(output_file, "w");
    if (!file) return;
    
    fprintf(file, "Goo Message Flow Visualization\n");
    fprintf(file, "==============================\n\n");
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Write basic statistics
    fprintf(file, "Messages sent: %llu\n", (unsigned long long)inspector->stats.messages_sent);
    fprintf(file, "Messages received: %llu\n", (unsigned long long)inspector->stats.messages_received);
    fprintf(file, "Active channels: %d\n\n", inspector->stats.active_channels);
    
    // Write recent events related to channels
    fprintf(file, "Recent Channel Events:\n");
    fprintf(file, "---------------------\n");
    
    int count = 0;
    int index = (inspector->event_head - 1 + GOO_INSPECTOR_MAX_EVENTS) % GOO_INSPECTOR_MAX_EVENTS;
    
    while (count < inspector->event_count && count < GOO_INSPECTOR_MAX_EVENTS) {
        if (inspector->events[index].type == GOO_EVENT_CHANNEL) {
            fprintf(file, "[%ld.%06ld] %s: %s\n", 
                   (long)inspector->events[index].timestamp.tv_sec,
                   (long)inspector->events[index].timestamp.tv_usec,
                   inspector->events[index].component,
                   inspector->events[index].message);
        }
        
        index = (index - 1 + GOO_INSPECTOR_MAX_EVENTS) % GOO_INSPECTOR_MAX_EVENTS;
        count++;
    }
    
    pthread_mutex_unlock(&inspector->mutex);
    
    fclose(file);
}

void goo_inspector_visualize_supervision_tree(GooInspector* inspector, const char* output_file) {
    if (!inspector || !inspector->enabled || !output_file) return;
    
    // In a real implementation, this would generate a visualization file (e.g., DOT graph)
    // For this simplified implementation, we'll just create a text file with basic info
    FILE* file = fopen(output_file, "w");
    if (!file) return;
    
    fprintf(file, "Goo Supervision Tree Visualization\n");
    fprintf(file, "=================================\n\n");
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Write basic statistics
    fprintf(file, "Active supervisors: %d\n", inspector->stats.active_supervisors);
    fprintf(file, "Supervised tasks: %d\n", inspector->stats.supervised_tasks);
    fprintf(file, "Task restarts: %d\n\n", inspector->stats.task_restarts);
    
    // Write recent events related to supervision
    fprintf(file, "Recent Supervision Events:\n");
    fprintf(file, "------------------------\n");
    
    int count = 0;
    int index = (inspector->event_head - 1 + GOO_INSPECTOR_MAX_EVENTS) % GOO_INSPECTOR_MAX_EVENTS;
    
    while (count < inspector->event_count && count < GOO_INSPECTOR_MAX_EVENTS) {
        if (inspector->events[index].type == GOO_EVENT_SUPERVISOR) {
            fprintf(file, "[%ld.%06ld] %s: %s\n", 
                   (long)inspector->events[index].timestamp.tv_sec,
                   (long)inspector->events[index].timestamp.tv_usec,
                   inspector->events[index].component,
                   inspector->events[index].message);
        }
        
        index = (index - 1 + GOO_INSPECTOR_MAX_EVENTS) % GOO_INSPECTOR_MAX_EVENTS;
        count++;
    }
    
    pthread_mutex_unlock(&inspector->mutex);
    
    fclose(file);
}

// Internal functions

// Record an event in the circular buffer
static void goo_inspector_record_event(GooInspector* inspector, GooEventType type, 
                                     GooInspectSeverity severity, const char* component, 
                                     const char* message, void* subject, size_t data_size) {
    if (!inspector || !inspector->enabled) return;
    
    pthread_mutex_lock(&inspector->mutex);
    
    // Update stats based on event
    goo_inspector_update_stats(inspector, type, subject, message);
    
    // Get current time
    struct timeval now;
    gettimeofday(&now, NULL);
    
    // Get current thread ID
    uint64_t thread_id = goo_get_thread_id();
    
    // If we need to overwrite an old event, free its strings
    if (inspector->event_count == GOO_INSPECTOR_MAX_EVENTS) {
        if (inspector->events[inspector->event_head].component) {
            free(inspector->events[inspector->event_head].component);
        }
        if (inspector->events[inspector->event_head].message) {
            free(inspector->events[inspector->event_head].message);
        }
    }
    
    // Record the new event
    inspector->events[inspector->event_head].type = type;
    inspector->events[inspector->event_head].timestamp = now;
    inspector->events[inspector->event_head].severity = severity;
    inspector->events[inspector->event_head].component = goo_duplicate_string(component);
    inspector->events[inspector->event_head].message = goo_duplicate_string(message);
    inspector->events[inspector->event_head].subject = subject;
    inspector->events[inspector->event_head].data_size = data_size;
    inspector->events[inspector->event_head].thread_id = thread_id;
    
    // Update circular buffer pointers
    inspector->event_head = (inspector->event_head + 1) % GOO_INSPECTOR_MAX_EVENTS;
    if (inspector->event_count < GOO_INSPECTOR_MAX_EVENTS) {
        inspector->event_count++;
    }
    
    pthread_mutex_unlock(&inspector->mutex);
}

// Update statistics based on events
static void goo_inspector_update_stats(GooInspector* inspector, GooEventType type, 
                                     void* subject, const char* event) {
    // This function would be responsible for updating various statistics
    // based on the type of event. For simplicity, we'll just handle a few cases.
    
    switch (type) {
        case GOO_EVENT_CHANNEL:
            inspector->stats.channel_operations++;
            if (strstr(event, "send") || strstr(event, "publish") || strstr(event, "push")) {
                inspector->stats.messages_sent++;
            }
            if (strstr(event, "recv") || strstr(event, "subscribe") || strstr(event, "pull")) {
                inspector->stats.messages_received++;
            }
            break;
            
        case GOO_EVENT_SUPERVISOR:
            if (strstr(event, "restart")) {
                inspector->stats.task_restarts++;
            }
            break;
            
        case GOO_EVENT_MEMORY:
            if (strstr(event, "allocate")) {
                inspector->stats.total_allocations++;
            }
            if (strstr(event, "free")) {
                inspector->stats.total_frees++;
            }
            break;
            
        case GOO_EVENT_THREAD:
            if (strstr(event, "create")) {
                inspector->stats.active_threads++;
                inspector->stats.total_threads_created++;
                if (inspector->stats.active_threads > inspector->stats.peak_threads) {
                    inspector->stats.peak_threads = inspector->stats.active_threads;
                }
            }
            if (strstr(event, "exit")) {
                inspector->stats.active_threads--;
            }
            break;
            
        default:
            break;
    }
}

// Helper to get the current thread ID
static uint64_t goo_get_thread_id() {
    return (uint64_t)pthread_self();
}

// Helper to duplicate a string
static char* goo_duplicate_string(const char* str) {
    if (!str) return NULL;
    
    size_t len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    
    strcpy(result, str);
    return result;
} 