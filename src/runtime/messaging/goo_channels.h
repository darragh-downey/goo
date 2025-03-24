#ifndef GOO_CHANNELS_H
#define GOO_CHANNELS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// Channel types
typedef enum {
    GOO_CHANNEL_NORMAL = 0,      // Standard in-process channel
    GOO_CHANNEL_PUB_SUB = 1,     // Publisher-subscriber pattern
    GOO_CHANNEL_PUSH_PULL = 2,    // Push-pull pattern
    GOO_CHANNEL_REQ_REP = 3,      // Request-reply pattern
    GOO_CHANNEL_DEALER_ROUTER = 4, // Dealer-router pattern
    GOO_CHANNEL_SUB = 5           // Subscriber (internal use)
} GooChannelType;

// Channel options
typedef enum {
    GOO_CHAN_BLOCKING = 0,        // Blocking operations (default)
    GOO_CHAN_NONBLOCKING = 1,     // Non-blocking operations
    GOO_CHAN_UNBUFFERED = 2       // Synchronous channel
} GooChannelOptions;

// Message flags
typedef enum {
    GOO_MESSAGE_NONE = 0,         // No special flags
    GOO_MSG_DONTWAIT = 1,         // Non-blocking operation
    GOO_MESSAGE_MULTIPART = 2,    // Multi-part message
    GOO_MESSAGE_PRIORITY = 4      // Message with priority
} GooMessageFlags;

// Forward declarations
typedef struct GooChannel GooChannel;
typedef struct GooMessage GooMessage;
typedef struct GooChannelSubscriber GooChannelSubscriber;
typedef struct GooChannelSubscription GooChannelSubscription;

// Channel configuration options
typedef struct {
    size_t buffer_size;           // Size of channel buffer (in elements)
    bool is_blocking;             // Whether operations block when buffer is full/empty
    GooChannelType pattern;       // Channel communication pattern
    int32_t timeout_ms;           // Timeout for operations (-1 for infinite)
} GooChannelOptions;

// Message structure
struct GooMessage {
    void* data;                   // Message data
    size_t size;                  // Size of message data
    GooMessageFlags flags;        // Message flags
    uint8_t priority;             // Message priority (0-255)
    void* context;                // User context (optional)
    void (*free_fn)(void*);       // Function to free data (optional)
    GooMessage* next;             // Next message in multi-part chain
};

// Channel subscriber structure
struct GooChannelSubscriber {
    GooChannel* channel;          // Subscriber channel
    GooChannelSubscriber* next;   // Next subscriber in list
};

// Channel subscription structure
struct GooChannelSubscription {
    char* topic;                  // Subscription topic
    GooChannelSubscription* next; // Next subscription in list
};

// Channel statistics
typedef struct {
    uint64_t messages_sent;       // Number of messages sent
    uint64_t messages_received;   // Number of messages received
    uint64_t bytes_sent;          // Number of bytes sent
    uint64_t bytes_received;      // Number of bytes received
    uint64_t send_errors;         // Number of send errors
    uint64_t receive_errors;      // Number of receive errors
    uint64_t max_queue_size;      // Maximum queue size reached
    uint32_t current_queue_size;  // Current queue size
} GooChannelStats;

// Channel structure
struct GooChannel {
    void* buffer;                 // Circular buffer for messages
    size_t buffer_size;           // Size of buffer (in elements)
    size_t elem_size;             // Size of each element
    size_t head;                  // Index of the next element to dequeue
    size_t tail;                  // Index of the last element enqueued
    size_t count;                 // Number of elements in buffer
    
    pthread_mutex_t mutex;        // Mutex for thread safety
    pthread_cond_t send_cond;     // Condition for send operations
    pthread_cond_t recv_cond;     // Condition for receive operations
    
    GooChannelType type;          // Channel type
    int options;                  // Channel options
    bool is_closed;               // Whether channel is closed
    bool is_distributed;          // Whether channel is distributed
    
    char* endpoint;               // Network endpoint for distributed channels
    GooChannelSubscription* subscriptions; // Subscriptions (for SUB channels)
    GooChannelSubscriber* subscribers;     // Subscribers (for PUB channels)
    
    uint32_t high_water_mark;     // High water mark for flow control
    uint32_t low_water_mark;      // Low water mark for flow control
    int32_t timeout_ms;           // Timeout for operations
    
    GooChannelStats stats;        // Channel statistics
};

// Channel creation and destruction
GooChannel* goo_channel_create(const GooChannelOptions* options);
void goo_channel_close(GooChannel* channel);
void goo_channel_destroy(GooChannel* channel);

// Channel operations
bool goo_channel_send(GooChannel* channel, const void* data, size_t size, GooMessageFlags flags);
bool goo_channel_receive(GooChannel* channel, void* data, size_t size, GooMessageFlags flags);
bool goo_channel_try_send(GooChannel* channel, const void* data, size_t size, GooMessageFlags flags);
bool goo_channel_try_receive(GooChannel* channel, void* data, size_t size, GooMessageFlags flags);

// Message functions
GooMessage* goo_message_create(const void* data, size_t size, GooMessageFlags flags);
void goo_message_destroy(GooMessage* message);
bool goo_message_add_part(GooMessage* message, const void* data, size_t size, GooMessageFlags flags);
GooMessage* goo_message_next_part(GooMessage* message);

// Message channel operations
bool goo_channel_send_message(GooChannel* channel, GooMessage* message);
GooMessage* goo_channel_receive_message(GooChannel* channel, GooMessageFlags flags);

// Channel information and configuration
GooChannelStats goo_channel_stats(GooChannel* channel);
void goo_channel_reset_stats(GooChannel* channel);
bool goo_channel_is_empty(GooChannel* channel);
bool goo_channel_is_full(GooChannel* channel);
size_t goo_channel_size(GooChannel* channel);
void goo_channel_set_high_water_mark(GooChannel* channel, uint32_t hwm);
void goo_channel_set_timeout(GooChannel* channel, uint32_t timeout_ms);

// Distributed channel functions
GooChannel* goo_create_distributed_channel(const GooChannelOptions* options, const char* endpoint, int elem_size);
bool goo_channel_connect(GooChannel* channel, const char* endpoint);
bool goo_channel_subscribe(GooChannel* channel, const char* topic);
bool goo_channel_unsubscribe(GooChannel* channel, const char* topic);

#ifdef __cplusplus
}
#endif

#endif /* GOO_CHANNELS_H */ 