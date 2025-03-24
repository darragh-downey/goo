#ifndef GOO_RUNTIME_H
#define GOO_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Channel pattern constants matching ChannelPattern enum in runtime.zig
typedef enum {
    GOO_CHANNEL_DEFAULT = 0,
    GOO_CHANNEL_PUB,
    GOO_CHANNEL_SUB,
    GOO_CHANNEL_PUSH,
    GOO_CHANNEL_PULL,
    GOO_CHANNEL_REQ,
    GOO_CHANNEL_REP,
    GOO_CHANNEL_DEALER,
    GOO_CHANNEL_ROUTER,
    GOO_CHANNEL_PAIR
} GooChannelPattern;

// Channel error codes
typedef enum {
    GOO_CHANNEL_OK = 0,
    GOO_CHANNEL_CLOSED = 1,
    GOO_CHANNEL_FULL = 2,
    GOO_CHANNEL_EMPTY = 3,
    GOO_CHANNEL_TIMEOUT = 4,
    GOO_CHANNEL_INTERRUPTED = 5,
    GOO_CHANNEL_INVALID_ENDPOINT = 6
} GooChannelError;

// Channel functions for C
void* goo_channel_create(size_t capacity, int pattern, const char* endpoint);
int goo_channel_send(void* channel, int value);
int goo_channel_receive(void* channel, int* value);
void goo_channel_close(void* channel);
void goo_channel_destroy(void* channel);

// Type definitions for functions passed to goroutines and parallel execution
typedef void (*GooGoroutineFunc)(void* arg);
typedef void (*GooParallelFunc)(int thread_id, int num_threads, void* arg);

// Goroutine functions
int goo_goroutine_spawn(GooGoroutineFunc func, void* arg);

// Parallel execution functions
int goo_parallel_execute(int num_threads, GooParallelFunc func, void* arg);

// Supervisor functions (to be implemented)
int goo_supervise_start(GooGoroutineFunc func, void* arg);
int goo_supervise_stop(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_RUNTIME_H 