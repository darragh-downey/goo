const std = @import("std");
const net = std.net;
const Thread = std.Thread;
const Mutex = std.Thread.Mutex;
const Condition = std.Thread.Condition;
const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;

/// Message types supported by the messaging system
pub const MessageType = enum(u8) {
    None,
    Int,
    Float,
    Bool,
    String,
    Binary,
    Json,
    Custom,
};

/// Message priority levels
pub const MessagePriority = enum(u8) {
    Low,
    Normal,
    High,
    Critical,
};

/// Channel types for different messaging patterns
pub const ChannelType = enum(u8) {
    Normal, // Standard queue channel
    PubSub, // Publish-subscribe pattern
    PushPull, // Distributed task distribution
    ReqRep, // Request-reply pattern
};

/// Message representation
pub const Message = struct {
    allocator: Allocator,
    msg_type: MessageType,
    priority: MessagePriority,
    data: []u8,
    topic: ?[]u8 = null,
    reply_to: ?*Channel = null,
    is_string: bool = false,

    /// Create a new message with specified type and data
    pub fn create(allocator: Allocator, msg_type: MessageType, data: []const u8) !*Message {
        const msg = try allocator.create(Message);
        msg.* = Message{
            .allocator = allocator,
            .msg_type = msg_type,
            .priority = .Normal,
            .data = try allocator.dupe(u8, data),
            .is_string = false,
        };
        return msg;
    }

    /// Create a new integer message
    pub fn createInt(allocator: Allocator, value: i64) !*Message {
        const bytes = std.mem.asBytes(&value);
        return try create(allocator, .Int, bytes);
    }

    /// Create a new float message
    pub fn createFloat(allocator: Allocator, value: f64) !*Message {
        const bytes = std.mem.asBytes(&value);
        return try create(allocator, .Float, bytes);
    }

    /// Create a new boolean message
    pub fn createBool(allocator: Allocator, value: bool) !*Message {
        const bytes = std.mem.asBytes(&value);
        return try create(allocator, .Bool, bytes);
    }

    /// Create a new string message
    pub fn createString(allocator: Allocator, value: []const u8) !*Message {
        return try create(allocator, .String, value);
    }

    /// Create a new binary message
    pub fn createBinary(allocator: Allocator, data: []const u8) !*Message {
        return try create(allocator, .Binary, data);
    }

    /// Set the message topic
    pub fn setTopic(self: *Message, topic: []const u8) !void {
        if (self.topic) |old_topic| {
            self.allocator.free(old_topic);
        }
        self.topic = try self.allocator.dupe(u8, topic);
    }

    /// Set the reply channel
    pub fn setReplyTo(self: *Message, channel: *Channel) void {
        self.reply_to = channel;
    }

    /// Get message data as integer
    pub fn getInt(self: *const Message) ?i64 {
        if (self.msg_type != .Int or self.data.len != @sizeOf(i64)) return null;
        return std.mem.bytesToValue(i64, self.data[0..@sizeOf(i64)]);
    }

    /// Get message data as float
    pub fn getFloat(self: *const Message) ?f64 {
        if (self.msg_type != .Float or self.data.len != @sizeOf(f64)) return null;
        return std.mem.bytesToValue(f64, self.data[0..@sizeOf(f64)]);
    }

    /// Get message data as boolean
    pub fn getBool(self: *const Message) ?bool {
        if (self.msg_type != .Bool or self.data.len != @sizeOf(bool)) return null;
        return std.mem.bytesToValue(bool, self.data[0..@sizeOf(bool)]);
    }

    /// Get message data as string
    pub fn getString(self: *const Message) ?[]const u8 {
        if (self.msg_type != .String) return null;
        return self.data;
    }

    /// Get raw message data
    pub fn getData(self: *const Message) []const u8 {
        return self.data;
    }

    /// Destroy the message and free all resources
    pub fn destroy(self: *Message) void {
        self.allocator.free(self.data);
        if (self.topic) |topic| {
            self.allocator.free(topic);
        }
        self.allocator.destroy(self);
    }
};

