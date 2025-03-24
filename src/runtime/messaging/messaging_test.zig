const std = @import("std");
const testing = std.testing;
const messaging = @import("messaging.zig");

const Message = messaging.Message;
const Channel = messaging.Channel;
const MessageType = messaging.MessageType;
const ChannelType = messaging.ChannelType;

// Basic test for creating and destroying a message
test "Create and destroy message" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const msg = try Message.createInt(testing.allocator, 42);
    defer msg.destroy();

    try testing.expectEqual(MessageType.Int, msg.msg_type);
    try testing.expectEqual(@as(i64, 42), msg.getInt().?);
}

// Test message type conversions
test "Message type conversions" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    // Test int
    {
        const msg = try Message.createInt(testing.allocator, 42);
        defer msg.destroy();

        try testing.expectEqual(@as(i64, 42), msg.getInt().?);
        try testing.expect(msg.getFloat() == null);
        try testing.expect(msg.getBool() == null);
        try testing.expect(msg.getString() == null);
    }

    // Test float
    {
        const msg = try Message.createFloat(testing.allocator, 3.14159);
        defer msg.destroy();

        try testing.expectEqual(@as(f64, 3.14159), msg.getFloat().?);
        try testing.expect(msg.getInt() == null);
        try testing.expect(msg.getBool() == null);
        try testing.expect(msg.getString() == null);
    }

    // Test bool
    {
        const msg = try Message.createBool(testing.allocator, true);
        defer msg.destroy();

        try testing.expectEqual(true, msg.getBool().?);
        try testing.expect(msg.getInt() == null);
        try testing.expect(msg.getFloat() == null);
        try testing.expect(msg.getString() == null);
    }

    // Test string
    {
        const msg = try Message.createString(testing.allocator, "Hello, Goo!");
        defer msg.destroy();

        try testing.expectEqualStrings("Hello, Goo!", msg.getString().?);
        try testing.expect(msg.getInt() == null);
        try testing.expect(msg.getFloat() == null);
        try testing.expect(msg.getBool() == null);
    }
}

// Test topic setting and getting
test "Message topics" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const msg = try Message.createString(testing.allocator, "Test message");
    defer msg.destroy();

    try testing.expect(msg.topic == null);

    try msg.setTopic("test.topic");
    try testing.expectEqualStrings("test.topic", msg.topic.?);

    // Change topic
    try msg.setTopic("another.topic");
    try testing.expectEqualStrings("another.topic", msg.topic.?);
}

// Test basic channel send/receive
test "Channel send and receive" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const channel = try Channel.create(testing.allocator, .Normal);
    defer channel.destroy();

    // Send a message
    const msg = try Message.createString(testing.allocator, "Test message");
    try channel.send(msg);

    // Receive the message
    const received = channel.receive().?;
    defer received.destroy();

    try testing.expectEqualStrings("Test message", received.getString().?);
}

// Test pubsub pattern
test "PubSub message pattern" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const publisher = try Channel.create(testing.allocator, .PubSub);
    defer publisher.destroy();

    const subscriber1 = try Channel.create(testing.allocator, .PubSub);
    defer subscriber1.destroy();

    const subscriber2 = try Channel.create(testing.allocator, .PubSub);
    defer subscriber2.destroy();

    // Subscribe to topics
    try subscriber1.subscribe("topic1");
    try subscriber2.subscribe("topic2");

    // Add subscribers to publisher
    try publisher.addSubscriber(subscriber1);
    try publisher.addSubscriber(subscriber2);

    // Publish to topic1
    {
        const msg = try Message.createString(testing.allocator, "Message for topic1");
        try msg.setTopic("topic1");
        try publisher.publish(null, msg);
    }

    // Publish to topic2
    {
        const msg = try Message.createString(testing.allocator, "Message for topic2");
        try msg.setTopic("topic2");
        try publisher.publish(null, msg);
    }

    // Check subscribers received appropriate messages
    {
        const received = subscriber1.receive().?;
        defer received.destroy();

        try testing.expectEqualStrings("Message for topic1", received.getString().?);
        try testing.expectEqualStrings("topic1", received.topic.?);
    }

    {
        const received = subscriber2.receive().?;
        defer received.destroy();

        try testing.expectEqualStrings("Message for topic2", received.getString().?);
        try testing.expectEqualStrings("topic2", received.topic.?);
    }

    // Subscribers should not have more messages
    try testing.expect(subscriber1.receive() == null);
    try testing.expect(subscriber2.receive() == null);
}

// Test message priorities
test "Message priorities" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const channel = try Channel.create(testing.allocator, .Normal);
    defer channel.destroy();

    // Create messages with different priorities
    const low_msg = try Message.createString(testing.allocator, "Low priority");
    low_msg.priority = .Low;

    const normal_msg = try Message.createString(testing.allocator, "Normal priority");
    normal_msg.priority = .Normal;

    const high_msg = try Message.createString(testing.allocator, "High priority");
    high_msg.priority = .High;

    const critical_msg = try Message.createString(testing.allocator, "Critical priority");
    critical_msg.priority = .Critical;

    // Send messages in reverse priority order
    try channel.send(low_msg);
    try channel.send(normal_msg);
    try channel.send(high_msg);
    try channel.send(critical_msg);

    // Receive messages - should come in priority order
    {
        const msg1 = channel.receive().?;
        defer msg1.destroy();
        try testing.expectEqualStrings("Critical priority", msg1.getString().?);
    }

    {
        const msg2 = channel.receive().?;
        defer msg2.destroy();
        try testing.expectEqualStrings("High priority", msg2.getString().?);
    }

    {
        const msg3 = channel.receive().?;
        defer msg3.destroy();
        try testing.expectEqualStrings("Normal priority", msg3.getString().?);
    }

    {
        const msg4 = channel.receive().?;
        defer msg4.destroy();
        try testing.expectEqualStrings("Low priority", msg4.getString().?);
    }

    // No more messages
    try testing.expect(channel.receive() == null);
}

// Test request-reply pattern
test "Request-Reply pattern" {
    try messaging.init(testing.allocator);
    defer messaging.deinit();

    const server = try Channel.create(testing.allocator, .ReqRep);
    defer server.destroy();

    // Start server thread
    const thread = try std.Thread.spawn(.{}, struct {
        fn run(srv: *Channel) !void {
            // Process server requests
            const processor = struct {
                fn process(request: *Message) ?*Message {
                    const allocator = request.allocator;
                    const req_str = request.getString() orelse return null;

                    // Echo the request with "REPLY: " prefix
                    var reply_str = std.ArrayList(u8).init(allocator);
                    defer reply_str.deinit();

                    reply_str.appendSlice("REPLY: ") catch return null;
                    reply_str.appendSlice(req_str) catch return null;

                    return Message.createString(allocator, reply_str.items) catch null;
                }
            }.process;

            // Process 3 requests
            for (0..3) |_| {
                try srv.processRequest(processor);
            }
        }
    }.run, .{server});

    // Send requests
    for (1..4) |i| {
        const request_str = try std.fmt.allocPrint(testing.allocator, "Request {d}", .{i});
        defer testing.allocator.free(request_str);

        const request = try Message.createString(testing.allocator, request_str);
        const reply = server.request(request, 1000) orelse {
            thread.join();
            return error.RequestFailed;
        };
        defer reply.destroy();

        const reply_str = try std.fmt.allocPrint(testing.allocator, "REPLY: Request {d}", .{i});
        defer testing.allocator.free(reply_str);

        try testing.expectEqualStrings(reply_str, reply.getString().?);
    }

    thread.join();
}

// Run all tests
test {
    std.testing.refAllDecls(@This());
}
