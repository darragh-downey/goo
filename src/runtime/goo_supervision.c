#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "goo_runtime.h"
#include "goo_error.h"

// Supervisor structure
struct GooSupervisor {
    GooSuperviseChild** children;
    int child_count;
    int restart_policy;
    int max_restarts;
    int time_window;
    int restart_count;
    time_t last_restart_time;
    pthread_mutex_t mutex;
    bool is_started;
    
    // State tracking
    char* name;        // Supervisor name
    void* state;       // Optional shared state
    GooTaskFunc init_func;   // Initialize state
    GooTaskFunc cleanup_func; // Cleanup state
    
    // Dependencies
    int** child_deps;  // Child dependencies matrix
    
    // Dynamic child creation
    bool dynamic_children; // Whether children can be added after start
};

// Child task wrapper
typedef struct {
    GooSuperviseChild* child;
    int child_index;
} GooChildTask;

// Static forward declarations
static void* goo_supervise_child_runner(void* arg);
static void goo_supervise_restart_child_and_dependents(GooSupervisor* supervisor, int child_index);
static bool goo_build_dependency_tree(GooSupervisor* supervisor);
static void goo_free_dependency_tree(GooSupervisor* supervisor);

// Create a new supervisor
GooSupervisor* goo_supervise_init(void) {
    GooSupervisor* supervisor = (GooSupervisor*)malloc(sizeof(GooSupervisor));
    if (!supervisor) {
        return NULL;
    }
    
    supervisor->children = NULL;
    supervisor->child_count = 0;
    supervisor->restart_policy = GOO_SUPERVISE_ONE_FOR_ONE;
    supervisor->max_restarts = 3;
    supervisor->time_window = 60; // 60 seconds
    supervisor->restart_count = 0;
    supervisor->last_restart_time = 0;
    supervisor->is_started = false;
    supervisor->name = strdup("anonymous_supervisor");
    supervisor->state = NULL;
    supervisor->init_func = NULL;
    supervisor->cleanup_func = NULL;
    supervisor->child_deps = NULL;
    supervisor->dynamic_children = false;
    
    if (pthread_mutex_init(&supervisor->mutex, NULL) != 0) {
        free(supervisor->name);
        free(supervisor);
        return NULL;
    }
    
    return supervisor;
}

// Free a supervisor and its resources
void goo_supervise_free(GooSupervisor* supervisor) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Cleanup any shared state
    if (supervisor->cleanup_func && supervisor->state) {
        supervisor->cleanup_func(supervisor->state);
    }
    
    // Free children
    for (int i = 0; i < supervisor->child_count; i++) {
        if (supervisor->children[i]) {
            free(supervisor->children[i]);
        }
    }
    
    free(supervisor->children);
    
    // Free dependency tree
    goo_free_dependency_tree(supervisor);
    
    // Free name
    if (supervisor->name) {
        free(supervisor->name);
    }
    
    pthread_mutex_unlock(&supervisor->mutex);
    pthread_mutex_destroy(&supervisor->mutex);
    
    free(supervisor);
}

// Set supervisor name
void goo_supervise_set_name(GooSupervisor* supervisor, const char* name) {
    if (!supervisor || !name) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    if (supervisor->name) {
        free(supervisor->name);
    }
    
    supervisor->name = strdup(name);
    
    pthread_mutex_unlock(&supervisor->mutex);
}

// Set shared state with init and cleanup functions
void goo_supervise_set_state(GooSupervisor* supervisor, void* state,
                            GooTaskFunc init_func, GooTaskFunc cleanup_func) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    supervisor->state = state;
    supervisor->init_func = init_func;
    supervisor->cleanup_func = cleanup_func;
    
    pthread_mutex_unlock(&supervisor->mutex);
}

// Allow dynamic child creation after supervisor start
void goo_supervise_allow_dynamic_children(GooSupervisor* supervisor, bool allow) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    supervisor->dynamic_children = allow;
    pthread_mutex_unlock(&supervisor->mutex);
}

