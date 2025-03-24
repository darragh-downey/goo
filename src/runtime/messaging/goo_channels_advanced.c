#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>

#include "goo_runtime.h"
#include "goo_channels.h"
#include "goo_transport.h"

// Message structure for advanced patterns
typedef struct GooMessage {
    void* data;             // Message data
    size_t size;            // Size of data
    int flags;              // Message flags
    char* topic;            // For pub/sub patterns
    struct GooMessage* next; // For multi-part messages
    GooTransportEndpoint* source_endpoint; // Source endpoint for reply
} GooMessage;

// Message flags
#define GOO_MSG_NONE     0x00  // No special behavior
#define GOO_MSG_DONTWAIT 0x01  // Non-blocking operation
#define GOO_MSG_MORE     0x02  // More parts coming
#define GOO_MSG_TOPIC    0x04  // Message has a topic
#define GOO_MSG_REQ      0x08  // Request message (requires reply)
#define GOO_MSG_REP      0x10  // Reply message

// Advanced channel
typedef struct GooAdvancedChannel {
    GooChannel* base_channel;  // Underlying Goo channel
    GooChannelType type;       // Type of channel (pub/sub, push/pull, etc.)
    GooTransportEndpoint* endpoint; // Transport endpoint for distributed channels
    pthread_mutex_t mutex;    // Mutex for thread safety
    
    // For pub/sub
    struct {
        char** topics;          // Subscribed topics
        int topic_count;        // Number of topics
        GooChannel** subscribers; // Subscriber channels
        int subscriber_count;    // Number of subscribers
    } pub_sub;
    
    // For push/pull
    struct {
        int worker_count;       // Number of workers
        int round_robin_index;  // For round-robin distribution
    } push_pull;
    
    // For req/rep
    struct {
        int timeout_ms;         // Timeout for requests
        GooMessage* pending_req; // Pending request for reply
    } req_rep;
    
    // For broadcast
    struct {
        GooChannel** receivers;   // Receiver channels
        int receiver_count;       // Number of receivers
    } broadcast;
} GooAdvancedChannel;

// Create a message
GooMessage* goo_message_create(void* data, size_t size, int flags) {
    GooMessage* msg = (GooMessage*)malloc(sizeof(GooMessage));
    if (!msg) return NULL;
    
    msg->data = malloc(size);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    memcpy(msg->data, data, size);
    msg->size = size;
    msg->flags = flags;
    msg->topic = NULL;
    msg->next = NULL;
    msg->source_endpoint = NULL;
    
    return msg;
}

// Add a topic to a message
void goo_message_set_topic(GooMessage* msg, const char* topic) {
    if (!msg || !topic) return;
    
    if (msg->topic) free(msg->topic);
    msg->topic = strdup(topic);
    msg->flags |= GOO_MSG_TOPIC;
}

// Add a part to a multi-part message
bool goo_message_add_part(GooMessage* msg, void* data, size_t size, int flags) {
    if (!msg) return false;
    
    GooMessage* part = goo_message_create(data, size, flags);
    if (!part) return false;
    
    // Find the last part
    GooMessage* last = msg;
    while (last->next) {
        last = last->next;
    }
    
    last->next = part;
    return true;
}

// Destroy a message and all its parts
void goo_message_destroy(GooMessage* msg) {
    if (!msg) return;
    
    // Free all parts recursively
    if (msg->next) {
        goo_message_destroy(msg->next);
    }
    
    if (msg->data) free(msg->data);
    if (msg->topic) free(msg->topic);
    
    free(msg);
}

// Get the next part of a multi-part message
GooMessage* goo_message_next_part(GooMessage* msg) {
    if (!msg) return NULL;
    return msg->next;
}

