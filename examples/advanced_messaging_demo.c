#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "goo_runtime.h"
#include "goo_channels.h"
#include "goo_channels_advanced.h"
#include "goo_supervision.h"

// Structure for our example message
typedef struct {
    int id;
    char text[128];
    double value;
} Message;

// Worker function for the push/pull pattern
void worker_function(void* arg) {
    int worker_id = *((int*)arg);
    GooAdvancedChannel* pull_channel = goo_pull_channel_create(sizeof(Message), 10);
    
    printf("Worker %d: Started\n", worker_id);
    
    // Connect to the push socket
    goo_advanced_channel_connect(pull_channel, GOO_PROTO_INPROC, "work_queue", 0);
    
    // Process tasks
    while (1) {
        Message msg;
        if (goo_channel_pull(pull_channel, &msg, sizeof(msg), 0)) {
            printf("Worker %d: Received task %d - %s (%.2f)\n", 
                   worker_id, msg.id, msg.text, msg.value);
            
            // Simulate work
            usleep(100000 + (rand() % 900000)); // 0.1-1 second
        }
    }
    
    // This is never reached in this demo, but good practice
    goo_advanced_channel_destroy(pull_channel);
}

// Publisher function
void publisher_function(void* arg) {
    GooAdvancedChannel* pub_channel = goo_pub_channel_create(sizeof(Message), 10);
    
    printf("Publisher: Started\n");
    
    // Bind the publisher socket
    goo_advanced_channel_bind(pub_channel, GOO_PROTO_INPROC, "weather_updates", 0);
    
    // Publish messages
    int count = 0;
    while (count < 10) {
        // Create different weather updates
        const char* weather_types[] = {"sunny", "cloudy", "rainy", "snowy", "windy"};
        const char* cities[] = {"New York", "London", "Tokyo", "Paris", "Sydney"};
        
        for (int i = 0; i < 5; i++) {
            Message msg = {
                .id = count * 5 + i,
                .value = 10.0 + (rand() % 300) / 10.0
            };
            
            // City-specific weather
            snprintf(msg.text, sizeof(msg.text), "%s: %s", 
                     cities[i], weather_types[rand() % 5]);
            
            // Publish to different city topics
            printf("Publisher: Publishing %s (%.1f°C)\n", msg.text, msg.value);
            goo_channel_publish(pub_channel, cities[i], &msg, sizeof(msg), 0);
            
            usleep(500000); // 0.5 second between updates
        }
        
        count++;
    }
    
    printf("Publisher: Finished\n");
    goo_advanced_channel_destroy(pub_channel);
}

// Subscriber function
void subscriber_function(void* arg) {
    const char* city = (const char*)arg;
    GooAdvancedChannel* sub_channel = goo_sub_channel_create(sizeof(Message), 10);
    
    printf("Subscriber for %s: Started\n", city);
    
    // Connect to the publisher
    goo_advanced_channel_connect(sub_channel, GOO_PROTO_INPROC, "weather_updates", 0);
    
    // Subscribe to the specific city
    goo_channel_subscribe(sub_channel, city);
    
    // Receive updates
    int received = 0;
    while (received < 10) {
        Message msg;
        if (goo_channel_pull(sub_channel, &msg, sizeof(msg), 0)) {
            printf("Subscriber for %s: Received update: %s (%.1f°C)\n", 
                   city, msg.text, msg.value);
            received++;
        }
        
        usleep(100000); // Check every 0.1 second
    }
    
    printf("Subscriber for %s: Finished\n", city);
    goo_advanced_channel_destroy(sub_channel);
}

// Server function for request/reply pattern
void server_function(void* arg) {
    GooAdvancedChannel* rep_channel = goo_rep_channel_create(sizeof(Message), 10);
    
    printf("Server: Started\n");
    
    // Bind the reply socket
    goo_advanced_channel_bind(rep_channel, GOO_PROTO_INPROC, "service", 0);
    
    // Process requests
    int handled = 0;
    while (handled < 15) { // Handle 15 requests then exit
        Message request;
        size_t request_size = sizeof(request);
        
        // Receive a request
        if (goo_channel_reply(rep_channel, &request, &request_size, NULL, 0, GOO_MSG_MORE)) {
            printf("Server: Received request %d - %s\n", request.id, request.text);
            
            // Prepare response
            Message response = {
                .id = request.id,
                .value = request.value * 2.0 // Double the value
            };
            snprintf(response.text, sizeof(response.text), "Response to: %s", request.text);
            
            // Send response
            goo_channel_reply(rep_channel, NULL, NULL, &response, sizeof(response), 0);
            printf("Server: Sent response for request %d\n", request.id);
            
            handled++;
        }
        
        usleep(100000); // Check every 0.1 second
    }
    
    printf("Server: Finished\n");
    goo_advanced_channel_destroy(rep_channel);
}

