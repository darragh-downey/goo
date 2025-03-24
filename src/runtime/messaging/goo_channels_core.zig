const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("string.h");
    @cInclude("errno.h");
    @cInclude("pthread.h");
});

// Import the transport module
const transport = @import("goo_transport.zig");

// Channel types
pub const GooChannelType = enum(c_int) {
    Normal = 0, // Standard Go-style channel
    Pub = 1, // Publish (one-to-many)
    Sub = 2, // Subscribe (many-to-one)
    Push = 3, // Push (distribute work)
    Pull = 4, // Pull (collect work)
    Req = 5, // Request (synchronous send+receive)
    Rep = 6, // Reply (synchronous receive+send)
    Dealer = 7, // Dealer (async req)
    Router = 8, // Router (async rep)
    Pair = 9, // Exclusive connection
};

// Channel options as bit flags
pub const GooChannelOptions = c_uint;
pub const GOO_CHAN_BLOCKING: GooChannelOptions = 0; // Default: blocking operations
pub const GOO_CHAN_NONBLOCKING: GooChannelOptions = 1 << 0; // Non-blocking operations
pub const GOO_CHAN_BUFFERED: GooChannelOptions = 1 << 1; // Buffered channel
pub const GOO_CHAN_UNBUFFERED: GooChannelOptions = 1 << 2; // Unbuffered channel (synchronous)
pub const GOO_CHAN_RELIABLE: GooChannelOptions = 1 << 3; // Guaranteed delivery (with retries)
pub const GOO_CHAN_DISTRIBUTED: GooChannelOptions = 1 << 4; // Accessible across processes/network
pub const GOO_CHAN_LOCAL: GooChannelOptions = 1 << 5; // Local to current process only
pub const GOO_CHAN_SECURE: GooChannelOptions = 1 << 6; // Encrypted communications
pub const GOO_CHAN_MULTICAST: GooChannelOptions = 1 << 7; // Use multicast for distribution
pub const GOO_CHAN_HIGH_WATER: GooChannelOptions = 1 << 8; // Use high water mark for flow control
pub const GOO_CHAN_CONFLATE: GooChannelOptions = 1 << 9; // Keep only last message
pub const GOO_CHAN_PRIORITY: GooChannelOptions = 1 << 10; // Support message priorities

// Transport protocols
pub const GooTransportProtocol = enum(c_int) {
    Inproc = 0, // In-process (fastest)
    Ipc = 1, // Inter-process (local)
    Tcp = 2, // TCP/IP (remote)
    Udp = 3, // UDP (unreliable)
    Pgm = 4, // PGM reliable multicast
    Epgm = 5, // Encapsulated PGM
    Vmci = 6, // VM channels
};

// Message flags for send/receive operations
pub const GooMessageFlags = enum(c_int) {
    MSG_NONE = 0, // No special flags
    MSG_NONBLOCK = 1, // Non-blocking operation
    MSG_PEEK = 2, // Peek at next message without removing
    MSG_OOB = 4, // Out-of-band data
    MSG_MORE = 8, // More data coming in subsequent messages
    MSG_PRIORITY = 16, // High priority message
};

// Channel events
pub const GooChannelEvent = enum(c_int) {
    Received = 0, // Message received
    Sent = 1, // Message sent
    Error = 2, // Error occurred
    Timeout = 3, // Operation timed out
    Connected = 4, // Connected to endpoint
    Disconnected = 5, // Disconnected from endpoint
    Closed = 6, // Channel closed
};

// Message structure - matches the C structure
pub const GooMessage = extern struct {
    data: ?*anyopaque, // Message data
    size: usize, // Size of data
    flags: GooMessageFlags, // Message flags
    priority: u32, // Message priority (if supported)
    context: ?*anyopaque, // User context
    free_fn: ?*const fn (?*anyopaque) callconv(.C) void, // Custom free function for data
    next: ?*GooMessage, // Next part (for multi-part messages)
};

