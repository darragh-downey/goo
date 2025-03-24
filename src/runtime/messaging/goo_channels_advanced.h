#ifndef GOO_CHANNELS_ADVANCED_H
#define GOO_CHANNELS_ADVANCED_H

#include <stdlib.h>
#include <stdbool.h>
#include "goo_runtime.h"
#include "goo_channels.h"
#include "goo_transport.h"

// Forward declarations
typedef struct GooMessage GooMessage;
typedef struct GooAdvancedChannel GooAdvancedChannel;

// Message flags
#define GOO_MSG_NONE     0x00  // No special behavior
#define GOO_MSG_DONTWAIT 0x01  // Non-blocking operation
#define GOO_MSG_MORE     0x02  // More parts coming
#define GOO_MSG_TOPIC    0x04  // Message has a topic
#define GOO_MSG_REQ      0x08  // Request message (requires reply)
#define GOO_MSG_REP      0x10  // Reply message

// ===== Message API =====

// Create a message
GooMessage* goo_message_create(void* data, size_t size, int flags);

// Add a topic to a message
void goo_message_set_topic(GooMessage* msg, const char* topic);

// Add a part to a multi-part message
bool goo_message_add_part(GooMessage* msg, void* data, size_t size, int flags);

// Destroy a message and all its parts
void goo_message_destroy(GooMessage* msg);

// Get the next part of a multi-part message
GooMessage* goo_message_next_part(GooMessage* msg);

// ===== Channel API =====

// Create an advanced channel
GooAdvancedChannel* goo_advanced_channel_create(GooChannelType type, size_t element_size, size_t capacity);

// Destroy an advanced channel
void goo_advanced_channel_destroy(GooAdvancedChannel* channel);

// Bind a distributed channel to an endpoint
bool goo_advanced_channel_bind(GooAdvancedChannel* channel, GooTransportProtocol protocol, 
                              const char* address, int port);

// Connect a distributed channel to an endpoint
bool goo_advanced_channel_connect(GooAdvancedChannel* channel, GooTransportProtocol protocol, 
                                 const char* address, int port);

// ===== Publish/Subscribe Pattern =====

// Create a publisher channel
GooAdvancedChannel* goo_pub_channel_create(size_t element_size, size_t capacity);

// Create a subscriber channel
GooAdvancedChannel* goo_sub_channel_create(size_t element_size, size_t capacity);

// Publish a message to a topic
bool goo_channel_publish(GooAdvancedChannel* channel, const char* topic, void* data, 
                        size_t size, int flags);

// Subscribe to a topic
bool goo_channel_subscribe(GooAdvancedChannel* channel, const char* topic);

// Connect a publisher to a subscriber
bool goo_channel_connect_pub_sub(GooAdvancedChannel* pub, GooAdvancedChannel* sub);

// ===== Push/Pull Pattern =====

// Create a push channel
GooAdvancedChannel* goo_push_channel_create(size_t element_size, size_t capacity);

// Create a pull channel
GooAdvancedChannel* goo_pull_channel_create(size_t element_size, size_t capacity);

// Push a task to workers
bool goo_channel_push(GooAdvancedChannel* channel, void* data, size_t size, int flags);

// Pull a task from pushers
bool goo_channel_pull(GooAdvancedChannel* channel, void* data, size_t size, int flags);

// ===== Request/Reply Pattern =====

// Create a request channel
GooAdvancedChannel* goo_req_channel_create(size_t element_size, size_t capacity);

// Create a reply channel
GooAdvancedChannel* goo_rep_channel_create(size_t element_size, size_t capacity);

// Send a request and get a reply (synchronous)
bool goo_channel_request(GooAdvancedChannel* channel, void* request_data, size_t request_size,
                        void* reply_data, size_t* reply_size, int flags);

// Receive a request and send a reply (server side)
bool goo_channel_reply(GooAdvancedChannel* channel, void* request_buffer, size_t* request_size,
                      void* reply_data, size_t reply_size, int flags);

// ===== Broadcast Pattern =====

// Create a broadcast channel
GooAdvancedChannel* goo_broadcast_channel_create(size_t element_size, size_t capacity);

// Add a receiver to a broadcast channel
bool goo_channel_add_receiver(GooAdvancedChannel* channel, GooChannel* receiver);

// Broadcast a message to all receivers
bool goo_channel_broadcast(GooAdvancedChannel* channel, void* data, size_t size, int flags);

#endif // GOO_CHANNELS_ADVANCED_H 