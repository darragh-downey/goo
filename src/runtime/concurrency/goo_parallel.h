#ifndef GOO_PARALLEL_H
#define GOO_PARALLEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>

// Forward declarations
struct GooThreadPool;
struct GooParallelLoop;
struct GooVector;

// Parallel operation modes
typedef enum {
    GOO_PARALLEL_FOR,      // Standard for loop parallelization
    GOO_PARALLEL_FOREACH,  // Foreach loop parallelization
    GOO_PARALLEL_SECTIONS, // Independent sections
    GOO_PARALLEL_TASK      // Task-based parallelism
} GooParallelMode;

// Work distribution strategies
typedef enum {
    GOO_SCHEDULE_STATIC,    // Divide work evenly among threads
    GOO_SCHEDULE_DYNAMIC,   // Work stealing approach
    GOO_SCHEDULE_GUIDED,    // Start with large chunks, then decrease
    GOO_SCHEDULE_AUTO       // Runtime decides best strategy
} GooScheduleType;

// Data sharing attributes
typedef enum {
    GOO_SHARE_PRIVATE,     // Each thread has its own copy
    GOO_SHARE_SHARED,      // All threads share the same variable
    GOO_SHARE_FIRSTPRIVATE, // Private, but initialized with master value
    GOO_SHARE_LASTPRIVATE,  // Private, but final value copied back
    GOO_SHARE_REDUCTION     // Shared with reduction operation
} GooSharingType;

// Reduction operations
typedef enum {
    GOO_REDUCE_SUM,        // Sum of values
    GOO_REDUCE_PRODUCT,    // Product of values
    GOO_REDUCE_MIN,        // Minimum value
    GOO_REDUCE_MAX,        // Maximum value
    GOO_REDUCE_AND,        // Logical AND
    GOO_REDUCE_OR,         // Logical OR
    GOO_REDUCE_XOR,        // Logical XOR
    GOO_REDUCE_CUSTOM      // Custom reduction function
} GooReductionOp;

// Vector operations for SIMD support
typedef enum {
    GOO_VECTOR_ADD,        // Vector addition
    GOO_VECTOR_SUB,        // Vector subtraction
    GOO_VECTOR_MUL,        // Vector multiplication
    GOO_VECTOR_DIV,        // Vector division
    GOO_VECTOR_CUSTOM      // Custom vector operation
} GooVectorOp;

// Parallel loop configuration
typedef struct GooParallelLoop {
    GooParallelMode mode;          // Type of parallelism
    GooScheduleType schedule;      // Work distribution strategy
    int chunk_size;                // Size of work chunks (for dynamic/guided)
    bool vectorize;                // Enable vectorization if possible
    int num_threads;               // Number of threads to use (0 = auto)
    uint64_t start;                // Loop start value
    uint64_t end;                  // Loop end value
    uint64_t step;                 // Loop step value
    void *context;                 // User context data
    void (*body)(uint64_t, void*); // Loop body function
    int priority;                  // Task priority (for task mode)
} GooParallelLoop;

// Variable sharing declaration
typedef struct GooSharedVar {
    void *ptr;                     // Pointer to the variable
    size_t size;                   // Size of the variable
    GooSharingType sharing;        // Sharing type
    GooReductionOp reduce_op;      // Reduction operation (if reduction)
    void (*custom_reduce)(void*, void*); // Custom reduction function
} GooSharedVar;

// Vector operation config
typedef struct GooVector {
    void *src1;                    // First source vector
    void *src2;                    // Second source vector
    void *dst;                     // Destination vector
    size_t elem_size;              // Element size in bytes
    size_t length;                 // Number of elements
    GooVectorOp op;                // Vector operation
    void (*custom_op)(void*, void*, void*); // Custom vector operation
} GooVector;

// Function declarations

// Initialize the parallel subsystem
bool goo_parallel_init(int num_threads);

// Clean up the parallel subsystem
void goo_parallel_cleanup(void);

// Execute a parallel for loop
bool goo_parallel_for(uint64_t start, uint64_t end, uint64_t step, 
                     void (*body)(uint64_t, void*), void *context,
                     GooScheduleType schedule, int chunk_size, int num_threads);

// Execute a parallel foreach loop
bool goo_parallel_foreach(void *items, size_t count, size_t item_size,
                         void (*body)(void*, void*), void *context,
                         GooScheduleType schedule, int chunk_size, int num_threads);

// Start a parallel region with shared variables
bool goo_parallel_begin(int num_threads, GooSharedVar *shared_vars, int var_count);

// End a parallel region
void goo_parallel_end(void);

// Execute a vector operation (SIMD)
bool goo_vector_execute(GooVector *vec_op);

// Set the maximum number of threads
void goo_parallel_set_threads(int num_threads);

// Get the current thread number
int goo_parallel_get_thread_num(void);

// Get the total number of threads
int goo_parallel_get_num_threads(void);

// Create a thread barrier
bool goo_parallel_barrier(void);

// Create a parallel task
bool goo_parallel_task(void (*task_func)(void*), void *context, int priority);

// Wait for all tasks to complete
bool goo_parallel_taskwait(void);

#endif // GOO_PARALLEL_H 