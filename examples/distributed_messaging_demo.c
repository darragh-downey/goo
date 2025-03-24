/**
 * Distributed Messaging Demo for Goo
 * 
 * Demonstrates the distributed messaging capabilities of the Goo messaging system.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>

// Define MOCK_IMPLEMENTATION to use our own mock implementations instead of real ones
#define MOCK_IMPLEMENTATION 1

#ifndef MOCK_IMPLEMENTATION
#include "messaging/goo_channels_core.h"
#else
// Define proper message flags - these should match the Zig definitions
#define GOO_MSG_NONE 0
#define GOO_MSG_NONBLOCK 1
#define GOO_MSG_PEEK 2
#define GOO_MSG_OOB 4
#define GOO_MSG_MORE 8

// Channel types
typedef enum {
    Normal = 0,  // Normal bidirectional channel
    Pub = 1,     // Publisher channel (one-to-many)
    Sub = 2,     // Subscriber channel (many-to-one)
    Push = 3,    // Push channel (distributes to workers)
    Pull = 4,    // Pull channel (receives from push)
    Req = 5,     // Request channel (sends requests)
    Rep = 6      // Reply channel (processes requests)
} GooChannelType;

/* Channel options */
#define GOO_CHAN_DEFAULT 0
#define GOO_CHAN_NONBLOCKING 1
#define GOO_CHAN_BUFFERED 2
#define GOO_CHAN_UNBUFFERED 4
#define GOO_CHAN_RELIABLE 8

/* Transport protocols */
#define GOO_TRANSPORT_INPROC 0
#define GOO_TRANSPORT_IPC 1
#define GOO_TRANSPORT_TCP 2

/* Forward declarations for structures */
typedef struct GooChannel GooChannel;
typedef struct GooMessage GooMessage;
#endif

// Global variables for control
volatile bool running = true;

// Signal handler to catch Ctrl+C
void handle_signal(int signal) {
    printf("\nCaught signal %d, shutting down...\n", signal);
    running = false;
    
    // Give threads time to notice the running flag changed
    usleep(500000); // 500ms
}

// Thread arguments structure
typedef struct {
    GooChannel* channel;
    int id;
    const char* role;
} ThreadArgs;

#ifdef MOCK_IMPLEMENTATION
// Mock channel structure
struct GooChannel {
    int type;
    int id;
    bool is_closed;
};

// Mock message structure
struct GooMessage {
    void* data;
    size_t size;
    int flags;
    void* context;
};

// Mock function implementations
int goo_channel_connect(GooChannel* channel, int protocol, const char* address, uint16_t port) {
    printf("[MOCK] Connecting to %s:%d using protocol %d\n", address, port, protocol);
    return 0; // Always succeed for now
}

int goo_channel_set_endpoint(GooChannel* channel, int protocol, const char* address, uint16_t port) {
    printf("[MOCK] Setting endpoint at %s:%d using protocol %d\n", address, port, protocol);
    return 0; // Always succeed for now
}

// Mock send function
int goo_channel_send(GooChannel* channel, const void* data, size_t size, int flags) {
    printf("[MOCK] Sending %zu bytes with flags %d\n", size, flags);
    return (int)size; // Return the number of bytes "sent"
}

// Mock receive function
int goo_channel_recv(GooChannel* channel, void* data, size_t size, int flags) {
    printf("[MOCK] Receiving up to %zu bytes with flags %d\n", size, flags);
    
    // For demo purposes, generate some fake data
    const char* fake_data = "MOCK DATA RECEIVED";
    size_t fake_size = strlen(fake_data);
    size_t copy_size = (fake_size < size) ? fake_size : size - 1;
    
    memcpy(data, fake_data, copy_size);
    ((char*)data)[copy_size] = '\0'; // Null terminate
    
    return (int)copy_size;
}

// Mock channel creation function
GooChannel* goo_channel_create(int type, size_t elem_size, size_t buffer_size, int options) {
    printf("[MOCK] Creating channel type %d, elem_size %zu, buffer_size %zu, options %d\n", 
            type, elem_size, buffer_size, options);
    
    GooChannel* channel = (GooChannel*)malloc(sizeof(GooChannel));
    if (channel) {
        channel->type = type;
        channel->id = rand() % 1000; // Random ID for demonstration
        channel->is_closed = false;
    }
    return channel;
}