/// Topic subscriber information
const Subscriber = struct {
    channel: *Channel,
    topics: ArrayList([]u8),

    fn init(allocator: Allocator, channel: *Channel) Subscriber {
        return Subscriber{
            .channel = channel,
            .topics = ArrayList([]u8).init(allocator),
        };
    }

    fn deinit(self: *Subscriber) void {
        for (self.topics.items) |topic| {
            self.topics.allocator.free(topic);
        }
        self.topics.deinit();
    }

    fn addTopic(self: *Subscriber, topic: []const u8) !void {
        const topic_copy = try self.topics.allocator.dupe(u8, topic);
        try self.topics.append(topic_copy);
    }

    fn isInterestedIn(self: *const Subscriber, topic: ?[]const u8) bool {
        if (topic == null) return false;

        for (self.topics.items) |t| {
            if (std.mem.eql(u8, t, topic.?)) {
                return true;
            }
        }
        return false;
    }
};

/// MessageQueue with prioritization
const MessageQueue = struct {
    allocator: Allocator,
    high_priority: ArrayList(*Message),
    normal_priority: ArrayList(*Message),
    low_priority: ArrayList(*Message),
    mutex: Mutex,
    cond: Condition,

    fn init(allocator: Allocator) MessageQueue {
        return MessageQueue{
            .allocator = allocator,
            .high_priority = ArrayList(*Message).init(allocator),
            .normal_priority = ArrayList(*Message).init(allocator),
            .low_priority = ArrayList(*Message).init(allocator),
            .mutex = .{},
            .cond = .{},
        };
    }

    fn deinit(self: *MessageQueue) void {
        self.high_priority.deinit();
        self.normal_priority.deinit();
        self.low_priority.deinit();
    }

    fn enqueue(self: *MessageQueue, msg: *Message) !void {
        self.mutex.lock();
        defer self.mutex.unlock();

        switch (msg.priority) {
            .Critical, .High => try self.high_priority.append(msg),
            .Normal => try self.normal_priority.append(msg),
            .Low => try self.low_priority.append(msg),
        }

        self.cond.signal();
    }

    fn dequeue(self: *MessageQueue) ?*Message {
        self.mutex.lock();
        defer self.mutex.unlock();

        return self.dequeueNoLock();
    }

    fn dequeueNoLock(self: *MessageQueue) ?*Message {
        // Check high priority queue first
        if (self.high_priority.items.len > 0) {
            return self.high_priority.orderedRemove(0);
        }

        // Then normal priority
        if (self.normal_priority.items.len > 0) {
            return self.normal_priority.orderedRemove(0);
        }

        // Finally low priority
        if (self.low_priority.items.len > 0) {
            return self.low_priority.orderedRemove(0);
        }

        return null;
    }

    fn waitAndDequeue(self: *MessageQueue, timeout_ms: ?u64) ?*Message {
        self.mutex.lock();
        defer self.mutex.unlock();

        // Fast path: check if any messages are available
        var msg = self.dequeueNoLock();
        if (msg != null) return msg;

        // Slow path: wait for a message to arrive
        if (timeout_ms) |timeout| {
            _ = self.cond.timedWait(&self.mutex, timeout * std.time.ns_per_ms) catch return null;
        } else {
            self.cond.wait(&self.mutex);
        }

        // Try again after waiting
        msg = self.dequeueNoLock();
        return msg;
    }

    fn count(self: *const MessageQueue) usize {
        self.mutex.lock();
        defer self.mutex.unlock();

        return self.high_priority.items.len +
            self.normal_priority.items.len +
            self.low_priority.items.len;
    }
};