// Endpoint structure for channel connections
pub const GooEndpoint = struct {
    protocol: GooTransportProtocol,
    address: [256]u8,
    address_len: usize,
    port: u16,
    is_server: bool,
    socket_fd: c_int,
    thread: c.pthread_t,
    thread_running: bool,
    next: ?*GooEndpoint,
    // For in-process transport
    queue: ?*transport.InprocQueue,

    pub fn init(protocol: GooTransportProtocol, address: []const u8, port: u16, is_server: bool) !*GooEndpoint {
        var endpoint = @ptrCast(?*GooEndpoint, c.malloc(@sizeOf(GooEndpoint)));
        if (endpoint == null) {
            return error.OutOfMemory;
        }

        endpoint.?.* = GooEndpoint{
            .protocol = protocol,
            .address = undefined,
            .address_len = 0,
            .port = port,
            .is_server = is_server,
            .socket_fd = -1,
            .thread = undefined,
            .thread_running = false,
            .next = null,
            .queue = null,
        };

        // Copy the address
        const copy_len = @min(address.len, endpoint.?.address.len - 1);
        std.mem.copy(u8, endpoint.?.address[0..copy_len], address[0..copy_len]);
        endpoint.?.address[copy_len] = 0; // Null-terminate
        endpoint.?.address_len = copy_len;

        return endpoint.?;
    }

    pub fn deinit(self: *GooEndpoint) void {
        // Close the socket if open
        if (self.socket_fd >= 0) {
            _ = c.close(self.socket_fd);
            self.socket_fd = -1;
        }

        // Clean up in-process queue if needed
        if (self.queue != null and self.is_server) {
            const addr_slice = self.address[0..self.address_len];
            transport.removeInprocEndpoint(addr_slice) catch {};
            self.queue = null;
        }

        // Free the memory
        c.free(@ptrCast(?*anyopaque, self));
    }
};

// Subscription structure - matches the C structure
pub const GooSubscription = extern struct {
    topic: [*:0]u8, // Topic filter string
    topic_length: usize, // Length of topic string
    context: ?*anyopaque, // User context
    callback: ?*const fn (?*GooMessage, ?*anyopaque) callconv(.C) void, // Callback function
    next: ?*GooSubscription, // Next subscription in list
};

// Channel statistics - matches the C structure
pub const GooChannelStats = extern struct {
    messages_sent: u64, // Total messages sent
    messages_received: u64, // Total messages received
    bytes_sent: u64, // Total bytes sent
    bytes_received: u64, // Total bytes received
    send_errors: u64, // Number of send errors
    receive_errors: u64, // Number of receive errors
    current_queue_size: u64, // Current number of messages in queue
    max_queue_size: u64, // Maximum queue size reached
    dropped_messages: u64, // Number of dropped messages
    retried_sends: u64, // Number of retried sends
};

// Channel structure - matches the C structure
pub const GooChannel = extern struct {
    type: GooChannelType, // Channel type
    options: GooChannelOptions, // Channel options
    buffer_size: usize, // Buffer capacity (for buffered channels)
    elem_size: usize, // Size of each element (for typed channels)

    // Synchronization
    mutex: c.pthread_mutex_t, // Channel mutex
    send_cond: c.pthread_cond_t, // Condition variable for send operations
    recv_cond: c.pthread_cond_t, // Condition variable for receive operations

    // Buffer management
    buffer: ?*anyopaque, // Circular buffer for messages
    head: usize, // Buffer head position
    tail: usize, // Buffer tail position
    count: usize, // Number of items in buffer

    // Distributed capabilities
    is_distributed: bool, // Is this a distributed channel?
    endpoints: ?*GooEndpoint, // List of endpoints (for distributed channels)

    // Pub/Sub specific
    subscriptions: ?*GooSubscription, // List of subscriptions (for SUB channels)

    // Statistics
    stats: GooChannelStats, // Channel statistics

    // Extra options
    high_water_mark: u32, // High water mark for flow control
    low_water_mark: u32, // Low water mark for flow control
    timeout_ms: u32, // Operation timeout in milliseconds

    // State flags
    closed: bool, // Is the channel closed?
    has_error: bool, // Is the channel in error state?

    // Additional fields for distributed messaging
    waiting_senders: c_int, // Number of waiting senders
    waiting_receivers: c_int, // Number of waiting receivers
};

// Helper functions
fn channelInitBuffer(channel: *GooChannel) bool {
    if (channel.buffer_size == 0) return false;

    // Calculate total buffer size
    const total_size = channel.buffer_size * channel.elem_size;

    // Allocate buffer with Zig's safety
    channel.buffer = c.malloc(total_size);
    if (channel.buffer == null) return false;

    // Initialize buffer to zero
    @memset(@as([*]u8, @ptrCast(channel.buffer.?))[0..total_size], 0);

    // Initialize buffer state
    channel.head = 0;
    channel.tail = 0;
    channel.count = 0;

    return true;
}