// Mock channel destruction
void goo_channel_destroy(GooChannel* channel) {
    if (channel) {
        printf("[MOCK] Destroying channel ID %d\n", channel->id);
        free(channel);
    }
}

// Mock channel close function
int goo_channel_close(GooChannel* channel) {
    if (channel) {
        printf("[MOCK] Closing channel ID %d\n", channel->id);
        channel->is_closed = true;
    }
    return 0;
}

// Mock message creation
GooMessage* goo_message_create(void* data, size_t size, int flags) {
    printf("[MOCK] Creating message with size %zu and flags %d\n", size, flags);
    
    GooMessage* msg = (GooMessage*)malloc(sizeof(GooMessage));
    if (msg) {
        if (data && size > 0) {
            msg->data = malloc(size);
            if (msg->data) {
                memcpy(msg->data, data, size);
                msg->size = size;
            } else {
                free(msg);
                return NULL;
            }
        } else {
            msg->data = NULL;
            msg->size = 0;
        }
        msg->flags = flags;
        msg->context = NULL;
    }
    return msg;
}

// Mock message destruction
void goo_message_destroy(GooMessage* msg) {
    if (msg) {
        printf("[MOCK] Destroying message with size %zu\n", msg->size);
        if (msg->data) {
            free(msg->data);
        }
        free(msg);
    }
}
#endif

// Server thread function
void* server_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GooChannel* channel = args->channel;
    int id = args->id;
    
    printf("[Server %d] Starting server...\n", id);
    
    char buffer[256];
    
    while (running) {
        // Receive a message
        int recv_bytes = goo_channel_recv(channel, buffer, sizeof(buffer), GOO_MSG_NONE);
        
        if (recv_bytes > 0) {
            buffer[recv_bytes] = '\0';
            printf("[Server %d] Received message: %s\n", id, buffer);
            
            // Send a reply
            char reply[256];
            snprintf(reply, sizeof(reply), "Reply to: %s", buffer);
            int sent = goo_channel_send(channel, reply, strlen(reply), GOO_MSG_NONE);
            
            if (sent < 0) {
                printf("[Server %d] Error sending reply: %d\n", id, sent);
            }
        } else if (recv_bytes < 0) {
            // Only report errors if we're still running
            if (running && recv_bytes != -EAGAIN) {
                printf("[Server %d] Error receiving message: %d\n", id, recv_bytes);
            }
            usleep(10000); // 10ms
        }
    }
    
    printf("[Server %d] Server stopped\n", id);
    return NULL;
}

// Client thread function
void* client_thread(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    GooChannel* channel = args->channel;
    int id = args->id;
    
    printf("[Client %d] Starting client...\n", id);
    
    char buffer[256];
    int message_count = 0;
    int timeout_count = 0;
    const int MAX_TIMEOUTS = 5;
    
    while (running && message_count < 10 && timeout_count < MAX_TIMEOUTS) {
        // Send a message
        char message[256];
        snprintf(message, sizeof(message), "Message %d from client %d", message_count, id);
        int sent = goo_channel_send(channel, message, strlen(message), GOO_MSG_NONE);
        
        if (sent < 0) {
            printf("[Client %d] Error sending message: %d\n", id, sent);
            usleep(10000); // 10ms
            timeout_count++;
            continue;
        }
        
        printf("[Client %d] Sent: %s\n", id, message);
        
        // Receive a reply
        int recv_bytes = goo_channel_recv(channel, buffer, sizeof(buffer), GOO_MSG_NONE);
        
        if (recv_bytes > 0) {
            buffer[recv_bytes] = '\0';
            printf("[Client %d] Received reply: %s\n", id, buffer);
            message_count++;
            timeout_count = 0; // Reset timeout counter on successful communication
        } else if (recv_bytes < 0) {
            if (recv_bytes != -EAGAIN) {
                printf("[Client %d] Error receiving reply: %d\n", id, recv_bytes);
            }
            timeout_count++;
        }
        
        // Sleep between messages
        usleep(500000); // 500ms
    }
    
    if (timeout_count >= MAX_TIMEOUTS) {
        printf("[Client %d] Client timed out after %d consecutive failures\n", id, MAX_TIMEOUTS);
    } else {
        printf("[Client %d] Client finished (sent %d messages)\n", id, message_count);
    }
    
    return NULL;
}

