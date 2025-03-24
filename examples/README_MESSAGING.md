# Goo Distributed Messaging Demo

This directory contains the implementation of a distributed messaging demo for Goo, showcasing the messaging capabilities of the Goo programming language through a C API.

## Phase 1 (Completed)

Phase 1 implements a mock version of the distributed messaging system to demonstrate the various messaging patterns that Goo will support:

- **Request/Reply**: A client sends a request to a server and waits for a reply
- **Publish/Subscribe**: A publisher broadcasts messages to multiple subscribers
- **Push/Pull**: A work distributor sends tasks to multiple workers for load balancing
- **IPC (Inter-Process Communication)**: Communication between processes on the same machine

### Implementation Details

The demo is designed with a flexible architecture:

1. **Mock Implementation**
   - Uses a preprocessor directive `MOCK_IMPLEMENTATION` to toggle between mock and real implementations
   - Provides mock structures and functions for testing and demonstration
   - Simulates network communication without actually requiring network setup

2. **Thread-based Testing**
   - Uses pthreads to simulate multiple components communicating with each other
   - Server and client threads demonstrate bidirectional communication

3. **Robust Error Handling**
   - Implements timeouts to prevent indefinite blocking
   - Provides graceful termination with signal handling
   - Handles edge cases like connection failures

4. **Zig Build Integration**
   - Integrated with the Zig build system through `build.zig`
   - Can be built and run with `zig build run-transport-demo`

## Phase 2 (Future Work)

In Phase 2, the mock implementation will be replaced with the actual Zig implementation, which will:

1. Implement real network transport layers:
   - TCP/IP for network communication
   - Unix domain sockets for IPC
   - In-process communication for high-performance messaging

2. Add more message patterns and options:
   - Survey/Respond for one-to-many request/reply patterns
   - Reliable delivery guarantees
   - Message priorities
   - Zero-copy messaging for performance

3. Improve the C API:
   - Better error handling
   - Asynchronous communication options
   - Zero-configuration discovery of services

## Usage

To build and run the demo:

```bash
zig build run-transport-demo
```

To modify the demo for your own experiments:

1. Examine `distributed_messaging_demo.c` to understand the API and patterns
2. Set `MOCK_IMPLEMENTATION` to 0 when the real implementation is ready
3. Create your own demo by copying and modifying the existing patterns

## API Documentation

### Channel Types

- `Normal`: Bidirectional channel for general-purpose messaging
- `Pub`: Publisher channel for one-to-many broadcasts
- `Sub`: Subscriber channel for receiving broadcasts
- `Push`: Push channel for distributing work to multiple receivers
- `Pull`: Pull channel for receiving work items
- `Req`: Request channel for sending requests and receiving replies
- `Rep`: Reply channel for processing requests and sending responses

### Channel Functions

- `goo_channel_create()`: Create a new channel of a specific type
- `goo_channel_destroy()`: Destroy a channel and free its resources
- `goo_channel_close()`: Close a channel but keep its instance
- `goo_channel_connect()`: Connect a channel to a remote endpoint
- `goo_channel_set_endpoint()`: Set up a channel to receive connections
- `goo_channel_send()`: Send data through a channel
- `goo_channel_recv()`: Receive data from a channel

### Message Functions

- `goo_message_create()`: Create a new message
- `goo_message_destroy()`: Destroy a message and free its resources 