fn channelIsDistributedType(type_val: GooChannelType) bool {
    return switch (type_val) {
        .Pub, .Sub, .Push, .Pull, .Req, .Rep, .Dealer, .Router => true,
        else => false,
    };
}

fn channelUpdateStatsSend(channel: *GooChannel, size: usize, success: bool) void {
    if (success) {
        channel.stats.messages_sent +%= 1;
        channel.stats.bytes_sent +%= size;
    } else {
        channel.stats.send_errors +%= 1;
    }
}

fn channelUpdateStatsReceive(channel: *GooChannel, size: usize, success: bool) void {
    if (success) {
        channel.stats.messages_received +%= 1;
        channel.stats.bytes_received +%= size;
    } else {
        channel.stats.receive_errors +%= 1;
    }
}

// C-compatible exported functions

// Create a new channel
export fn goo_channel_create(type_val: GooChannelType, elem_size: usize, buffer_size: usize, options: GooChannelOptions) ?*GooChannel {
    // Allocate channel struct
    const channel = @as(*GooChannel, @alignCast(@ptrCast(c.malloc(@sizeOf(GooChannel)) orelse return null)));

    // Initialize with zeros
    @memset(@as([*]u8, @ptrCast(channel))[0..@sizeOf(GooChannel)], 0);

    // Set initial values
    channel.type = type_val;
    channel.options = options;
    channel.buffer_size = if (buffer_size > 0) buffer_size else 1; // Minimum buffer size is 1
    channel.elem_size = elem_size;
    channel.high_water_mark = @intCast(buffer_size);
    channel.low_water_mark = @intCast(buffer_size / 2);
    channel.timeout_ms = 0; // Default: no timeout

    // Initialize synchronization primitives
    if (c.pthread_mutex_init(&channel.mutex, null) != 0) {
        c.free(channel);
        return null;
    }

    if (c.pthread_cond_init(&channel.send_cond, null) != 0) {
        _ = c.pthread_mutex_destroy(&channel.mutex);
        c.free(channel);
        return null;
    }

    if (c.pthread_cond_init(&channel.recv_cond, null) != 0) {
        _ = c.pthread_cond_destroy(&channel.send_cond);
        _ = c.pthread_mutex_destroy(&channel.mutex);
        c.free(channel);
        return null;
    }

    // Distributed channel setup
    channel.is_distributed = channelIsDistributedType(type_val) or (options & GOO_CHAN_DISTRIBUTED) != 0;

    // Allocate buffer for buffered channels
    if ((options & GOO_CHAN_UNBUFFERED) == 0 and !channelInitBuffer(channel)) {
        _ = c.pthread_cond_destroy(&channel.recv_cond);
        _ = c.pthread_cond_destroy(&channel.send_cond);
        _ = c.pthread_mutex_destroy(&channel.mutex);
        c.free(channel);
        return null;
    }

    return channel;
}

// Close a channel
export fn goo_channel_close(channel: ?*GooChannel) c_int {
    if (channel == null) return -c.EINVAL;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    channel.?.closed = true;

    // Wake up any waiting senders/receivers
    _ = c.pthread_cond_broadcast(&channel.?.send_cond);
    _ = c.pthread_cond_broadcast(&channel.?.recv_cond);

    _ = c.pthread_mutex_unlock(&channel.?.mutex);

    return 0;
}

// Destroy a channel and free resources
export fn goo_channel_destroy(channel: ?*GooChannel) void {
    if (channel == null) return;

    // Close the channel first
    if (!channel.?.closed) {
        _ = goo_channel_close(channel);
    }

    // Clean up any subscriptions
    if (channel.?.type == .Sub) {
        var sub = channel.?.subscriptions;
        while (sub != null) {
            const next = sub.?.next;
            c.free(sub.?.topic);
            c.free(sub);
            sub = next;
        }
    }

    // Clean up any endpoints
    if (channel.?.is_distributed) {
        var ep = channel.?.endpoints;
        while (ep != null) {
            const next = ep.?.next;
            // TODO: Properly clean up connections
            c.free(ep.?.address);
            c.free(ep);
            ep = next;
        }
    }

    // Free the buffer
    if (channel.?.buffer != null) {
        c.free(channel.?.buffer);
    }

    // Destroy synchronization primitives
    _ = c.pthread_cond_destroy(&channel.?.recv_cond);
    _ = c.pthread_cond_destroy(&channel.?.send_cond);
    _ = c.pthread_mutex_destroy(&channel.?.mutex);

    // Free the channel structure
    c.free(channel);
}