// PubSub demo function
void run_pubsub_demo() {
    printf("\n===== PUBLISH/SUBSCRIBE OVER NETWORK DEMO =====\n");
    
    // Create publisher channel
    GooChannel* pub = goo_channel_create(Pub, 256, 10, GOO_CHAN_DEFAULT);
    if (pub == NULL) {
        printf("Failed to create publisher channel\n");
        return;
    }
    
    // Set up server endpoint
    int result = goo_channel_set_endpoint(pub, GOO_TRANSPORT_TCP, "127.0.0.1", 5555);
    if (result < 0) {
        printf("Failed to set up publisher endpoint: %d\n", result);
        goo_channel_destroy(pub);
        return;
    }
    
    printf("Publisher listening on tcp://127.0.0.1:5555\n");
    
    // Create subscriber channels
    GooChannel* sub1 = goo_channel_create(Sub, 256, 10, GOO_CHAN_DEFAULT);
    GooChannel* sub2 = goo_channel_create(Sub, 256, 10, GOO_CHAN_DEFAULT);
    
    if (sub1 == NULL || sub2 == NULL) {
        printf("Failed to create subscriber channels\n");
        goo_channel_destroy(pub);
        if (sub1) goo_channel_destroy(sub1);
        if (sub2) goo_channel_destroy(sub2);
        return;
    }
    
    // Connect subscribers to publisher
    result = goo_channel_connect(sub1, GOO_TRANSPORT_TCP, "127.0.0.1", 5555);
    if (result < 0) {
        printf("Failed to connect subscriber 1: %d\n", result);
        goo_channel_destroy(pub);
        goo_channel_destroy(sub1);
        goo_channel_destroy(sub2);
        return;
    }
    
    result = goo_channel_connect(sub2, GOO_TRANSPORT_TCP, "127.0.0.1", 5555);
    if (result < 0) {
        printf("Failed to connect subscriber 2: %d\n", result);
        goo_channel_destroy(pub);
        goo_channel_destroy(sub1);
        goo_channel_destroy(sub2);
        return;
    }
    
    printf("Subscribers connected to publisher\n");
    
    // Create subscriber threads
    pthread_t sub1_thread, sub2_thread;
    ThreadArgs sub1_args = {sub1, 1, "Subscriber"};
    ThreadArgs sub2_args = {sub2, 2, "Subscriber"};
    
    pthread_create(&sub1_thread, NULL, server_thread, &sub1_args);
    pthread_create(&sub2_thread, NULL, server_thread, &sub2_args);
    
    // Publish messages
    printf("Publishing messages...\n");
    for (int i = 0; i < 5 && running; i++) {
        char message[256];
        snprintf(message, sizeof(message), "Broadcast message %d", i);
        
        result = goo_channel_send(pub, message, strlen(message), GOO_MSG_NONE);
        if (result < 0) {
            printf("Error publishing message: %d\n", result);
        } else {
            printf("Published: %s\n", message);
        }
        
        // Sleep between messages
        sleep(1);
    }
    
    // Wait a bit for subscribers to process messages
    sleep(2);
    
    // Clean up
    running = false;
    pthread_join(sub1_thread, NULL);
    pthread_join(sub2_thread, NULL);
    
    goo_channel_destroy(pub);
    goo_channel_destroy(sub1);
    goo_channel_destroy(sub2);
    
    printf("PubSub demo completed\n");
}

