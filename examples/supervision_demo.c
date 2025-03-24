#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "goo_runtime.h"
#include "goo_supervision.h"

// Global variables for demo
volatile sig_atomic_t keep_running = 1;
volatile int error_injection_counter = 0;

// Signal handler for clean shutdown
void handle_sigint(int sig) {
    keep_running = 0;
}

// Shared state structure for supervised tasks
typedef struct {
    int counter;
    int error_rate;   // 1-100, percentage chance of error
    pthread_mutex_t mutex;
} SharedState;

// Initialize the shared state
void init_shared_state(void* arg) {
    SharedState* state = (SharedState*)arg;
    pthread_mutex_init(&state->mutex, NULL);
    state->counter = 0;
    printf("Shared state initialized\n");
}

// Clean up the shared state
void cleanup_shared_state(void* arg) {
    SharedState* state = (SharedState*)arg;
    pthread_mutex_destroy(&state->mutex);
    printf("Shared state cleaned up\n");
}

// Task that occasionally fails
void unreliable_task(void* arg) {
    int task_id = *((int*)arg);
    SharedState* state = (SharedState*)(((void**)arg)[1]);
    
    printf("Task %d: Started\n", task_id);
    
    // Run until signaled to stop
    while (keep_running) {
        // Increment the counter
        pthread_mutex_lock(&state->mutex);
        state->counter++;
        int current = state->counter;
        pthread_mutex_unlock(&state->mutex);
        
        printf("Task %d: Counter = %d\n", task_id, current);
        
        // Randomly fail based on error rate
        if ((rand() % 100) < state->error_rate) {
            error_injection_counter++;
            printf("Task %d: Simulating failure (error #%d)!\n", task_id, error_injection_counter);
            
            // Different failure modes
            switch (rand() % 3) {
                case 0:
                    // Crash with null pointer dereference (will be caught by supervisor)
                    printf("Task %d: Null pointer dereference\n", task_id);
                    *((int*)NULL) = 42;
                    break;
                    
                case 1:
                    // Exit the task
                    printf("Task %d: Exiting prematurely\n", task_id);
                    return;
                    
                case 2:
                    // Raise a signal
                    printf("Task %d: Raising SIGILL\n", task_id);
                    raise(SIGILL);
                    break;
            }
        }
        
        // Sleep for a bit
        usleep(500000 + (rand() % 1000000)); // 0.5-1.5 seconds
    }
    
    printf("Task %d: Terminated normally\n", task_id);
}

// Worker task with dependencies
void dependent_worker(void* arg) {
    int worker_id = *((int*)arg);
    SharedState* state = (SharedState*)(((void**)arg)[1]);
    
    printf("Worker %d: Started\n", worker_id);
    
    // Run until signaled to stop
    while (keep_running) {
        // Do some work with the shared state
        pthread_mutex_lock(&state->mutex);
        int current = state->counter;
        pthread_mutex_unlock(&state->mutex);
        
        printf("Worker %d: Processing counter value %d\n", worker_id, current);
        
        // Sleep for a bit
        usleep(800000 + (rand() % 400000)); // 0.8-1.2 seconds
    }
    
    printf("Worker %d: Terminated normally\n", worker_id);
}