// Get channel statistics
export fn goo_channel_stats(channel: ?*GooChannel) GooChannelStats {
    if (channel == null) return std.mem.zeroes(GooChannelStats);

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    const stats = channel.?.stats;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);

    return stats;
}

// Reset channel statistics
export fn goo_channel_reset_stats(channel: ?*GooChannel) void {
    if (channel == null) return;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    channel.?.stats = std.mem.zeroes(GooChannelStats);
    _ = c.pthread_mutex_unlock(&channel.?.mutex);
}

// Check if channel is empty
export fn goo_channel_is_empty(channel: ?*GooChannel) bool {
    if (channel == null) return true;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    const result = channel.?.count == 0;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);

    return result;
}

// Check if channel is full
export fn goo_channel_is_full(channel: ?*GooChannel) bool {
    if (channel == null) return false;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    const result = channel.?.count >= channel.?.buffer_size;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);

    return result;
}

// Get the number of items in the channel
export fn goo_channel_size(channel: ?*GooChannel) usize {
    if (channel == null) return 0;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    const size = channel.?.count;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);

    return size;
}

// Set high water mark for flow control
export fn goo_channel_set_high_water_mark(channel: ?*GooChannel, hwm: u32) void {
    if (channel == null) return;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    channel.?.high_water_mark = hwm;
    channel.?.low_water_mark = hwm / 2;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);
}

// Send a message to a channel
export fn goo_channel_send(channel: ?*GooChannel, msg: ?*GooMessage, flags: c_int) c_int {
    if (channel == null or msg == null) {
        return -c.EINVAL;
    }

    // Convert C flags to Zig enum
    const msg_flags = @as(GooMessageFlags, @enumFromInt(flags));

    if (channel.?.closed) return -c.EPIPE;

    // For distributed channels, implementation moved to transport module
    if (channel.?.is_distributed) {
        // Implementation moved to transport module
    }

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Check if the channel is full
    if (channel.?.count >= channel.?.buffer_size and (msg_flags == .MSG_NONBLOCK)) {
        channelUpdateStatsSend(channel.?, 0, false);
        return -c.EAGAIN;
    }

    // Get the next available buffer slot
    const buf_index = (channel.?.head + channel.?.count) % channel.?.buffer_size;

    // Copy data to buffer
    const src_ptr = @as([*]const u8, @ptrCast(msg.?.data));
    const dst_ptr = @as([*]u8, @ptrCast(channel.?.buffer.?)) + (buf_index * channel.?.elem_size);

    // Use Zig's safe memory operations
    const copy_size = @min(msg.?.size, channel.?.elem_size);
    @memcpy(dst_ptr[0..copy_size], src_ptr[0..copy_size]);

    // Update channel state
    if (channel.?.count == 0 and channel.?.waiting_receivers > 0) {
        // Signal one receiver
        _ = c.pthread_cond_signal(&channel.?.recv_cond);
    }

    channel.?.count += 1;
    channelUpdateStatsSend(channel.?, @intCast(copy_size), true);

    return @intCast(copy_size);
}

