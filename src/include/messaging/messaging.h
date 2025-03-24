/**
 * @file messaging.h
 * @brief First-class messaging system for the Goo programming language.
 *
 * This header provides a ZeroMQ-inspired messaging system with support for
 * multiple messaging patterns including publish-subscribe, push-pull,
 * and request-reply.
 */

#ifndef GOO_MESSAGING_H
#define GOO_MESSAGING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a message.
 */
typedef struct GooMessage GooMessage;

/**
 * @brief Opaque type representing a channel.
 */
typedef struct GooChannel GooChannel;

/**
 * @brief Enumeration of channel types/patterns.
 */
typedef enum GooChannelType {
    GOO_CHANNEL_TYPE_NORMAL,   /**< Regular buffered channel */
    GOO_CHANNEL_TYPE_PUBSUB,   /**< Publish-subscribe pattern */
    GOO_CHANNEL_TYPE_PUSHPULL, /**< Push-pull pattern */
    GOO_CHANNEL_TYPE_REQREP    /**< Request-reply pattern */
} GooChannelType;

/**
 * @brief Channel options for configuration.
 */
typedef struct GooChannelOptions {
    size_t buffer_size;       /**< Buffer size for message queuing */
    bool is_blocking;         /**< Whether operations block when buffer is full/empty */
    GooChannelType pattern;   /**< Channel messaging pattern */
    int timeout_ms;           /**< Timeout for blocking operations (-1 for infinite) */
    bool is_distributed;      /**< Whether the channel can communicate over network */
    const char* endpoint;     /**< Network endpoint for distributed channels */
} GooChannelOptions;

/**
 * @brief Initialize the messaging subsystem.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_messaging_init(void);

/**
 * @brief Clean up the messaging subsystem.
 */
void goo_messaging_cleanup(void);

/**
 * @brief Create a new message.
 *
 * @param data Pointer to the message data.
 * @param size Size of the message data in bytes.
 * @return A pointer to the new message, or NULL if creation fails.
 */
GooMessage* goo_message_create(const void* data, size_t size);

/**
 * @brief Create a new message with string data.
 *
 * @param str The string data (null-terminated).
 * @return A pointer to the new message, or NULL if creation fails.
 */
GooMessage* goo_message_create_string(const char* str);

/**
 * @brief Destroy a message.
 *
 * @param message The message to destroy.
 */
void goo_message_destroy(GooMessage* message);

/**
 * @brief Get the data from a message.
 *
 * @param message The message to get data from.
 * @param size Pointer to store the size of the data.
 * @return Pointer to the message data, or NULL if the message is invalid.
 */
const void* goo_message_get_data(const GooMessage* message, size_t* size);

/**
 * @brief Get the string data from a message.
 *
 * @param message The message to get data from.
 * @return Pointer to the string data, or NULL if the message is invalid or not a string.
 */
const char* goo_message_get_string(const GooMessage* message);

/**
 * @brief Create a new channel with default options.
 *
 * @param type The type of channel to create.
 * @return A pointer to the new channel, or NULL if creation fails.
 */
GooChannel* goo_channel_create(GooChannelType type);

/**
 * @brief Create a new channel with custom options.
 *
 * @param options The options to configure the channel with.
 * @return A pointer to the new channel, or NULL if creation fails.
 */
GooChannel* goo_channel_create_with_options(const GooChannelOptions* options);

/**
 * @brief Create a distributed channel that can communicate over network.
 *
 * @param type The type of channel to create.
 * @param endpoint The network endpoint (e.g., "tcp://127.0.0.1:5555").
 * @return A pointer to the new distributed channel, or NULL if creation fails.
 */
GooChannel* goo_channel_create_distributed(GooChannelType type, const char* endpoint);

/**
 * @brief Connect a channel to a remote endpoint.
 *
 * @param channel The channel to connect.
 * @param endpoint The network endpoint to connect to.
 * @return true if connection succeeds, false otherwise.
 */
bool goo_channel_connect(GooChannel* channel, const char* endpoint);

/**
 * @brief Destroy a channel.
 *
 * @param channel The channel to destroy.
 */
void goo_channel_destroy(GooChannel* channel);

/**
 * @brief Close a channel, preventing further sends but allowing receives.
 *
 * @param channel The channel to close.
 */
void goo_channel_close(GooChannel* channel);

/**
 * @brief Send a message on a channel.
 *
 * @param channel The channel to send on.
 * @param message The message to send.
 * @return true if the send succeeds, false otherwise.
 */