// Create an advanced channel
GooAdvancedChannel* goo_advanced_channel_create(GooChannelType type, size_t element_size, size_t capacity) {
    GooAdvancedChannel* channel = (GooAdvancedChannel*)malloc(sizeof(GooAdvancedChannel));
    if (!channel) return NULL;
    
    // Create the base channel
    channel->base_channel = goo_channel_create(element_size, capacity, type);
    if (!channel->base_channel) {
        free(channel);
        return NULL;
    }
    
    channel->type = type;
    channel->endpoint = NULL;
    
    if (pthread_mutex_init(&channel->mutex, NULL) != 0) {
        goo_channel_free(channel->base_channel);
        free(channel);
        return NULL;
    }
    
    // Initialize pattern-specific data
    memset(&channel->pub_sub, 0, sizeof(channel->pub_sub));
    memset(&channel->push_pull, 0, sizeof(channel->push_pull));
    memset(&channel->req_rep, 0, sizeof(channel->req_rep));
    memset(&channel->broadcast, 0, sizeof(channel->broadcast));
    
    // Set defaults based on channel type
    switch (type) {
        case GOO_CHANNEL_PUB:
        case GOO_CHANNEL_SUB:
            channel->pub_sub.topics = NULL;
            channel->pub_sub.topic_count = 0;
            channel->pub_sub.subscribers = NULL;
            channel->pub_sub.subscriber_count = 0;
            break;
            
        case GOO_CHANNEL_PUSH:
        case GOO_CHANNEL_PULL:
            channel->push_pull.worker_count = 0;
            channel->push_pull.round_robin_index = 0;
            break;
            
        case GOO_CHANNEL_REQ:
        case GOO_CHANNEL_REP:
            channel->req_rep.timeout_ms = 5000; // Default 5 second timeout
            channel->req_rep.pending_req = NULL;
            break;
            
        case GOO_CHANNEL_BROADCAST:
            channel->broadcast.receivers = NULL;
            channel->broadcast.receiver_count = 0;
            break;
            
        default:
            break;
    }
    
    return channel;
}

// Destroy an advanced channel
void goo_advanced_channel_destroy(GooAdvancedChannel* channel) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Clean up pattern-specific resources
    switch (channel->type) {
        case GOO_CHANNEL_PUB:
        case GOO_CHANNEL_SUB:
            // Free topic strings
            for (int i = 0; i < channel->pub_sub.topic_count; i++) {
                if (channel->pub_sub.topics[i]) {
                    free(channel->pub_sub.topics[i]);
                }
            }
            free(channel->pub_sub.topics);
            free(channel->pub_sub.subscribers);
            break;
            
        case GOO_CHANNEL_REQ:
        case GOO_CHANNEL_REP:
            if (channel->req_rep.pending_req) {
                goo_message_destroy(channel->req_rep.pending_req);
            }
            break;
            
        case GOO_CHANNEL_BROADCAST:
            free(channel->broadcast.receivers);
            break;
            
        default:
            break;
    }
    
    // Destroy the transport endpoint if it exists
    if (channel->endpoint) {
        goo_transport_destroy(channel->endpoint);
    }
    
    pthread_mutex_unlock(&channel->mutex);
    pthread_mutex_destroy(&channel->mutex);
    
    // Destroy the base channel
    goo_channel_free(channel->base_channel);
    
    free(channel);
}

// Bind a distributed channel to an endpoint
bool goo_advanced_channel_bind(GooAdvancedChannel* channel, GooTransportProtocol protocol, const char* address, int port) {
    if (!channel || !address) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Create the endpoint if it doesn't exist
    if (!channel->endpoint) {
        channel->endpoint = goo_transport_create(protocol);
        if (!channel->endpoint) {
            pthread_mutex_unlock(&channel->mutex);
            return false;
        }
    }
    
    // Bind the endpoint
    bool success = goo_transport_bind(channel->endpoint, address, port);
    
    pthread_mutex_unlock(&channel->mutex);
    return success;
}

// Connect a distributed channel to an endpoint
bool goo_advanced_channel_connect(GooAdvancedChannel* channel, GooTransportProtocol protocol, const char* address, int port) {
    if (!channel || !address) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Create the endpoint if it doesn't exist
    if (!channel->endpoint) {
        channel->endpoint = goo_transport_create(protocol);
        if (!channel->endpoint) {
            pthread_mutex_unlock(&channel->mutex);
            return false;
        }
    }
    
    // Connect the endpoint
    bool success = goo_transport_connect(channel->endpoint, address, port);
    
    pthread_mutex_unlock(&channel->mutex);
    return success;
}

// ===== Publish/Subscribe Pattern =====

// Publish a message to a topic
bool goo_channel_publish(GooAdvancedChannel* channel, const char* topic, void* data, size_t size, int flags) {
    if (!channel || !data || size <= 0) return false;
    if (channel->type != GOO_CHANNEL_PUB) return false;
    
    // Create a message with the topic
    GooMessage* msg = goo_message_create(data, size, flags | GOO_MSG_TOPIC);
    if (!msg) return false;
    
    goo_message_set_topic(msg, topic);
    
    pthread_mutex_lock(&channel->mutex);
    
    // Send to all subscribers
    bool success = true;
    
    // If we have local subscribers, send to them
    for (int i = 0; i < channel->pub_sub.subscriber_count; i++) {
        GooChannel* sub = channel->pub_sub.subscribers[i];
        if (!goo_channel_send(sub, msg, flags)) {
            success = false;
        }
    }
    
    // If we have a transport endpoint, send over the network
    if (channel->endpoint) {
        // Serialize the message with topic
        size_t topic_len = strlen(topic);
        size_t total_size = sizeof(size_t) + topic_len + size;
        
        void* buffer = malloc(total_size);
        if (buffer) {
            size_t* size_ptr = (size_t*)buffer;
            *size_ptr = topic_len;
            
            char* topic_ptr = (char*)(size_ptr + 1);
            memcpy(topic_ptr, topic, topic_len);
            
            void* data_ptr = topic_ptr + topic_len;
            memcpy(data_ptr, data, size);
            
            int sent = goo_transport_send(channel->endpoint, buffer, total_size);
            free(buffer);
            
            if (sent < (int)total_size) {
                success = false;
            }
        } else {
            success = false;
        }
    }
    
    pthread_mutex_unlock(&channel->mutex);
    
    // Clean up
    goo_message_destroy(msg);
    
    return success;
}