// Register a child task with the supervisor
bool goo_supervise_register(GooSupervisor* supervisor, GooTaskFunc func, void* arg) {
    if (!supervisor || !func) return false;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Check if we can add children (either not started or dynamic children enabled)
    if (supervisor->is_started && !supervisor->dynamic_children) {
        pthread_mutex_unlock(&supervisor->mutex);
        return false;
    }
    
    // Create a new child
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
    supervisor->children[supervisor->child_count] = child;
    supervisor->child_count++;
    
    // If the supervisor is already running, start this child immediately
    if (supervisor->is_started) {
        GooChildTask* task = (GooChildTask*)malloc(sizeof(GooChildTask));
        if (task) {
            task->child = child;
            task->child_index = supervisor->child_count - 1;
            
            // Schedule the child task
            GooTask goo_task = {
                .func = goo_supervise_child_runner,
                .arg = task,
                .supervisor = supervisor
            };
            
            if (!goo_schedule_task(&goo_task)) {
                free(task);
                pthread_mutex_unlock(&supervisor->mutex);
                return false;
            }
        } else {
            pthread_mutex_unlock(&supervisor->mutex);
            return false;
        }
    }
    
    pthread_mutex_unlock(&supervisor->mutex);
    return true;
}

// Set the dependency between two children
bool goo_supervise_set_dependency(GooSupervisor* supervisor, int child_index, int depends_on_index) {
    if (!supervisor || child_index < 0 || depends_on_index < 0 ||
        child_index >= supervisor->child_count || depends_on_index >= supervisor->child_count) {
        return false;
    }
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Create dependency matrix if it doesn't exist
    if (!supervisor->child_deps) {
        if (!goo_build_dependency_tree(supervisor)) {
            pthread_mutex_unlock(&supervisor->mutex);
            return false;
        }
    }
    
    // Set the dependency
    supervisor->child_deps[child_index][depends_on_index] = 1;
    
    pthread_mutex_unlock(&supervisor->mutex);
    return true;
}

// Set supervisor policy
void goo_supervise_set_policy(GooSupervisor* supervisor, int policy, int max_restarts, int time_window) {
    if (!supervisor) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    supervisor->restart_policy = policy;
    supervisor->max_restarts = max_restarts;
    supervisor->time_window = time_window;
    
    pthread_mutex_unlock(&supervisor->mutex);
}

// Start the supervisor
bool goo_supervise_start(GooSupervisor* supervisor) {
    if (!supervisor) return false;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Already started?
    if (supervisor->is_started) {
        pthread_mutex_unlock(&supervisor->mutex);
        return true;
    }
    
    // Initialize the shared state if needed
    if (supervisor->init_func && supervisor->state) {
        supervisor->init_func(supervisor->state);
    }
    
    // Build dependency tree if not already built
    if (supervisor->restart_policy == GOO_SUPERVISE_REST_FOR_ONE && !supervisor->child_deps) {
        if (!goo_build_dependency_tree(supervisor)) {
            pthread_mutex_unlock(&supervisor->mutex);
            return false;
        }
    }
    
    // Start each child in a separate task
    for (int i = 0; i < supervisor->child_count; i++) {
        GooChildTask* task = (GooChildTask*)malloc(sizeof(GooChildTask));
        if (!task) {
            pthread_mutex_unlock(&supervisor->mutex);
            return false;
        }
        
        task->child = supervisor->children[i];
        task->child_index = i;
        
        // Schedule the child task
        GooTask goo_task = {
            .func = goo_supervise_child_runner,
            .arg = task,
            .supervisor = supervisor
        };
        
        if (!goo_schedule_task(&goo_task)) {
            free(task);
            pthread_mutex_unlock(&supervisor->mutex);
            return false;
        }
    }
    
    supervisor->is_started = true;
    
    pthread_mutex_unlock(&supervisor->mutex);
    return true;
}