// ReqRep demo function
void run_reqrep_demo() {
    printf("\n===== REQUEST/REPLY OVER NETWORK DEMO =====\n");
    
    // Create reply channel (server)
    GooChannel* rep = goo_channel_create(Rep, 256, 10, GOO_CHAN_DEFAULT);
    if (rep == NULL) {
        printf("Failed to create reply channel\n");
        return;
    }
    
    // Set up server endpoint
    int result = goo_channel_set_endpoint(rep, GOO_TRANSPORT_TCP, "127.0.0.1", 5556);
    if (result < 0) {
        printf("Failed to set up reply endpoint: %d\n", result);
        goo_channel_destroy(rep);
        return;
    }
    
    printf("Reply server listening on tcp://127.0.0.1:5556\n");
    
    // Create request channels (clients)
    GooChannel* req1 = goo_channel_create(Req, 256, 10, GOO_CHAN_DEFAULT);
    GooChannel* req2 = goo_channel_create(Req, 256, 10, GOO_CHAN_DEFAULT);
    
    if (req1 == NULL || req2 == NULL) {
        printf("Failed to create request channels\n");
        goo_channel_destroy(rep);
        if (req1) goo_channel_destroy(req1);
        if (req2) goo_channel_destroy(req2);
        return;
    }
    
    // Connect clients to server
    result = goo_channel_connect(req1, GOO_TRANSPORT_TCP, "127.0.0.1", 5556);
    if (result < 0) {
        printf("Failed to connect request client 1: %d\n", result);
        goo_channel_destroy(rep);
        goo_channel_destroy(req1);
        goo_channel_destroy(req2);
        return;
    }
    
    result = goo_channel_connect(req2, GOO_TRANSPORT_TCP, "127.0.0.1", 5556);
    if (result < 0) {
        printf("Failed to connect request client 2: %d\n", result);
        goo_channel_destroy(rep);
        goo_channel_destroy(req1);
        goo_channel_destroy(req2);
        return;
    }
    
    printf("Request clients connected to server\n");
    
    // Start server thread
    pthread_t server_tid;
    ThreadArgs server_args = {rep, 0, "Server"};
    pthread_create(&server_tid, NULL, server_thread, &server_args);
    
    // Start client threads
    pthread_t client1_tid, client2_tid;
    ThreadArgs client1_args = {req1, 1, "Client"};
    ThreadArgs client2_args = {req2, 2, "Client"};
    
    pthread_create(&client1_tid, NULL, client_thread, &client1_args);
    pthread_create(&client2_tid, NULL, client_thread, &client2_args);
    
    // Wait for clients to finish
    pthread_join(client1_tid, NULL);
    pthread_join(client2_tid, NULL);
    
    // Stop server
    running = false;
    pthread_join(server_tid, NULL);
    
    // Cleanup
    goo_channel_destroy(rep);
    goo_channel_destroy(req1);
    goo_channel_destroy(req2);
    
    printf("ReqRep demo completed\n");
}

// Push/Pull (load balancing) demo function
void run_pushpull_demo() {
    printf("\n===== PUSH/PULL OVER NETWORK DEMO =====\n");
    
    // Create push channel (work distributor)
    GooChannel* push = goo_channel_create(Push, 256, 10, GOO_CHAN_DEFAULT);
    if (push == NULL) {
        printf("Failed to create push channel\n");
        return;
    }
    
    // Set up server endpoint
    int result = goo_channel_set_endpoint(push, GOO_TRANSPORT_TCP, "127.0.0.1", 5557);
    if (result < 0) {
        printf("Failed to set up push endpoint: %d\n", result);
        goo_channel_destroy(push);
        return;
    }
    
    printf("Push distributor listening on tcp://127.0.0.1:5557\n");
    
    // Create pull channels (workers)
    GooChannel* pull1 = goo_channel_create(Pull, 256, 10, GOO_CHAN_DEFAULT);
    GooChannel* pull2 = goo_channel_create(Pull, 256, 10, GOO_CHAN_DEFAULT);
    GooChannel* pull3 = goo_channel_create(Pull, 256, 10, GOO_CHAN_DEFAULT);
    
    if (pull1 == NULL || pull2 == NULL || pull3 == NULL) {
        printf("Failed to create pull channels\n");
        goo_channel_destroy(push);
        if (pull1) goo_channel_destroy(pull1);
        if (pull2) goo_channel_destroy(pull2);
        if (pull3) goo_channel_destroy(pull3);
        return;
    }
    
    // Connect workers to distributor
    result = goo_channel_connect(pull1, GOO_TRANSPORT_TCP, "127.0.0.1", 5557);
    if (result < 0) {
        printf("Failed to connect worker 1: %d\n", result);
        goo_channel_destroy(push);
        goo_channel_destroy(pull1);
        goo_channel_destroy(pull2);
        goo_channel_destroy(pull3);
        return;
    }
    
    result = goo_channel_connect(pull2, GOO_TRANSPORT_TCP, "127.0.0.1", 5557);
    if (result < 0) {
        printf("Failed to connect worker 2: %d\n", result);
        goo_channel_destroy(push);
        goo_channel_destroy(pull1);
        goo_channel_destroy(pull2);
        goo_channel_destroy(pull3);
        return;
    }
    
    result = goo_channel_connect(pull3, GOO_TRANSPORT_TCP, "127.0.0.1", 5557);
    if (result < 0) {
        printf("Failed to connect worker 3: %d\n", result);
        goo_channel_destroy(push);
        goo_channel_destroy(pull1);
        goo_channel_destroy(pull2);
        goo_channel_destroy(pull3);
        return;
    }
    
    printf("Workers connected to distributor\n");
    
    // Start worker threads
    pthread_t worker1_tid, worker2_tid, worker3_tid;
    ThreadArgs worker1_args = {pull1, 1, "Worker"};
    ThreadArgs worker2_args = {pull2, 2, "Worker"};
    ThreadArgs worker3_args = {pull3, 3, "Worker"};
    
    pthread_create(&worker1_tid, NULL, server_thread, &worker1_args);
    pthread_create(&worker2_tid, NULL, server_thread, &worker2_args);
    pthread_create(&worker3_tid, NULL, server_thread, &worker3_args);
    
    // Push work items
    printf("Pushing work items to workers...\n");
    for (int i = 0; i < 15 && running; i++) {
        char work_item[256];
        snprintf(work_item, sizeof(work_item), "Work item %d", i);
        
        result = goo_channel_send(push, work_item, strlen(work_item), GOO_MSG_NONE);
        if (result < 0) {
            printf("Error pushing work item: %d\n", result);
        } else {
            printf("Pushed: %s\n", work_item);
        }
        
        // Sleep between work items
        usleep(200000); // 200ms
    }
    
    // Wait for workers to process items
    sleep(2);
    
    // Cleanup
    running = false;
    pthread_join(worker1_tid, NULL);
    pthread_join(worker2_tid, NULL);
    pthread_join(worker3_tid, NULL);
    
    goo_channel_destroy(push);
    goo_channel_destroy(pull1);
    goo_channel_destroy(pull2);
    goo_channel_destroy(pull3);
    
    printf("Push/Pull demo completed\n");
}

