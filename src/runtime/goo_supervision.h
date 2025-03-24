#ifndef GOO_SUPERVISION_H
#define GOO_SUPERVISION_H

#include <stdbool.h>
#include "goo_runtime.h"

// Supervision restart policies
enum {
    GOO_SUPERVISE_ONE_FOR_ONE = 0,  // Restart only the failed child
    GOO_SUPERVISE_ONE_FOR_ALL,       // Restart all children when one fails
    GOO_SUPERVISE_REST_FOR_ONE       // Restart the failed child and all that depend on it
};

// Create a new supervisor
GooSupervisor* goo_supervise_init(void);

// Free a supervisor and its resources
void goo_supervise_free(GooSupervisor* supervisor);

// Set supervisor name
void goo_supervise_set_name(GooSupervisor* supervisor, const char* name);

// Set shared state with init and cleanup functions
void goo_supervise_set_state(GooSupervisor* supervisor, void* state,
                            GooTaskFunc init_func, GooTaskFunc cleanup_func);

// Allow dynamic child creation after supervisor start
void goo_supervise_allow_dynamic_children(GooSupervisor* supervisor, bool allow);

// Register a child task with the supervisor
bool goo_supervise_register(GooSupervisor* supervisor, GooTaskFunc func, void* arg);

// Set the dependency between two children (child_index depends on depends_on_index)
bool goo_supervise_set_dependency(GooSupervisor* supervisor, int child_index, int depends_on_index);

// Set supervisor policy
void goo_supervise_set_policy(GooSupervisor* supervisor, int policy, int max_restarts, int time_window);

// Start the supervisor
bool goo_supervise_start(GooSupervisor* supervisor);

// Handle errors in supervised tasks
void goo_supervise_handle_error(GooSupervisor* supervisor, GooTask* failed_task, void* error_info);

// Restart a specific child
void goo_supervise_restart_child(GooSupervisor* supervisor, int child_index);

// High-level supervision helper functions
// Create a supervised worker pool (parallel workers with supervision)
GooSupervisor* goo_create_worker_pool(int worker_count, GooTaskFunc worker_func, void* shared_data);

// Create a supervision tree (hierarchical supervision)
GooSupervisor* goo_create_supervision_tree(GooSupervisor** children, int child_count, int policy);

// Create a supervised channel system (channels with supervision and fault tolerance)
GooSupervisor* goo_create_supervised_channels(GooChannel** channels, int channel_count);

#endif // GOO_SUPERVISION_H 