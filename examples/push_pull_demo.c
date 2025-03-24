/**
 * push_pull_demo.c
 * 
 * Demonstration of the push-pull (worker distribution) messaging pattern in Goo.
 * This uses a simple implementation that doesn't rely on the full Zig library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

// Simple opaque types
typedef struct Message Message;
typedef struct Channel Channel;

// Message types
typedef enum {
    MESSAGE_STRING,
    MESSAGE_INT,
    MESSAGE_BINARY,
    MESSAGE_TASK
} MessageType;

// Basic message implementation
struct Message {
    MessageType type;
    void* data;
    size_t size;
    int task_id;    // For push-pull pattern
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
    msg->task_id = -1;
    
    return msg;
}

// Destroy a message
void message_destroy(Message* msg) {
    if (!msg) return;
    
    if (msg->data) {
        free(msg->data);
    }
    
    free(msg);
}

// Create a task message
Message* message_create_task(const char* description, int task_id) {
    Message* msg = message_create_string(description);
    if (!msg) return NULL;
    
    msg->type = MESSAGE_TASK;
    msg->task_id = task_id;
    
    return msg;
}

// Get string data from a message
const char* message_get_string(const Message* msg) {
    if (!msg || (msg->type != MESSAGE_STRING && msg->type != MESSAGE_TASK)) 
        return NULL;
    return (const char*)msg->data;
}

// Get task ID from a message
int message_get_task_id(const Message* msg) {
    if (!msg) return -1;
    return msg->task_id;
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

// Push a task to a channel
int channel_push_task(Channel* chan, const char* description, int task_id) {
    Message* msg = message_create_task(description, task_id);
    if (!msg) return 0;
    
    int result = channel_send(chan, msg);
    
    if (!result) {
        message_destroy(msg);
    }
    
    return result;
}

// Pull a task from a channel
Message* channel_pull_task(Channel* chan) {
    return channel_receive_wait(chan);
}

// Thread function for task producer
void* producer_thread(void* arg) {
    Channel* chan = (Channel*)arg;
    
    printf("Producer: Starting\n");
    
    // Produce 20 tasks
    for (int i = 0; i < 20; i++) {
        char task_desc[64];
        snprintf(task_desc, sizeof(task_desc), "Task %d", i);
        
        if (channel_push_task(chan, task_desc, i)) {
            printf("Producer: Pushed task %d\n", i);
        } else {
            printf("Producer: Failed to push task %d\n", i);
        }
        
        // Random delay between pushing tasks
        usleep(rand() % 200000 + 100000);  // 100-300ms
    }
    
    // Close the channel when done
    printf("Producer: Closing channel\n");
    channel_close(chan);
    
    printf("Producer: Done\n");
    return NULL;
}

// Thread function for task consumer (worker)
void* worker_thread(void* arg) {
    struct {
        Channel* chan;
        int worker_id;
    } *data = arg;
    
    Channel* chan = data->chan;
    int worker_id = data->worker_id;
    
    printf("Worker %d: Starting\n", worker_id);
    
    // Process tasks until the channel is closed
    while (true) {
        Message* task = channel_pull_task(chan);
        if (!task) {
            printf("Worker %d: Channel closed\n", worker_id);
            break;
        }
        
        const char* desc = message_get_string(task);
        int task_id = message_get_task_id(task);
        
        printf("Worker %d: Processing %s (ID: %d)\n", worker_id, desc ? desc : "unknown task", task_id);
        
        // Simulate work
        usleep(rand() % 500000 + 200000);  // 200-700ms
        
        printf("Worker %d: Completed %s (ID: %d)\n", worker_id, desc ? desc : "unknown task", task_id);
        
        message_destroy(task);
    }
    
    printf("Worker %d: Done\n", worker_id);
    return NULL;
}

int main() {
    printf("Push-Pull Demo\n");
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Create a channel for tasks
    Channel* task_channel = channel_create(10);
    if (!task_channel) {
        printf("Failed to create task channel\n");
        return 1;
    }
    
    // Create threads
    pthread_t producer;
    pthread_t workers[3];
    
    struct {
        Channel* chan;
        int worker_id;
    } worker_data[3] = {
        { task_channel, 1 },
        { task_channel, 2 },
        { task_channel, 3 }
    };
    
    // Start producer
    if (pthread_create(&producer, NULL, producer_thread, task_channel) != 0) {
        printf("Failed to create producer thread\n");
        goto cleanup;
    }
    
    // Start workers
    for (int i = 0; i < 3; i++) {
        if (pthread_create(&workers[i], NULL, worker_thread, &worker_data[i]) != 0) {
            printf("Failed to create worker thread %d\n", i + 1);
            goto cleanup;
        }
    }
    
    // Wait for threads to finish
    pthread_join(producer, NULL);
    
    for (int i = 0; i < 3; i++) {
        pthread_join(workers[i], NULL);
    }
    
cleanup:
    // Clean up
    channel_destroy(task_channel);
    
    printf("Demo completed successfully\n");
    return 0;
} 