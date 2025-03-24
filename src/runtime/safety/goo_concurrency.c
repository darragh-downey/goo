/**
 * @file goo_concurrency.c
 * @brief Implementation of concurrency safety functions for the Goo language runtime
 */

#include "../../include/goo_concurrency.h"

/* Thread-local error information */
_Thread_local GooErrorInfo tls_error_info = {0};

/* Atomic operations with explicit memory ordering */

int32_t goo_atomic_load_i32(const atomic_int* ptr, GooMemoryOrder order) {
    if (!ptr) {
        goo_set_error(EINVAL, "Null pointer passed to goo_atomic_load_i32");
        return 0;
    }
    return atomic_load_explicit(ptr, order);
}

void goo_atomic_store_i32(atomic_int* ptr, int32_t value, GooMemoryOrder order) {
    if (!ptr) {
        goo_set_error(EINVAL, "Null pointer passed to goo_atomic_store_i32");
        return;
    }
    atomic_store_explicit(ptr, value, order);
}

int32_t goo_atomic_fetch_add_i32(atomic_int* ptr, int32_t value, GooMemoryOrder order) {
    if (!ptr) {
        goo_set_error(EINVAL, "Null pointer passed to goo_atomic_fetch_add_i32");
        return 0;
    }
    return atomic_fetch_add_explicit(ptr, value, order);
}

bool goo_atomic_compare_exchange_i32(atomic_int* ptr, int32_t* expected, int32_t desired,
                                   GooMemoryOrder success_order, GooMemoryOrder failure_order) {
    if (!ptr || !expected) {
        goo_set_error(EINVAL, "Null pointer passed to goo_atomic_compare_exchange_i32");
        return false;
    }
    return atomic_compare_exchange_strong_explicit(ptr, expected, desired, success_order, failure_order);
}

/* Read-write lock functions */

int goo_rwlock_init(GooRWLock* lock) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_init");
        return EINVAL;
    }
    
    atomic_init(&lock->readers, 0);
    atomic_init(&lock->writer, false);
    
    int ret = pthread_mutex_init(&lock->mutex, NULL);
    if (ret != 0) {
        goo_set_error(ret, "Failed to initialize mutex in goo_rwlock_init");
        return ret;
    }
    
    ret = pthread_cond_init(&lock->readers_done, NULL);
    if (ret != 0) {
        pthread_mutex_destroy(&lock->mutex);
        goo_set_error(ret, "Failed to initialize condition variable in goo_rwlock_init");
        return ret;
    }
    
    return 0;
}

int goo_rwlock_destroy(GooRWLock* lock) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_destroy");
        return EINVAL;
    }
    
    int ret = pthread_cond_destroy(&lock->readers_done);
    if (ret != 0) {
        goo_set_error(ret, "Failed to destroy condition variable in goo_rwlock_destroy");
        return ret;
    }
    
    ret = pthread_mutex_destroy(&lock->mutex);
    if (ret != 0) {
        goo_set_error(ret, "Failed to destroy mutex in goo_rwlock_destroy");
        return ret;
    }
    
    return 0;
}

bool goo_rwlock_read_acquire(GooRWLock* lock, uint32_t timeout_ms) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_read_acquire");
        return false;
    }
    
    struct timespec ts;
    if (timeout_ms > 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += timeout_ms * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += ts.tv_nsec / 1000000000;
            ts.tv_nsec %= 1000000000;
        }
    }
    
    int ret = pthread_mutex_lock(&lock->mutex);
    if (ret != 0) {
        goo_set_error(ret, "Failed to lock mutex in goo_rwlock_read_acquire");
        return false;
    }
    
    /* Wait while there's a writer */
    while (atomic_load(&lock->writer)) {
        if (timeout_ms == 0) {
            ret = pthread_cond_wait(&lock->readers_done, &lock->mutex);
            if (ret != 0) {
                pthread_mutex_unlock(&lock->mutex);
                goo_set_error(ret, "Failed to wait on condition variable in goo_rwlock_read_acquire");
                return false;
            }
        } else {
            ret = pthread_cond_timedwait(&lock->readers_done, &lock->mutex, &ts);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&lock->mutex);
                goo_set_error(ETIMEDOUT, "Timeout waiting for read lock");
                return false;
            } else if (ret != 0) {
                pthread_mutex_unlock(&lock->mutex);
                goo_set_error(ret, "Failed to wait on condition variable in goo_rwlock_read_acquire");
                return false;
            }
        }
    }
    
    /* Increment reader count */
    atomic_fetch_add(&lock->readers, 1);
    
    ret = pthread_mutex_unlock(&lock->mutex);
    if (ret != 0) {
        goo_set_error(ret, "Failed to unlock mutex in goo_rwlock_read_acquire");
        atomic_fetch_add(&lock->readers, -1); /* Decrement reader count on error */
        return false;
    }
    
    GOO_ANNOTATE_HAPPENS_AFTER(&lock->readers);
    return true;
}

