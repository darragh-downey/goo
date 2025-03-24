#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <setjmp.h>
#include "../include/goo_runtime.h"

// Thread-local storage for panic state
static pthread_key_t panic_key;
static pthread_once_t panic_key_once = PTHREAD_ONCE_INIT;

// Thread-local storage for recovery points
static pthread_key_t recovery_key;
static pthread_once_t recovery_key_once = PTHREAD_ONCE_INIT;

// Panic state structure
typedef struct {
    bool in_panic;           // Whether the thread is in a panic state
    void* panic_value;       // The value passed to panic
    char* panic_message;     // The panic message
    void* stack_trace;       // Stack trace information (for future implementation)
} GooPanicState;

// Recovery point structure
typedef struct RecoveryPoint {
    jmp_buf env;                    // Jump buffer for setjmp/longjmp
    struct RecoveryPoint* prev;     // Previous recovery point in the stack
    bool active;                    // Whether this recovery point is active
} RecoveryPoint;

// Initialize thread-local storage for panic state
static void init_panic_key(void) {
    pthread_key_create(&panic_key, free);
}

// Initialize thread-local storage for recovery points
static void init_recovery_key(void) {
    pthread_key_create(&recovery_key, NULL);
}

// Get the current panic state, creating it if necessary
static GooPanicState* get_panic_state(void) {
    pthread_once(&panic_key_once, init_panic_key);
    
    GooPanicState* state = (GooPanicState*)pthread_getspecific(panic_key);
    if (!state) {
        state = (GooPanicState*)calloc(1, sizeof(GooPanicState));
        pthread_setspecific(panic_key, state);
    }
    
    return state;
}

// Get the current recovery point stack, creating it if necessary
static RecoveryPoint** get_recovery_stack(void) {
    pthread_once(&recovery_key_once, init_recovery_key);
    
    RecoveryPoint** stack_ptr = (RecoveryPoint**)pthread_getspecific(recovery_key);
    if (!stack_ptr) {
        stack_ptr = (RecoveryPoint**)calloc(1, sizeof(RecoveryPoint*));
        pthread_setspecific(recovery_key, stack_ptr);
    }
    
    return stack_ptr;
}

// Push a new recovery point onto the stack
static void push_recovery_point(RecoveryPoint* point) {
    RecoveryPoint** stack_ptr = get_recovery_stack();
    point->prev = *stack_ptr;
    *stack_ptr = point;
}

// Pop the top recovery point from the stack
static RecoveryPoint* pop_recovery_point(void) {
    RecoveryPoint** stack_ptr = get_recovery_stack();
    RecoveryPoint* point = *stack_ptr;
    
    if (point) {
        *stack_ptr = point->prev;
    }
    
    return point;
}

// Setup error recovery point
// Returns: true on initial setup, false if recovering from panic
bool goo_recover_setup(void) {
    RecoveryPoint* point = (RecoveryPoint*)malloc(sizeof(RecoveryPoint));
    if (!point) {
        fprintf(stderr, "Failed to allocate recovery point\n");
        exit(1);
    }
    
    point->active = true;
    
    // Push the recovery point onto the stack
    push_recovery_point(point);
    
    // Set up the jump buffer
    if (setjmp(point->env) == 0) {
        // Normal execution path
        return true;
    } else {
        // Recovery path (returning from longjmp)
        point->active = false;
        return false;
    }
}

// Finish a recovery block (clean up the recovery point)
void goo_recover_finish(void) {
    RecoveryPoint* point = pop_recovery_point();
    if (point) {
        free(point);
    }
}

// Trigger a panic with a value
void goo_panic(void* value, const char* message) {
    GooPanicState* state = get_panic_state();
    
    // Only process the first panic
    if (state->in_panic) {
        return;
    }
    
    // Set panic state
    state->in_panic = true;
    state->panic_value = value;
    
    // Copy the message if provided
    if (message) {
        state->panic_message = strdup(message);
    }
    
    // Find the most recent recovery point
    RecoveryPoint** stack_ptr = get_recovery_stack();
    RecoveryPoint* point = *stack_ptr;
    
    if (point && point->active) {
        // Jump to the recovery point
        longjmp(point->env, 1);
    } else {
        // No recovery point, print message and abort
        fprintf(stderr, "PANIC: %s\n", message ? message : "No message");
        abort();
    }
}

// Check if the current thread is in a panic state
bool goo_is_panic(void) {
    GooPanicState* state = get_panic_state();
    return state->in_panic;
}

// Get the value passed to panic
void* goo_get_panic_value(void) {
    GooPanicState* state = get_panic_state();
    if (state->in_panic) {
        return state->panic_value;
    } else {
        return NULL;
    }
}

// Get the panic message
const char* goo_get_panic_message(void) {
    GooPanicState* state = get_panic_state();
    if (state->in_panic) {
        return state->panic_message;
    } else {
        return NULL;
    }
}

// Clear the panic state
void goo_clear_panic(void) {
    GooPanicState* state = get_panic_state();
    state->in_panic = false;
    
    // Free the panic message if allocated
    if (state->panic_message) {
        free(state->panic_message);
        state->panic_message = NULL;
    }
    
    state->panic_value = NULL;
}

// Runtime panic function (unrecoverable error)
void goo_runtime_panic(const char* message) {
    goo_panic(NULL, message);
    
    // If we get here, there was no recovery point
    fprintf(stderr, "RUNTIME PANIC: %s\n", message);
    abort();
} 