/// A channel for message passing
pub const Channel = struct {
    allocator: Allocator,
    channel_type: ChannelType,
    queue: MessageQueue,
    subscribers: ArrayList(Subscriber),
    mutex: Mutex,
    closed: bool,
    buffer: std.fifo.LinearFifo(*Message, .Dynamic),
    not_empty: Condition,
    not_full: Condition,
    is_distributed: bool,
    socket: ?net.Stream = null,
    listener: ?net.Server = null,
    endpoint: ?[]u8 = null,
    thread: ?Thread = null,

    /// Create a new channel with specified type
    pub fn create(allocator: Allocator, channel_type: ChannelType) !*Channel {
        const channel = try allocator.create(Channel);
        channel.* = Channel{
            .allocator = allocator,
            .channel_type = channel_type,
            .queue = MessageQueue.init(allocator),
            .subscribers = ArrayList(Subscriber).init(allocator),
            .mutex = .{},
            .closed = false,
            .buffer = std.fifo.LinearFifo(*Message, .Dynamic).init(allocator),
            .not_empty = .{},
            .not_full = .{},
            .is_distributed = false,
        };

        try channel.buffer.ensureTotalCapacity(16);

        if (channel_type == .PubSub) {
            channel.subscribers = ArrayList(Subscriber).init(allocator);
        }

        return channel;
    }

    /// Destroy the channel and free resources
    pub fn destroy(self: *Channel) void {
        self.queue.deinit();

        for (self.subscribers.items) |*subscriber| {
            subscriber.deinit();
        }
        self.subscribers.deinit();

        self.allocator.destroy(self);
    }

    /// Send a message to the channel
    pub fn send(self: *Channel, msg: *Message) !void {
        if (self.channel_type == .PubSub) {
            // For PubSub, route to subscribers
            return self.publish(msg.topic, msg);
        } else {
            // For other types, just queue the message
            try self.queue.enqueue(msg);
        }
    }

    /// Receive a message from the channel
    pub fn receive(self: *Channel) ?*Message {
        return self.queue.dequeue();
    }

    /// Receive a message with timeout
    pub fn receiveTimeout(self: *Channel, timeout_ms: u64) ?*Message {
        return self.queue.waitAndDequeue(timeout_ms);
    }

    /// Wait indefinitely for a message
    pub fn receiveWait(self: *Channel) ?*Message {
        return self.queue.waitAndDequeue(null);
    }

    /// Subscribe to a topic (PubSub only)
    pub fn subscribe(self: *Channel, topic: []const u8) !void {
        if (self.channel_type != .PubSub) return error.InvalidChannelType;

        self.mutex.lock();
        defer self.mutex.unlock();

        // Look for existing subscription
        for (self.subscribers.items) |*subscriber| {
            if (subscriber.channel == self) {
                return subscriber.addTopic(topic);
            }
        }

        // Create new subscription
        var sub = Subscriber.init(self.allocator, self);
        try sub.addTopic(topic);
        try self.subscribers.append(sub);
    }

    /// Add a subscriber channel that will receive published messages
    pub fn addSubscriber(self: *Channel, subscriber: *Channel) !bool {
        if (self.closed or subscriber.closed) return error.ChannelClosed;

        // Check if this subscriber is already added
        for (self.subscribers.items) |*sub| {
            if (sub.channel == subscriber) {
                return true; // Already a subscriber
            }
        }

        // Add a new subscriber
        const new_sub = Subscriber.init(self.allocator, subscriber);
        try self.subscribers.append(new_sub);
        return true;
    }

    /// Publish a message to a topic
    pub fn publish(self: *Channel, topic: ?[]const u8, msg: *Message) !void {
        if (self.channel_type != .PubSub) return error.InvalidChannelType;

        // If message has no topic but one is provided, set it
        if (msg.topic == null and topic != null) {
            try msg.setTopic(topic.?);
        }

        self.mutex.lock();
        defer self.mutex.unlock();

        // Distribute to interested subscribers
        for (self.subscribers.items) |*subscriber| {
            if (msg.topic) |msg_topic| {
                if (subscriber.isInterestedIn(msg_topic)) {
                    // Clone the message for each subscriber
                    const new_msg = try Message.create(self.allocator, msg.msg_type, msg.data);
                    new_msg.priority = msg.priority;

                    if (msg.topic) |t| {
                        try new_msg.setTopic(t);
                    }

                    if (msg.reply_to) |r| {
                        new_msg.setReplyTo(r);
                    }

                    try subscriber.channel.queue.enqueue(new_msg);
                }
            } else {
                // If no topic specified, send to all subscribers
                const new_msg = try Message.create(self.allocator, msg.msg_type, msg.data);
                new_msg.priority = msg.priority;

                if (msg.reply_to) |r| {
                    new_msg.setReplyTo(r);
                }

                try subscriber.channel.queue.enqueue(new_msg);
            }
        }
    }

    /// Process received requests with a handler function
    pub fn processRequest(self: *Channel, handler: fn (*Message) ?*Message) !void {
        if (self.closed) return error.ChannelClosed;

        // Get a request message
        const msg = self.queue.dequeue() orelse return error.NoRequestAvailable;

        // Process it with the handler
        if (handler(msg)) |response| {
            // Send the response
            try self.send(response);
        }
    }

    /// Request-reply pattern - send a request and wait for a response
    pub fn request(self: *Channel, msg: *Message, timeout_ms: ?u64) ?*Message {
        if (self.closed) return null;

        // Send the request
        self.send(msg) catch return null;

        // Wait for a response with optional timeout
        const response = if (timeout_ms) |timeout|
            self.receiveTimeout(timeout)
        else
            self.receiveWait();

        return response;
    }

    /// Create a distributed channel
    pub fn createDistributed(allocator: Allocator, channel_type: ChannelType, endpoint_str: []const u8) !*Channel {
        _ = endpoint_str; // Currently unused in this implementation

        // Just create a regular channel for now
        return try Channel.create(allocator, channel_type);
    }

    /// Start network listener for distributed channels
    fn startListener(self: *Channel) !void {
        if (!self.is_distributed) return;

        if (std.mem.startsWith(u8, self.endpoint, "tcp://")) {
            // Extract host:port
            const endpoint = self.endpoint[6..];
            const parts = std.mem.split(u8, endpoint, ":");
            const host = parts.next() orelse "localhost";
            const port_str = parts.next() orelse "0";
            const port = std.fmt.parseInt(u16, port_str, 10) catch 0;

            const address = try net.Address.resolveIp(host, port);

            // Not implemented yet
            _ = address;
            return error.NotImplemented;
        } else if (std.mem.startsWith(u8, self.endpoint, "ipc://")) {
            // Not implemented yet
            return error.NotImplemented;
        } else {
            return error.InvalidEndpoint;
        }
    }

    /// Connect to a remote endpoint
    fn connectToEndpoint(self: *Channel) !void {
        if (!self.is_distributed) return;

        if (std.mem.startsWith(u8, self.endpoint, "tcp://")) {
            // Extract host:port
            const endpoint = self.endpoint[6..];
            const parts = std.mem.split(u8, endpoint, ":");
            const host = parts.next() orelse "localhost";
            const port_str = parts.next() orelse "0";
            const port = std.fmt.parseInt(u16, port_str, 10) catch 0;

            const address = try net.Address.resolveIp(host, port);

            // Not implemented yet
            _ = address;
            return error.NotImplemented;
        } else if (std.mem.startsWith(u8, self.endpoint, "ipc://")) {
            // Not implemented yet
            return error.NotImplemented;
        } else {
            return error.InvalidEndpoint;
        }
    }

    pub fn connect(self: *Channel, endpoint_str: []const u8) !void {
        if (!self.is_distributed) {
            return error.NotDistributed;
        }

        // Parse endpoint (assume tcp://host:port format)
        if (!std.mem.startsWith(u8, endpoint_str, "tcp://")) {
            return error.UnsupportedProtocol;
        }

        const addr_part = endpoint_str["tcp://".len..];
        const colon_pos = std.mem.indexOf(u8, addr_part, ":");
        if (colon_pos == null) {
            return error.InvalidEndpoint;
        }

        const host = addr_part[0..colon_pos.?];
        const port_str = addr_part[colon_pos.? + 1 ..];
        const port = try std.fmt.parseInt(u16, port_str, 10);

        // Connect to remote endpoint
        var address = try net.Address.resolveIp(host, port);
        self.socket = try net.tcpConnectToAddress(address);
    }

    pub fn close(self: *Channel) void {
        const lock = self.mutex.acquire();
        defer lock.release();

        self.closed = true;
        self.not_empty.broadcast();
        self.not_full.broadcast();
    }

    /// Connect to a local server
    pub fn connectLocal(self: *Channel, host: []const u8, port: u16) !void {
        _ = self;
        const address = try net.Address.resolveIp(host, port);
        _ = address; // Currently unused

        // Not implemented yet
        return error.NotImplemented;
    }

    /// Bind to a local port to accept connections
    pub fn bindLocal(self: *Channel, host: []const u8, port: u16) !void {
        _ = self;
        const address = try net.Address.resolveIp(host, port);
        _ = address; // Currently unused

        // Not implemented yet
        return error.NotImplemented;
    }

    fn connectRemote(_self: *Channel, host: []const u8, port: u16) !void {
        const address = try net.Address.resolveIp(host, port);

        // Not implemented yet
        _ = address;
        return error.NotImplemented;
    }
};

