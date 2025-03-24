/**
 * messaging_wrapper.c
 * 
 * C wrapper functions for the Goo messaging system implemented in Zig.
 * This file provides C functions to interact with the messaging system
 * including message creation, channel management, and messaging patterns.
 */

#include "../../include/messaging/messaging.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations for Zig-exported functions
extern bool goo_messaging_init(void);
extern void goo_messaging_cleanup(void);

// Message-related functions
extern GooMessage* goo_message_create(const void* data, size_t size);
extern GooMessage* goo_message_create_int(int64_t value);
extern GooMessage* goo_message_create_float(double value);
extern GooMessage* goo_message_create_bool(bool value);
extern GooMessage* goo_message_create_string(const char* value);
extern void goo_message_destroy(GooMessage* msg);
extern bool goo_message_get_int(const GooMessage* msg, int64_t* value);
extern bool goo_message_get_float(const GooMessage* msg, double* value);
extern bool goo_message_get_bool(const GooMessage* msg, bool* value);
extern const char* goo_message_get_string(const GooMessage* msg);
extern const char* goo_message_get_topic(const GooMessage* msg);
extern bool goo_message_set_topic(GooMessage* msg, const char* topic);

// Channel-related functions
extern GooChannel* goo_channel_create(GooChannelType type);
extern void goo_channel_destroy(GooChannel* channel);
extern bool goo_channel_send(GooChannel* channel, GooMessage* msg);
extern GooMessage* goo_channel_receive(GooChannel* channel);
extern GooMessage* goo_channel_receive_timeout(GooChannel* channel, uint64_t timeout_ms);
extern GooMessage* goo_channel_receive_wait(GooChannel* channel);
extern bool goo_channel_subscribe(GooChannel* channel, const char* topic);
extern bool goo_channel_add_subscriber(GooChannel* publisher, GooChannel* subscriber);
extern bool goo_channel_publish(GooChannel* publisher, const char* topic, GooMessage* msg);
extern bool goo_channel_process_request(GooChannel* channel, GooMessage* (*processor)(GooMessage*));

// C implementation of messaging functions

GooMessage* goo_message_create_with_data(const void* data, size_t size) {
    return goo_message_create(data, size);
}

GooChannel* goo_channel_create_with_options(const GooChannelOptions* options) {
    GooChannel* channel = goo_channel_create(options->pattern);
    
    if (channel == NULL) {
        return NULL;
    }
    
    // We'd implement custom options here if needed
    return channel;
}

GooChannel* goo_channel_create_distributed(GooChannelType type, const char* endpoint) {
    // This is a stub - actual distributed support would be implemented here
    GooChannel* channel = goo_channel_create(type);
    
    // Set up distributed channel properties
    if (channel != NULL && endpoint != NULL) {
        // Future: Configure distributed properties
    }
    
    return channel;
}

bool goo_channel_connect(GooChannel* channel, const char* endpoint) {
    // This is a stub - actual connection code would be implemented here
    return channel != NULL && endpoint != NULL;
}

void goo_channel_close(GooChannel* channel) {
    // For now, we just destroy the channel
    // Future: Implement proper closing behavior
    if (channel != NULL) {
        // Mark as closed but don't destroy yet
    }
}

GooMessage* goo_channel_receive_with_topic(GooChannel* channel, char** topic) {
    if (channel == NULL) {
        return NULL;
    }
    
    GooMessage* msg = goo_channel_receive(channel);
    if (msg != NULL && topic != NULL) {
        const char* msg_topic = goo_message_get_topic(msg);
        if (msg_topic != NULL) {
            *topic = strdup(msg_topic);
        } else {
            *topic = NULL;
        }
    }
    
    return msg;
}

bool goo_channel_try_send(GooChannel* channel, GooMessage* message) {
    return goo_channel_send(channel, message);
}

GooMessage* goo_channel_try_receive(GooChannel* channel) {
    return goo_channel_receive(channel);
}

bool goo_channel_unsubscribe(GooChannel* channel, const char* topic) {
    // Not implemented yet
    return false;
}

bool goo_channel_remove_subscriber(GooChannel* publisher, GooChannel* subscriber) {
    // Not implemented yet
    return false;
}

GooMessage* goo_channel_request(GooChannel* channel, GooMessage* request) {
    // Stub implementation - would need to be connected to the Zig implementation
    // in the future
    
    // Send the request
    if (!goo_channel_send(channel, request)) {
        return NULL;
    }
    
    // Wait for a reply
    return goo_channel_receive_timeout(channel, 5000); // 5 second timeout
}

bool goo_channel_reply(GooChannel* channel, GooMessage* request, GooMessage* reply) {
    // This is a stub - actual implementation would need better connection to Zig
    return goo_channel_send(channel, reply);
}

bool goo_channel_push(GooChannel* channel, GooMessage* message) {
    return goo_channel_send(channel, message);
}

GooMessage* goo_channel_pull(GooChannel* channel) {
    return goo_channel_receive(channel);
}

size_t goo_channel_message_count(GooChannel* channel) {
    // Not implemented yet
    return 0;
}

bool goo_channel_is_closed(GooChannel* channel) {
    // Not implemented yet
    return false;
}

size_t goo_channel_capacity(GooChannel* channel) {
    // Not implemented yet
    return 0;
}

// Convenience functions for specific messaging patterns

GooChannel* goo_publisher_create(const char* endpoint) {
    return goo_channel_create_distributed(GOO_CHANNEL_TYPE_PUBSUB, endpoint);
}

GooChannel* goo_subscriber_create(const char* endpoint) {
    GooChannel* sub = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    if (sub && endpoint) {
        goo_channel_connect(sub, endpoint);
    }
    return sub;
}

GooChannel* goo_push_socket_create(const char* endpoint) {
    return goo_channel_create_distributed(GOO_CHANNEL_TYPE_PUSHPULL, endpoint);
}

GooChannel* goo_pull_socket_create(const char* endpoint) {
    GooChannel* pull = goo_channel_create(GOO_CHANNEL_TYPE_PUSHPULL);
    if (pull && endpoint) {
        goo_channel_connect(pull, endpoint);
    }
    return pull;
}

GooChannel* goo_request_socket_create(const char* endpoint) {
    GooChannel* req = goo_channel_create(GOO_CHANNEL_TYPE_REQREP);
    if (req && endpoint) {
        goo_channel_connect(req, endpoint);
    }
    return req;
}

GooChannel* goo_reply_socket_create(const char* endpoint) {
    return goo_channel_create_distributed(GOO_CHANNEL_TYPE_REQREP, endpoint);
} 