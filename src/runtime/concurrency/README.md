# Goo Parallel Module

This module provides OpenMP-like parallelism for the Goo programming language. It enables efficient utilization of multi-core processors with a simple and intuitive API.

## Features

- **Thread Pool Management**: Efficient thread creation and reuse
- **Parallel For Loops**: Execute loops in parallel with different scheduling strategies
- **Work Sharing Strategies**: Static, dynamic, and guided work distribution
- **Synchronization Primitives**: Barriers for thread synchronization
- **Data Sharing Controls**: Private, shared, and reduction variables
- **Vectorization Support**: SIMD operations for data-parallel tasks

## API Overview

### Initialization

```c
// Initialize the parallel subsystem with a specific number of threads
// Use 0 to automatically select based on available cores
bool goo_parallel_init(int num_threads);

// Clean up parallel resources
void goo_parallel_cleanup(void);
```

### Parallel For Loops

```c
// Execute a loop in parallel
bool goo_parallel_for(uint64_t start, uint64_t end, uint64_t step, 
                     void (*body)(uint64_t, void*), void *context,
                     GooScheduleType schedule, int chunk_size, int num_threads);
```

### Scheduling Strategies

The module supports different work distribution strategies:

- `GOO_SCHEDULE_STATIC`: Divides work evenly among threads
- `GOO_SCHEDULE_DYNAMIC`: Uses a work-stealing approach for better load balancing
- `GOO_SCHEDULE_GUIDED`: Starts with large chunks, then decreases chunk size
- `GOO_SCHEDULE_AUTO`: Runtime decides the best strategy

### Synchronization

```c
// Create a barrier for thread synchronization
bool goo_parallel_barrier(void);
```

### Thread Information

```c
// Get the current thread's ID
int goo_parallel_get_thread_num(void);

// Get the total number of threads
int goo_parallel_get_num_threads(void);
```

## Usage Example

Here's a simple example of initializing the parallel runtime and executing a loop in parallel:

```c
#include "goo_parallel.h"

// Task function for parallel execution
void my_task(uint64_t index, void* context) {
    int* array = (int*)context;
    array[index] = index * 2;  // Some computation
}

int main() {
    int array[1000];
    
    // Initialize parallel runtime with default thread count
    goo_parallel_init(0);
    
    // Execute a loop in parallel with static scheduling
    goo_parallel_for(0, 1000, 1, my_task, array, GOO_SCHEDULE_STATIC, 0, 0);
    
    // Clean up
    goo_parallel_cleanup();
    
    return 0;
}
```

## Integration with Goo Language

In Goo code, this functionality is exposed through the `go parallel` syntax:

```go
// Parallel for loop in Goo
go parallel for 0..1000 {
    array[i] = i * 2
}

// With scheduling options
go parallel [schedule(dynamic, 100)] for 0..1000 {
    array[i] = i * 2
}

// With data sharing control
go parallel [shared(array), private(i, temp)] for 0..1000 {
    temp = i * 2
    array[i] = temp
}
```

## Implementation Notes

- The implementation uses POSIX threads (pthreads) for portability
- Thread-local storage is used to track thread IDs
- A thread pool is created once and reused for all parallel operations
- Work is distributed using task queues with different scheduling strategies
- Race conditions are avoided in the core implementation

## Future Enhancements

- Nested parallelism support
- Thread affinity controls
- Task dependencies
- Complete implementation of data sharing controls
- Vectorization optimizations
- Improved work-stealing scheduler

## Building and Testing

To build the parallel module and run the example:

```bash
make
make run-example
```

## Performance Considerations

- Best performance is achieved with CPU-bound tasks that have minimal synchronization
- The ideal chunk size depends on the workload and should be tuned for best performance
- Consider using static scheduling for regular workloads and dynamic for irregular ones 