bool goo_channel_send(GooChannel* channel, GooMessage* message);

/**
 * @brief Send a message on a channel with a topic (for publish-subscribe channels).
 *
 * @param channel The channel to send on.
 * @param topic The topic to publish to.
 * @param message The message to send.
 * @return true if the send succeeds, false otherwise.
 */
bool goo_channel_publish(GooChannel* channel, const char* topic, GooMessage* message);

/**
 * @brief Receive a message from a channel.
 *
 * @param channel The channel to receive from.
 * @return The received message, or NULL if receive fails or channel is closed.
 */
GooMessage* goo_channel_receive(GooChannel* channel);

/**
 * @brief Receive a message with its topic from a publish-subscribe channel.
 *
 * @param channel The channel to receive from.
 * @param topic Pointer to store the received topic.
 * @return The received message, or NULL if receive fails or channel is closed.
 */
GooMessage* goo_channel_receive_with_topic(GooChannel* channel, char** topic);

/**
 * @brief Try to send a message on a channel (non-blocking).
 *
 * @param channel The channel to send on.
 * @param message The message to send.
 * @return true if the send succeeds, false if the channel buffer is full or channel is closed.
 */
bool goo_channel_try_send(GooChannel* channel, GooMessage* message);

/**
 * @brief Try to receive a message from a channel (non-blocking).
 *
 * @param channel The channel to receive from.
 * @return The received message, or NULL if no message is available or channel is closed.
 */
GooMessage* goo_channel_try_receive(GooChannel* channel);

/**
 * @brief Subscribe to a topic on a publish-subscribe channel.
 *
 * @param channel The channel to subscribe on.
 * @param topic The topic to subscribe to.
 * @return true if subscription succeeds, false otherwise.
 */
bool goo_channel_subscribe(GooChannel* channel, const char* topic);

/**
 * @brief Unsubscribe from a topic on a publish-subscribe channel.
 *
 * @param channel The channel to unsubscribe from.
 * @param topic The topic to unsubscribe from.
 * @return true if unsubscription succeeds, false otherwise.
 */
bool goo_channel_unsubscribe(GooChannel* channel, const char* topic);

/**
 * @brief Add a subscriber to a publisher channel.
 *
 * @param publisher The publisher channel.
 * @param subscriber The subscriber channel.
 * @return true if adding the subscriber succeeds, false otherwise.
 */
bool goo_channel_add_subscriber(GooChannel* publisher, GooChannel* subscriber);

/**
 * @brief Remove a subscriber from a publisher channel.
 *
 * @param publisher The publisher channel.
 * @param subscriber The subscriber to remove.
 * @return true if removing the subscriber succeeds, false otherwise.
 */
bool goo_channel_remove_subscriber(GooChannel* publisher, GooChannel* subscriber);

/**
 * @brief Send a request and wait for a reply.
 *
 * @param channel The request-reply channel.
 * @param request The request message.
 * @return The reply message, or NULL if the request fails or times out.
 */
GooMessage* goo_channel_request(GooChannel* channel, GooMessage* request);

/**
 * @brief Reply to a request.
 *
 * @param channel The request-reply channel.
 * @param request The request message to reply to.
 * @param reply The reply message.
 * @return true if the reply succeeds, false otherwise.
 */
bool goo_channel_reply(GooChannel* channel, GooMessage* request, GooMessage* reply);

/**
 * @brief Push a message to workers.
 *
 * @param channel The push-pull channel.
 * @param message The message to push.
 * @return true if the push succeeds, false otherwise.
 */
bool goo_channel_push(GooChannel* channel, GooMessage* message);

/**
 * @brief Pull a message from a distributor.
 *
 * @param channel The push-pull channel.
 * @return The pulled message, or NULL if the pull fails or times out.
 */
GooMessage* goo_channel_pull(GooChannel* channel);

/**
 * @brief Get the number of messages in a channel buffer.
 *
 * @param channel The channel to check.
 * @return The number of messages in the channel buffer.
 */
size_t goo_channel_message_count(GooChannel* channel);

/**
 * @brief Check if a channel is closed.
 *
 * @param channel The channel to check.
 * @return true if the channel is closed, false otherwise.
 */
bool goo_channel_is_closed(GooChannel* channel);

/**
 * @brief Get the capacity of a channel buffer.
 *
 * @param channel The channel to check.
 * @return The capacity of the channel buffer.
 */
size_t goo_channel_capacity(GooChannel* channel);

#ifdef __cplusplus
}
#endif

#endif /* GOO_MESSAGING_H */ 