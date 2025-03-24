/**
 * pubsub_demo.c
 * 
 * Demonstration of the publish-subscribe messaging pattern in Goo.
 * This uses a simple implementation that doesn't rely on the full Zig library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>

// Simple opaque types
typedef struct Message Message;
typedef struct Subscriber Subscriber;
typedef struct Publisher Publisher;

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

// Subscriber implementation
struct Subscriber {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    Message** buffer;
    size_t capacity;
    size_t count;
    size_t head;
    size_t tail;
    char** topics;
    size_t topic_count;
    char* name;
};

// Publisher implementation
struct Publisher {
    pthread_mutex_t mutex;
    Subscriber** subscribers;
    size_t subscriber_count;
    size_t subscriber_capacity;
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

// Create a subscriber
Subscriber* subscriber_create(size_t capacity, const char* name) {
    Subscriber* sub = (Subscriber*)malloc(sizeof(Subscriber));
    if (!sub) return NULL;
    
    sub->buffer = (Message**)malloc(capacity * sizeof(Message*));
    if (!sub->buffer) {
        free(sub);
        return NULL;
    }
    
    sub->topics = (char**)malloc(10 * sizeof(char*)); // Start with capacity for 10 topics
    if (!sub->topics) {
        free(sub->buffer);
        free(sub);
        return NULL;
    }
    
    pthread_mutex_init(&sub->mutex, NULL);
    pthread_cond_init(&sub->cond, NULL);
    
    sub->capacity = capacity;
    sub->count = 0;
    sub->head = 0;
    sub->tail = 0;
    sub->topic_count = 0;
    
    sub->name = name ? strdup(name) : NULL;
    
    return sub;
}

// Destroy a subscriber
void subscriber_destroy(Subscriber* sub) {
    if (!sub) return;
    
    pthread_mutex_lock(&sub->mutex);
    
    // Free any messages still in the buffer
    for (size_t i = 0; i < sub->count; i++) {
        size_t idx = (sub->head + i) % sub->capacity;
        message_destroy(sub->buffer[idx]);
    }
    
    free(sub->buffer);
    
    // Free topics
    for (size_t i = 0; i < sub->topic_count; i++) {
        free(sub->topics[i]);
    }
    free(sub->topics);
    
    if (sub->name) {
        free(sub->name);
    }
    
    pthread_mutex_unlock(&sub->mutex);
    
    pthread_mutex_destroy(&sub->mutex);
    pthread_cond_destroy(&sub->cond);
    
    free(sub);
}

// Subscribe to a topic
int subscriber_subscribe(Subscriber* sub, const char* topic) {
    if (!sub || !topic) return 0;
    
    pthread_mutex_lock(&sub->mutex);
    
    // Check if already subscribed
    for (size_t i = 0; i < sub->topic_count; i++) {
        if (strcmp(sub->topics[i], topic) == 0) {
            pthread_mutex_unlock(&sub->mutex);
            return 1; // Already subscribed
        }
    }
    
    // Add topic
    if (sub->topic_count >= 10) { // Simplistic approach, fixed size
        pthread_mutex_unlock(&sub->mutex);
        return 0; // Too many topics
    }
    
    sub->topics[sub->topic_count] = strdup(topic);
    if (!sub->topics[sub->topic_count]) {
        pthread_mutex_unlock(&sub->mutex);
        return 0;
    }
    
    sub->topic_count++;
    
    pthread_mutex_unlock(&sub->mutex);
    
    return 1;
}

// Check if subscribed to a topic
int subscriber_is_subscribed(Subscriber* sub, const char* topic) {
    if (!sub || !topic) return 0;
    
    pthread_mutex_lock(&sub->mutex);
    
    int result = 0;
    for (size_t i = 0; i < sub->topic_count; i++) {
        if (strcmp(sub->topics[i], topic) == 0) {
            result = 1;
            break;
        }
    }
    
    pthread_mutex_unlock(&sub->mutex);
    
    return result;
}

// Deliver a message to a subscriber
int subscriber_deliver(Subscriber* sub, Message* msg) {
    if (!sub || !msg) return 0;
    
    pthread_mutex_lock(&sub->mutex);
    
    // Check if the buffer is full
    if (sub->count == sub->capacity) {
        pthread_mutex_unlock(&sub->mutex);
        return 0;
    }
    
    // Add the message to the buffer
    sub->buffer[sub->tail] = msg;
    sub->tail = (sub->tail + 1) % sub->capacity;
    sub->count++;
    
    // Signal waiting receivers
    pthread_cond_signal(&sub->cond);
    
    pthread_mutex_unlock(&sub->mutex);
    
    return 1;
}

// Receive a message from a subscriber
Message* subscriber_receive(Subscriber* sub) {
    if (!sub) return NULL;
    
    pthread_mutex_lock(&sub->mutex);
    
    // Check if the buffer is empty
    if (sub->count == 0) {
        pthread_mutex_unlock(&sub->mutex);
        return NULL;
    }
    
    // Get the message from the buffer
    Message* msg = sub->buffer[sub->head];
    sub->head = (sub->head + 1) % sub->capacity;
    sub->count--;
    
    pthread_mutex_unlock(&sub->mutex);
    
    return msg;
}

// Receive a message from a subscriber, waiting if necessary
Message* subscriber_receive_wait(Subscriber* sub) {
    if (!sub) return NULL;
    
    pthread_mutex_lock(&sub->mutex);
    
    // Wait until a message is available
    while (sub->count == 0) {
        pthread_cond_wait(&sub->cond, &sub->mutex);
    }
    
    // Get the message from the buffer
    Message* msg = sub->buffer[sub->head];
    sub->head = (sub->head + 1) % sub->capacity;
    sub->count--;
    
    pthread_mutex_unlock(&sub->mutex);
    
    return msg;
}

// Create a publisher
Publisher* publisher_create(const char* name) {
    Publisher* pub = (Publisher*)malloc(sizeof(Publisher));
    if (!pub) return NULL;
    
    pub->subscribers = (Subscriber**)malloc(10 * sizeof(Subscriber*)); // Start with capacity for 10 subscribers
    if (!pub->subscribers) {
        free(pub);
        return NULL;
    }
    
    pthread_mutex_init(&pub->mutex, NULL);
    
    pub->subscriber_count = 0;
    pub->subscriber_capacity = 10;
    
    pub->name = name ? strdup(name) : NULL;
    
    return pub;
}

// Destroy a publisher
void publisher_destroy(Publisher* pub) {
    if (!pub) return;
    
    pthread_mutex_lock(&pub->mutex);
    
    free(pub->subscribers);
    
    if (pub->name) {
        free(pub->name);
    }
    
    pthread_mutex_unlock(&pub->mutex);
    
    pthread_mutex_destroy(&pub->mutex);
    
    free(pub);
}

// Add a subscriber to a publisher
int publisher_add_subscriber(Publisher* pub, Subscriber* sub) {
    if (!pub || !sub) return 0;
    
    pthread_mutex_lock(&pub->mutex);
    
    // Check if already added
    for (size_t i = 0; i < pub->subscriber_count; i++) {
        if (pub->subscribers[i] == sub) {
            pthread_mutex_unlock(&pub->mutex);
            return 1; // Already added
        }
    }
    
    // Add subscriber
    if (pub->subscriber_count >= pub->subscriber_capacity) {
        pthread_mutex_unlock(&pub->mutex);
        return 0; // Too many subscribers
    }
    
    pub->subscribers[pub->subscriber_count++] = sub;
    
    pthread_mutex_unlock(&pub->mutex);
    
    return 1;
}

// Publish a message to all subscribers interested in the topic
int publisher_publish(Publisher* pub, const char* topic, const char* data) {
    if (!pub || !topic || !data) return 0;
    
    pthread_mutex_lock(&pub->mutex);
    
    int delivered = 0;
    
    for (size_t i = 0; i < pub->subscriber_count; i++) {
        Subscriber* sub = pub->subscribers[i];
        
        // Check if this subscriber is interested in this topic
        if (subscriber_is_subscribed(sub, topic)) {
            // Create a new message for each subscriber
            Message* msg = message_create_string(data);
            if (!msg) continue;
            
            // Set the topic
            if (!message_set_topic(msg, topic)) {
                message_destroy(msg);
                continue;
            }
            
            // Deliver the message
            if (subscriber_deliver(sub, msg)) {
                delivered++;
            } else {
                message_destroy(msg);
            }
        }
    }
    
    pthread_mutex_unlock(&pub->mutex);
    
    return delivered > 0;
}

// Thread function for publisher
void* publisher_thread(void* arg) {
    Publisher* pub = (Publisher*)arg;
    
    // Publish 5 messages to each topic
    const char* topics[] = {"weather", "sports", "tech"};
    
    for (int t = 0; t < 3; t++) {
        const char* topic = topics[t];
        
        for (int i = 0; i < 5; i++) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "Message %d for %s", i, topic);
            
            printf("Publishing to '%s': %s\n", topic, buffer);
            
            if (!publisher_publish(pub, topic, buffer)) {
                printf("No subscribers received the message\n");
            }
            
            usleep(300000); // Sleep for 0.3 seconds
        }
    }
    
    printf("Publisher done\n");
    return NULL;
}

// Thread function for subscriber
void* subscriber_thread(void* arg) {
    Subscriber* sub = (Subscriber*)arg;
    
    // Receive 10 messages
    for (int i = 0; i < 10; i++) {
        Message* msg = subscriber_receive_wait(sub);
        if (!msg) {
            printf("Failed to receive message\n");
            continue;
        }
        
        const char* topic = message_get_topic(msg);
        const char* data = message_get_string(msg);
        
        printf("Subscriber '%s' received on topic '%s': %s\n", 
               sub->name ? sub->name : "unknown",
               topic ? topic : "none", 
               data ? data : "empty");
        
        message_destroy(msg);
    }
    
    printf("Subscriber '%s' done\n", sub->name ? sub->name : "unknown");
    return NULL;
}

// Main function
int main() {
    printf("Publish-Subscribe Demo\n");
    
    // Create a publisher
    Publisher* publisher = publisher_create("main-publisher");
    if (!publisher) {
        printf("Failed to create publisher\n");
        return 1;
    }
    
    // Create subscribers with different interests
    Subscriber* weather_sub = subscriber_create(20, "weather-follower");
    Subscriber* sports_sub = subscriber_create(20, "sports-fan");
    Subscriber* all_sub = subscriber_create(20, "news-junkie");
    
    if (!weather_sub || !sports_sub || !all_sub) {
        printf("Failed to create subscribers\n");
        if (publisher) publisher_destroy(publisher);
        if (weather_sub) subscriber_destroy(weather_sub);
        if (sports_sub) subscriber_destroy(sports_sub);
        if (all_sub) subscriber_destroy(all_sub);
        return 1;
    }
    
    // Subscribe to topics
    subscriber_subscribe(weather_sub, "weather");
    subscriber_subscribe(sports_sub, "sports");
    subscriber_subscribe(all_sub, "weather");
    subscriber_subscribe(all_sub, "sports");
    subscriber_subscribe(all_sub, "tech");
    
    // Add subscribers to publisher
    publisher_add_subscriber(publisher, weather_sub);
    publisher_add_subscriber(publisher, sports_sub);
    publisher_add_subscriber(publisher, all_sub);
    
    // Create threads
    pthread_t pub_thread, weather_thread, sports_thread, all_thread;
    
    if (pthread_create(&pub_thread, NULL, publisher_thread, publisher) != 0) {
        printf("Failed to create publisher thread\n");
        goto cleanup;
    }
    
    if (pthread_create(&weather_thread, NULL, subscriber_thread, weather_sub) != 0) {
        printf("Failed to create weather subscriber thread\n");
        goto cleanup;
    }
    
    if (pthread_create(&sports_thread, NULL, subscriber_thread, sports_sub) != 0) {
        printf("Failed to create sports subscriber thread\n");
        goto cleanup;
    }
    
    if (pthread_create(&all_thread, NULL, subscriber_thread, all_sub) != 0) {
        printf("Failed to create all-topics subscriber thread\n");
        goto cleanup;
    }
    
    // Wait for threads to finish
    pthread_join(pub_thread, NULL);
    pthread_join(weather_thread, NULL);
    pthread_join(sports_thread, NULL);
    pthread_join(all_thread, NULL);
    
cleanup:
    // Clean up
    publisher_destroy(publisher);
    subscriber_destroy(weather_sub);
    subscriber_destroy(sports_sub);
    subscriber_destroy(all_sub);
    
    printf("Demo completed successfully\n");
    return 0;
} 