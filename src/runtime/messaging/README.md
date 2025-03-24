# Goo Messaging System

This module provides a ZeroMQ-inspired channel system for the Goo programming language. It extends Go's simple channel concept with powerful messaging patterns for concurrent, parallel, and distributed communication.

## Features

- **Multiple Channel Types**: Various messaging patterns for different communication needs
- **Distributed Messaging**: Communication across threads, processes, and machines
- **Flexible Transport**: Support for in-process, IPC, TCP, UDP, and more
- **Pub/Sub Pattern**: Topic-based publish-subscribe messaging
- **Flow Control**: High water marks and back pressure mechanisms
- **Reliability Options**: Guaranteed delivery with automatic retries
- **Extensibility**: Modular design allowing custom transport protocols

## Channel Types

### Standard Channel

The basic Go-style channel with buffered or unbuffered operation:

```c
// Create a buffered channel of integers with capacity 10
GooChannel* ch = goo_chan(int, 10);

// Send a value
int value = 42;
goo_channel_send(ch, &value, sizeof(int), GOO_MSG_NONE);

// Receive a value
int received;
goo_channel_receive(ch, &received, sizeof(int), GOO_MSG_NONE);

// Close and destroy
goo_channel_close(ch);
goo_channel_destroy(ch);
```

### Pub/Sub Channel

For publishing messages to multiple subscribers based on topics:

```c
// Publisher
GooChannel* pub = goo_pub_chan(char*, 100);
goo_channel_bind(pub, GOO_PROTO_TCP, "*", 5555);

// Publish a message on topic "weather"
const char* message = "Sunny day";
goo_channel_publish(pub, "weather", message, strlen(message)+1, GOO_MSG_NONE);

// Subscriber
GooChannel* sub = goo_sub_chan(char*, 100);
goo_channel_connect(sub, GOO_PROTO_TCP, "localhost", 5555);

// Subscribe to "weather" topic
void on_message(GooMessage* msg, void* context) {
    printf("Received: %s\n", (char*)msg->data);
}
goo_channel_subscribe(sub, "weather", on_message, NULL);

// Manual receive instead of callback
char buffer[256];
goo_channel_receive(sub, buffer, sizeof(buffer), GOO_MSG_NONE);
```

### Push/Pull Channel

For distributing work among workers:

```c
// Distributor
GooChannel* push = goo_push_chan(Task, 100);
goo_channel_bind(push, GOO_PROTO_INPROC, "tasks", 0);

// Push tasks
Task task = {.id = 1, .data = "Process file"};
goo_channel_push(push, &task, sizeof(Task), GOO_MSG_NONE);

// Worker
GooChannel* pull = goo_pull_chan(Task, 100);
goo_channel_connect(pull, GOO_PROTO_INPROC, "tasks", 0);

// Pull task
Task received_task;
goo_channel_pull(pull, &received_task, sizeof(Task), GOO_MSG_NONE);
```

### Request/Reply Channel

For synchronous communication:

```c
// Client
GooChannel* req = goo_req_chan(char*, 10);
goo_channel_connect(req, GOO_PROTO_TCP, "localhost", 5556);

// Send request and get reply
const char* request = "Get data";
char reply[256];
size_t reply_size = sizeof(reply);
goo_channel_request(req, request, strlen(request)+1, reply, &reply_size, GOO_MSG_NONE);

// Server
GooChannel* rep = goo_rep_chan(char*, 10);
goo_channel_bind(rep, GOO_PROTO_TCP, "*", 5556);

// Receive request and send reply
char request_buffer[256];
size_t request_size = sizeof(request_buffer);
const char* response = "Here's your data";
goo_channel_reply(rep, request_buffer, &request_size, response, strlen(response)+1, GOO_MSG_NONE);
```

## Transport Protocols

The module supports various transport protocols:

- `GOO_PROTO_INPROC`: Fastest in-process communication
- `GOO_PROTO_IPC`: Inter-process communication on the same machine
- `GOO_PROTO_TCP`: TCP/IP networking for distributed systems
- `GOO_PROTO_UDP`: Unreliable but fast UDP communication
- `GOO_PROTO_PGM`: Reliable multicast with PGM protocol
- `GOO_PROTO_EPGM`: Encapsulated PGM
- `GOO_PROTO_VMCI`: Virtual Machine Communication Interface

## Advanced Features

### Multi-part Messages

Send and receive multi-part messages:

```c
// Create multi-part message
GooMessage* msg = goo_message_create("Header", 7, GOO_MSG_MORE);
goo_message_add_part(msg, "Body", 5, GOO_MSG_NONE);

// Send message
goo_channel_send_message(channel, msg);

// Receive message
GooMessage* received = goo_channel_receive_message(channel, GOO_MSG_NONE);
// Access first part
printf("Part 1: %s\n", (char*)received->data);
// Access second part
printf("Part 2: %s\n", (char*)goo_message_next_part(received)->data);

// Free the message
goo_message_destroy(received);
```

### Non-blocking Operations

Try to send or receive without blocking:

```c
// Non-blocking send
if (!goo_channel_try_send(channel, &value, sizeof(value), GOO_MSG_NONE)) {
    printf("Channel full, couldn't send\n");
}

// Non-blocking receive
if (!goo_channel_try_receive(channel, &value, sizeof(value), GOO_MSG_NONE)) {
    printf("Channel empty, nothing to receive\n");
}
```

### Channel Statistics

Track channel performance metrics:

```c
GooChannelStats stats = goo_channel_stats(channel);
printf("Messages sent: %llu\n", stats.messages_sent);
printf("Messages received: %llu\n", stats.messages_received);
printf("Current queue size: %llu\n", stats.current_queue_size);
printf("Dropped messages: %llu\n", stats.dropped_messages);
```

## Integration with Goo Language

In Goo code, these features are exposed through built-in syntax:

```go
// Standard channel
ch := make(chan int, 10)
ch <- 42
value := <-ch

// Publish channel
pub := make(pub chan string)
pub <- "Hello"

// Subscribe channel
sub := make(sub chan string)
msg := <-sub

// Distributed channel
remoteCh := make(chan int) @ "tcp://server:5555"
remoteCh <- 100

// Multiple patterns
reqCh := make(req chan string)
repCh := make(rep chan string)
pushCh := make(push chan Task)
pullCh := make(pull chan Task)
```

## Implementation Notes

- The messaging system is built on C23 and uses POSIX threads for concurrency
- For distributed messaging, it leverages standard network sockets
- The module is designed for both performance and flexibility
- Reliability and guaranteed delivery are optional features with associated overhead

## Future Enhancements

- Full encryption support for secure communications
- More advanced reliability protocols
- Additional transport protocols
- Load balancing features
- Message filtering capabilities
- Integration with external messaging systems

## Building and Testing

To build the messaging module and run tests:

```bash
zig build
zig build test-messaging
```

## Performance Considerations

- In-process channels are the fastest and should be preferred when possible
- For distributed communication, consider message size and frequency
- Use buffered channels for bursty traffic patterns
- The publish-subscribe pattern scales well for one-to-many communication
- Request-reply patterns add overhead due to their synchronous nature 