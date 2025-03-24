#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "goo_runtime.h"
#include "goo_channels.h"
#include "goo_channels_advanced.h"
#include "goo_supervision.h"
#include "goo_inspector.h"

// Volatile flag for graceful shutdown
volatile sig_atomic_t running = 1;

// Signal handler for clean shutdown
void handle_sigint(int sig) {
    printf("Received signal %d, shutting down...\n", sig);
    running = 0;
}

// Custom log callback function
void log_callback(void* context, GooInspectSeverity severity, const char* component, const char* message) {
    const char* severity_str = "UNKNOWN";
    
    switch (severity) {
        case GOO_INSPECT_DEBUG:
            severity_str = "DEBUG";
            break;
        case GOO_INSPECT_INFO:
            severity_str = "INFO";
            break;
        case GOO_INSPECT_WARNING:
            severity_str = "WARNING";
            break;
        case GOO_INSPECT_ERROR:
            severity_str = "ERROR";
            break;
        case GOO_INSPECT_CRITICAL:
            severity_str = "CRITICAL";
            break;
    }
    
    printf("[%s] %s: %s\n", severity_str, component, message);
}

// Custom channel event callback
void channel_callback(void* context, GooChannel* channel, const char* event, size_t data_size) {
    printf("Channel Event: %s (Size: %zu)\n", event, data_size);
}

// Worker thread function
void* worker_thread(void* arg) {
    int id = *((int*)arg);
    GooInspector* inspector = goo_inspector_get_global();
    
    printf("Worker %d started\n", id);
    
    // Create a channel for demonstration
    GooChannel* channel = goo_channel_create(sizeof(int), 10, GOO_CHANNEL_STANDARD);
    
    // Trace the creation
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "worker", 
                               "Worker %d created a channel", id);
    
    // Start profiling
    goo_inspector_start_profiling(inspector, "worker_operations");
    
    // Perform some operations
    for (int i = 0; i < 100 && running; i++) {
        // Trace function entry
        GOO_TRACE_FUNCTION_ENTRY(inspector);
        
        // Send a value
        int value = i + (id * 1000);
        goo_channel_send(channel, &value, 0);
        
        goo_inspector_trace_message(inspector, GOO_INSPECT_DEBUG, "worker", 
                                  "Worker %d sent value %d", id, value);
        
        // Mark event for profiling
        goo_inspector_mark_event(inspector, "channel_send");
        
        // Receive a value
        int received;
        if (goo_channel_recv(channel, &received, 0)) {
            goo_inspector_trace_message(inspector, GOO_INSPECT_DEBUG, "worker", 
                                      "Worker %d received value %d", id, received);
        }
        
        // Trace function exit
        GOO_TRACE_FUNCTION_EXIT(inspector);
        
        // Sleep a bit
        usleep(10000);
    }
    
    // Stop profiling
    goo_inspector_stop_profiling(inspector);
    
    // Clean up
    goo_channel_free(channel);
    
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "worker", 
                               "Worker %d finished", id);
    
    printf("Worker %d completed\n", id);
    return NULL;
}

// Supervisor task function
void supervisor_task(void* arg) {
    GooInspector* inspector = goo_inspector_get_global();
    
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "supervisor", 
                               "Supervisor task started");
    
    // Random failure for demonstration purposes
    if (rand() % 5 == 0) {
        goo_inspector_trace_message(inspector, GOO_INSPECT_ERROR, "supervisor", 
                                   "Supervisor task failing intentionally for demonstration");
        
        // Trigger a breakpoint
        goo_inspector_breakpoint(inspector, "Intentional task failure");
        
        // Simulate an error
        exit(1);
    }
    
    // Sleep a bit
    sleep(1);
    
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "supervisor", 
                               "Supervisor task completed successfully");
}

// Functions to create channel visualization
void generate_visualizations(GooInspector* inspector) {
    // Generate channel visualization
    goo_inspector_visualize_message_flow(inspector, "message_flow.txt");
    printf("Message flow visualization generated to message_flow.txt\n");
    
    // Generate supervision tree visualization
    goo_inspector_visualize_supervision_tree(inspector, "supervision_tree.txt");
    printf("Supervision tree visualization generated to supervision_tree.txt\n");
}

