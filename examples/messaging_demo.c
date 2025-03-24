/**
 * messaging_demo.c
 * 
 * A demonstration of Goo's first-class messaging system with different patterns.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "../src/include/goo_runtime.h"
#include "../src/include/messaging/messaging.h"

// Thread function for publisher
void* publisher_thread(void* arg) {
    GooChannel* publisher = (GooChannel*)arg;
    
    for (int i = 0; i < 10; i++) {
        char buffer[64];
        sprintf(buffer, "Message %d", i);
        
        GooMessage* msg = goo_message_create_string(buffer);
        if (!msg) {
            printf("Failed to create message\n");
            continue;
        }
        
        // Publish to different topics
        const char* topic = (i % 2 == 0) ? "even" : "odd";
        printf("Publishing to topic '%s': %s\n", topic, buffer);
        
        if (!goo_channel_publish(publisher, topic, msg)) {
            printf("Failed to publish message\n");
            goo_message_destroy(msg);
        }
        
        sleep(1); // Simulate some work
    }
    
    printf("Publisher done\n");
    return NULL;
}

// Thread function for subscriber
void* subscriber_thread(void* arg) {
    GooChannel* subscriber = (GooChannel*)arg;
    
    for (int i = 0; i < 10; i++) {
        char* topic = NULL;
        GooMessage* msg = goo_channel_receive_with_topic(subscriber, &topic);
        if (!msg) {
            printf("Failed to receive message\n");
            continue;
        }
        
        const char* data = goo_message_get_string(msg);
        printf("Subscriber received on topic '%s': %s\n", topic ? topic : "none", data);
        
        goo_message_destroy(msg);
    }
    
    printf("Subscriber done\n");
    return NULL;
}

// Thread function for push worker
void* push_thread(void* arg) {
    GooChannel* pusher = (GooChannel*)arg;
    
    for (int i = 0; i < 5; i++) {
        char buffer[64];
        sprintf(buffer, "Work item %d", i);
        
        GooMessage* msg = goo_message_create_string(buffer);
        if (!msg) {
            printf("Failed to create work item\n");
            continue;
        }
        
        printf("Pushing work: %s\n", buffer);
        
        if (!goo_channel_push(pusher, msg)) {
            printf("Failed to push work item\n");
            goo_message_destroy(msg);
        }
        
        sleep(1); // Simulate some work
    }
    
    printf("Pusher done\n");
    return NULL;
}

// Thread function for pull worker
void* pull_thread(void* arg) {
    GooChannel* puller = (GooChannel*)arg;
    
    for (int i = 0; i < 5; i++) {
        GooMessage* msg = goo_channel_pull(puller);
        if (!msg) {
            printf("Failed to pull work item\n");
            continue;
        }
        
        const char* data = goo_message_get_string(msg);
        printf("Worker received: %s\n", data);
        
        goo_message_destroy(msg);
        sleep(2); // Simulate processing time
    }
    
    printf("Worker done\n");
    return NULL;
}

// Thread function for request client
void* request_thread(void* arg) {
    GooChannel* requester = (GooChannel*)arg;
    
    for (int i = 0; i < 3; i++) {
        char buffer[64];
        sprintf(buffer, "Request %d", i);
        
        GooMessage* request = goo_message_create_string(buffer);
        if (!request) {
            printf("Failed to create request\n");
            continue;
        }
        
        printf("Sending request: %s\n", buffer);
        
        GooMessage* reply = goo_channel_request(requester, request);
        if (!reply) {
            printf("Failed to receive reply\n");
            continue;
        }
        
        const char* reply_data = goo_message_get_string(reply);
        printf("Received reply: %s\n", reply_data);
        
        goo_message_destroy(reply);
        sleep(1);
    }
    
    printf("Requester done\n");
    return NULL;
}

// Thread function for reply server
void* reply_thread(void* arg) {
    GooChannel* replier = (GooChannel*)arg;
    
    for (int i = 0; i < 3; i++) {
        GooMessage* request = goo_channel_receive(replier);
        if (!request) {
            printf("Failed to receive request\n");
            continue;
        }
        
        const char* request_data = goo_message_get_string(request);
        printf("Server received request: %s\n", request_data);
        
        // Create reply
        char buffer[64];
        sprintf(buffer, "Reply to '%s'", request_data);
        GooMessage* reply = goo_message_create_string(buffer);
        
        if (!reply) {
            printf("Failed to create reply\n");
            goo_message_destroy(request);
            continue;
        }
        
        // Send reply
        if (!goo_channel_reply(replier, request, reply)) {
            printf("Failed to send reply\n");
            goo_message_destroy(reply);
        }
        
        goo_message_destroy(request);
    }
    
    printf("Replier done\n");
    return NULL;
}

// Demonstrate the Publish-Subscribe pattern
void demonstrate_pubsub(void) {
    printf("\n=== Demonstrating Publish-Subscribe Pattern ===\n");
    
    // Create a publisher channel
    GooChannel* publisher = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    if (!publisher) {
        printf("Failed to create publisher channel\n");
        return;
    }
    
    // Create subscriber channels
    GooChannel* even_subscriber = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    GooChannel* odd_subscriber = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    GooChannel* all_subscriber = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    
    if (!even_subscriber || !odd_subscriber || !all_subscriber) {
        printf("Failed to create subscriber channels\n");
        if (publisher) goo_channel_destroy(publisher);
        if (even_subscriber) goo_channel_destroy(even_subscriber);
        if (odd_subscriber) goo_channel_destroy(odd_subscriber);
        if (all_subscriber) goo_channel_destroy(all_subscriber);
        return;
    }
    
    // Subscribe to topics
    if (!goo_channel_subscribe(even_subscriber, "even")) {
        printf("Failed to subscribe to 'even' topic\n");
    }
    
    if (!goo_channel_subscribe(odd_subscriber, "odd")) {
        printf("Failed to subscribe to 'odd' topic\n");
    }
    
    if (!goo_channel_subscribe(all_subscriber, "even") || 
        !goo_channel_subscribe(all_subscriber, "odd")) {
        printf("Failed to subscribe to all topics\n");
    }
    
    // Add subscribers to publisher
    if (!goo_channel_add_subscriber(publisher, even_subscriber) ||
        !goo_channel_add_subscriber(publisher, odd_subscriber) ||
        !goo_channel_add_subscriber(publisher, all_subscriber)) {
        printf("Failed to add subscribers\n");
    }
    
    // Create threads
    pthread_t pub_thread, even_sub_thread, odd_sub_thread, all_sub_thread;
    
    if (pthread_create(&pub_thread, NULL, publisher_thread, publisher) != 0) {
        printf("Failed to create publisher thread\n");
    }
    
    if (pthread_create(&even_sub_thread, NULL, subscriber_thread, even_subscriber) != 0) {
        printf("Failed to create even subscriber thread\n");
    }
    
    if (pthread_create(&odd_sub_thread, NULL, subscriber_thread, odd_subscriber) != 0) {
        printf("Failed to create odd subscriber thread\n");
    }
    
    if (pthread_create(&all_sub_thread, NULL, subscriber_thread, all_subscriber) != 0) {
        printf("Failed to create all subscriber thread\n");
    }
    
    // Wait for threads to finish
    pthread_join(pub_thread, NULL);
    pthread_join(even_sub_thread, NULL);
    pthread_join(odd_sub_thread, NULL);
    pthread_join(all_sub_thread, NULL);
    
    // Clean up
    goo_channel_destroy(publisher);
    goo_channel_destroy(even_subscriber);
    goo_channel_destroy(odd_subscriber);
    goo_channel_destroy(all_subscriber);
}

// Demonstrate the Push-Pull pattern
void demonstrate_pushpull(void) {
    printf("\n=== Demonstrating Push-Pull Pattern ===\n");
    
    // Create channels
    GooChannel* pusher = goo_channel_create(GOO_CHANNEL_TYPE_PUSHPULL);
    GooChannel* puller1 = goo_channel_create(GOO_CHANNEL_TYPE_PUSHPULL);
    GooChannel* puller2 = goo_channel_create(GOO_CHANNEL_TYPE_PUSHPULL);
    
    if (!pusher || !puller1 || !puller2) {
        printf("Failed to create push-pull channels\n");
        if (pusher) goo_channel_destroy(pusher);
        if (puller1) goo_channel_destroy(puller1);
        if (puller2) goo_channel_destroy(puller2);
        return;
    }
    
    // Connect pullers to pusher
    if (!goo_channel_add_subscriber(pusher, puller1) ||
        !goo_channel_add_subscriber(pusher, puller2)) {
        printf("Failed to connect push-pull channels\n");
    }
    
    // Create threads
    pthread_t push_thread_id, pull_thread1_id, pull_thread2_id;
    
    if (pthread_create(&push_thread_id, NULL, push_thread, pusher) != 0) {
        printf("Failed to create push thread\n");
    }
    
    if (pthread_create(&pull_thread1_id, NULL, pull_thread, puller1) != 0) {
        printf("Failed to create pull thread 1\n");
    }
    
    if (pthread_create(&pull_thread2_id, NULL, pull_thread, puller2) != 0) {
        printf("Failed to create pull thread 2\n");
    }
    
    // Wait for threads to finish
    pthread_join(push_thread_id, NULL);
    pthread_join(pull_thread1_id, NULL);
    pthread_join(pull_thread2_id, NULL);
    
    // Clean up
    goo_channel_destroy(pusher);
    goo_channel_destroy(puller1);
    goo_channel_destroy(puller2);
}

// Demonstrate the Request-Reply pattern
void demonstrate_reqrep(void) {
    printf("\n=== Demonstrating Request-Reply Pattern ===\n");
    
    // Create channels
    GooChannel* requester = goo_channel_create(GOO_CHANNEL_TYPE_REQREP);
    GooChannel* replier = goo_channel_create(GOO_CHANNEL_TYPE_REQREP);
    
    if (!requester || !replier) {
        printf("Failed to create request-reply channels\n");
        if (requester) goo_channel_destroy(requester);
        if (replier) goo_channel_destroy(replier);
        return;
    }
    
    // Connect channels
    if (!goo_channel_add_subscriber(requester, replier)) {
        printf("Failed to connect request-reply channels\n");
    }
    
    // Create threads
    pthread_t req_thread, rep_thread;
    
    if (pthread_create(&req_thread, NULL, request_thread, requester) != 0) {
        printf("Failed to create request thread\n");
    }
    
    if (pthread_create(&rep_thread, NULL, reply_thread, replier) != 0) {
        printf("Failed to create reply thread\n");
    }
    
    // Wait for threads to finish
    pthread_join(req_thread, NULL);
    pthread_join(rep_thread, NULL);
    
    // Clean up
    goo_channel_destroy(requester);
    goo_channel_destroy(replier);
}

// Demonstrate basic communication between two channels
void demonstrate_basic_channels(void) {
    printf("\n=== Demonstrating Basic Channel Communication ===\n");
    
    // Create a channel
    GooChannel* channel = goo_channel_create(GOO_CHANNEL_TYPE_NORMAL);
    if (!channel) {
        printf("Failed to create channel\n");
        return;
    }
    
    // Create and send messages
    for (int i = 0; i < 5; i++) {
        char buffer[64];
        sprintf(buffer, "Basic message %d", i);
        
        GooMessage* msg = goo_message_create_string(buffer);
        if (!msg) {
            printf("Failed to create message\n");
            continue;
        }
        
        printf("Sending: %s\n", buffer);
        
        if (!goo_channel_send(channel, msg)) {
            printf("Failed to send message\n");
            goo_message_destroy(msg);
        }
    }
    
    // Receive and process messages
    for (int i = 0; i < 5; i++) {
        GooMessage* msg = goo_channel_receive(channel);
        if (!msg) {
            printf("Failed to receive message\n");
            continue;
        }
        
        const char* data = goo_message_get_string(msg);
        printf("Received: %s\n", data);
        
        goo_message_destroy(msg);
    }
    
    // Check channel stats
    printf("Channel stats: %zu messages, capacity %zu\n", 
           goo_channel_message_count(channel),
           goo_channel_capacity(channel));
    
    // Clean up
    goo_channel_destroy(channel);
}

int main() {
    printf("Goo Messaging System Demo\n");
    printf("=========================\n");
    
    // Initialize the Goo runtime
    if (!goo_runtime_init()) {
        printf("Failed to initialize Goo runtime\n");
        return 1;
    }
    
    // Initialize messaging subsystem
    if (!goo_messaging_init()) {
        printf("Failed to initialize messaging subsystem\n");
        goo_runtime_cleanup();
        return 1;
    }
    
    printf("Goo Runtime Version: %s\n", goo_runtime_version());
    
    // Demonstrate basic channels
    demonstrate_basic_channels();
    
    // Demonstrate Publish-Subscribe pattern
    demonstrate_pubsub();
    
    // Demonstrate Push-Pull pattern
    demonstrate_pushpull();
    
    // Demonstrate Request-Reply pattern
    demonstrate_reqrep();
    
    // Clean up
    goo_messaging_cleanup();
    goo_runtime_cleanup();
    
    printf("\nDemo completed successfully\n");
    return 0;
} 