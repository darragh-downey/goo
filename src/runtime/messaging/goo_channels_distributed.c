/**
 * goo_channels_distributed.c
 * 
 * Implementation of distributed channels for the Goo programming language.
 * This extends the basic channel system with network capabilities.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "messaging/goo_channels.h"
#include "memory.h"

// Forward declarations of transport layer functions
extern bool goo_transport_init(void);
extern void* goo_transport_create_socket(int type);
extern bool goo_transport_bind(void* socket, const char* endpoint);
extern bool goo_transport_connect(void* socket, const char* endpoint);
extern bool goo_transport_send(void* socket, const void* data, size_t size, int flags);
extern bool goo_transport_receive(void* socket, void* data, size_t size, size_t* received, int flags);
extern void goo_transport_close(void* socket);

// Local helper function prototypes
static void* channel_listener_thread(void* arg);
static bool init_distributed_channel(GooChannel* channel, const char* endpoint, GooChannelType type);

/**
 * Create a distributed channel that can communicate over the network.
 */
GooChannel* goo_create_distributed_channel(const GooChannelOptions* options, const char* endpoint, int elem_size) {
    if (!options || !endpoint) return NULL;
    
    // First create a normal channel
    GooChannel* channel = goo_channel_create(options);
    if (!channel) return NULL;
    
    // Then initialize the distributed features
    if (!init_distributed_channel(channel, endpoint, options->pattern)) {
        goo_channel_destroy(channel);
        return NULL;
    }
    
    // Set as distributed channel
    channel->is_distributed = true;
    
    // Store the element size for transport
    channel->elem_size = elem_size > 0 ? elem_size : sizeof(void*);
    
    return channel;
}

/**
 * Connect a channel to a remote endpoint.
 */
bool goo_channel_connect(GooChannel* channel, const char* endpoint) {
    if (!channel || !endpoint) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if channel is already distributed
    if (!channel->is_distributed) {
        // Initialize as distributed
        if (!init_distributed_channel(channel, endpoint, channel->type)) {
            pthread_mutex_unlock(&channel->mutex);
            return false;
        }
        channel->is_distributed = true;
    } else {
        // Already distributed, store the new endpoint
        char* new_endpoint = goo_strdup(endpoint);
        if (!new_endpoint) {
            pthread_mutex_unlock(&channel->mutex);
            return false;
        }
        
        if (channel->endpoint) {
            goo_free(channel->endpoint, strlen(channel->endpoint) + 1);
        }
        channel->endpoint = new_endpoint;
    }
    
    pthread_mutex_unlock(&channel->mutex);
    return true;
}

/**
 * Subscribe a channel to a topic (for SUB channels).
 */