/// Messaging system state
var global_allocator: ?Allocator = null;
var is_initialized: bool = false;

/// Initialize the messaging system
pub fn init(allocator: Allocator) !void {
    if (is_initialized) return;

    global_allocator = allocator;
    is_initialized = true;
}

/// Clean up the messaging system
pub fn deinit() void {
    if (!is_initialized) return;

    is_initialized = false;
    global_allocator = null;
}

/// Get the global allocator
fn getAllocator() Allocator {
    return global_allocator orelse @panic("Messaging system not initialized");
}

//------------------------------------------------------------------------------
// C API
//------------------------------------------------------------------------------

/// Message type for C API
pub const GooMessageType = MessageType;

/// Message priority for C API
pub const GooMessagePriority = MessagePriority;

/// Channel type for C API
pub const GooChannelType = ChannelType;

/// Initialize the messaging system
export fn goo_messaging_init() bool {
    if (is_initialized) return true;

    const allocator = std.heap.c_allocator;
    init(allocator) catch return false;
    return true;
}

/// Clean up the messaging system
export fn goo_messaging_cleanup() void {
    deinit();
}

/// Create a new message
export fn goo_message_create(msg_type: u8, data: [*]const u8, data_len: usize) ?*Message {
    const allocator = getAllocator();
    const type_value = @as(MessageType, @enumFromInt(msg_type));
    return Message.create(allocator, type_value, data[0..data_len]) catch null;
}