// Receive a message from a channel
export fn goo_channel_recv(channel: ?*GooChannel, msg: ?**GooMessage, flags: c_int) c_int {
    if (channel == null or msg == null) {
        return -c.EINVAL;
    }

    // Convert C flags to Zig enum
    const msg_flags = @as(GooMessageFlags, @enumFromInt(flags));

    if (channel.?.closed) return -c.EPIPE;

    // For distributed channels that primarily receive, try endpoints first
    if (channel.?.is_distributed and
        (channel.?.type == .Sub or channel.?.type == .Pull or channel.?.type == .Rep))
    {
        // Implementation moved to transport module
    }

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Check if the channel is empty
    if (channel.?.count == 0) {
        // If non-blocking, return would-block error
        if (msg_flags == .MSG_NONBLOCK) {
            return -c.EAGAIN;
        }

        // Wait for data to arrive
        channel.?.waiting_receivers += 1;
        while (channel.?.count == 0 and !channel.?.closed) {
            _ = c.pthread_cond_wait(&channel.?.recv_cond, &channel.?.mutex);
        }
        channel.?.waiting_receivers -= 1;

        // Check if the channel was closed while waiting
        if (channel.?.count == 0) {
            return -c.EPIPE;
        }
    }

    // Get the data from the buffer
    const buf_index = channel.?.head;
    const src = @as([*]const u8, @ptrCast(channel.?.buffer.?)) + (buf_index * channel.?.elem_size);

    // Create a new message with the received data
    const new_msg = createMessage(src, channel.?.elem_size);
    if (new_msg == null) {
        return -c.ENOMEM;
    }

    // Set the output pointer
    msg.?.* = new_msg.?;

    // Update channel state
    channel.?.head = (channel.?.head + 1) % channel.?.buffer_size;
    channel.?.count -= 1;

    if (channel.?.count == channel.?.buffer_size - 1 and channel.?.waiting_senders > 0) {
        // Signal one sender
        _ = c.pthread_cond_signal(&channel.?.send_cond);
    }

    channelUpdateStatsReceive(channel.?, new_msg.?.size, true);

    return @intCast(new_msg.?.size);
}

// Send a structured message to the channel
export fn goo_channel_send_msg(channel: ?*GooChannel, msg: ?*GooMessage) c_int {
    if (channel == null or msg == null) return -c.EINVAL;

    // Check if channel is closed
    if (channel.?.closed) return -c.EPIPE;

    // Special handling for pub/sub channels
    if (channel.?.type == .Pub) {
        return goo_channel_publish(channel, msg);
    }

    return goo_channel_send(channel, msg, 0);
}

// Receive a structured message from the channel
export fn goo_channel_recv_msg(channel: ?*GooChannel, msg: ?*GooMessage) c_int {
    if (channel == null or msg == null) return -c.EINVAL;

    // Special handling for pub/sub channels
    if (channel.?.type == .Sub) {
        return goo_channel_subscribe(channel, msg);
    }

    // We'll do a direct implementation here to avoid pointer type issues
    if (channel.?.closed) return -c.EPIPE;

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Check if the channel is empty
    if (channel.?.count == 0) {
        // Wait for data to arrive
        channel.?.waiting_receivers += 1;
        while (channel.?.count == 0 and !channel.?.closed) {
            _ = c.pthread_cond_wait(&channel.?.recv_cond, &channel.?.mutex);
        }
        channel.?.waiting_receivers -= 1;

        // Check if the channel was closed while waiting
        if (channel.?.count == 0) {
            return -c.EPIPE;
        }
    }

    // Get the data from the buffer
    const buf_index = channel.?.head;
    const src = @as([*]const u8, @ptrCast(channel.?.buffer.?)) + (buf_index * channel.?.elem_size);

    // Copy the data if available
    if (msg.?.data != null) {
        const copy_size = @min(msg.?.size, channel.?.elem_size);
        @memcpy(@as([*]u8, @ptrCast(msg.?.data))[0..copy_size], src[0..copy_size]);
        msg.?.size = copy_size;
    }

    // Update channel state
    channel.?.head = (channel.?.head + 1) % channel.?.buffer_size;
    channel.?.count -= 1;

    if (channel.?.count == channel.?.buffer_size - 1 and channel.?.waiting_senders > 0) {
        // Signal one sender
        _ = c.pthread_cond_signal(&channel.?.send_cond);
    }

    channelUpdateStatsReceive(channel.?, msg.?.size, true);

    return @intCast(msg.?.size);
}

// Publish a message to all subscribers (for PUB channels)
export fn goo_channel_publish(channel: ?*GooChannel, msg: ?*GooMessage) c_int {
    if (channel == null or msg == null) return -c.EINVAL;

    if (channel.?.type != .Pub) return -c.EINVAL;

    // For in-process channels, just send to buffer
    const result = goo_channel_send(channel, msg, 0);

    // For distributed channels, send to all endpoints
    if (channel.?.is_distributed) {
        // TODO: Implement distributed publishing
    }

    return result;
}

