/**
 * goo_channels.c
 * 
 * Implementation of the channel system for the Goo programming language.
 * This implements the basic channel operations, focusing on in-process communication.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include "messaging/goo_channels.h"

// Local helper functions
static bool channel_init_buffer(GooChannel* channel);
static bool channel_is_distributed_type(GooChannelType type);
static void channel_update_stats_send(GooChannel* channel, size_t size, bool success);
static void channel_update_stats_receive(GooChannel* channel, size_t size, bool success);

// Create a new channel
GooChannel* goo_channel_create(const GooChannelOptions* options) {
    if (!options) return NULL;
    
    GooChannel* channel = (GooChannel*)malloc(sizeof(GooChannel));
    if (!channel) return NULL;
    
    memset(channel, 0, sizeof(GooChannel));
    channel->type = options->pattern;
    channel->options = options->is_blocking ? 0 : 1; // Set non-blocking flag
    channel->buffer_size = options->buffer_size > 0 ? options->buffer_size : 1; // Minimum buffer size is 1
    channel->elem_size = sizeof(GooMessage*); // We store message pointers in the buffer
    channel->high_water_mark = channel->buffer_size;
    channel->low_water_mark = channel->buffer_size / 2;
    channel->timeout_ms = options->timeout_ms; // Default timeout from options
    
    // Initialize buffer
    channel->buffer = malloc(channel->buffer_size * channel->elem_size);
    if (!channel->buffer) {
        free(channel);
        return NULL;
    }
    
    // Initialize synchronization primitives
    if (pthread_mutex_init(&channel->mutex, NULL) != 0) {
        free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    if (pthread_cond_init(&channel->send_cond, NULL) != 0) {
        pthread_mutex_destroy(&channel->mutex);
        free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    if (pthread_cond_init(&channel->recv_cond, NULL) != 0) {
        pthread_cond_destroy(&channel->send_cond);
        pthread_mutex_destroy(&channel->mutex);
        free(channel->buffer);
        free(channel);
        return NULL;
    }
    
    // Setup is complete
    channel->is_distributed = false; // Default to local channels
    
    return channel;
}

// Close a channel
void goo_channel_close(GooChannel* channel) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Mark channel as closed
    channel->is_closed = true;
    
    // Wake up any waiting senders and receivers
    pthread_cond_broadcast(&channel->send_cond);
    pthread_cond_broadcast(&channel->recv_cond);
    
    // Clean up network connections for distributed channels
    if (channel->is_distributed && channel->endpoint) {
        // Close network connections
        if (channel->type == GOO_CHANNEL_PUB_SUB) {
            // Close publisher endpoints
            // This would call into the networking layer to properly shut down connections
        } else if (channel->type == GOO_CHANNEL_PULL_PUSH) {
            // Clean up pull/push connections
        } else if (channel->type == GOO_CHANNEL_REQ_REP) {
            // Close request/reply sockets
        }
        
        // Free the endpoint resources
        free(channel->endpoint);
        channel->endpoint = NULL;
    }
    
    // If there are subscribers, notify them that the channel is closed
    if (channel->type == GOO_CHANNEL_PUB_SUB && channel->subscribers) {
        GooChannelSubscriber* sub = (GooChannelSubscriber*)channel->subscribers;
        while (sub) {
            GooChannelSubscriber* next = sub->next;
            // Notify subscriber of channel closure
            if (sub->channel) {
                pthread_mutex_lock(&sub->channel->mutex);
                sub->channel->is_closed = true;
                pthread_cond_broadcast(&sub->channel->recv_cond);
                pthread_mutex_unlock(&sub->channel->mutex);
            }
            sub = next;
        }
    }
    
    pthread_mutex_unlock(&channel->mutex);
}

// Destroy a channel and free resources
void goo_channel_destroy(GooChannel* channel) {
    if (!channel) return;
    
    // Close the channel first
    if (!channel->is_closed) {
        goo_channel_close(channel);
    }
    
    // Clean up any subscriptions
    if (channel->type == GOO_CHANNEL_SUB) {
        GooChannelSubscription* sub = channel->subscriptions;
        while (sub) {
            GooChannelSubscription* next = sub->next;
            free(sub->topic);
            free(sub);
            sub = next;
        }
    }
    
    // Clean up subscribers if this is a publisher
    if (channel->type == GOO_CHANNEL_PUB_SUB && channel->subscribers) {
        GooChannelSubscriber* sub = channel->subscribers;
        while (sub) {
            GooChannelSubscriber* next = sub->next;
            free(sub);
            sub = next;
        }
    }
    
    // Free the buffer
    if (channel->buffer) {
        free(channel->buffer);
    }
    
    // Destroy synchronization primitives
    pthread_cond_destroy(&channel->recv_cond);
    pthread_cond_destroy(&channel->send_cond);
    pthread_mutex_destroy(&channel->mutex);
    
    // Free the channel structure
    free(channel);
}

// Send data to a channel
bool goo_channel_send(GooChannel* channel, const void* data, size_t size, GooMessageFlags flags) {
    if (!channel || !data || size == 0) return false;
    
    // Non-blocking mode
    if (flags & GOO_MSG_DONTWAIT || channel->options & GOO_CHAN_NONBLOCKING) {
        return goo_channel_try_send(channel, data, size, flags);
    }
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if channel is closed
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_send(channel, size, false);
        return false;
    }
    
    // Handle unbuffered channels (direct synchronization with receiver)
    if (channel->options & GOO_CHAN_UNBUFFERED) {
        // Wait for a receiver
        while (channel->count == 0 && !channel->is_closed) {
            pthread_cond_wait(&channel->recv_cond, &channel->mutex);
        }
        
        // Check if channel was closed while waiting
        if (channel->is_closed) {
            pthread_mutex_unlock(&channel->mutex);
            channel_update_stats_send(channel, size, false);
            return false;
        }
        
        // Copy data directly to receiver
        memcpy(channel->buffer, data, size < channel->elem_size ? size : channel->elem_size);
        channel->count = 1;
        
        // Signal receiver
        pthread_cond_signal(&channel->send_cond);
        pthread_mutex_unlock(&channel->mutex);
        
        channel_update_stats_send(channel, size, true);
        return true;
    }
    
    // Handle buffered channels
    // Wait if buffer is full
    while (channel->count >= channel->buffer_size && !channel->is_closed) {
        pthread_cond_wait(&channel->recv_cond, &channel->mutex);
    }
    
    // Check if channel was closed while waiting
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_send(channel, size, false);
        return false;
    }
    
    // Calculate position in circular buffer
    size_t pos = (channel->tail + channel->count) % channel->buffer_size;
    
    // Copy data to buffer
    void* dest = (char*)channel->buffer + (pos * channel->elem_size);
    memcpy(dest, data, size < channel->elem_size ? size : channel->elem_size);
    
    // Update buffer state
    channel->count++;
    
    // Signal waiting receivers
    pthread_cond_signal(&channel->send_cond);
    
    pthread_mutex_unlock(&channel->mutex);
    
    channel_update_stats_send(channel, size, true);
    return true;
}

// Receive data from a channel
bool goo_channel_receive(GooChannel* channel, void* data, size_t size, GooMessageFlags flags) {
    if (!channel || !data || size == 0) return false;
    
    // Non-blocking mode
    if (flags & GOO_MSG_DONTWAIT || channel->options & GOO_CHAN_NONBLOCKING) {
        return goo_channel_try_receive(channel, data, size, flags);
    }
    
    pthread_mutex_lock(&channel->mutex);
    
    // Handle unbuffered channels (direct synchronization with sender)
    if (channel->options & GOO_CHAN_UNBUFFERED) {
        // Signal that we're ready to receive
        channel->count = 0;
        pthread_cond_signal(&channel->recv_cond);
        
        // Wait for sender
        while (channel->count == 0 && !channel->is_closed) {
            pthread_cond_wait(&channel->send_cond, &channel->mutex);
        }
        
        // Check if channel was closed while waiting
        if (channel->is_closed && channel->count == 0) {
            pthread_mutex_unlock(&channel->mutex);
            channel_update_stats_receive(channel, size, false);
            return false;
        }
        
        // Copy data directly from sender
        memcpy(data, channel->buffer, size < channel->elem_size ? size : channel->elem_size);
        channel->count = 0;
        
        pthread_mutex_unlock(&channel->mutex);
        
        channel_update_stats_receive(channel, size, true);
        return true;
    }
    
    // Handle buffered channels
    // Wait if buffer is empty
    while (channel->count == 0 && !channel->is_closed) {
        pthread_cond_wait(&channel->send_cond, &channel->mutex);
    }
    
    // Handle closed channel with no data
    if (channel->count == 0 && channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_receive(channel, size, false);
        return false;
    }
    
    // Copy data from buffer
    void* src = (char*)channel->buffer + (channel->head * channel->elem_size);
    memcpy(data, src, size < channel->elem_size ? size : channel->elem_size);
    
    // Update buffer state
    channel->head = (channel->head + 1) % channel->buffer_size;
    channel->count--;
    
    // Signal waiting senders
    pthread_cond_signal(&channel->recv_cond);
    
    pthread_mutex_unlock(&channel->mutex);
    
    channel_update_stats_receive(channel, size, true);
    return true;
}

// Try sending data to a channel (non-blocking)
bool goo_channel_try_send(GooChannel* channel, const void* data, size_t size, GooMessageFlags flags) {
    if (!channel || !data || size == 0) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if channel is closed
    if (channel->is_closed) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_send(channel, size, false);
        return false;
    }
    
    // Handle unbuffered channels
    if (channel->options & GOO_CHAN_UNBUFFERED) {
        // Check if there's a receiver waiting
        if (channel->count == 0) {
            // No receiver waiting, can't send
            pthread_mutex_unlock(&channel->mutex);
            channel_update_stats_send(channel, size, false);
            return false;
        }
        
        // Copy data directly to receiver
        memcpy(channel->buffer, data, size < channel->elem_size ? size : channel->elem_size);
        channel->count = 1;
        
        // Signal receiver
        pthread_cond_signal(&channel->send_cond);
        pthread_mutex_unlock(&channel->mutex);
        
        channel_update_stats_send(channel, size, true);
        return true;
    }
    
    // Handle buffered channels
    // Check if buffer is full
    if (channel->count >= channel->buffer_size) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_send(channel, size, false);
        return false;
    }
    
    // Calculate position in circular buffer
    size_t pos = (channel->tail + channel->count) % channel->buffer_size;
    
    // Copy data to buffer
    void* dest = (char*)channel->buffer + (pos * channel->elem_size);
    memcpy(dest, data, size < channel->elem_size ? size : channel->elem_size);
    
    // Update buffer state
    channel->count++;
    
    // Signal waiting receivers
    pthread_cond_signal(&channel->send_cond);
    
    pthread_mutex_unlock(&channel->mutex);
    
    channel_update_stats_send(channel, size, true);
    return true;
}

// Try receiving data from a channel (non-blocking)
bool goo_channel_try_receive(GooChannel* channel, void* data, size_t size, GooMessageFlags flags) {
    if (!channel || !data || size == 0) return false;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Handle unbuffered channels
    if (channel->options & GOO_CHAN_UNBUFFERED) {
        // Signal that we're ready to receive
        channel->count = 0;
        pthread_cond_signal(&channel->recv_cond);
        
        // Check if there's data available
        if (channel->count == 0) {
            pthread_mutex_unlock(&channel->mutex);
            channel_update_stats_receive(channel, size, false);
            return false;
        }
        
        // Copy data directly from sender
        memcpy(data, channel->buffer, size < channel->elem_size ? size : channel->elem_size);
        channel->count = 0;
        
        pthread_mutex_unlock(&channel->mutex);
        
        channel_update_stats_receive(channel, size, true);
        return true;
    }
    
    // Handle buffered channels
    // Check if buffer is empty
    if (channel->count == 0) {
        pthread_mutex_unlock(&channel->mutex);
        channel_update_stats_receive(channel, size, false);
        return false;
    }
    
    // Copy data from buffer
    void* src = (char*)channel->buffer + (channel->head * channel->elem_size);
    memcpy(data, src, size < channel->elem_size ? size : channel->elem_size);
    
    // Update buffer state
    channel->head = (channel->head + 1) % channel->buffer_size;
    channel->count--;
    
    // Signal waiting senders
    pthread_cond_signal(&channel->recv_cond);
    
    pthread_mutex_unlock(&channel->mutex);
    
    channel_update_stats_receive(channel, size, true);
    return true;
}

// Create a message object
GooMessage* goo_message_create(const void* data, size_t size, GooMessageFlags flags) {
    if (!data || size == 0) return NULL;
    
    GooMessage* msg = (GooMessage*)malloc(sizeof(GooMessage));
    if (!msg) return NULL;
    
    // Initialize message
    msg->data = malloc(size);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    memcpy(msg->data, data, size);
    msg->size = size;
    msg->flags = flags;
    msg->priority = 0;
    msg->context = NULL;
    msg->free_fn = free; // Default free function
    msg->next = NULL;
    
    return msg;
}

// Destroy a message and free resources
void goo_message_destroy(GooMessage* message) {
    if (!message) return;
    
    // Free the data with the appropriate function
    if (message->data && message->free_fn) {
        message->free_fn(message->data);
    }
    
    // Free any attached message parts
    if (message->next) {
        goo_message_destroy(message->next);
    }
    
    // Free the message structure
    free(message);
}

// Add a part to a multi-part message
bool goo_message_add_part(GooMessage* message, const void* data, size_t size, GooMessageFlags flags) {
    if (!message || !data || size == 0) return false;
    
    // Create a new message for the part
    GooMessage* part = goo_message_create(data, size, flags);
    if (!part) return false;
    
    // Find the last part in the chain
    GooMessage* last = message;
    while (last->next) {
        last = last->next;
    }
    
    // Attach the new part
    last->next = part;
    
    return true;
}

// Get the next part of a multi-part message
GooMessage* goo_message_next_part(GooMessage* message) {
    if (!message) return NULL;
    return message->next;
}

// Send a message object to a channel
bool goo_channel_send_message(GooChannel* channel, GooMessage* message) {
    if (!channel || !message) return false;
    
    // Special handling based on channel type
    switch (channel->type) {
        case GOO_CHANNEL_PUB_SUB: {
            // For pub/sub channels, distribute to all subscribers
            pthread_mutex_lock(&channel->mutex);
            
            if (channel->is_closed) {
                pthread_mutex_unlock(&channel->mutex);
                return false;
            }
            
            // Update channel statistics
            channel_update_stats_send(channel, message->size, true);
            
            // If this is a multi-part message, handle each part
            if (message->flags & GOO_MESSAGE_MULTIPART) {
                GooMessage* current = message;
                while (current) {
                    // Publish to each subscriber
                    GooChannelSubscriber* sub = (GooChannelSubscriber*)channel->subscribers;
                    while (sub) {
                        if (sub->channel) {
                            // Copy message to subscriber's buffer
                            goo_channel_send(sub->channel, current->data, current->size, current->flags);
                        }
                        sub = sub->next;
                    }
                    current = current->next;
                }
            } else {
                // Single-part message, publish to subscribers
                GooChannelSubscriber* sub = (GooChannelSubscriber*)channel->subscribers;
                while (sub) {
                    if (sub->channel) {
                        goo_channel_send(sub->channel, message->data, message->size, message->flags);
                    }
                    sub = sub->next;
                }
            }
            
            pthread_mutex_unlock(&channel->mutex);
            return true;
        }
        
        case GOO_CHANNEL_REQ_REP: {
            // For request/reply channels, store message ID for correlation
            pthread_mutex_lock(&channel->mutex);
            
            if (channel->is_closed) {
                pthread_mutex_unlock(&channel->mutex);
                return false;
            }
            
            // Standard send for the message
            bool result = goo_channel_send(channel, message->data, message->size, message->flags);
            
            pthread_mutex_unlock(&channel->mutex);
            return result;
        }
        
        default:
            // For normal channels, just send the data
            return goo_channel_send(channel, message->data, message->size, message->flags);
    }
}

// Receive a message object from a channel
GooMessage* goo_channel_receive_message(GooChannel* channel, GooMessageFlags flags) {
    if (!channel) return NULL;
    
    // Allocate a buffer for the message data (using channel element size)
    void* buffer = malloc(channel->elem_size);
    if (!buffer) return NULL;
    
    // Receive data into the buffer
    if (!goo_channel_receive(channel, buffer, channel->elem_size, flags)) {
        free(buffer);
        return NULL;
    }
    
    // Create a message from the received data
    GooMessage* message = goo_message_create(buffer, channel->elem_size, flags);
    
    // Free the temporary buffer
    free(buffer);
    
    return message;
}

// Get channel statistics
GooChannelStats goo_channel_stats(GooChannel* channel) {
    GooChannelStats stats = {0};
    if (!channel) return stats;
    
    pthread_mutex_lock(&channel->mutex);
    memcpy(&stats, &channel->stats, sizeof(GooChannelStats));
    stats.current_queue_size = channel->count;
    pthread_mutex_unlock(&channel->mutex);
    
    return stats;
}

// Reset channel statistics
void goo_channel_reset_stats(GooChannel* channel) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    memset(&channel->stats, 0, sizeof(GooChannelStats));
    pthread_mutex_unlock(&channel->mutex);
}

// Check if channel is empty
bool goo_channel_is_empty(GooChannel* channel) {
    if (!channel) return true;
    
    pthread_mutex_lock(&channel->mutex);
    bool empty = (channel->count == 0);
    pthread_mutex_unlock(&channel->mutex);
    
    return empty;
}

// Check if channel is full
bool goo_channel_is_full(GooChannel* channel) {
    if (!channel) return true;
    
    pthread_mutex_lock(&channel->mutex);
    bool full = (channel->count >= channel->buffer_size);
    pthread_mutex_unlock(&channel->mutex);
    
    return full;
}

// Get the number of items in the channel
size_t goo_channel_size(GooChannel* channel) {
    if (!channel) return 0;
    
    pthread_mutex_lock(&channel->mutex);
    size_t size = channel->count;
    pthread_mutex_unlock(&channel->mutex);
    
    return size;
}

// Set high water mark for flow control
void goo_channel_set_high_water_mark(GooChannel* channel, uint32_t hwm) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    channel->high_water_mark = hwm;
    channel->low_water_mark = hwm / 2;
    pthread_mutex_unlock(&channel->mutex);
}

// Set timeout for channel operations
void goo_channel_set_timeout(GooChannel* channel, uint32_t timeout_ms) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    channel->timeout_ms = timeout_ms;
    pthread_mutex_unlock(&channel->mutex);
}

// Helper function: Initialize channel buffer
static bool channel_init_buffer(GooChannel* channel) {
    if (!channel) return false;
    
    // Allocate buffer memory
    channel->buffer = malloc(channel->buffer_size * channel->elem_size);
    if (!channel->buffer) return false;
    
    // Initialize buffer state
    channel->head = 0;
    channel->tail = 0;
    channel->count = 0;
    
    return true;
}

// Helper function: Check if a channel type is distributed
static bool channel_is_distributed_type(GooChannelType type) {
    switch (type) {
        case GOO_CHAN_PUB:
        case GOO_CHAN_SUB:
        case GOO_CHAN_PUSH:
        case GOO_CHAN_PULL:
        case GOO_CHAN_REQ:
        case GOO_CHAN_REP:
        case GOO_CHAN_DEALER:
        case GOO_CHAN_ROUTER:
            return true;
        default:
            return false;
    }
}

// Helper function: Update channel statistics for send operations
static void channel_update_stats_send(GooChannel* channel, size_t size, bool success) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    
    if (success) {
        channel->stats.messages_sent++;
        channel->stats.bytes_sent += size;
        
        if (channel->count > channel->stats.max_queue_size) {
            channel->stats.max_queue_size = channel->count;
        }
    } else {
        channel->stats.send_errors++;
    }
    
    pthread_mutex_unlock(&channel->mutex);
}

// Helper function: Update channel statistics for receive operations
static void channel_update_stats_receive(GooChannel* channel, size_t size, bool success) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    
    if (success) {
        channel->stats.messages_received++;
        channel->stats.bytes_received += size;
    } else {
        channel->stats.receive_errors++;
    }
    
    pthread_mutex_unlock(&channel->mutex);
} 