// Demo of basic supervision with one-for-one restart policy
void run_basic_supervision_demo() {
    printf("\n=== Starting Basic Supervision Demo (one-for-one) ===\n\n");
    
    // Create a supervisor
    GooSupervisor* supervisor = goo_supervise_init();
    goo_supervise_set_name(supervisor, "BasicSupervisor");
    goo_supervise_set_policy(supervisor, GOO_SUPERVISE_ONE_FOR_ONE, 10, 60);
    
    // Create shared state
    SharedState shared_state = {
        .counter = 0,
        .error_rate = 20  // 20% chance of error
    };
    
    // Set the shared state in the supervisor
    goo_supervise_set_state(supervisor, &shared_state, init_shared_state, cleanup_shared_state);
    
    // Create task arguments
    void* task1_args[2] = { malloc(sizeof(int)), &shared_state };
    void* task2_args[2] = { malloc(sizeof(int)), &shared_state };
    void* task3_args[2] = { malloc(sizeof(int)), &shared_state };
    
    *((int*)task1_args[0]) = 1;
    *((int*)task2_args[0]) = 2;
    *((int*)task3_args[0]) = 3;
    
    // Register supervised tasks
    goo_supervise_register(supervisor, unreliable_task, task1_args);
    goo_supervise_register(supervisor, unreliable_task, task2_args);
    goo_supervise_register(supervisor, unreliable_task, task3_args);
    
    // Start the supervisor
    goo_supervise_start(supervisor);
    
    // Let it run for a while
    printf("Running tasks with one-for-one restart policy for 15 seconds...\n");
    sleep(15);
    
    // Stop the tasks
    keep_running = 0;
    
    // Clean up
    goo_supervise_free(supervisor);
    free(task1_args[0]);
    free(task2_args[0]);
    free(task3_args[0]);
    
    printf("\n=== Basic Supervision Demo Completed ===\n");
    
    // Reset for next demo
    keep_running = 1;
    error_injection_counter = 0;
}

// Demo of supervision with one-for-all restart policy
void run_one_for_all_demo() {
    printf("\n=== Starting One-For-All Supervision Demo ===\n\n");
    
    // Create a supervisor
    GooSupervisor* supervisor = goo_supervise_init();
    goo_supervise_set_name(supervisor, "OneForAllSupervisor");
    goo_supervise_set_policy(supervisor, GOO_SUPERVISE_ONE_FOR_ALL, 5, 60);
    
    // Create shared state
    SharedState shared_state = {
        .counter = 0,
        .error_rate = 15  // 15% chance of error
    };
    
    // Set the shared state in the supervisor
    goo_supervise_set_state(supervisor, &shared_state, init_shared_state, cleanup_shared_state);
    
    // Create task arguments
    void* task1_args[2] = { malloc(sizeof(int)), &shared_state };
    void* task2_args[2] = { malloc(sizeof(int)), &shared_state };
    void* task3_args[2] = { malloc(sizeof(int)), &shared_state };
    
    *((int*)task1_args[0]) = 1;
    *((int*)task2_args[0]) = 2;
    *((int*)task3_args[0]) = 3;
    
    // Register supervised tasks
    goo_supervise_register(supervisor, unreliable_task, task1_args);
    goo_supervise_register(supervisor, unreliable_task, task2_args);
    goo_supervise_register(supervisor, unreliable_task, task3_args);
    
    // Start the supervisor
    goo_supervise_start(supervisor);
    
    // Let it run for a while
    printf("Running tasks with one-for-all restart policy for 15 seconds...\n");
    printf("When one task fails, all will be restarted\n");
    sleep(15);
    
    // Stop the tasks
    keep_running = 0;
    
    // Clean up
    goo_supervise_free(supervisor);
    free(task1_args[0]);
    free(task2_args[0]);
    free(task3_args[0]);
    
    printf("\n=== One-For-All Supervision Demo Completed ===\n");
    
    // Reset for next demo
    keep_running = 1;
    error_injection_counter = 0;
}

