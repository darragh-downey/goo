/**
 * Goo Messaging System - C API Header
 * 
 * This header defines the C API for the Goo messaging system
 * which is implemented in Zig.
 */

#ifndef GOO_CHANNELS_CORE_H
#define GOO_CHANNELS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Channel types */
typedef enum {
    Normal = 0,  // Normal bidirectional channel
    Pub = 1,     // Publisher channel (one-to-many)
    Sub = 2,     // Subscriber channel (many-to-one)
    Push = 3,    // Push channel (distributes to workers)
    Pull = 4,    // Pull channel (receives from push)
    Req = 5,     // Request channel (sends requests)
    Rep = 6      // Reply channel (processes requests)
} GooChannelType;

/* Channel options */
typedef enum {
    GOO_CHAN_DEFAULT = 0,     // Default options
    GOO_CHAN_NONBLOCKING = 1, // Non-blocking operations
    GOO_CHAN_BUFFERED = 2,    // Buffered channel
    GOO_CHAN_UNBUFFERED = 4,  // Unbuffered (synchronous) channel
    GOO_CHAN_RELIABLE = 8     // Reliable delivery
} GooChannelOptions;

/* Transport protocols for distributed channels */
typedef enum {
    GOO_TRANSPORT_INPROC = 0, // In-process transport
    GOO_TRANSPORT_IPC = 1,    // Inter-process transport
    GOO_TRANSPORT_TCP = 2     // TCP/IP transport
} GooTransportType;

/* Message flags */
typedef enum {
    GOO_MSG_NONE = 0,     // No special flags
    GOO_MSG_DONTWAIT = 1, // Non-blocking operation
    GOO_MSG_MORE = 2,     // More message parts to follow
    GOO_MSG_PRIORITY = 4  // High priority message
} GooMessageFlags;

/* Opaque structure pointers */
typedef struct GooChannel GooChannel;
typedef struct GooMessage GooMessage;
typedef struct GooEndpoint GooEndpoint;
typedef struct GooSubscription GooSubscription;

/* Channel statistics structure */
typedef struct {
    uint64_t messages_sent;     // Total messages sent
    uint64_t messages_received; // Total messages received
    uint64_t bytes_sent;        // Total bytes sent
    uint64_t bytes_received;    // Total bytes received
    uint64_t max_queue_size;    // Maximum queue size reached
    uint64_t send_timeouts;     // Send operation timeouts
    uint64_t recv_timeouts;     // Receive operation timeouts
} GooChannelStats;

/* Core channel API */
GooChannel* goo_channel_create(GooChannelType type, size_t elem_size, size_t buffer_size, GooChannelOptions options);
void goo_channel_destroy(GooChannel* channel);
int goo_channel_send(GooChannel* channel, const void* data, size_t size, GooMessageFlags flags);
int goo_channel_recv(GooChannel* channel, void* data, size_t size, GooMessageFlags flags);
int goo_channel_close(GooChannel* channel);
GooChannelStats goo_channel_stats(GooChannel* channel);

/* Publish/Subscribe API */
int goo_channel_subscribe_topic(GooChannel* channel, const char* topic,
                               void (*callback)(GooMessage* msg, void* context),
                               void* context);
int goo_channel_unsubscribe_topic(GooChannel* channel, const char* topic);
int goo_channel_publish(GooChannel* channel, GooMessage* msg);

/* Message API */
GooMessage* goo_message_create(void* data, size_t size, GooMessageFlags flags);
GooMessage* goo_message_create_copy(const void* data, size_t size, GooMessageFlags flags);
void goo_message_destroy(GooMessage* msg);
GooMessage* goo_message_multipart(GooMessage** parts, size_t count);
size_t goo_message_part_count(GooMessage* msg);

/* Distributed channels API */
int goo_channel_set_endpoint(GooChannel* channel, int protocol, const char* address, uint16_t port);
int goo_channel_connect(GooChannel* channel, int protocol, const char* address, uint16_t port);
int goo_channel_disconnect(GooChannel* channel, const char* endpoint);

#ifdef __cplusplus
}
#endif

#endif /* GOO_CHANNELS_CORE_H */ 