/// Create a new integer message
export fn goo_message_create_int(value: i64) ?*Message {
    const allocator = getAllocator();
    return Message.createInt(allocator, value) catch null;
}

/// Create a new float message
export fn goo_message_create_float(value: f64) ?*Message {
    const allocator = getAllocator();
    return Message.createFloat(allocator, value) catch null;
}

/// Create a new boolean message
export fn goo_message_create_bool(value: bool) ?*Message {
    const allocator = getAllocator();
    return Message.createBool(allocator, value) catch null;
}

/// Create a new string message
export fn goo_message_create_string(value: [*:0]const u8) ?*Message {
    const allocator = getAllocator();
    const len = std.mem.len(value);
    return Message.createString(allocator, value[0..len]) catch null;
}

/// Destroy a message
export fn goo_message_destroy(msg: *Message) void {
    msg.destroy();
}

/// Get integer value from message
export fn goo_message_get_int(msg: *const Message, value: *i64) bool {
    if (msg.getInt()) |val| {
        value.* = val;
        return true;
    }
    return false;
}

/// Get float value from message
export fn goo_message_get_float(msg: *const Message, value: *f64) bool {
    if (msg.getFloat()) |val| {
        value.* = val;
        return true;
    }
    return false;
}

/// Get boolean value from message
export fn goo_message_get_bool(msg: *const Message, value: *bool) bool {
    if (msg.getBool()) |val| {
        value.* = val;
        return true;
    }
    return false;
}

/// Get string value from message
export fn goo_message_get_string(msg: *const Message) ?[*:0]const u8 {
    if (msg.getString()) |val| {
        if (val.len > 0 and val[val.len - 1] == 0) {
            return @ptrCast(val.ptr);
        }
    }
    return null;
}

/// Get message topic
export fn goo_message_get_topic(msg: *const Message) ?[*:0]const u8 {
    if (msg.topic) |topic| {
        if (topic.len > 0 and topic[topic.len - 1] == 0) {
            return @ptrCast(topic.ptr);
        }
    }
    return null;
}

/// Set message topic
export fn goo_message_set_topic(msg: *Message, topic: [*:0]const u8) bool {
    const len = std.mem.len(topic);
    msg.setTopic(topic[0..len]) catch return false;
    return true;
}

/// Create a new channel
export fn goo_channel_create(channel_type: u8) ?*Channel {
    const allocator = getAllocator();
    const type_value = @as(ChannelType, @enumFromInt(channel_type));
    return Channel.create(allocator, type_value) catch null;
}

/// Destroy a channel
export fn goo_channel_destroy(channel: *Channel) void {
    channel.destroy();
}