int goo_rwlock_read_release(GooRWLock* lock) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_read_release");
        return EINVAL;
    }
    
    GOO_ANNOTATE_HAPPENS_BEFORE(&lock->readers);
    
    /* Decrement reader count */
    int reader_count = atomic_fetch_add(&lock->readers, -1);
    
    if (reader_count <= 0) {
        goo_set_error(EINVAL, "Invalid reader count in goo_rwlock_read_release");
        atomic_fetch_add(&lock->readers, 1); /* Revert decrement on error */
        return EINVAL;
    }
    
    /* If this was the last reader and there's a writer waiting, signal */
    if (reader_count == 1 && atomic_load(&lock->writer)) {
        int ret = pthread_mutex_lock(&lock->mutex);
        if (ret != 0) {
            goo_set_error(ret, "Failed to lock mutex in goo_rwlock_read_release");
            return ret;
        }
        
        ret = pthread_cond_signal(&lock->readers_done);
        
        int unlock_ret = pthread_mutex_unlock(&lock->mutex);
        if (unlock_ret != 0) {
            goo_set_error(unlock_ret, "Failed to unlock mutex in goo_rwlock_read_release");
            return unlock_ret;
        }
        
        if (ret != 0) {
            goo_set_error(ret, "Failed to signal condition variable in goo_rwlock_read_release");
            return ret;
        }
    }
    
    return 0;
}

bool goo_rwlock_write_acquire(GooRWLock* lock, uint32_t timeout_ms) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_write_acquire");
        return false;
    }
    
    struct timespec ts;
    if (timeout_ms > 0) {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_nsec += timeout_ms * 1000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_sec += ts.tv_nsec / 1000000000;
            ts.tv_nsec %= 1000000000;
        }
    }
    
    /* Mark that a writer wants to write */
    bool expected = false;
    if (!atomic_compare_exchange_strong(&lock->writer, &expected, true)) {
        goo_set_error(EBUSY, "Another writer already has the lock");
        return false;
    }
    
    int ret = pthread_mutex_lock(&lock->mutex);
    if (ret != 0) {
        atomic_store(&lock->writer, false); /* Reset writer flag on error */
        goo_set_error(ret, "Failed to lock mutex in goo_rwlock_write_acquire");
        return false;
    }
    
    /* Wait until there are no readers */
    while (atomic_load(&lock->readers) > 0) {
        if (timeout_ms == 0) {
            ret = pthread_cond_wait(&lock->readers_done, &lock->mutex);
            if (ret != 0) {
                pthread_mutex_unlock(&lock->mutex);
                atomic_store(&lock->writer, false); /* Reset writer flag on error */
                goo_set_error(ret, "Failed to wait on condition variable in goo_rwlock_write_acquire");
                return false;
            }
        } else {
            ret = pthread_cond_timedwait(&lock->readers_done, &lock->mutex, &ts);
            if (ret == ETIMEDOUT) {
                pthread_mutex_unlock(&lock->mutex);
                atomic_store(&lock->writer, false); /* Reset writer flag on timeout */
                goo_set_error(ETIMEDOUT, "Timeout waiting for write lock");
                return false;
            } else if (ret != 0) {
                pthread_mutex_unlock(&lock->mutex);
                atomic_store(&lock->writer, false); /* Reset writer flag on error */
                goo_set_error(ret, "Failed to wait on condition variable in goo_rwlock_write_acquire");
                return false;
            }
        }
    }
    
    ret = pthread_mutex_unlock(&lock->mutex);
    if (ret != 0) {
        atomic_store(&lock->writer, false); /* Reset writer flag on error */
        goo_set_error(ret, "Failed to unlock mutex in goo_rwlock_write_acquire");
        return false;
    }
    
    GOO_ANNOTATE_HAPPENS_AFTER(&lock->writer);
    return true;
}

int goo_rwlock_write_release(GooRWLock* lock) {
    if (!lock) {
        goo_set_error(EINVAL, "Null pointer passed to goo_rwlock_write_release");
        return EINVAL;
    }
    
    if (!atomic_load(&lock->writer)) {
        goo_set_error(EINVAL, "No active writer in goo_rwlock_write_release");
        return EINVAL;
    }
    
    GOO_ANNOTATE_HAPPENS_BEFORE(&lock->writer);
    
    /* Reset writer flag */
    atomic_store(&lock->writer, false);
    
    /* Signal all waiting readers */
    int ret = pthread_mutex_lock(&lock->mutex);
    if (ret != 0) {
        goo_set_error(ret, "Failed to lock mutex in goo_rwlock_write_release");
        return ret;
    }
    
    ret = pthread_cond_broadcast(&lock->readers_done);
    
    int unlock_ret = pthread_mutex_unlock(&lock->mutex);
    if (unlock_ret != 0) {
        goo_set_error(unlock_ret, "Failed to unlock mutex in goo_rwlock_write_release");
        return unlock_ret;
    }
    
    if (ret != 0) {
        goo_set_error(ret, "Failed to broadcast condition variable in goo_rwlock_write_release");
        return ret;
    }
    
    return 0;
}

