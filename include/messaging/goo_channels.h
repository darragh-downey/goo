#ifndef GOO_CHANNELS_H
#define GOO_CHANNELS_H

/**
 * Goo Language Messaging Channels
 * Provides communication channels between concurrent processes and threads
 */

#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include "../goo_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// Complete channel structure
struct GooChannel {
    GooChannelPattern type;         // Channel type/pattern
    size_t elem_size;               // Size of each element
    size_t buffer_size;             // Buffer capacity
    void* buffer;                   // Channel buffer
    size_t read_pos;                // Current read position
    size_t write_pos;               // Current write position
    size_t count;                   // Number of items in buffer
    pthread_mutex_t mutex;          // Mutex for synchronization
    pthread_cond_t send_cond;       // Condition for send operations
    pthread_cond_t recv_cond;       // Condition for receive operations
    size_t high_water_mark;         // High water mark for buffered channels
    size_t low_water_mark;          // Low water mark for buffered channels
    int timeout_ms;                 // Default timeout in milliseconds
    int options;                    // Channel options
    bool is_closed;                 // Whether channel is closed
    bool is_distributed;            // Whether channel is distributed
    void* subscribers;              // List of subscribers (for pub/sub pattern)
    void* endpoint;                 // Endpoint for distributed channels
};

// Forward declaration for message
typedef struct GooMessage GooMessage;

// Channel options
typedef struct {
    size_t buffer_size;          // Buffer size for buffered channels (0 for unbuffered)
    bool is_blocking;            // Whether operations block when buffer is full/empty
    GooChannelPattern pattern;   // Channel pattern (pub/sub, push/pull, etc.)
    int timeout_ms;              // Timeout in milliseconds (-1 for infinite)
} GooChannelOptions;

// Message options
typedef struct {
    bool copy_data;              // Whether to copy message data or just reference it
    int priority;                // Message priority (higher = more priority)
    int timeout_ms;              // Timeout in milliseconds (-1 for infinite)
} GooMessageOptions;

// Complete message structure
struct GooMessage {
    void* data;                  // Message data
    size_t size;                 // Size of message data
    int priority;                // Message priority
    bool owns_data;              // Whether message owns data (should free it)
    GooMessage* next;            // Next message in queue
};

/**
 * Initialize channels subsystem
 */
bool goo_channels_init(void);

/**
 * Clean up channels subsystem
 */
void goo_channels_cleanup(void);

/**
 * Create a new channel
 */
GooChannel* goo_channel_create(const GooChannelOptions* options);

/**
 * Destroy a channel
 */
void goo_channel_destroy(GooChannel* channel);

/**
 * Send data on a channel
 */
bool goo_channel_send(GooChannel* channel, const void* data, size_t size, const GooMessageOptions* options);

/**
 * Receive data from a channel
 */
bool goo_channel_receive(GooChannel* channel, void* buffer, size_t buffer_size, size_t* size_received, int timeout_ms);

/**
 * Check if a channel is ready for sending
 */
bool goo_channel_can_send(GooChannel* channel, int timeout_ms);

/**
 * Check if a channel has data ready to receive
 */
bool goo_channel_can_receive(GooChannel* channel, int timeout_ms);

/**
 * Create a new message
 */
GooMessage* goo_message_create(const void* data, size_t size, const GooMessageOptions* options);

/**
 * Destroy a message
 */
void goo_message_destroy(GooMessage* message);

/**
 * Send a message on a channel
 */
bool goo_channel_send_message(GooChannel* channel, GooMessage* message);

/**
 * Receive a message from a channel
 */
GooMessage* goo_channel_receive_message(GooChannel* channel, int timeout_ms);

/**
 * Get data from a message
 */
const void* goo_message_get_data(const GooMessage* message, size_t* size);

#ifdef __cplusplus
}
#endif

#endif // GOO_CHANNELS_H 