/// Send a message to a channel
export fn goo_channel_send(channel: *Channel, msg: *Message) bool {
    channel.send(msg) catch return false;
    return true;
}

/// Receive a message from a channel
export fn goo_channel_receive(channel: *Channel) ?*Message {
    return channel.receive();
}

/// Receive a message with timeout
export fn goo_channel_receive_timeout(channel: *Channel, timeout_ms: u64) ?*Message {
    return channel.receiveTimeout(timeout_ms);
}

/// Wait indefinitely for a message
export fn goo_channel_receive_wait(channel: *Channel) ?*Message {
    return channel.receiveWait();
}

/// Subscribe to a topic
export fn goo_channel_subscribe(channel: *Channel, topic: [*:0]const u8) bool {
    const len = std.mem.len(topic);
    channel.subscribe(topic[0..len]) catch return false;
    return true;
}

/// Add a subscriber channel
export fn goo_channel_add_subscriber(publisher: ?*Channel, subscriber: ?*Channel) bool {
    if (publisher != null and subscriber != null) {
        _ = publisher.?.addSubscriber(subscriber.?) catch return false;
        return true;
    }
    return false;
}

/// Publish a message to a topic
export fn goo_channel_publish(publisher: *Channel, topic: ?[*:0]const u8, msg: *Message) bool {
    var topic_slice: ?[]const u8 = null;

    if (topic) |t| {
        const len = std.mem.len(t);
        topic_slice = t[0..len];
    }

    publisher.publish(topic_slice, msg) catch return false;
    return true;
}

/// Define a processor callback type with the correct calling convention
const ProcessorFn = fn (msg: ?*Message) callconv(.C) ?*Message;

/// Process a request with custom callback
export fn goo_channel_process_request(channel: ?*Channel, processor: ?*const ProcessorFn) bool {
    if (channel == null or processor == null) {
        return false;
    }

    // Receive a message
    if (channel.?.receive()) |msg| {
        // Process it with the callback
        if (processor.?(msg)) |response| {
            // If we have a response, send it back
            _ = channel.?.send(response) catch return false;
            return true;
        }
    }

    return false;
}

// Define test functions
fn testBasicMessaging() !void {
    const allocator = std.testing.allocator;

    const channel = try Channel.create(allocator, .Normal);
    defer channel.destroy();

    // Send an integer message
    const msg = try Message.createInt(allocator, 42);
    try channel.send(msg);

    // Receive the message
    const received = channel.receive() orelse return error.MessageNotReceived;
    defer received.destroy();

    // Verify the value
    const value = received.getInt() orelse return error.InvalidMessageType;
    try std.testing.expectEqual(@as(i64, 42), value);
}

fn testPubSub() !void {
    const allocator = std.testing.allocator;

    const publisher = try Channel.create(allocator, .PubSub);
    defer publisher.destroy();

    const subscriber1 = try Channel.create(allocator, .PubSub);
    defer subscriber1.destroy();

    const subscriber2 = try Channel.create(allocator, .PubSub);
    defer subscriber2.destroy();

    // Subscribe to topics
    try subscriber1.subscribe("topic1");
    try subscriber2.subscribe("topic2");

    // Add subscribers to publisher
    try publisher.addSubscriber(subscriber1);
    try publisher.addSubscriber(subscriber2);

    // Publish to topic1
    const msg1 = try Message.createString(allocator, "Message for topic1");
    try msg1.setTopic("topic1");
    try publisher.publish(null, msg1);

    // Publish to topic2
    const msg2 = try Message.createString(allocator, "Message for topic2");
    try msg2.setTopic("topic2");
    try publisher.publish(null, msg2);

    // Check subscribers received appropriate messages
    const received1 = subscriber1.receive() orelse return error.MessageNotReceived;
    defer received1.destroy();

    const received2 = subscriber2.receive() orelse return error.MessageNotReceived;
    defer received2.destroy();

    const str1 = received1.getString() orelse return error.InvalidMessageType;
    try std.testing.expectEqualStrings("Message for topic1", str1);

    const str2 = received2.getString() orelse return error.InvalidMessageType;
    try std.testing.expectEqualStrings("Message for topic2", str2);
}

// Test suite
test "Messaging System" {
    try init(std.testing.allocator);
    defer deinit();

    try testBasicMessaging();
    try testPubSub();
}
