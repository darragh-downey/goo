/**
 * PGM Example - Reliable Multicast Messaging in Goo
 * 
 * This example demonstrates how to use PGM (Pragmatic General Multicast)
 * for reliable multicast communication in the Goo messaging system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include "../include/goo_runtime.h"
#include "../include/goo_distributed.h"
#include "../src/messaging/goo_channels.h"
#include "../src/messaging/goo_pgm.h"

#define NUM_SUBSCRIBERS 3
#define NUM_MESSAGES 10

// Message structure
typedef struct {
    int id;
    char content[128];
} Message;

// Subscriber thread function
void* subscriber_thread(void* arg) {
    int id = *((int*)arg);
    printf("Subscriber %d starting...\n", id);
    
    // Initialize PGM
    if (!goo_pgm_init()) {
        fprintf(stderr, "Failed to initialize PGM\n");
        return NULL;
    }
    
    // Create a receiver socket with default options
    int socket_fd = goo_pgm_create_receiver("239.255.1.1", 7500, NULL);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to create PGM receiver socket\n");
        return NULL;
    }
    
    printf("Subscriber %d waiting for messages...\n", id);
    
    // Receive messages
    int received_count = 0;
    Message msg;
    
    while (received_count < NUM_MESSAGES) {
        ssize_t result = goo_pgm_receive(socket_fd, &msg, sizeof(Message), 1000);
        
        if (result < 0) {
            perror("Error receiving message");
            break;
        }
        
        if (result == 0) {
            // Timeout, continue polling
            continue;
        }
        
        printf("Subscriber %d received message %d: %s\n", id, msg.id, msg.content);
        received_count++;
    }
    
    // Clean up
    goo_pgm_close(socket_fd);
    goo_pgm_cleanup();
    
    printf("Subscriber %d finished, received %d messages\n", id, received_count);
    return NULL;
}

// Publisher function
void run_publisher() {
    printf("Publisher starting...\n");
    
    // Initialize PGM
    if (!goo_pgm_init()) {
        fprintf(stderr, "Failed to initialize PGM\n");
        return;
    }
    
    // Create a sender socket with default options
    int socket_fd = goo_pgm_create_sender("239.255.1.1", 7500, NULL);
    if (socket_fd < 0) {
        fprintf(stderr, "Failed to create PGM sender socket\n");
        return;
    }
    
    // Give subscribers time to start
    sleep(1);
    
    // Send messages
    Message msg;
    for (int i = 0; i < NUM_MESSAGES; i++) {
        msg.id = i;
        snprintf(msg.content, sizeof(msg.content), "Multicast message %d", i);
        
        printf("Publishing message %d...\n", i);
        if (!goo_pgm_send(socket_fd, &msg, sizeof(Message))) {
            fprintf(stderr, "Failed to send message %d\n", i);
        }
        
        // Small delay between messages
        usleep(200000); // 200ms
    }
    
    // Allow time for delivery
    sleep(1);
    
    // Get statistics
    GooPGMStats stats;
    if (goo_pgm_get_stats(socket_fd, &stats)) {
        printf("\nPublisher statistics:\n");
        printf("  Data bytes sent: %lu\n", stats.data_bytes_sent);
        printf("  Packets retransmitted: %lu\n", stats.packets_retransmitted);
    }
    
    // Clean up
    goo_pgm_close(socket_fd);
    goo_pgm_cleanup();
    
    printf("Publisher finished\n");
}

// Using channels with PGM transport
void demonstrate_channel_pgm() {
    printf("\nDemonstrating PGM with Goo channels...\n");
    
    // Create a publisher channel
    GooChannel* pub = goo_channel_create(GOO_CHAN_PUB, sizeof(Message), 10, GOO_CHAN_DISTRIBUTED);
    if (!pub) {
        fprintf(stderr, "Failed to create publisher channel\n");
        return;
    }
    
    // Bind to PGM endpoint
    if (!goo_channel_set_endpoint(pub, "pgm://239.255.1.1:7600")) {
        fprintf(stderr, "Failed to bind publisher to PGM endpoint\n");
        goo_channel_destroy(pub);
        return;
    }
    
    // Create a subscriber channel
    GooChannel* sub = goo_channel_create(GOO_CHAN_SUB, sizeof(Message), 10, GOO_CHAN_DISTRIBUTED);
    if (!sub) {
        fprintf(stderr, "Failed to create subscriber channel\n");
        goo_channel_destroy(pub);
        return;
    }
    
    // Connect to PGM endpoint
    if (!goo_channel_set_endpoint(sub, "pgm://239.255.1.1:7600")) {
        fprintf(stderr, "Failed to connect subscriber to PGM endpoint\n");
        goo_channel_destroy(pub);
        goo_channel_destroy(sub);
        return;
    }
    
    // Send a message
    Message msg = {.id = 100, .content = "Channel PGM test message"};
    if (!goo_channel_send(pub, &msg, sizeof(Message), GOO_MSG_NONE)) {
        fprintf(stderr, "Failed to send message via channel\n");
    }
    
    // Allow time for delivery
    sleep(1);
    
    // Try to receive the message
    Message received;
    if (goo_channel_try_receive(sub, &received, sizeof(Message), GOO_MSG_NONE)) {
        printf("Received via channel: id=%d, content=%s\n", received.id, received.content);
    } else {
        printf("No message received via channel\n");
    }
    
    // Clean up
    goo_channel_destroy(pub);
    goo_channel_destroy(sub);
}

int main() {
    // Initialize the Goo runtime
    goo_runtime_init();
    
    printf("=== Goo PGM Example - Reliable Multicast Messaging ===\n\n");
    
    // Create subscriber threads
    pthread_t subscribers[NUM_SUBSCRIBERS];
    int ids[NUM_SUBSCRIBERS];
    
    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        ids[i] = i + 1;
        if (pthread_create(&subscribers[i], NULL, subscriber_thread, &ids[i]) != 0) {
            perror("Failed to create subscriber thread");
            return 1;
        }
    }
    
    // Run the publisher
    run_publisher();
    
    // Demonstrate using channels with PGM
    demonstrate_channel_pgm();
    
    // Wait for all subscribers to finish
    for (int i = 0; i < NUM_SUBSCRIBERS; i++) {
        pthread_join(subscribers[i], NULL);
    }
    
    printf("\n=== PGM Example Complete ===\n");
    
    // Clean up the Goo runtime
    goo_runtime_cleanup();
    
    return 0;
} 