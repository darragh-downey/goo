/**
 * simple_messaging_demo.c
 * 
 * A simple demonstration of Goo's messaging system concepts.
 * This is a minimal implementation that doesn't rely on the full Zig library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Simple opaque types for the messaging system
typedef struct Message Message;
typedef struct Channel Channel;

// Message types
typedef enum {
    MESSAGE_STRING,
    MESSAGE_INT,
    MESSAGE_BINARY
} MessageType;

// Basic message implementation
struct Message {
    MessageType type;
    void* data;
    size_t size;
    char* topic;
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
    char* name;
};

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
    msg->topic = NULL;
    
    return msg;
}

// Create a message with integer data
Message* message_create_int(int value) {
    Message* msg = (Message*)malloc(sizeof(Message));
    if (!msg) return NULL;
    
    msg->type = MESSAGE_INT;
    msg->size = sizeof(int);
    msg->data = malloc(msg->size);
    if (!msg->data) {
        free(msg);
        return NULL;
    }
    
    *(int*)msg->data = value;
    msg->topic = NULL;
    
    return msg;
}

// Destroy a message
void message_destroy(Message* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
    }
    
    if (msg->topic) {
        free(msg->topic);
    }
    
    free(msg);
}

// Get string data from a message
const char* message_get_string(const Message* msg) {
    if (!msg || msg->type != MESSAGE_STRING) return NULL;
    return (const char*)msg->data;
}

// Get integer data from a message
int message_get_int(const Message* msg, int* value) {
    if (!msg || msg->type != MESSAGE_INT || !value) return 0;
    *value = *(int*)msg->data;
    return 1;
}

// Set topic for a message
int message_set_topic(Message* msg, const char* topic) {
    if (!msg || !topic) return 0;
    
    if (msg->topic) {
        free(msg->topic);
    }
    
    msg->topic = strdup(topic);
    return msg->topic != NULL;
}

// Get topic from a message
const char* message_get_topic(const Message* msg) {
    if (!msg) return NULL;
    return msg->topic;
}

// Create a channel
Channel* channel_create(size_t capacity, const char* name) {
    Channel* channel = (Channel*)malloc(sizeof(Channel));
    if (!channel) return NULL;
    
    channel->buffer = (Message**)malloc(capacity * sizeof(Message*));
    if (!channel->buffer) {
        free(channel);
        return NULL;
    }
    
    pthread_mutex_init(&channel->mutex, NULL);
    pthread_cond_init(&channel->cond, NULL);
    
    channel->capacity = capacity;
    channel->count = 0;
    channel->head = 0;
    channel->tail = 0;
    
    channel->name = name ? strdup(name) : NULL;
    
    return channel;
}

// Destroy a channel
void channel_destroy(Channel* channel) {
    if (!channel) return;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Free any messages still in the channel
    for (size_t i = 0; i < channel->count; i++) {
        size_t idx = (channel->head + i) % channel->capacity;
        message_destroy(channel->buffer[idx]);
    }
    
    free(channel->buffer);
    
    if (channel->name) {
        free(channel->name);
    }
    
    pthread_mutex_unlock(&channel->mutex);
    
    pthread_mutex_destroy(&channel->mutex);
    pthread_cond_destroy(&channel->cond);
    
    free(channel);
}

// Send a message to a channel
int channel_send(Channel* channel, Message* msg) {
    if (!channel || !msg) return 0;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if the channel is full
    if (channel->count == channel->capacity) {
        pthread_mutex_unlock(&channel->mutex);
        return 0;
    }
    
    // Add the message to the buffer
    channel->buffer[channel->tail] = msg;
    channel->tail = (channel->tail + 1) % channel->capacity;
    channel->count++;
    
    // Signal waiting receivers
    pthread_cond_signal(&channel->cond);
    
    pthread_mutex_unlock(&channel->mutex);
    
    return 1;
}

// Receive a message from a channel
Message* channel_receive(Channel* channel) {
    if (!channel) return NULL;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Check if the channel is empty
    if (channel->count == 0) {
        pthread_mutex_unlock(&channel->mutex);
        return NULL;
    }
    
    // Get the message from the buffer
    Message* msg = channel->buffer[channel->head];
    channel->head = (channel->head + 1) % channel->capacity;
    channel->count--;
    
    pthread_mutex_unlock(&channel->mutex);
    
    return msg;
}

// Receive a message from a channel, waiting if necessary
Message* channel_receive_wait(Channel* channel) {
    if (!channel) return NULL;
    
    pthread_mutex_lock(&channel->mutex);
    
    // Wait until a message is available
    while (channel->count == 0) {
        pthread_cond_wait(&channel->cond, &channel->mutex);
    }
    
    // Get the message from the buffer
    Message* msg = channel->buffer[channel->head];
    channel->head = (channel->head + 1) % channel->capacity;
    channel->count--;
    
    pthread_mutex_unlock(&channel->mutex);
    
    return msg;
}

// Thread function for sender
void* sender_thread(void* arg) {
    Channel* channel = (Channel*)arg;
    
    for (int i = 0; i < 10; i++) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "Message %d", i);
        
        Message* msg = message_create_string(buffer);
        if (!msg) {
            printf("Failed to create message\n");
            continue;
        }
        
        printf("Sending: %s\n", buffer);
        
        if (!channel_send(channel, msg)) {
            printf("Failed to send message\n");
            message_destroy(msg);
        }
        
        usleep(500000); // Sleep for 0.5 seconds
    }
    
    printf("Sender done\n");
    return NULL;
}

// Thread function for receiver
void* receiver_thread(void* arg) {
    Channel* channel = (Channel*)arg;
    
    for (int i = 0; i < 10; i++) {
        Message* msg = channel_receive_wait(channel);
        if (!msg) {
            printf("Failed to receive message\n");
            continue;
        }
        
        const char* data = message_get_string(msg);
        printf("Received: %s\n", data);
        
        message_destroy(msg);
        
        usleep(200000); // Sleep for 0.2 seconds
    }
    
    printf("Receiver done\n");
    return NULL;
}

// Main function
int main() {
    printf("Simple Messaging Demo\n");
    
    // Create a channel
    Channel* channel = channel_create(20, "main-channel");
    if (!channel) {
        printf("Failed to create channel\n");
        return 1;
    }
    
    // Create threads
    pthread_t sender, receiver;
    
    if (pthread_create(&sender, NULL, sender_thread, channel) != 0) {
        printf("Failed to create sender thread\n");
        channel_destroy(channel);
        return 1;
    }
    
    if (pthread_create(&receiver, NULL, receiver_thread, channel) != 0) {
        printf("Failed to create receiver thread\n");
        // Wait for sender to finish
        pthread_join(sender, NULL);
        channel_destroy(channel);
        return 1;
    }
    
    // Wait for threads to finish
    pthread_join(sender, NULL);
    pthread_join(receiver, NULL);
    
    // Clean up
    channel_destroy(channel);
    
    printf("Demo completed successfully\n");
    return 0;
} 