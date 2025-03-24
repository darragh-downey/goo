/**
 * killer_features_demo.c
 * 
 * A demonstration of Goo's killer features: scoped allocation,
 * compile-time evaluation, meta-programming, reflection, and messaging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#include "../src/include/goo_runtime.h"

// Example struct for reflection
typedef struct {
    int id;
    const char* name;
    double value;
} Person;

// Thread function for publisher
void* publisher_thread(void* arg) {
    GooChannel* publisher = (GooChannel*)arg;
    
    for (int i = 0; i < 5; i++) {
        char buffer[64];
        sprintf(buffer, "Message %d", i);
        
        GooMessage* msg = goo_message_create_string(buffer);
        if (!msg) {
            printf("Failed to create message\n");
            continue;
        }
        
        printf("Publishing: %s\n", buffer);
        
        if (!goo_channel_send(publisher, msg)) {
            printf("Failed to publish message\n");
            goo_message_destroy(msg);
        }
        
        usleep(500000); // 0.5 second delay
    }
    
    return NULL;
}

// Thread function for subscriber
void* subscriber_thread(void* arg) {
    GooChannel* subscriber = (GooChannel*)arg;
    
    for (int i = 0; i < 5; i++) {
        GooMessage* msg = goo_channel_receive(subscriber);
        if (!msg) {
            printf("Failed to receive message\n");
            continue;
        }
        
        const char* data = goo_message_get_string(msg);
        printf("Subscriber received: %s\n", data);
        
        goo_message_destroy(msg);
    }
    
    return NULL;
}

// Helper function to demonstrate scope-based allocation
bool demonstrate_scoped_allocation(GooScopedAllocator* scope, void* data) {
    printf("\n--- Demonstrating Scope-Based Allocation ---\n");
    
    // Allocate some memory in the scope
    char* message = goo_scoped_alloc_malloc(scope, 100);
    if (!message) {
        printf("Failed to allocate memory\n");
        return false;
    }
    
    strcpy(message, "This memory will be automatically freed when the scope ends");
    printf("Allocated message: %s\n", message);
    
    // Nested scope demonstration
    GOO_BEGIN_SCOPE() {
        char* nested_message = goo_scoped_alloc_malloc(goo_get_global_scoped_allocator(), 100);
        if (nested_message) {
            strcpy(nested_message, "This memory is in a nested scope and will be freed first");
            printf("Nested message: %s\n", nested_message);
        }
    } GOO_END_SCOPE();
    
    printf("Nested scope has ended, its memory has been freed\n");
    printf("Original scope's memory is still valid: %s\n", message);
    
    // Memory will be freed when demonstrate_scoped_allocation returns
    return true;
}

// Function to demonstrate compile-time evaluation
void demonstrate_compile_time_evaluation() {
    printf("\n--- Demonstrating Compile-Time Evaluation ---\n");
    
    // Create a compile-time context
    GooComptimeContext* ctx = goo_comptime_context_create();
    if (!ctx) {
        printf("Failed to create compile-time context\n");
        return;
    }
    
    // Create some compile-time values
    GooComptimeValue* int_val = goo_comptime_create_int(ctx, 42);
    GooComptimeValue* float_val = goo_comptime_create_float(ctx, 3.14159);
    GooComptimeValue* bool_val = goo_comptime_create_bool(ctx, true);
    GooComptimeValue* str_val = goo_comptime_create_string(ctx, "Hello, Goo!");
    
    // Convert values to strings and print
    int64_t int_value;
    double float_value;
    bool bool_value;
    const char* str_value;
    
    if (goo_comptime_get_int(ctx, int_val, &int_value)) {
        printf("Integer value: %lld\n", (long long)int_value);
    }
    
    if (goo_comptime_get_float(ctx, float_val, &float_value)) {
        printf("Float value: %f\n", float_value);
    }
    
    if (goo_comptime_get_bool(ctx, bool_val, &bool_value)) {
        printf("Boolean value: %s\n", bool_value ? "true" : "false");
    }
    
    if (goo_comptime_get_string(ctx, str_val, &str_value)) {
        printf("String value: %s\n", str_value);
    }
    
    // Perform compile-time arithmetic
    GooComptimeValue* result_add = goo_comptime_eval_add(ctx, int_val, int_val);
    GooComptimeValue* result_mul = goo_comptime_eval_mul(ctx, int_val, float_val);
    
    if (result_add && goo_comptime_get_int(ctx, result_add, &int_value)) {
        printf("42 + 42 = %lld\n", (long long)int_value);
    }
    
    if (result_mul && goo_comptime_get_float(ctx, result_mul, &float_value)) {
        printf("42 * 3.14159 = %f\n", float_value);
    }
    
    // Destroy compile-time values
    goo_comptime_destroy_value(ctx, int_val);
    goo_comptime_destroy_value(ctx, float_val);
    goo_comptime_destroy_value(ctx, bool_val);
    goo_comptime_destroy_value(ctx, str_val);
    
    if (result_add) goo_comptime_destroy_value(ctx, result_add);
    if (result_mul) goo_comptime_destroy_value(ctx, result_mul);
    
    // Destroy the context
    goo_comptime_context_destroy(ctx);
}

// Function to demonstrate reflection and meta-programming
void demonstrate_reflection() {
    printf("\n--- Demonstrating Reflection and Meta-Programming ---\n");
    
    // Create a reflection context
    GooReflectionContext* ctx = goo_reflection_context_create();
    if (!ctx) {
        printf("Failed to create reflection context\n");
        return;
    }
    
    // Register some basic types
    printf("Registering basic types...\n");
    
    GooTypeInfo* bool_type = goo_reflection_register_basic_type(ctx, GOO_TYPE_BOOL, "bool", sizeof(bool));
    GooTypeInfo* int_type = goo_reflection_register_basic_type(ctx, GOO_TYPE_I32, "int", sizeof(int));
    GooTypeInfo* float_type = goo_reflection_register_basic_type(ctx, GOO_TYPE_F64, "double", sizeof(double));
    GooTypeInfo* string_type = goo_reflection_register_basic_type(ctx, GOO_TYPE_STRING, "string", sizeof(char*));
    
    printf("  Registered bool type: %p\n", (void*)bool_type);
    printf("  Registered int type: %p\n", (void*)int_type);
    printf("  Registered float type: %p\n", (void*)float_type);
    printf("  Registered string type: %p\n", (void*)string_type);
    
    // Register a struct type
    GooTypeInfo* person_type = goo_reflection_register_struct(ctx, "Person", sizeof(Person));
    if (person_type) {
        printf("  Registered Person struct: %p\n", (void*)person_type);
        
        // Add fields to the struct
        GooFieldInfo* id_field = goo_reflection_add_field(ctx, person_type, "id", int_type, offsetof(Person, id));
        GooFieldInfo* name_field = goo_reflection_add_field(ctx, person_type, "name", string_type, offsetof(Person, name));
        GooFieldInfo* value_field = goo_reflection_add_field(ctx, person_type, "value", float_type, offsetof(Person, value));
        
        printf("  Added fields: id(%p), name(%p), value(%p)\n",
            (void*)id_field, (void*)name_field, (void*)value_field);
    }
    
    // Create values
    printf("\nCreating values...\n");
    
    GooValue* bool_val = goo_reflection_create_value(ctx, bool_type);
    GooValue* int_val = goo_reflection_create_value(ctx, int_type);
    GooValue* float_val = goo_reflection_create_value(ctx, float_type);
    GooValue* string_val = goo_reflection_create_value(ctx, string_type);
    GooValue* person_val = goo_reflection_create_value(ctx, person_type);
    
    printf("  Created bool value: %p\n", (void*)bool_val);
    printf("  Created int value: %p\n", (void*)int_val);
    printf("  Created float value: %p\n", (void*)float_val);
    printf("  Created string value: %p\n", (void*)string_val);
    printf("  Created person value: %p\n", (void*)person_val);
    
    // Set values
    if (bool_val) {
        goo_reflection_set_bool(ctx, bool_val, true);
    }
    
    if (int_val) {
        goo_reflection_set_int(ctx, int_val, 42);
    }
    
    if (float_val) {
        goo_reflection_set_float(ctx, float_val, 3.14159);
    }
    
    if (string_val) {
        goo_reflection_set_string(ctx, string_val, "Hello, Reflection!");
    }
    
    if (person_val) {
        // Set field values
        GooValue* id_val = goo_reflection_create_value(ctx, int_type);
        GooValue* name_val = goo_reflection_create_value(ctx, string_type);
        GooValue* value_val = goo_reflection_create_value(ctx, float_type);
        
        if (id_val && name_val && value_val) {
            goo_reflection_set_int(ctx, id_val, 1);
            goo_reflection_set_string(ctx, name_val, "John Doe");
            goo_reflection_set_float(ctx, value_val, 42.0);
            
            goo_reflection_set_field(ctx, person_val, "id", id_val);
            goo_reflection_set_field(ctx, person_val, "name", name_val);
            goo_reflection_set_field(ctx, person_val, "value", value_val);
        }
        
        // Clean up
        if (id_val) goo_reflection_destroy_value(ctx, id_val);
        if (name_val) goo_reflection_destroy_value(ctx, name_val);
        if (value_val) goo_reflection_destroy_value(ctx, value_val);
    }
    
    // Get and display values
    if (bool_val) {
        bool value;
        if (goo_reflection_get_bool(ctx, bool_val, &value)) {
            printf("  Boolean value: %s\n", value ? "true" : "false");
        }
    }
    
    if (int_val) {
        int64_t value;
        if (goo_reflection_get_int(ctx, int_val, &value)) {
            printf("  Integer value: %lld\n", (long long)value);
        }
    }
    
    if (float_val) {
        double value;
        if (goo_reflection_get_float(ctx, float_val, &value)) {
            printf("  Float value: %f\n", value);
        }
    }
    
    if (string_val) {
        const char* value;
        if (goo_reflection_get_string(ctx, string_val, &value)) {
            printf("  String value: %s\n", value);
        }
    }
    
    if (person_val) {
        GooValue* field_val;
        
        // Get and display id field
        if (goo_reflection_get_field_value(ctx, person_val, "id", &field_val) && field_val) {
            int64_t id;
            if (goo_reflection_get_int(ctx, field_val, &id)) {
                printf("  Person.id: %lld\n", (long long)id);
            }
        }
        
        // Get and display name field
        if (goo_reflection_get_field_value(ctx, person_val, "name", &field_val) && field_val) {
            const char* name;
            if (goo_reflection_get_string(ctx, field_val, &name)) {
                printf("  Person.name: %s\n", name);
            }
        }
        
        // Get and display value field
        if (goo_reflection_get_field_value(ctx, person_val, "value", &field_val) && field_val) {
            double value;
            if (goo_reflection_get_float(ctx, field_val, &value)) {
                printf("  Person.value: %f\n", value);
            }
        }
    }
    
    // Clean up
    if (bool_val) goo_reflection_destroy_value(ctx, bool_val);
    if (int_val) goo_reflection_destroy_value(ctx, int_val);
    if (float_val) goo_reflection_destroy_value(ctx, float_val);
    if (string_val) goo_reflection_destroy_value(ctx, string_val);
    if (person_val) goo_reflection_destroy_value(ctx, person_val);
    
    goo_reflection_context_destroy(ctx);
}

// Function to demonstrate messaging
void demonstrate_messaging() {
    printf("\n--- Demonstrating First-Class Messaging System ---\n");
    
    // Create a channel
    GooChannel* channel = goo_channel_create(GOO_CHANNEL_TYPE_NORMAL);
    if (!channel) {
        printf("Failed to create channel\n");
        return;
    }
    
    // Create publisher thread
    pthread_t pub_thread;
    if (pthread_create(&pub_thread, NULL, publisher_thread, channel) != 0) {
        printf("Failed to create publisher thread\n");
        goo_channel_destroy(channel);
        return;
    }
    
    // Create subscriber thread
    pthread_t sub_thread;
    if (pthread_create(&sub_thread, NULL, subscriber_thread, channel) != 0) {
        printf("Failed to create subscriber thread\n");
        pthread_cancel(pub_thread);
        pthread_join(pub_thread, NULL);
        goo_channel_destroy(channel);
        return;
    }
    
    // Wait for threads to finish
    pthread_join(pub_thread, NULL);
    pthread_join(sub_thread, NULL);
    
    // Demonstrate pub-sub pattern
    printf("\nDemonstrating Publish-Subscribe Pattern\n");
    
    GooChannel* publisher = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    GooChannel* subscriber1 = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    GooChannel* subscriber2 = goo_channel_create(GOO_CHANNEL_TYPE_PUBSUB);
    
    if (!publisher || !subscriber1 || !subscriber2) {
        printf("Failed to create pub-sub channels\n");
        if (publisher) goo_channel_destroy(publisher);
        if (subscriber1) goo_channel_destroy(subscriber1);
        if (subscriber2) goo_channel_destroy(subscriber2);
        return;
    }
    
    // Subscribe to topics
    goo_channel_subscribe(subscriber1, "topic1");
    goo_channel_subscribe(subscriber2, "topic2");
    
    // Add subscribers to publisher
    goo_channel_add_subscriber(publisher, subscriber1);
    goo_channel_add_subscriber(publisher, subscriber2);
    
    // Publish messages
    GooMessage* msg1 = goo_message_create_string("Message for topic1");
    GooMessage* msg2 = goo_message_create_string("Message for topic2");
    
    if (msg1 && msg2) {
        printf("Publishing to topic1: %s\n", goo_message_get_string(msg1));
        goo_channel_publish(publisher, "topic1", msg1);
        
        printf("Publishing to topic2: %s\n", goo_message_get_string(msg2));
        goo_channel_publish(publisher, "topic2", msg2);
        
        // Receive from subscribers
        char* topic1 = NULL;
        GooMessage* recv1 = goo_channel_receive_with_topic(subscriber1, &topic1);
        if (recv1) {
            printf("Subscriber1 received on topic '%s': %s\n", 
                topic1 ? topic1 : "none", goo_message_get_string(recv1));
            goo_message_destroy(recv1);
        }
        
        char* topic2 = NULL;
        GooMessage* recv2 = goo_channel_receive_with_topic(subscriber2, &topic2);
        if (recv2) {
            printf("Subscriber2 received on topic '%s': %s\n", 
                topic2 ? topic2 : "none", goo_message_get_string(recv2));
            goo_message_destroy(recv2);
        }
    }
    
    // Clean up
    goo_channel_destroy(publisher);
    goo_channel_destroy(subscriber1);
    goo_channel_destroy(subscriber2);
    
    // Clean up original channel
    goo_channel_destroy(channel);
}

// Function to demonstrate parallel execution
void demonstrate_parallel_execution() {
    printf("\n--- Demonstrating Parallel Execution ---\n");
    
    // Create thread pool
    GooThreadPool* pool = goo_thread_pool_create(4);
    if (!pool) {
        printf("Failed to create thread pool\n");
        return;
    }
    
    printf("Created thread pool with %zu threads\n", goo_thread_pool_get_num_threads(pool));
    
    // Define parallel for function
    GooParallelForFunc process_item = [](size_t index, void* context) {
        int* sum = (int*)context;
        *sum += index;
        printf("Processing item %zu\n", index);
        usleep(100000); // Sleep 100ms to simulate work
    };
    
    // Create parallel for
    int sum = 0;
    GooParallelFor* parallel_for = goo_parallel_for_create(pool, 0, 10, process_item, &sum);
    if (!parallel_for) {
        printf("Failed to create parallel for\n");
        goo_thread_pool_destroy(pool);
        return;
    }
    
    // Execute parallel for
    printf("Executing parallel for...\n");
    if (!goo_parallel_for_execute(parallel_for)) {
        printf("Failed to execute parallel for\n");
    }
    
    // Wait for completion
    if (!goo_parallel_for_wait(parallel_for)) {
        printf("Failed to wait for parallel for\n");
    }
    
    printf("Parallel for complete, sum: %d\n", sum);
    
    // Define parallel reduce functions
    GooParallelForFunc map_func = [](size_t index, void* context) {
        printf("Mapping item %zu\n", index);
        int* result = malloc(sizeof(int));
        *result = index;
        return result;
    };
    
    GooParallelReduceFunc reduce_func = [](void* a, void* b, void* context) {
        int* result_a = (int*)a;
        int* result_b = (int*)b;
        int* result = malloc(sizeof(int));
        *result = *result_a + *result_b;
        printf("Reducing %d + %d = %d\n", *result_a, *result_b, *result);
        free(result_a);
        free(result_b);
        return result;
    };
    
    // Create parallel reduce
    int* init = malloc(sizeof(int));
    *init = 0;
    
    GooParallelReduce* parallel_reduce = goo_parallel_reduce_create(
        pool, 0, 10, init, map_func, reduce_func, NULL
    );
    
    if (!parallel_reduce) {
        printf("Failed to create parallel reduce\n");
        free(init);
        goo_parallel_for_destroy(parallel_for);
        goo_thread_pool_destroy(pool);
        return;
    }
    
    // Execute parallel reduce
    printf("Executing parallel reduce...\n");
    if (!goo_parallel_reduce_execute(parallel_reduce)) {
        printf("Failed to execute parallel reduce\n");
    }
    
    // Wait for completion and get result
    void* reduce_result = NULL;
    if (!goo_parallel_reduce_wait(parallel_reduce, &reduce_result)) {
        printf("Failed to wait for parallel reduce\n");
    } else if (reduce_result) {
        printf("Parallel reduce complete, sum: %d\n", *(int*)reduce_result);
        free(reduce_result);
    }
    
    // Clean up
    goo_parallel_for_destroy(parallel_for);
    goo_parallel_reduce_destroy(parallel_reduce);
    goo_thread_pool_destroy(pool);
}

int main() {
    printf("Goo Killer Features Demo\n");
    printf("=======================\n");
    
    // Initialize the Goo runtime
    if (!goo_runtime_init()) {
        printf("Failed to initialize Goo runtime\n");
        return 1;
    }
    
    printf("Goo Runtime Version: %s\n", goo_runtime_version());
    
    // Demonstrate scope-based allocation
    GooScopedAllocator* scope = goo_scoped_alloc_create();
    if (!scope) {
        printf("Failed to create scoped allocator\n");
        goo_runtime_cleanup();
        return 1;
    }
    
    goo_scoped_alloc_with_scope(scope, (GooScopedFunc)demonstrate_scoped_allocation, NULL);
    
    // Demonstrate compile-time evaluation
    demonstrate_compile_time_evaluation();
    
    // Demonstrate reflection
    demonstrate_reflection();
    
    // Demonstrate messaging
    demonstrate_messaging();
    
    // Demonstrate parallel execution
    demonstrate_parallel_execution();
    
    // Clean up
    goo_scoped_alloc_destroy(scope);
    goo_runtime_cleanup();
    
    printf("\nDemo completed successfully\n");
    return 0;
} 