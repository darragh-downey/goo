/**
 * @file goo_concurrency.h
 * @brief Concurrency safety implementation for the Goo language runtime
 *
 * This header defines structures and functions for preventing race conditions
 * and ensuring thread safety in the Goo codebase. It implements thread safety
 * annotations, explicit memory ordering, and lock-free data structures.
 */

#ifndef GOO_CONCURRENCY_H
#define GOO_CONCURRENCY_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdatomic.h>
#include "goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Thread safety annotations (compatible with Clang) */
#if defined(__clang__) && defined(GOO_THREAD_SAFETY_ANALYSIS)
    #define GOO_GUARDED_BY(x) __attribute__((guarded_by(x)))
    #define GOO_PT_GUARDED_BY(x) __attribute__((pt_guarded_by(x)))
    #define GOO_ACQUIRED_BEFORE(...) __attribute__((acquired_before(__VA_ARGS__)))
    #define GOO_ACQUIRED_AFTER(...) __attribute__((acquired_after(__VA_ARGS__)))
    #define GOO_REQUIRES(...) __attribute__((requires_capability(__VA_ARGS__)))
    #define GOO_EXCLUDES(...) __attribute__((excludes_capability(__VA_ARGS__)))
    #define GOO_ACQUIRE(...) __attribute__((acquire_capability(__VA_ARGS__)))
    #define GOO_RELEASE(...) __attribute__((release_capability(__VA_ARGS__)))
    #define GOO_SCOPED_CAPABILITY __attribute__((scoped_lockable))
#else
    #define GOO_GUARDED_BY(x)
    #define GOO_PT_GUARDED_BY(x)
    #define GOO_ACQUIRED_BEFORE(...)
    #define GOO_ACQUIRED_AFTER(...)
    #define GOO_REQUIRES(...)
    #define GOO_EXCLUDES(...)
    #define GOO_ACQUIRE(...)
    #define GOO_RELEASE(...)
    #define GOO_SCOPED_CAPABILITY
#endif

/* ThreadSanitizer annotations */
#ifdef GOO_THREAD_SANITIZER
    void AnnotateHappensBefore(const char* file, int line, const void* addr);
    void AnnotateHappensAfter(const char* file, int line, const void* addr);
    void AnnotateBenignRace(const char* file, int line, const void* addr, const char* desc);
    
    #define GOO_ANNOTATE_HAPPENS_BEFORE(addr) \
        AnnotateHappensBefore(__FILE__, __LINE__, (void*)(addr))
    #define GOO_ANNOTATE_HAPPENS_AFTER(addr) \
        AnnotateHappensAfter(__FILE__, __LINE__, (void*)(addr))
    #define GOO_ANNOTATE_BENIGN_RACE(addr, desc) \
        AnnotateBenignRace(__FILE__, __LINE__, (void*)(addr), (desc))
#else
    #define GOO_ANNOTATE_HAPPENS_BEFORE(addr)
    #define GOO_ANNOTATE_HAPPENS_AFTER(addr)
    #define GOO_ANNOTATE_BENIGN_RACE(addr, desc)
#endif

/**
 * @brief Memory ordering for atomic operations
 */
typedef enum {
    GOO_MEMORY_RELAXED = __ATOMIC_RELAXED, /**< No synchronization or ordering constraints */
    GOO_MEMORY_CONSUME = __ATOMIC_CONSUME, /**< Data dependency ordering (deprecated in C++17) */
    GOO_MEMORY_ACQUIRE = __ATOMIC_ACQUIRE, /**< Acquire operation: no reads/writes can be reordered before */
    GOO_MEMORY_RELEASE = __ATOMIC_RELEASE, /**< Release operation: no reads/writes can be reordered after */
    GOO_MEMORY_ACQ_REL = __ATOMIC_ACQ_REL, /**< Both acquire and release semantics */
    GOO_MEMORY_SEQ_CST = __ATOMIC_SEQ_CST  /**< Sequential consistency: total order exists */
} GooMemoryOrder;

/**
 * @brief Read-write lock implementation with timeout support
 */
typedef struct {
    atomic_int readers;          /**< Number of active readers */
    atomic_bool writer;          /**< Whether a writer is active */
    pthread_mutex_t mutex;       /**< Mutex for synchronization */
    pthread_cond_t readers_done; /**< Condition variable for readers being done */
} GooRWLock;

/**
 * @brief Node in a lock-free queue
 */
typedef struct GooQueueNode {
    void* data;                      /**< Pointer to the node's data */
    _Atomic(struct GooQueueNode*) next; /**< Pointer to the next node */
} GooQueueNode;

/**
 * @brief Lock-free queue implementation
 */
typedef struct {
    _Atomic(GooQueueNode*) head; /**< Head of the queue */
    _Atomic(GooQueueNode*) tail; /**< Tail of the queue */
} GooLockFreeQueue;

/**
 * @brief Thread-local error information
 */
typedef struct {
    int error_code;           /**< Error code */
    char message[256];        /**< Error message */
} GooErrorInfo;

/* Atomic operations with explicit memory ordering */

/**
 * @brief Atomically load a 32-bit integer
 * 
 * @param ptr Pointer to the atomic integer
 * @param order Memory ordering to use
 * @return int32_t The loaded value
 */
