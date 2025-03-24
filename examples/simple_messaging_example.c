/**
 * Simple Messaging Example for Goo
 * 
 * This example demonstrates the most basic usage of the Goo messaging API.
 * It creates a simple client-server communication within a single process.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define MOCK_IMPLEMENTATION 1

#ifndef MOCK_IMPLEMENTATION
#include "messaging/goo_channels_core.h"
#else
// Simple mock implementation for this example
typedef struct GooChannel GooChannel;

struct GooChannel {
    int id;
    bool is_closed;
};

GooChannel* goo_channel_create(int type, size_t elem_size, size_t buffer_size, int options) {
    printf("[MOCK] Creating channel\n");
    GooChannel* channel = (GooChannel*)malloc(sizeof(GooChannel));
    if (channel) {
        channel->id = rand() % 1000;
        channel->is_closed = false;
    }
    return channel;
}

void goo_channel_destroy(GooChannel* channel) {
    if (channel) {
        printf("[MOCK] Destroying channel\n");
        free(channel);
    }
}

int goo_channel_send(GooChannel* channel, const void* data, size_t size, int flags) {
    printf("[MOCK] Sending message: %s\n", (const char*)data);
    return (int)size;
}

int goo_channel_recv(GooChannel* channel, void* data, size_t size, int flags) {
    const char* msg = "Hello from the other side!";
    size_t msg_len = strlen(msg);
    size_t copy_len = (msg_len < size) ? msg_len : size - 1;
    
    memcpy(data, msg, copy_len);
    ((char*)data)[copy_len] = '\0';
    
    printf("[MOCK] Received message: %s\n", (char*)data);
    return (int)copy_len;
}
#endif

// Thread function for the server
void* server_thread(void* arg) {
    GooChannel* channel = (GooChannel*)arg;
    char buffer[256];
    
    printf("Server: Waiting for messages...\n");
    
    // Receive a message
    int received = goo_channel_recv(channel, buffer, sizeof(buffer), 0);
    if (received > 0) {
        printf("Server: Received: %s\n", buffer);
        
        // Send a response
        const char* response = "Message received, thank you!";
        int sent = goo_channel_send(channel, response, strlen(response), 0);
        if (sent > 0) {
            printf("Server: Response sent.\n");
        } else {
            printf("Server: Failed to send response.\n");
        }
    } else {
        printf("Server: Failed to receive message.\n");
    }
    
    return NULL;
}

int main() {
    printf("Starting simple messaging example...\n");
    
    // Create a channel
    GooChannel* channel = goo_channel_create(0, 256, 10, 0);
    if (!channel) {
        printf("Failed to create channel!\n");
        return 1;
    }
    
    // Start server thread
    pthread_t server_tid;
    pthread_create(&server_tid, NULL, server_thread, channel);
    
    // Give the server a moment to start
    usleep(100000);  // 100ms
    
    // Client side: send a message
    const char* message = "Hello, server!";
    printf("Client: Sending: %s\n", message);
    int sent = goo_channel_send(channel, message, strlen(message), 0);
    
    if (sent > 0) {
        printf("Client: Message sent successfully.\n");
        
        // Receive response
        char response[256];
        int received = goo_channel_recv(channel, response, sizeof(response), 0);
        
        if (received > 0) {
            printf("Client: Received response: %s\n", response);
        } else {
            printf("Client: Failed to receive response.\n");
        }
    } else {
        printf("Client: Failed to send message.\n");
    }
    
    // Wait for server thread to finish
    pthread_join(server_tid, NULL);
    
    // Clean up
    goo_channel_destroy(channel);
    
    printf("Simple messaging example completed.\n");
    return 0;
} 