// Subscribe to a topic
bool goo_channel_subscribe(GooAdvancedChannel* channel, const char* topic) {
    if (!channel || !topic) return false;
    if (channel->type != GOO_CHANNEL_SUB) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if we're already subscribed to this topic
    for (int i = 0; i < channel->pub_sub.topic_count; i++) {
        if (strcmp(channel->pub_sub.topics[i], topic) == 0) {
            pthread_mutex_unlock(&channel->mutex);
            return true; // Already subscribed
        }
    }
    
    // Add the topic to our list
    char** new_topics = realloc(channel->pub_sub.topics, 
                               (channel->pub_sub.topic_count + 1) * sizeof(char*));
    if (!new_topics) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    channel->pub_sub.topics = new_topics;
    channel->pub_sub.topics[channel->pub_sub.topic_count] = strdup(topic);
    if (!channel->pub_sub.topics[channel->pub_sub.topic_count]) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    channel->pub_sub.topic_count++;
    
    pthread_mutex_unlock(&channel->mutex);
    return true;
}

// Connect a publisher to a subscriber
bool goo_channel_connect_pub_sub(GooAdvancedChannel* pub, GooAdvancedChannel* sub) {
    if (!pub || !sub) return false;
    if (pub->type != GOO_CHANNEL_PUB || sub->type != GOO_CHANNEL_SUB) return false;
    
    pthread_mutex_lock(&pub->mutex);
    
    // Add the subscriber to our list
    GooChannel** new_subscribers = realloc(pub->pub_sub.subscribers, 
                                         (pub->pub_sub.subscriber_count + 1) * sizeof(GooChannel*));
    if (!new_subscribers) {
        pthread_mutex_unlock(&pub->mutex);
        return false;
    }
    
    pub->pub_sub.subscribers = new_subscribers;
    pub->pub_sub.subscribers[pub->pub_sub.subscriber_count] = sub->base_channel;
    pub->pub_sub.subscriber_count++;
    
    pthread_mutex_unlock(&pub->mutex);
    return true;
}

// ===== Push/Pull Pattern =====

// Push a task to workers
bool goo_channel_push(GooAdvancedChannel* channel, void* data, size_t size, int flags) {
    if (!channel || !data || size <= 0) return false;
    if (channel->type != GOO_CHANNEL_PUSH) return false;
    
    // For local push, just send to the base channel
    if (!channel->endpoint) {
        return goo_channel_send(channel->base_channel, data, flags);
    }
    
    // For distributed push, send over the transport
    int sent = goo_transport_send(channel->endpoint, data, size);
    return (sent == (int)size);
}

// Pull a task from pushers
bool goo_channel_pull(GooAdvancedChannel* channel, void* data, size_t size, int flags) {
    if (!channel || !data || size <= 0) return false;
    if (channel->type != GOO_CHANNEL_PULL) return false;
    
    // For local pull, just receive from the base channel
    if (!channel->endpoint) {
        return goo_channel_recv(channel->base_channel, data, flags);
    }
    
    // For distributed pull, receive from the transport
    int received = goo_transport_recv(channel->endpoint, data, size);
    return (received == (int)size);
}

// ===== Request/Reply Pattern =====