// Client function for request/reply pattern
void client_function(void* arg) {
    int client_id = *((int*)arg);
    GooAdvancedChannel* req_channel = goo_req_channel_create(sizeof(Message), 10);
    
    printf("Client %d: Started\n", client_id);
    
    // Connect to the server
    goo_advanced_channel_connect(req_channel, GOO_PROTO_INPROC, "service", 0);
    
    // Send requests
    for (int i = 0; i < 5; i++) {
        // Prepare request
        Message request = {
            .id = client_id * 100 + i,
            .value = (double)(client_id * 10 + i)
        };
        snprintf(request.text, sizeof(request.text), "Request %d from client %d", i, client_id);
        
        // Send request and get response
        Message response;
        size_t response_size = sizeof(response);
        
        printf("Client %d: Sending request: %s\n", client_id, request.text);
        if (goo_channel_request(req_channel, &request, sizeof(request), 
                               &response, &response_size, 0)) {
            printf("Client %d: Received response: %s (%.1f)\n", 
                   client_id, response.text, response.value);
        } else {
            printf("Client %d: Request failed\n", client_id);
        }
        
        usleep(500000 + (rand() % 500000)); // 0.5-1 second between requests
    }
    
    printf("Client %d: Finished\n", client_id);
    goo_advanced_channel_destroy(req_channel);
}

// Demo of the pub/sub pattern
void run_pubsub_demo() {
    printf("\n=== Starting Publish-Subscribe Demo ===\n\n");
    
    // Initialize cities to subscribe to
    const char* cities[] = {"New York", "London", "Tokyo"};
    
    // Create the publisher thread
    pthread_t pub_thread;
    pthread_create(&pub_thread, NULL, (void*(*)(void*))publisher_function, NULL);
    
    // Create subscriber threads
    pthread_t sub_threads[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&sub_threads[i], NULL, (void*(*)(void*))subscriber_function, (void*)cities[i]);
    }
    
    // Wait for all threads to finish
    pthread_join(pub_thread, NULL);
    for (int i = 0; i < 3; i++) {
        pthread_join(sub_threads[i], NULL);
    }
    
    printf("\n=== Publish-Subscribe Demo Completed ===\n");
}

// Demo of the push/pull pattern
void run_pushpull_demo() {
    printf("\n=== Starting Push-Pull Demo ===\n\n");
    
    // Create the push channel
    GooAdvancedChannel* push_channel = goo_push_channel_create(sizeof(Message), 10);
    goo_advanced_channel_bind(push_channel, GOO_PROTO_INPROC, "work_queue", 0);
    
    // Create worker IDs
    int worker_ids[3] = {1, 2, 3};
    
    // Create worker threads
    pthread_t worker_threads[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&worker_threads[i], NULL, (void*(*)(void*))worker_function, &worker_ids[i]);
    }
    
    // Give workers time to start
    usleep(500000);
    
    // Push tasks
    printf("Distributor: Started pushing tasks\n");
    for (int i = 0; i < 10; i++) {
        Message task = {
            .id = i,
            .value = (double)(i * 10)
        };
        snprintf(task.text, sizeof(task.text), "Task %d", i);
        
        printf("Distributor: Pushing %s\n", task.text);
        goo_channel_push(push_channel, &task, sizeof(task), 0);
        
        usleep(300000); // 0.3 seconds between tasks
    }
    
    printf("Distributor: Finished pushing tasks\n");
    
    // Wait a bit for workers to process the tasks
    sleep(5);
    
    // Clean up
    goo_advanced_channel_destroy(push_channel);
    
    // We're not joining worker threads since they run indefinitely in this demo
    printf("\n=== Push-Pull Demo Completed ===\n");
}

// Demo of the request/reply pattern
void run_reqrep_demo() {
    printf("\n=== Starting Request-Reply Demo ===\n\n");
    
    // Create server thread
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, (void*(*)(void*))server_function, NULL);
    
    // Give server time to start
    usleep(500000);
    
    // Create client IDs
    int client_ids[3] = {1, 2, 3};
    
    // Create client threads
    pthread_t client_threads[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&client_threads[i], NULL, (void*(*)(void*))client_function, &client_ids[i]);
    }
    
    // Wait for all threads to finish
    for (int i = 0; i < 3; i++) {
        pthread_join(client_threads[i], NULL);
    }
    pthread_join(server_thread, NULL);
    
    printf("\n=== Request-Reply Demo Completed ===\n");
}

int main() {
    // Seed random number generator
    srand(time(NULL));
    
    // Initialize the runtime
    goo_runtime_init(2); // Log level 2
    goo_thread_pool_init(8); // 8 worker threads
    
    printf("Advanced Messaging Patterns Demo\n");
    printf("================================\n\n");
    
    // Run the demos
    run_pubsub_demo();
    run_pushpull_demo();
    run_reqrep_demo();
    
    // Cleanup
    goo_thread_pool_cleanup();
    goo_runtime_shutdown();
    
    printf("\nAll demos completed successfully!\n");
    return 0;
} 