/* Lock-free queue functions */

int goo_lockfree_queue_init(GooLockFreeQueue* queue) {
    if (!queue) {
        goo_set_error(EINVAL, "Null pointer passed to goo_lockfree_queue_init");
        return EINVAL;
    }
    
    GooQueueNode* dummy = malloc(sizeof(GooQueueNode));
    if (!dummy) {
        goo_set_error(ENOMEM, "Failed to allocate dummy node in goo_lockfree_queue_init");
        return ENOMEM;
    }
    
    dummy->data = NULL;
    atomic_init(&dummy->next, NULL);
    
    atomic_init(&queue->head, dummy);
    atomic_init(&queue->tail, dummy);
    
    return 0;
}

int goo_lockfree_queue_destroy(GooLockFreeQueue* queue) {
    if (!queue) {
        goo_set_error(EINVAL, "Null pointer passed to goo_lockfree_queue_destroy");
        return EINVAL;
    }
    
    /* Free all nodes in the queue */
    GooQueueNode* current = atomic_load(&queue->head);
    while (current) {
        GooQueueNode* next = atomic_load(&current->next);
        free(current);
        current = next;
    }
    
    atomic_store(&queue->head, NULL);
    atomic_store(&queue->tail, NULL);
    
    return 0;
}

int goo_lockfree_queue_push(GooLockFreeQueue* queue, void* data) {
    if (!queue) {
        goo_set_error(EINVAL, "Null pointer passed to goo_lockfree_queue_push");
        return EINVAL;
    }
    
    GooQueueNode* node = malloc(sizeof(GooQueueNode));
    if (!node) {
        goo_set_error(ENOMEM, "Failed to allocate node in goo_lockfree_queue_push");
        return ENOMEM;
    }
    
    node->data = data;
    atomic_init(&node->next, NULL);
    
    GooQueueNode* tail;
    GooQueueNode* next;
    
    while (1) {
        tail = atomic_load(&queue->tail);
        next = atomic_load(&tail->next);
        
        /* Check if tail is still the tail */
        if (tail == atomic_load(&queue->tail)) {
            if (next == NULL) {
                /* Try to append to the tail */
                if (atomic_compare_exchange_weak(&tail->next, &next, node)) {
                    break;
                }
            } else {
                /* Tail is falling behind, try to advance it */
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
            }
        }
    }
    
    /* Try to update the tail */
    atomic_compare_exchange_weak(&queue->tail, &tail, node);
    
    return 0;
}

int goo_lockfree_queue_pop(GooLockFreeQueue* queue, void** data_out) {
    if (!queue || !data_out) {
        goo_set_error(EINVAL, "Null pointer passed to goo_lockfree_queue_pop");
        return EINVAL;
    }
    
    *data_out = NULL;
    GooQueueNode* head;
    GooQueueNode* tail;
    GooQueueNode* next;
    
    while (1) {
        head = atomic_load(&queue->head);
        tail = atomic_load(&queue->tail);
        next = atomic_load(&head->next);
        
        /* Check if head is still the head */
        if (head == atomic_load(&queue->head)) {
            /* Is queue empty or tail falling behind? */
            if (head == tail) {
                if (next == NULL) {
                    /* Queue is empty */
                    goo_set_error(ENODATA, "Queue is empty in goo_lockfree_queue_pop");
                    return ENODATA;
                }
                /* Tail is falling behind, try to advance it */
                atomic_compare_exchange_weak(&queue->tail, &tail, next);
            } else {
                /* Try to get the data before CAS, since we're about to free head */
                *data_out = next->data;
                
                /* Try to advance head */
                if (atomic_compare_exchange_weak(&queue->head, &head, next)) {
                    break;
                }
            }
        }
    }
    
    /* Free the old dummy node */
    free(head);
    
    return 0;
}

bool goo_lockfree_queue_is_empty(GooLockFreeQueue* queue) {
    if (!queue) {
        goo_set_error(EINVAL, "Null pointer passed to goo_lockfree_queue_is_empty");
        return true;
    }
    
    GooQueueNode* head = atomic_load(&queue->head);
    GooQueueNode* next = atomic_load(&head->next);
    
    return next == NULL;
}

/* Thread-local error handling */

GooErrorInfo* goo_get_error_info(void) {
    return &tls_error_info;
}

void goo_set_error(int error_code, const char* message) {
    GooErrorInfo* info = goo_get_error_info();
    info->error_code = error_code;
    
    if (!message) {
        info->message[0] = '\0';
        return;
    }
    
    size_t message_len = strlen(message);
    if (message_len >= sizeof(info->message)) {
        message_len = sizeof(info->message) - 1;
    }
    
    memcpy(info->message, message, message_len);
    info->message[message_len] = '\0';
}

void goo_clear_error(void) {
    GooErrorInfo* info = goo_get_error_info();
    info->error_code = 0;
    info->message[0] = '\0';
} 