// Print a summary of runtime statistics
void print_runtime_stats(GooInspector* inspector) {
    if (!inspector) return;
    
    GooRuntimeStats* stats = goo_inspector_get_stats(inspector);
    if (!stats) return;
    
    printf("\nRuntime Statistics:\n");
    printf("------------------\n");
    printf("Active threads: %d (peak: %d, total created: %d)\n", 
           stats->active_threads, stats->peak_threads, stats->total_threads_created);
    
    printf("Memory: Current %zu bytes, Peak %zu bytes\n", 
           stats->current_allocated_bytes, stats->peak_allocated_bytes);
    
    printf("Allocations: %llu, Frees: %llu\n", 
           (unsigned long long)stats->total_allocations, 
           (unsigned long long)stats->total_frees);
    
    printf("Channels: Active %d, Messages sent: %llu, received: %llu\n", 
           stats->active_channels, 
           (unsigned long long)stats->messages_sent, 
           (unsigned long long)stats->messages_received);
    
    printf("Supervision: Active supervisors: %d, Tasks: %d, Restarts: %d\n", 
           stats->active_supervisors, stats->supervised_tasks, stats->task_restarts);
    
    printf("Performance: CPU: %.1f%%, Memory: %.1f%%, Runtime: %.1f sec\n", 
           stats->cpu_usage_percent, stats->memory_usage_percent, stats->runtime_ms / 1000.0);
    
    free(stats);
}

// Main function
int main() {
    // Set up signal handler
    signal(SIGINT, handle_sigint);
    
    // Initialize the Goo runtime
    goo_runtime_init(2);
    goo_thread_pool_init(4);
    
    printf("Runtime Inspector Demo\n");
    printf("=====================\n\n");
    
    // Create inspector with default configuration
    GooInspectorConfig config = {
        .enable_channel_tracing = true,
        .enable_supervision_tracing = true,
        .enable_memory_tracing = true,
        .enable_thread_tracing = true,
        .enable_call_tracing = true,
        .collect_statistics = true,
        .sampling_rate = 100,  // 100% sampling for demo purposes
        .log_level = GOO_INSPECT_DEBUG
    };
    
    GooInspector* inspector = goo_inspector_create(&config);
    goo_inspector_set_global(inspector);
    
    // Register callbacks
    goo_inspector_set_log_callback(inspector, log_callback, NULL);
    goo_inspector_set_channel_callback(inspector, channel_callback, NULL);
    
    // Log the start
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "main", 
                               "Inspector demo starting");
    
    // Create a supervisor
    GooSupervisor* supervisor = goo_supervise_init();
    goo_supervise_set_name(supervisor, "DemoSupervisor");
    goo_supervise_set_policy(supervisor, GOO_SUPERVISE_ONE_FOR_ONE, 5, 60);
    
    // Register supervised tasks
    for (int i = 0; i < 3; i++) {
        goo_supervise_register(supervisor, supervisor_task, NULL);
    }
    
    // Start the supervisor
    goo_supervise_start(supervisor);
    
    // Create worker threads
    pthread_t workers[3];
    int worker_ids[3] = {1, 2, 3};
    
    for (int i = 0; i < 3; i++) {
        goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "main", 
                                   "Creating worker thread %d", i + 1);
        
        pthread_create(&workers[i], NULL, worker_thread, &worker_ids[i]);
    }
    
    // Demo loop
    printf("Demo running. Press Ctrl+C to exit.\n");
    
    int iterations = 0;
    while (running && iterations < 10) {
        // Take a channel snapshot (dummy for demo)
        GooChannel* dummy_channel = goo_channel_create(sizeof(int), 5, GOO_CHANNEL_STANDARD);
        GooChannelSnapshot* snapshot = goo_inspector_snapshot_channel(inspector, dummy_channel);
        
        goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "main", 
                                   "Took channel snapshot (iteration %d)", iterations);
        
        goo_inspector_free_snapshot(snapshot);
        goo_channel_free(dummy_channel);
        
        // Sleep a bit
        sleep(1);
        iterations++;
    }
    
    // Generate visualizations
    generate_visualizations(inspector);
    
    // Print runtime statistics
    print_runtime_stats(inspector);
    
    // Clean up
    goo_inspector_trace_message(inspector, GOO_INSPECT_INFO, "main", 
                               "Inspector demo shutting down");
    
    for (int i = 0; i < 3; i++) {
        pthread_join(workers[i], NULL);
    }
    
    goo_supervise_free(supervisor);
    goo_inspector_destroy(inspector);
    
    goo_thread_pool_cleanup();
    goo_runtime_shutdown();
    
    printf("\nInspector demo completed successfully!\n");
    return 0;
} 