// Subscribe to messages (for SUB channels)
export fn goo_channel_subscribe(channel: ?*GooChannel, msg: ?*GooMessage) c_int {
    if (channel == null or msg == null) return -c.EINVAL;

    if (channel.?.type != .Sub) return -c.EINVAL;

    // For in-process channels, implement direct subscription
    if (channel.?.closed) return -c.EPIPE;

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Check if the channel is empty
    if (channel.?.count == 0) {
        // Wait for data to arrive
        channel.?.waiting_receivers += 1;
        while (channel.?.count == 0 and !channel.?.closed) {
            _ = c.pthread_cond_wait(&channel.?.recv_cond, &channel.?.mutex);
        }
        channel.?.waiting_receivers -= 1;

        // Check if the channel was closed while waiting
        if (channel.?.count == 0) {
            return -c.EPIPE;
        }
    }

    // Get the data from the buffer
    const buf_index = channel.?.head;
    const src = @as([*]const u8, @ptrCast(channel.?.buffer.?)) + (buf_index * channel.?.elem_size);

    // Copy the data if available
    if (msg.?.data != null) {
        const copy_size = @min(msg.?.size, channel.?.elem_size);
        @memcpy(@as([*]u8, @ptrCast(msg.?.data))[0..copy_size], src[0..copy_size]);
        msg.?.size = copy_size;
    }

    // Update channel state
    channel.?.head = (channel.?.head + 1) % channel.?.buffer_size;
    channel.?.count -= 1;

    if (channel.?.count == channel.?.buffer_size - 1 and channel.?.waiting_senders > 0) {
        // Signal one sender
        _ = c.pthread_cond_signal(&channel.?.send_cond);
    }

    // For distributed channels, handle subscriptions
    if (channel.?.is_distributed) {
        // TODO: Implement distributed subscription handling
    }

    channelUpdateStatsReceive(channel.?, msg.?.size, true);

    return @intCast(msg.?.size);
}

// Add a subscription filter (for SUB channels)
export fn goo_channel_subscribe_topic(channel: ?*GooChannel, topic: [*:0]const u8, callback: ?*const fn (?*GooMessage, ?*anyopaque) callconv(.C) void, context: ?*anyopaque) c_int {
    if (channel == null) return -c.EINVAL;
    if (topic[0] == 0) return -c.EINVAL;

    if (channel.?.type != .Sub) return -c.EINVAL;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Allocate new subscription
    const sub = @as(*GooSubscription, @alignCast(@ptrCast(c.malloc(@sizeOf(GooSubscription)) orelse return -c.ENOMEM)));

    // Copy topic string
    const topic_len = std.mem.len(topic);
    sub.topic = @as([*:0]u8, @ptrCast(c.malloc(topic_len + 1) orelse {
        c.free(sub);
        return -c.ENOMEM;
    }));
    @memcpy(sub.topic[0..topic_len], topic[0..topic_len]);
    sub.topic[topic_len] = 0;

    // Set subscription parameters
    sub.topic_length = topic_len;
    sub.callback = callback;
    sub.context = context;

    // Add to subscription list
    sub.next = channel.?.subscriptions;
    channel.?.subscriptions = sub;

    return 0;
}

// Remove a subscription filter (for SUB channels)
export fn goo_channel_unsubscribe_topic(channel: ?*GooChannel, topic: [*:0]const u8) c_int {
    if (channel == null) return -c.EINVAL;
    if (topic[0] == 0) return -c.EINVAL;

    if (channel.?.type != .Sub) return -c.EINVAL;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    // Check if we have any subscriptions
    if (channel.?.subscriptions == null) {
        return -c.ENOENT; // Not found
    }

    var prev: ?*GooSubscription = null;
    var sub = channel.?.subscriptions;

    while (sub != null) {
        if (std.mem.eql(u8, sub.?.topic[0..sub.?.topic_length], topic[0..std.mem.len(topic)])) {
            // Remove from list
            if (prev == null) {
                channel.?.subscriptions = sub.?.next;
            } else {
                prev.?.next = sub.?.next;
            }

            // Free resources
            c.free(sub.?.topic);
            c.free(sub);

            return 0;
        }

        prev = sub;
        sub = sub.?.next;
    }

    return -c.ENOENT; // Not found
}