// Demo of supervision with rest-for-one restart policy and dependencies
void run_rest_for_one_demo() {
    printf("\n=== Starting Rest-For-One Supervision Demo ===\n\n");
    
    // Create a supervisor
    GooSupervisor* supervisor = goo_supervise_init();
    goo_supervise_set_name(supervisor, "RestForOneSupervisor");
    goo_supervise_set_policy(supervisor, GOO_SUPERVISE_REST_FOR_ONE, 8, 60);
    
    // Create shared state
    SharedState shared_state = {
        .counter = 0,
        .error_rate = 10  // 10% chance of error
    };
    
    // Set the shared state in the supervisor
    goo_supervise_set_state(supervisor, &shared_state, init_shared_state, cleanup_shared_state);
    
    // Create task arguments
    void* task1_args[2] = { malloc(sizeof(int)), &shared_state };
    void* worker1_args[2] = { malloc(sizeof(int)), &shared_state };
    void* worker2_args[2] = { malloc(sizeof(int)), &shared_state };
    
    *((int*)task1_args[0]) = 1;
    *((int*)worker1_args[0]) = 101;
    *((int*)worker2_args[0]) = 102;
    
    // Register supervised tasks
    int task1_index = 0;
    int worker1_index = 1;
    int worker2_index = 2;
    
    goo_supervise_register(supervisor, unreliable_task, task1_args);
    goo_supervise_register(supervisor, dependent_worker, worker1_args);
    goo_supervise_register(supervisor, dependent_worker, worker2_args);
    
    // Set dependencies: workers depend on the main task
    goo_supervise_set_dependency(supervisor, worker1_index, task1_index);
    goo_supervise_set_dependency(supervisor, worker2_index, task1_index);
    
    // Start the supervisor
    goo_supervise_start(supervisor);
    
    // Let it run for a while
    printf("Running tasks with rest-for-one restart policy for 20 seconds...\n");
    printf("When task 1 fails, workers will be restarted too\n");
    sleep(20);
    
    // Stop the tasks
    keep_running = 0;
    
    // Clean up
    goo_supervise_free(supervisor);
    free(task1_args[0]);
    free(worker1_args[0]);
    free(worker2_args[0]);
    
    printf("\n=== Rest-For-One Supervision Demo Completed ===\n");
    
    // Reset for next demo
    keep_running = 1;
    error_injection_counter = 0;
}

// Demo of dynamic supervisor
void run_dynamic_supervision_demo() {
    printf("\n=== Starting Dynamic Supervision Demo ===\n\n");
    
    // Create a supervisor
    GooSupervisor* supervisor = goo_supervise_init();
    goo_supervise_set_name(supervisor, "DynamicSupervisor");
    goo_supervise_set_policy(supervisor, GOO_SUPERVISE_ONE_FOR_ONE, 10, 60);
    
    // Enable dynamic child creation
    goo_supervise_allow_dynamic_children(supervisor, true);
    
    // Create shared state
    SharedState shared_state = {
        .counter = 0,
        .error_rate = 5  // 5% chance of error
    };
    
    // Set the shared state in the supervisor
    goo_supervise_set_state(supervisor, &shared_state, init_shared_state, cleanup_shared_state);
    
    // Start with just one task
    void* task1_args[2] = { malloc(sizeof(int)), &shared_state };
    *((int*)task1_args[0]) = 1;
    
    // Register initial task
    goo_supervise_register(supervisor, unreliable_task, task1_args);
    
    // Start the supervisor
    goo_supervise_start(supervisor);
    
    printf("Started with one task, adding new tasks dynamically...\n");
    
    // Add more tasks dynamically
    for (int i = 0; i < 3; i++) {
        sleep(3);
        
        void* new_task_args[2] = { malloc(sizeof(int)), &shared_state };
        *((int*)new_task_args[0]) = 2 + i;
        
        printf("Dynamically adding task %d\n", 2 + i);
        goo_supervise_register(supervisor, unreliable_task, new_task_args);
    }
    
    // Let it run for a while longer
    printf("All tasks added. Running for 10 more seconds...\n");
    sleep(10);
    
    // Stop the tasks
    keep_running = 0;
    
    // Clean up
    goo_supervise_free(supervisor);
    free(task1_args[0]);
    // Note: we should free the other task args, but for demo simplicity we skip it
    
    printf("\n=== Dynamic Supervision Demo Completed ===\n");
}

int main() {
    // Set up signal handler for clean shutdown
    signal(SIGINT, handle_sigint);
    
    // Seed random number generator
    srand(time(NULL));
    
    // Initialize the runtime
    goo_runtime_init(2); // Log level 2
    goo_thread_pool_init(8); // 8 worker threads
    
    printf("Supervision System Demo\n");
    printf("======================\n\n");
    printf("This demo showcases the fault-tolerance features of Goo's supervision system.\n");
    printf("Tasks will randomly fail, and the supervisor will restart them based on policy.\n\n");
    
    // Run the demos
    run_basic_supervision_demo();
    run_one_for_all_demo();
    run_rest_for_one_demo();
    run_dynamic_supervision_demo();
    
    // Cleanup
    goo_thread_pool_cleanup();
    goo_runtime_shutdown();
    
    printf("\nAll supervision demos completed successfully!\n");
    return 0;
} 