// IPC (Inter-Process Communication) demo function
void run_ipc_demo() {
    printf("\n===== IPC (UNIX SOCKET) DEMO =====\n");
    
    // Create server channel
    GooChannel* server = goo_channel_create(Normal, 256, 10, GOO_CHAN_DEFAULT);
    if (server == NULL) {
        printf("Failed to create server channel\n");
        return;
    }
    
    // Set up IPC endpoint
    const char* socket_path = "/tmp/goo_ipc_demo.sock";
    int result = goo_channel_set_endpoint(server, GOO_TRANSPORT_IPC, "/tmp/goo_ipc_demo.sock", 0);
    if (result < 0) {
        printf("Failed to set up IPC endpoint: %d\n", result);
        goo_channel_destroy(server);
        return;
    }
    
    printf("IPC server listening on %s\n", socket_path);
    
    // Create client channel
    GooChannel* client = goo_channel_create(Normal, 256, 10, GOO_CHAN_DEFAULT);
    if (client == NULL) {
        printf("Failed to create client channel\n");
        goo_channel_destroy(server);
        return;
    }
    
    // Connect client to server
    result = goo_channel_connect(client, GOO_TRANSPORT_IPC, "/tmp/goo_ipc_demo.sock", 0);
    if (result < 0) {
        printf("Failed to connect IPC client: %d\n", result);
        goo_channel_destroy(server);
        goo_channel_destroy(client);
        return;
    }
    
    printf("IPC client connected to server\n");
    
    // Start server thread
    pthread_t server_tid;
    ThreadArgs server_args = {server, 0, "IPC Server"};
    pthread_create(&server_tid, NULL, server_thread, &server_args);
    
    // Start client thread
    pthread_t client_tid;
    ThreadArgs client_args = {client, 0, "IPC Client"};
    pthread_create(&client_tid, NULL, client_thread, &client_args);
    
    // Wait for client to finish
    pthread_join(client_tid, NULL);
    
    // Stop server
    running = false;
    pthread_join(server_tid, NULL);
    
    // Cleanup
    goo_channel_destroy(server);
    goo_channel_destroy(client);
    
    printf("IPC demo completed\n");
}

int main() {
    // Set up signal handler
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    printf("==============================================\n");
    printf("    Goo Distributed Messaging Demo\n");
    printf("==============================================\n");
    
    // Run demonstration of different messaging patterns
    if (running) run_ipc_demo();
    if (running) run_pubsub_demo();
    if (running) run_reqrep_demo();
    if (running) run_pushpull_demo();
    
    printf("\n==============================================\n");
    printf("All demos completed%s\n", running ? "" : " (interrupted)");
    printf("==============================================\n");
    
    return 0;
} 