// Set timeout for operations
export fn goo_channel_set_timeout(channel: ?*GooChannel, timeout_ms: u32) void {
    if (channel == null) return;

    _ = c.pthread_mutex_lock(&channel.?.mutex);
    channel.?.timeout_ms = timeout_ms;
    _ = c.pthread_mutex_unlock(&channel.?.mutex);
}

// Create a message with the given data
export fn goo_message_create(data: ?*anyopaque, size: usize, flags: GooMessageFlags) ?*GooMessage {
    const msg = @as(*GooMessage, @alignCast(@ptrCast(c.malloc(@sizeOf(GooMessage)) orelse return null)));

    // Initialize with zeros
    @memset(@as([*]u8, @ptrCast(msg))[0..@sizeOf(GooMessage)], 0);

    // Set initial values
    msg.data = data;
    msg.size = size;
    msg.flags = flags;
    msg.priority = 0;
    msg.context = null;
    msg.free_fn = null;
    msg.next = null;

    return msg;
}

// Create a message with a copy of the data
export fn goo_message_create_copy(data: ?*const anyopaque, size: usize, flags: GooMessageFlags) ?*GooMessage {
    if (data == null) return null;

    // Allocate memory for data
    const data_copy = c.malloc(size) orelse return null;
    @memcpy(@as([*]u8, @ptrCast(data_copy))[0..size], @as([*]const u8, @ptrCast(data.?))[0..size]);

    // Create message
    const msg = goo_message_create(data_copy, size, flags) orelse {
        c.free(data_copy);
        return null;
    };

    // Set free function
    msg.free_fn = freeMessageData;

    return msg;
}

// Free message data callback
fn freeMessageData(data: ?*anyopaque) callconv(.C) void {
    if (data != null) c.free(data);
}

// Destroy a message and free resources
export fn goo_message_destroy(msg: ?*GooMessage) void {
    if (msg == null) return;

    // Free data if we have a free function
    if (msg.?.data != null and msg.?.free_fn != null) {
        msg.?.free_fn.?(msg.?.data);
    }

    // Free any chained messages
    if (msg.?.next != null) {
        goo_message_destroy(msg.?.next);
    }

    // Free the message structure
    c.free(msg);
}

// Chain multiple messages together
export fn goo_message_chain(first: ?*GooMessage, second: ?*GooMessage) ?*GooMessage {
    if (first == null) return second;
    if (second == null) return first;

    // Find the last message in the chain
    var last = first;
    while (last.?.next != null) {
        last = last.?.next;
    }

    // Append second message to the chain
    last.?.next = second;

    return first;
}

// Create a multipart message
export fn goo_message_multipart(parts: [*]?*GooMessage, count: usize) ?*GooMessage {
    if (count == 0) return null;

    var result: ?*GooMessage = null;

    // Chain all parts together
    for (0..count) |i| {
        result = goo_message_chain(result, parts[i]);
    }

    return result;
}

// For priority message handling
pub fn goo_message_set_priority(msg: ?*GooMessage, priority: u8) void {
    if (msg == null) return;

    msg.?.priority = priority;
    msg.?.flags |= @intFromEnum(GooMessageFlags.MSG_PRIORITY);
}

// Get number of parts in a multipart message
export fn goo_message_part_count(msg: ?*GooMessage) usize {
    if (msg == null) return 0;

    var count: usize = 0;
    var current = msg;

    while (current != null) {
        count += 1;
        current = current.?.next;
    }

    return count;
}

// Helper functions for distributed messaging

// These functions are declared but not implemented to satisfy the interface.
// The actual implementations are in the transport module.

// Create a new message
fn createMessage(data: ?*const anyopaque, size: usize) ?*GooMessage {
    var msg = @as(?*GooMessage, @ptrCast(@alignCast(c.malloc(@sizeOf(GooMessage)))));
    if (msg == null) {
        return null;
    }

    if (data != null) {
        msg.?.data = c.malloc(size);
        if (msg.?.data == null) {
            c.free(msg);
            return null;
        }
        @memcpy(@as([*]u8, @ptrCast(msg.?.data))[0..size], @as([*]const u8, @ptrCast(data))[0..size]);
    } else {
        msg.?.data = null;
    }

    msg.?.size = size;
    msg.?.flags = .MSG_NONE;
    msg.?.priority = 0;

    return msg;
}
