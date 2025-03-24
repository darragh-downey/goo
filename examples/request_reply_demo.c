/**
 * request_reply_demo.c
 * 
 * Demonstration of the request-reply messaging pattern in Goo.
 * This uses a simple implementation that doesn't rely on the full Zig library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

// Simple opaque types
typedef struct Message Message;
typedef struct Channel Channel;

// Message types
typedef enum {
    MESSAGE_STRING,
    MESSAGE_INT,
    MESSAGE_BINARY,
    MESSAGE_REQUEST,
    MESSAGE_REPLY
} MessageType;

// Basic message implementation
struct Message {
    MessageType type;
    void* data;
    size_t size;
    char* request_id;  // For request-reply pattern
};

// Channel implementation
struct Channel {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Message** buffer;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    bool closed;
};

// Forward declarations
void message_destroy(Message* msg);

// Create a message with string data
Message* message_create_string(const char* str) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (!msg) return NULL;
    
    msg->type = MESSAGE_STRING;
    msg->size = strlen(str) + 1;
    msg->data = malloc(msg->size);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    memcpy(msg->data, str, msg->size);
    msg->request_id = NULL;
    
    return msg;
}

// Destroy a message
void message_destroy(Message* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
    }
    
    if (msg->request_id) {
        free(msg->request_id);
    }
    
    free(msg);
}

// Create a request message
Message* message_create_request(const char* str, const char* request_id) {
    Message* msg = message_create_string(str);
    if (!msg) return NULL;
    
    msg->type = MESSAGE_REQUEST;
    
    if (request_id) {
        msg->request_id = strdup(request_id);
        if (!msg->request_id) {
            message_destroy(msg);
            return NULL;
        }
    }
    
    return msg;
}

// Create a reply message
Message* message_create_reply(const char* str, const char* request_id) {
    Message* msg = message_create_string(str);
    if (!msg) return NULL;
    
    msg->type = MESSAGE_REPLY;
    
    if (request_id) {
        msg->request_id = strdup(request_id);
        if (!msg->request_id) {
            message_destroy(msg);
            return NULL;
        }
    }
    
    return msg;
}

// Get the request ID from a message
const char* message_get_request_id(const Message* msg) {
    if (!msg) return NULL;
    return msg->request_id;
}

// Get string data from a message
const char* message_get_string(const Message* msg) {
    if (!msg || (msg->type != MESSAGE_STRING && 
                msg->type != MESSAGE_REQUEST && 
                msg->type != MESSAGE_REPLY)) 
        return NULL;
    return (const char*)msg->data;
}

// Create a channel
Channel* channel_create(size_t capacity) {
    Channel* chan = (Channel*)malloc(sizeof(Channel));
    if (!chan) return NULL;
    
    chan->buffer = (Message**)malloc(capacity * sizeof(Message*));
    if (!chan->buffer) {
        free(chan);
        return NULL;
    }
    
    pthread_mutex_init(&chan->mutex, NULL);
    pthread_cond_init(&chan->cond, NULL);
    
    chan->capacity = capacity;
    chan->count = 0;
    chan->head = 0;
    chan->tail = 0;
    chan->closed = false;
    
    return chan;
}

// Destroy a channel
void channel_destroy(Channel* chan) {
    if (!chan) return;
    
    pthread_mutex_lock(&chan->mutex);
    
    // Free any messages still in the buffer
    for (size_t i = 0; i < chan->count; i++) {
        size_t idx = (chan->head + i) % chan->capacity;
        message_destroy(chan->buffer[idx]);
    }
    
    free(chan->buffer);
    
    pthread_mutex_unlock(&chan->mutex);
    
    pthread_mutex_destroy(&chan->mutex);
    pthread_cond_destroy(&chan->cond);
    
    free(chan);
}

// Send a message to a channel
int channel_send(Channel* chan, Message* msg) {
    if (!chan || !msg) return 0;
    
    pthread_mutex_lock(&chan->mutex);
    
    // Check if the channel is closed
    if (chan->closed) {
        pthread_mutex_unlock(&chan->mutex);
        return 0;
    }
    
    // Check if the buffer is full
    if (chan->count == chan->capacity) {
        pthread_mutex_unlock(&chan->mutex);
        return 0;
    }
    
    // Add the message to the buffer
    chan->buffer[chan->tail] = msg;
    chan->tail = (chan->tail + 1) % chan->capacity;
    chan->count++;
    
    // Signal waiting receivers
    pthread_cond_signal(&chan->cond);
    
    pthread_mutex_unlock(&chan->mutex);
    
    return 1;
}

// Receive a message from a channel
Message* channel_receive(Channel* chan) {
    if (!chan) return NULL;
    
    pthread_mutex_lock(&chan->mutex);
    
    // Check if the buffer is empty
    if (chan->count == 0) {
        pthread_mutex_unlock(&chan->mutex);
        return NULL;
    }
    
    // Get the message from the buffer
    Message* msg = chan->buffer[chan->head];
    chan->head = (chan->head + 1) % chan->capacity;
    chan->count--;
    
    pthread_mutex_unlock(&chan->mutex);
    
    return msg;
}

// Receive a message from a channel, waiting if necessary
Message* channel_receive_wait(Channel* chan) {
    if (!chan) return NULL;
    
    pthread_mutex_lock(&chan->mutex);
    
    // Wait until a message is available or the channel is closed
    while (chan->count == 0 && !chan->closed) {
        pthread_cond_wait(&chan->cond, &chan->mutex);
    }
    
    // Check if the channel is closed and empty
    if (chan->count == 0) {
        pthread_mutex_unlock(&chan->mutex);
        return NULL;
    }
    
    // Get the message from the buffer
    Message* msg = chan->buffer[chan->head];
    chan->head = (chan->head + 1) % chan->capacity;
    chan->count--;
    
    pthread_mutex_unlock(&chan->mutex);
    
    return msg;
}

// Close a channel
void channel_close(Channel* chan) {
    if (!chan) return;
    
    pthread_mutex_lock(&chan->mutex);
    
    chan->closed = true;
    
    // Signal all waiting receivers
    pthread_cond_broadcast(&chan->cond);
    
    pthread_mutex_unlock(&chan->mutex);
}

// Request-response pattern
Message* channel_request(Channel* request_chan, Channel* reply_chan, const char* request_str) {
    // Create a unique request ID
    char request_id[64];
    snprintf(request_id, sizeof(request_id), "req-%ld-%d", (long)time(NULL), rand() % 10000);
    
    // Create request message
    Message* request = message_create_request(request_str, request_id);
    if (!request) return NULL;
    
    // Send the request
    if (!channel_send(request_chan, request)) {
        message_destroy(request);
        return NULL;
    }
    
    // Wait for a matching reply
    while (true) {
        Message* reply = channel_receive_wait(reply_chan);
        if (!reply) return NULL;  // Channel was closed
        
        if (reply->type == MESSAGE_REPLY && 
            reply->request_id && 
            strcmp(reply->request_id, request_id) == 0) {
            return reply;  // Found matching reply
        }
        
        // Not our reply, put it back
        channel_send(reply_chan, reply);
        usleep(10000);  // Small delay to avoid busy-waiting
    }
}

// Thread function for the server
void* server_thread(void* arg) {
    struct {
        Channel* request_chan;
        Channel* reply_chan;
    } *channels = arg;
    
    Channel* request_chan = channels->request_chan;
    Channel* reply_chan = channels->reply_chan;
    
    printf("Server: Starting\n");
    
    // Process 5 requests
    for (int i = 0; i < 5; i++) {
        // Wait for a request
        Message* request = channel_receive_wait(request_chan);
        if (!request) {
            printf("Server: Channel closed\n");
            break;
        }
        
        const char* request_str = message_get_string(request);
        const char* request_id = message_get_request_id(request);
        
        printf("Server: Received request '%s' with ID '%s'\n", 
               request_str ? request_str : "empty", 
               request_id ? request_id : "none");
        
        // Process the request
        char response[256];
        snprintf(response, sizeof(response), "Response to '%s'", request_str ? request_str : "empty");
        
        // Create and send the reply
        Message* reply = message_create_reply(response, request_id);
        if (reply) {
            channel_send(reply_chan, reply);
            printf("Server: Sent response '%s'\n", response);
        } else {
            printf("Server: Failed to create reply\n");
        }
        
        message_destroy(request);
        
        // Add some processing delay
        usleep(500000);  // Sleep for 0.5 seconds
    }
    
    printf("Server: Done\n");
    return NULL;
}

// Thread function for the client
void* client_thread(void* arg) {
    struct {
        Channel* request_chan;
        Channel* reply_chan;
    } *channels = arg;
    
    Channel* request_chan = channels->request_chan;
    Channel* reply_chan = channels->reply_chan;
    
    printf("Client: Starting\n");
    
    // Send 5 requests
    for (int i = 0; i < 5; i++) {
        char request_str[128];
        snprintf(request_str, sizeof(request_str), "Request %d", i);
        
        printf("Client: Sending request '%s'\n", request_str);
        
        // Send request and wait for reply
        Message* reply = channel_request(request_chan, reply_chan, request_str);
        
        if (reply) {
            const char* reply_str = message_get_string(reply);
            const char* request_id = message_get_request_id(reply);
            
            printf("Client: Received reply '%s' for request ID '%s'\n", 
                   reply_str ? reply_str : "empty", 
                   request_id ? request_id : "none");
            
            message_destroy(reply);
        } else {
            printf("Client: Failed to get reply\n");
        }
        
        // Add some delay between requests
        usleep(200000);  // Sleep for 0.2 seconds
    }
    
    printf("Client: Done\n");
    return NULL;
}

int main() {
    printf("Request-Reply Demo\n");
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Create channels
    Channel* request_chan = channel_create(10);
    Channel* reply_chan = channel_create(10);
    
    if (!request_chan || !reply_chan) {
        printf("Failed to create channels\n");
        if (request_chan) channel_destroy(request_chan);
        if (reply_chan) channel_destroy(reply_chan);
        return 1;
    }
    
    // Prepare channel structure for threads
    struct {
        Channel* request_chan;
        Channel* reply_chan;
    } channels = {
        .request_chan = request_chan,
        .reply_chan = reply_chan
    };
    
    // Create threads
    pthread_t server, client;
    
    if (pthread_create(&server, NULL, server_thread, &channels) != 0) {
        printf("Failed to create server thread\n");
        goto cleanup;
    }
    
    if (pthread_create(&client, NULL, client_thread, &channels) != 0) {
        printf("Failed to create client thread\n");
        goto cleanup;
    }
    
    // Wait for threads to finish
    pthread_join(client, NULL);
    pthread_join(server, NULL);
    
cleanup:
    // Clean up
    channel_destroy(request_chan);
    channel_destroy(reply_chan);
    
    printf("Demo completed successfully\n");
    return 0;
} 