// Send a request and get a reply (synchronous)
bool goo_channel_request(GooAdvancedChannel* channel, void* request_data, size_t request_size, 
                        void* reply_data, size_t* reply_size, int flags) {
    if (!channel || !request_data || request_size <= 0 || !reply_data || !reply_size) return false;
    if (channel->type != GOO_CHANNEL_REQ) return false;
    
    // Create a request message
    GooMessage* request = goo_message_create(request_data, request_size, flags | GOO_MSG_REQ);
    if (!request) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    bool success = false;
    
    // Store the pending request
    channel->req_rep.pending_req = request;
    
    // For local request, send to the base channel
    if (!channel->endpoint) {
        // Send the request
        if (goo_channel_send(channel->base_channel, request_data, flags)) {
            // Wait for reply
            void* temp_reply = malloc(*reply_size);
            if (temp_reply) {
                if (goo_channel_recv(channel->base_channel, temp_reply, flags)) {
                    // Copy the reply to the output buffer
                    memcpy(reply_data, temp_reply, *reply_size);
                    success = true;
                }
                free(temp_reply);
            }
        }
    } else {
        // For distributed request, send over the transport
        int sent = goo_transport_send(channel->endpoint, request_data, request_size);
        if (sent == (int)request_size) {
            // Wait for reply
            int received = goo_transport_recv(channel->endpoint, reply_data, *reply_size);
            if (received > 0) {
                *reply_size = received;
                success = true;
            }
        }
    }
    
    // Clear the pending request
    channel->req_rep.pending_req = NULL;
    
    pthread_mutex_unlock(&channel->mutex);
    
    // Clean up
    goo_message_destroy(request);
    
    return success;
}

// Receive a request and send a reply (server side)
bool goo_channel_reply(GooAdvancedChannel* channel, void* request_buffer, size_t* request_size,
                      void* reply_data, size_t reply_size, int flags) {
    if (!channel || !request_buffer || !request_size || !reply_data || reply_size <= 0) return false;
    if (channel->type != GOO_CHANNEL_REP) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    bool success = false;
    
    // For local reply, receive from and send to the base channel
    if (!channel->endpoint) {
        // Receive the request
        if (goo_channel_recv(channel->base_channel, request_buffer, flags)) {
            // Send the reply
            if (goo_channel_send(channel->base_channel, reply_data, flags)) {
                success = true;
            }
        }
    } else {
        // For distributed reply, receive from and send to the transport
        int received = goo_transport_recv(channel->endpoint, request_buffer, *request_size);
        if (received > 0) {
            *request_size = received;
            
            // Send the reply
            int sent = goo_transport_send(channel->endpoint, reply_data, reply_size);
            if (sent == (int)reply_size) {
                success = true;
            }
        }
    }
    
    pthread_mutex_unlock(&channel->mutex);
    return success;
}

// ===== Broadcast Pattern =====

// Add a receiver to a broadcast channel
bool goo_channel_add_receiver(GooAdvancedChannel* channel, GooChannel* receiver) {
    if (!channel || !receiver) return false;
    if (channel->type != GOO_CHANNEL_BROADCAST) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Add the receiver to our list
    GooChannel** new_receivers = realloc(channel->broadcast.receivers, 
                                       (channel->broadcast.receiver_count + 1) * sizeof(GooChannel*));
    if (!new_receivers) {
        pthread_mutex_unlock(&channel->mutex);
        return false;
    }
    
    channel->broadcast.receivers = new_receivers;
    channel->broadcast.receivers[channel->broadcast.receiver_count] = receiver;
    channel->broadcast.receiver_count++;
    
    pthread_mutex_unlock(&channel->mutex);
    return true;
}

// Broadcast a message to all receivers
bool goo_channel_broadcast(GooAdvancedChannel* channel, void* data, size_t size, int flags) {
    if (!channel || !data || size <= 0) return false;
    if (channel->type != GOO_CHANNEL_BROADCAST) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    bool success = true;
    
    // Send to all receivers
    for (int i = 0; i < channel->broadcast.receiver_count; i++) {
        GooChannel* receiver = channel->broadcast.receivers[i];
        if (!goo_channel_send(receiver, data, flags)) {
            success = false;
        }
    }
    
    // If we have a transport endpoint, broadcast over the network
    if (channel->endpoint) {
        int sent = goo_transport_send(channel->endpoint, data, size);
        if (sent < (int)size) {
            success = false;
        }
    }
    
    pthread_mutex_unlock(&channel->mutex);
    return success;
}

// ===== Convenience Functions =====

// Create a publisher channel
GooAdvancedChannel* goo_pub_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_PUB, element_size, capacity);
}

// Create a subscriber channel
GooAdvancedChannel* goo_sub_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_SUB, element_size, capacity);
}

// Create a push channel
GooAdvancedChannel* goo_push_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_PUSH, element_size, capacity);
}

// Create a pull channel
GooAdvancedChannel* goo_pull_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_PULL, element_size, capacity);
}

// Create a request channel
GooAdvancedChannel* goo_req_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_REQ, element_size, capacity);
}

// Create a reply channel
GooAdvancedChannel* goo_rep_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_REP, element_size, capacity);
}

// Create a broadcast channel
GooAdvancedChannel* goo_broadcast_channel_create(size_t element_size, size_t capacity) {
    return goo_advanced_channel_create(GOO_CHANNEL_BROADCAST, element_size, capacity);
}