bool goo_channel_subscribe(GooChannel* channel, const char* topic) {
    if (!channel || !topic) return false;
    
    // Only SUB channels can subscribe to topics
    if (channel->type != GOO_CHANNEL_SUB) {
        return false;
    }
    
    pthread_mutex_lock(&channel->mutex);
    
    // Create new subscription
    GooChannelSubscription* sub = (GooChannelSubscription*)goo_alloc(sizeof(GooChannelSubscription));
    if (!sub) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Copy the topic
    sub->topic = goo_strdup(topic);
    if (!sub->topic) {
        goo_free(sub, sizeof(GooChannelSubscription));
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    // Add to subscription list
    sub->next = channel->subscriptions;
    channel->subscriptions = sub;
    
    pthread_mutex_unlock(&channel->mutex);
    return true;
}

/**
 * Unsubscribe a channel from a topic.
 */
bool goo_channel_unsubscribe(GooChannel* channel, const char* topic) {
    if (!channel || !topic) return false;
    
    // Only SUB channels can unsubscribe from topics
    if (channel->type != GOO_CHANNEL_SUB) {
        return false;
    }
    
    pthread_mutex_lock(&channel->mutex);
    
    // Find the subscription
    GooChannelSubscription* prev = NULL;
    GooChannelSubscription* sub = channel->subscriptions;
    
    while (sub) {
        if (strcmp(sub->topic, topic) == 0) {
            // Found it, remove from list
            if (prev) {
                prev->next = sub->next;
            } else {
                channel->subscriptions = sub->next;
            }
            
            // Free resources
            goo_free(sub->topic, strlen(sub->topic) + 1);
            goo_free(sub, sizeof(GooChannelSubscription));
            
            pthread_mutex_unlock(&channel->mutex);
            return true;
        }
        
        prev = sub;
        sub = sub->next;
    }
    
    // Not found
    pthread_mutex_unlock(&channel->mutex);
    return false;
}

/**
 * Add a subscriber to a publisher channel.
 */
bool goo_channel_add_subscriber(GooChannel* publisher, GooChannel* subscriber) {
    if (!publisher || !subscriber) return false;
    
    // Only PUB channels can have subscribers
    if (publisher->type != GOO_CHANNEL_PUB_SUB) {
        return false;
    }
    
    pthread_mutex_lock(&publisher->mutex);
    
    // Create new subscriber
    GooChannelSubscriber* sub = (GooChannelSubscriber*)goo_alloc(sizeof(GooChannelSubscriber));
    if (!sub) {
        pthread_mutex_unlock(&publisher->mutex);
        return false;
    }
    
    // Set subscriber channel
    sub->channel = subscriber;
    
    // Add to subscriber list
    sub->next = publisher->subscribers;
    publisher->subscribers = sub;
    
    pthread_mutex_unlock(&publisher->mutex);
    return true;
}

/**
 * Remove a subscriber from a publisher channel.
 */
bool goo_channel_remove_subscriber(GooChannel* publisher, GooChannel* subscriber) {
    if (!publisher || !subscriber) return false;
    
    // Only PUB channels can have subscribers
    if (publisher->type != GOO_CHANNEL_PUB_SUB) {
        return false;
    }
    
    pthread_mutex_lock(&publisher->mutex);
    
    // Find the subscriber
    GooChannelSubscriber* prev = NULL;
    GooChannelSubscriber* sub = publisher->subscribers;
    
    while (sub) {
        if (sub->channel == subscriber) {
            // Found it, remove from list
            if (prev) {
                prev->next = sub->next;
            } else {
                publisher->subscribers = sub->next;
            }
            
            // Free resources
            goo_free(sub, sizeof(GooChannelSubscriber));
            
            pthread_mutex_unlock(&publisher->mutex);
            return true;
        }
        
        prev = sub;
        sub = sub->next;
    }
    
    // Not found
    pthread_mutex_unlock(&publisher->mutex);
    return false;
}

/**
 * Initialize a channel as a distributed channel.
 */
static bool init_distributed_channel(GooChannel* channel, const char* endpoint, GooChannelType type) {
    // Initialize the transport layer if needed
    if (!goo_transport_init()) {
        return false;
    }
    
    // Store the endpoint
    channel->endpoint = goo_strdup(endpoint);
    if (!channel->endpoint) {
        return false;
    }
    
    // Set up appropriate transport sockets based on channel type
    void* socket = NULL;
    
    switch (type) {
        case GOO_CHANNEL_PUB_SUB:
            // For PUB_SUB, we need a socket for publishing
            socket = goo_transport_create_socket(GOO_CHANNEL_PUB_SUB);
            if (!socket) goto error;
            
            // Bind or connect depending on endpoint format
            if (strncmp(endpoint, "bind:", 5) == 0) {
                const char* bind_addr = endpoint + 5;
                if (!goo_transport_bind(socket, bind_addr)) goto error;
            } else {
                if (!goo_transport_connect(socket, endpoint)) goto error;
            }
            break;
            
        case GOO_CHANNEL_PUSH_PULL:
            // For PUSH_PULL, create appropriate socket
            socket = goo_transport_create_socket(GOO_CHANNEL_PUSH_PULL);
            if (!socket) goto error;
            
            // Connect to endpoint
            if (!goo_transport_connect(socket, endpoint)) goto error;
            break;
            
        case GOO_CHANNEL_REQ_REP:
            // For REQ_REP, create appropriate socket
            socket = goo_transport_create_socket(GOO_CHANNEL_REQ_REP);
            if (!socket) goto error;
            
            // Connect to endpoint
            if (!goo_transport_connect(socket, endpoint)) goto error;
            break;
            
        default:
            // Not a distributed channel type
            goo_free(channel->endpoint, strlen(channel->endpoint) + 1);
            channel->endpoint = NULL;
            return false;
    }
    
    // Store the socket in the channel context field
    channel->context = socket;
    
    // Start listener thread for remote messages
    pthread_t thread;
    if (pthread_create(&thread, NULL, channel_listener_thread, channel) != 0) {
        goto error;
    }
    
    return true;
    
error:
    if (socket) goo_transport_close(socket);
    if (channel->endpoint) {
        goo_free(channel->endpoint, strlen(channel->endpoint) + 1);
        channel->endpoint = NULL;
    }
    return false;
}

/**
 * Thread function for listening to remote messages.
 */
static void* channel_listener_thread(void* arg) {
    GooChannel* channel = (GooChannel*)arg;
    if (!channel || !channel->context) return NULL;
    
    void* socket = channel->context;
    unsigned char buffer[4096]; // Temporary receive buffer
    size_t received;
    
    // Loop until channel is closed
    while (!channel->is_closed) {
        // Try to receive a message
        if (goo_transport_receive(socket, buffer, sizeof(buffer), &received, 0)) {
            // Got a message, forward it to the channel
            if (received > 0) {
                // Lock the channel
                pthread_mutex_lock(&channel->mutex);
                
                // Check if channel is still open
                if (!channel->is_closed) {
                    // Handle based on channel type
                    switch (channel->type) {
                        case GOO_CHANNEL_PUB_SUB:
                            // For PUB_SUB, distribute to subscribers
                            if (channel->subscribers) {
                                GooChannelSubscriber* sub = channel->subscribers;
                                while (sub) {
                                    if (sub->channel) {
                                        goo_channel_send(sub->channel, buffer, received, 0);
                                    }
                                    sub = sub->next;
                                }
                            } else {
                                // No subscribers, just add to this channel's buffer
                                goo_channel_send(channel, buffer, received, 0);
                            }
                            break;
                            
                        default:
                            // For other types, just add to this channel's buffer
                            goo_channel_send(channel, buffer, received, 0);
                            break;
                    }
                }
                
                pthread_mutex_unlock(&channel->mutex);
            }
        }
    }
    
    return NULL;
} 