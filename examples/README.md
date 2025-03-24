# Goo Programming Language Examples

This directory contains example programs that demonstrate the features and usage of the Goo programming language.

## Example Categories

The examples are organized by difficulty level and feature coverage:

- **Beginner**: Simple examples for those new to Goo
- **Intermediate**: More complex examples demonstrating common programming patterns
- **Advanced**: Sophisticated examples showcasing advanced language features

## Directory Structure

- `beginner/` - Introductory examples
  - Basic syntax
  - Simple data structures
  - Control flow
  - Functions and procedures
- `intermediate/` - More complex patterns
  - Error handling
  - Concurrency basics
  - Standard library usage
  - Package management
- `advanced/` - Advanced language features
  - Memory management techniques
  - Advanced concurrency
  - Metaprogramming
  - Compiler extensions
  - FFI (Foreign Function Interface)

## Running Examples

To build and run an example:

```bash
goo build examples/beginner/hello_world.goo
./hello_world
```

For interactive examples:

```bash
goo run examples/intermediate/interactive_demo.goo
```

## Example Guidelines

Each example file should:

1. Begin with a comment explaining what it demonstrates
2. Be well-commented throughout to explain key concepts
3. Follow best practices for Goo programming
4. Include expected output where appropriate
5. Demonstrate error handling when relevant

## Example Template

```goo
// example_name.goo
//
// This example demonstrates [feature/concept].
// Expected output:
// > Sample output line 1
// > Sample output line 2

import std.io

func main() {
    // Example code with comments explaining important concepts
    
    // Feature demonstration
    let value = example_function()
    
    // Output results
    io.println("Result: {}", value)
}

// Supporting functions with clear documentation
func example_function() -> int {
    return 42
}

# Goo Messaging System Demos

This directory contains a set of demonstration programs for the Goo messaging system. These examples are implemented in C and showcase the fundamental messaging patterns supported by Goo's messaging subsystem.

## Overview

Goo's messaging system is inspired by ZeroMQ and provides flexible message-passing primitives. The demos showcase the following messaging patterns:

1. **Simple Messaging** - Basic send/receive messaging between threads
2. **Publish-Subscribe** - Topic-based publisher/subscriber pattern 
3. **Request-Reply** - Synchronous request/response pattern
4. **Push-Pull** - Load distribution (worker pool) pattern

## Demo Programs

### Simple Messaging Demo (`simple_messaging_demo.c`)

Demonstrates basic messaging between threads using channels and messages. This example shows how to:
- Create message channels
- Send messages from one thread to another
- Receive messages with or without waiting
- Implement thread-safe messaging

### Publish-Subscribe Demo (`pubsub_demo.c`)

Demonstrates the publish-subscribe messaging pattern where:
- Publishers produce messages on specific topics
- Subscribers register interest in specific topics
- Messages are routed based on topic matching
- Multiple subscribers can receive the same message

### Request-Reply Demo (`request_reply_demo.c`)

Demonstrates the request-reply pattern where:
- A client sends requests and waits for responses
- A server processes requests and sends back replies  
- Requests and replies are matched using unique request IDs
- The pattern is synchronous from the client's perspective

### Push-Pull Demo (`push_pull_demo.c`) 

Demonstrates the push-pull (worker distribution) pattern where:
- A producer pushes tasks to a shared queue
- Multiple workers pull tasks from the queue
- Tasks are distributed among available workers
- Each task is processed by exactly one worker

## Building and Running

To build all demos:

```bash
gcc -o simple_messaging_demo simple_messaging_demo.c -pthread
gcc -o pubsub_demo pubsub_demo.c -pthread
gcc -o request_reply_demo request_reply_demo.c -pthread
gcc -o push_pull_demo push_pull_demo.c -pthread
```

To run a specific demo:

```bash
./simple_messaging_demo
./pubsub_demo
./request_reply_demo
./push_pull_demo
```

## Implementation Notes

These demos use a simplified implementation in C that focuses on the core messaging concepts, rather than the complete Goo implementation in Zig. Key features demonstrated include:

- Thread-safe message passing using mutexes and condition variables
- Various message types (string, int, binary, task, request, reply)
- Topic-based routing for publish-subscribe
- Request ID tracking for request-reply
- Worker distribution for push-pull
- Graceful channel closing

The actual Goo implementation expands on these concepts with additional features such as:
- Network communication
- More sophisticated buffer management
- Timeouts and error handling
- Additional message types