// Handle errors in supervised tasks
void goo_supervise_handle_error(GooSupervisor* supervisor, GooTask* failed_task, void* error_info) {
    if (!supervisor || !failed_task) return;
    
    pthread_mutex_lock(&supervisor->mutex);
    
    // Find the child that failed
    int failed_child_index = -1;
    for (int i = 0; i < supervisor->child_count; i++) {
        if (supervisor->children[i] && 
            supervisor->children[i]->func == failed_task->func && 
            supervisor->children[i]->arg == failed_task->arg) {
            failed_child_index = i;
            supervisor->children[i]->failed = true;
            break;
        }
    }
    
    if (failed_child_index == -1) {
        pthread_mutex_unlock(&supervisor->mutex);
        return;
    }
    
    // Update restart tracking
    time_t now = time(NULL);
    if (now - supervisor->last_restart_time > supervisor->time_window) {
        // Reset restart count if time window has passed
        supervisor->restart_count = 0;
        supervisor->last_restart_time = now;
    }
    
    supervisor->restart_count++;
    
    // Check if we've exceeded max restarts
    if (supervisor->restart_count > supervisor->max_restarts) {
        fprintf(stderr, "Supervisor %s: exceeded max restarts (%d) in time window (%d seconds)\n",
                supervisor->name, supervisor->max_restarts, supervisor->time_window);
        pthread_mutex_unlock(&supervisor->mutex);
        return;
    }
    
    // Apply restart policy
    switch (supervisor->restart_policy) {
        case GOO_SUPERVISE_ONE_FOR_ONE:
            // Restart only the failed child
            goo_supervise_restart_child(supervisor, failed_child_index);
            break;
            
        case GOO_SUPERVISE_ONE_FOR_ALL:
            // Restart all children
            for (int i = 0; i < supervisor->child_count; i++) {
                goo_supervise_restart_child(supervisor, i);
            }
            break;
            
        case GOO_SUPERVISE_REST_FOR_ONE:
            // Restart the failed child and all that depend on it
            goo_supervise_restart_child_and_dependents(supervisor, failed_child_index);
            break;
    }
    
    pthread_mutex_unlock(&supervisor->mutex);
}

// Child task runner function
static void* goo_supervise_child_runner(void* arg) {
    GooChildTask* task = (GooChildTask*)arg;
    GooSuperviseChild* child = task->child;
    int child_index = task->child_index;
    
    // Free the task structure
    free(task);
    
    // Run the child function
    child->func(child->arg);
    
    // If we get here normally (without error), mark the child as not failed
    pthread_mutex_lock(&child->supervisor->mutex);
    if (child_index < child->supervisor->child_count && 
        child->supervisor->children[child_index] == child) {
        child->failed = false;
    }
    pthread_mutex_unlock(&child->supervisor->mutex);
    
    return NULL;
}

// Restart a specific child
void goo_supervise_restart_child(GooSupervisor* supervisor, int child_index) {
    if (!supervisor || child_index < 0 || child_index >= supervisor->child_count) return;
    
    // Create a new task for the child
    GooChildTask* task = (GooChildTask*)malloc(sizeof(GooChildTask));
    if (!task) return;
    
    task->child = supervisor->children[child_index];
    task->child_index = child_index;
    
    // Reset failed flag
    supervisor->children[child_index]->failed = false;
    
    // Schedule the child task
    GooTask goo_task = {
        .func = goo_supervise_child_runner,
        .arg = task,
        .supervisor = supervisor
    };
    
    if (!goo_schedule_task(&goo_task)) {
        free(task);
    }
}

// Build the dependency tree (matrix)
static bool goo_build_dependency_tree(GooSupervisor* supervisor) {
    if (!supervisor) return false;
    
    // Allocate the dependency matrix
    supervisor->child_deps = (int**)malloc(supervisor->child_count * sizeof(int*));
    if (!supervisor->child_deps) {
        return false;
    }
    
    for (int i = 0; i < supervisor->child_count; i++) {
        supervisor->child_deps[i] = (int*)calloc(supervisor->child_count, sizeof(int));
        if (!supervisor->child_deps[i]) {
            // Free already allocated rows
            for (int j = 0; j < i; j++) {
                free(supervisor->child_deps[j]);
            }
            free(supervisor->child_deps);
            supervisor->child_deps = NULL;
            return false;
        }
    }
    
    return true;
}

// Free the dependency tree
static void goo_free_dependency_tree(GooSupervisor* supervisor) {
    if (!supervisor || !supervisor->child_deps) return;
    
    for (int i = 0; i < supervisor->child_count; i++) {
        if (supervisor->child_deps[i]) {
            free(supervisor->child_deps[i]);
        }
    }
    
    free(supervisor->child_deps);
    supervisor->child_deps = NULL;
}

// Restart a child and all dependents
static void goo_supervise_restart_child_and_dependents(GooSupervisor* supervisor, int child_index) {
    if (!supervisor || !supervisor->child_deps) return;
    
    // First restart the specified child
    goo_supervise_restart_child(supervisor, child_index);
    
    // Then find and restart all dependencies
    for (int i = 0; i < supervisor->child_count; i++) {
        if (supervisor->child_deps[i][child_index]) {
            // This child depends on the one that failed, restart it
            goo_supervise_restart_child(supervisor, i);
            
            // Recursively restart any dependents of this child
            goo_supervise_restart_child_and_dependents(supervisor, i);
        }
    }
} 