int32_t goo_atomic_load_i32(const atomic_int* ptr, GooMemoryOrder order);

/**
 * @brief Atomically store a 32-bit integer
 * 
 * @param ptr Pointer to the atomic integer
 * @param value Value to store
 * @param order Memory ordering to use
 */
void goo_atomic_store_i32(atomic_int* ptr, int32_t value, GooMemoryOrder order);

/**
 * @brief Atomically add to a 32-bit integer and return the previous value
 * 
 * @param ptr Pointer to the atomic integer
 * @param value Value to add
 * @param order Memory ordering to use
 * @return int32_t The previous value
 */
int32_t goo_atomic_fetch_add_i32(atomic_int* ptr, int32_t value, GooMemoryOrder order);

/**
 * @brief Atomically compare and exchange a 32-bit integer
 * 
 * @param ptr Pointer to the atomic integer
 * @param expected Pointer to the expected value, updated if comparison fails
 * @param desired Value to store if comparison succeeds
 * @param success_order Memory ordering to use if comparison succeeds
 * @param failure_order Memory ordering to use if comparison fails
 * @return bool true if the comparison succeeded, false otherwise
 */
bool goo_atomic_compare_exchange_i32(atomic_int* ptr, int32_t* expected, int32_t desired,
                                   GooMemoryOrder success_order, GooMemoryOrder failure_order);

/* Read-write lock functions */

/**
 * @brief Initialize a read-write lock
 * 
 * @param lock Pointer to the lock to initialize
 * @return int 0 on success, error code on failure
 */
int goo_rwlock_init(GooRWLock* lock);

/**
 * @brief Destroy a read-write lock
 * 
 * @param lock Pointer to the lock to destroy
 * @return int 0 on success, error code on failure
 */
int goo_rwlock_destroy(GooRWLock* lock);

/**
 * @brief Acquire a read-write lock for reading with timeout
 * 
 * @param lock Pointer to the lock to acquire
 * @param timeout_ms Timeout in milliseconds, or 0 for no timeout
 * @return bool true if the lock was acquired, false on timeout
 */
bool goo_rwlock_read_acquire(GooRWLock* lock, uint32_t timeout_ms);

/**
 * @brief Release a read-write lock after reading
 * 
 * @param lock Pointer to the lock to release
 * @return int 0 on success, error code on failure
 */
int goo_rwlock_read_release(GooRWLock* lock);

/**
 * @brief Acquire a read-write lock for writing with timeout
 * 
 * @param lock Pointer to the lock to acquire
 * @param timeout_ms Timeout in milliseconds, or 0 for no timeout
 * @return bool true if the lock was acquired, false on timeout
 */
bool goo_rwlock_write_acquire(GooRWLock* lock, uint32_t timeout_ms);

/**
 * @brief Release a read-write lock after writing
 * 
 * @param lock Pointer to the lock to release
 * @return int 0 on success, error code on failure
 */
int goo_rwlock_write_release(GooRWLock* lock);

/* Lock-free queue functions */

/**
 * @brief Initialize a lock-free queue
 * 
 * @param queue Pointer to the queue to initialize
 * @return int 0 on success, error code on failure
 */
int goo_lockfree_queue_init(GooLockFreeQueue* queue);

/**
 * @brief Destroy a lock-free queue
 * 
 * @param queue Pointer to the queue to destroy
 * @return int 0 on success, error code on failure
 */
int goo_lockfree_queue_destroy(GooLockFreeQueue* queue);

/**
 * @brief Push a value onto a lock-free queue
 * 
 * @param queue Pointer to the queue
 * @param data Pointer to the data to push
 * @return int 0 on success, error code on failure
 */
int goo_lockfree_queue_push(GooLockFreeQueue* queue, void* data);

/**
 * @brief Pop a value from a lock-free queue
 * 
 * @param queue Pointer to the queue
 * @param data_out Pointer to store the popped data
 * @return int 0 on success, error code on failure (including empty queue)
 */
int goo_lockfree_queue_pop(GooLockFreeQueue* queue, void** data_out);

/**
 * @brief Check if a lock-free queue is empty
 * 
 * @param queue Pointer to the queue
 * @return bool true if the queue is empty, false otherwise
 */
bool goo_lockfree_queue_is_empty(GooLockFreeQueue* queue);

/* Thread-local error handling */

/**
 * @brief Get the thread-local error information
 * 
 * @return GooErrorInfo* Pointer to the thread-local error information
 */
GooErrorInfo* goo_get_error_info(void);

/**
 * @brief Set the thread-local error information
 * 
 * @param error_code Error code
 * @param message Error message
 */
void goo_set_error(int error_code, const char* message);

/**
 * @brief Clear the thread-local error information
 */
void goo_clear_error(void);

// Scheduling policies for parallel execution
typedef enum {
    GOO_SCHEDULE_STATIC,     // Static workload division
    GOO_SCHEDULE_DYNAMIC,    // Dynamic workload division
    GOO_SCHEDULE_GUIDED,     // Guided auto workload division
    GOO_SCHEDULE_AUTO,       // Automatic selection based on workload
    GOO_SCHEDULE_RUNTIME     // Determined at runtime
} GooSchedulingPolicy;

#ifdef __cplusplus
}
#endif

#endif /* GOO_CONCURRENCY_H */ 