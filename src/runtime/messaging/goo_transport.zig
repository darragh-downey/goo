const std = @import("std");
const c = @cImport({
    @cInclude("stdlib.h");
    @cInclude("string.h");
    @cInclude("errno.h");
    @cInclude("unistd.h");
    @cInclude("fcntl.h");
    @cInclude("sys/socket.h");
    @cInclude("sys/un.h");
    @cInclude("netinet/in.h");
    @cInclude("arpa/inet.h");
    @cInclude("pthread.h");
});

// Import the channel types from the core module
const core = @import("goo_channels_core.zig");
const GooChannel = core.GooChannel;
const GooEndpoint = core.GooEndpoint;
const GooMessage = core.GooMessage;
const GooTransportProtocol = core.GooTransportProtocol;
const GooMessageFlags = core.GooMessageFlags;

// Import channel types and definitions
const GooChannelType = @import("goo_channels_core.zig").GooChannelType;

// Constants
const MAX_CONNECTIONS = 10;
const DEFAULT_BACKLOG = 5;
const SOCKET_TIMEOUT_MS = 1000;
const MAX_MSG_SIZE = 65536;

// Error codes
const TRANSPORT_ERROR = -1;
const TRANSPORT_OK = 0;
const TRANSPORT_WOULD_BLOCK = 1;

// Socket options struct
pub const SocketOptions = struct {
    nonblocking: bool = false,
    reuse_addr: bool = true,
    keep_alive: bool = true,
    no_delay: bool = true,
    receive_timeout_ms: c_int = SOCKET_TIMEOUT_MS,
    send_timeout_ms: c_int = SOCKET_TIMEOUT_MS,
};

// Socket message flags
const MSG_PEEK: c_int = 2; // Peek at incoming message
const MSG_DONTWAIT: c_int = 64; // Non-blocking operation

// Endpoint management functions

// Create a new endpoint
pub fn createEndpoint(protocol: GooTransportProtocol, address: [*:0]const u8, port: u16, is_server: bool) ?*GooEndpoint {
    // Allocate memory for the endpoint and the address string
    const ep = @as(*GooEndpoint, @ptrCast(c.malloc(@sizeOf(GooEndpoint))));
    if (ep == null) return null;

    // Initialize the endpoint
    ep.* = GooEndpoint{
        .protocol = protocol,
        .address = @as([*:0]u8, @ptrCast(c.strdup(address))),
        .port = port,
        .is_server = is_server,
        .socket_fd = -1,
        .thread = undefined,
        .thread_running = false,
        .next = null,
    };

    return ep;
}

// Destroy an endpoint
pub fn destroyEndpoint(endpoint: *GooEndpoint) void {
    // Close the socket if it's open
    if (endpoint.socket_fd >= 0) {
        _ = c.close(endpoint.socket_fd);
    }

    // Free the address string
    if (endpoint.address[0] != 0) {
        c.free(endpoint.address);
    }

    // Free the endpoint structure
    c.free(endpoint);
}

// Create a socket based on the transport protocol
pub fn createSocket(protocol: GooTransportProtocol, is_server: bool, address: []const u8, port: u16, options: SocketOptions) !c_int {
    switch (protocol) {
        .Inproc => {
            // For in-process communication, we don't need a real socket
            // We will use our own queue system
            return error.InprocTransportShouldUseQueue;
        },
        .Ipc => {
            // For IPC, we use Unix domain sockets
            return try createSocketIpc(is_server, address, options);
        },
        .Tcp => {
            // For TCP, we use regular network sockets
            return try createSocketTcp(is_server, address, port, options);
        },
        .Udp => {
            // For UDP, use datagram sockets
            var socket_fd = c.socket(c.AF_INET, c.SOCK_DGRAM, 0);
            if (socket_fd < 0) {
                std.log.err("Socket creation failed: {s}", .{std.os.strerror(c.errno)});
                return error.SocketCreationFailed;
            }

            // Apply common socket options
            if (options.reuse_addr) {
                const value: c_int = 1;
                if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_REUSEADDR, &value, @sizeOf(c_int)) < 0) {
                    std.log.err("Failed to set SO_REUSEADDR: {s}", .{std.os.strerror(c.errno)});
                    _ = c.close(socket_fd);
                    return error.SocketOptionFailed;
                }
            }

            // Set non-blocking mode if requested
            if (options.nonblocking) {
                var flags = c.fcntl(socket_fd, c.F_GETFL, 0);
                if (flags < 0) {
                    std.log.err("Failed to get socket flags: {s}", .{std.os.strerror(c.errno)});
                    _ = c.close(socket_fd);
                    return error.SocketOptionFailed;
                }
                flags |= c.O_NONBLOCK;
                if (c.fcntl(socket_fd, c.F_SETFL, flags) < 0) {
                    std.log.err("Failed to set socket to non-blocking: {s}", .{std.os.strerror(c.errno)});
                    _ = c.close(socket_fd);
                    return error.SocketOptionFailed;
                }
            }

            // Same as TCP binding/connecting but for UDP
            if (is_server) {
                var server_addr: c.sockaddr_in = undefined;
                std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_in));
                server_addr.sin_family = c.AF_INET;
                server_addr.sin_port = c.htons(port);

                if (std.mem.eql(u8, address, "*")) {
                    // Bind to all interfaces
                    server_addr.sin_addr.s_addr = c.INADDR_ANY;
                } else {
                    // Bind to specific interface
                    const c_host = @ptrCast([*]const u8, address.ptr);
                    if (c.inet_pton(c.AF_INET, c_host, &server_addr.sin_addr) != 1) {
                        std.log.err("Invalid address: {s}", .{address});
                        _ = c.close(socket_fd);
                        return error.InvalidAddress;
                    }
                }

                if (c.bind(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @sizeOf(c.sockaddr_in)) < 0) {
                    std.log.err("Bind failed: {s}", .{std.os.strerror(c.errno)});
                    _ = c.close(socket_fd);
                    return error.BindFailed;
                }

                // No need to call listen() for UDP - it's connectionless
            } else {
                // For UDP clients, we just connect to the server
                var server_addr: c.sockaddr_in = undefined;
                std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_in));
                server_addr.sin_family = c.AF_INET;
                server_addr.sin_port = c.htons(port);

                const c_host = @ptrCast([*]const u8, address.ptr);
                if (c.inet_pton(c.AF_INET, c_host, &server_addr.sin_addr) != 1) {
                    std.log.err("Invalid address: {s}", .{address});
                    _ = c.close(socket_fd);
                    return error.InvalidAddress;
                }

                // Connect makes the socket send to a specific address by default
                if (c.connect(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @sizeOf(c.sockaddr_in)) < 0) {
                    std.log.err("Connect failed: {s}", .{std.os.strerror(c.errno)});
                    _ = c.close(socket_fd);
                    return error.ConnectFailed;
                }
            }

            return socket_fd;
        },
        .Pgm, .Epgm => {
            // PGM and EPGM protocols require additional libraries and are complex
            // They're for reliable multicast and would require integration with a PGM implementation
            return error.PgmNotImplemented;
        },
        .Vmci => {
            // VMCI is for VM communication and would require specific platform support
            return error.VmciNotImplemented;
        },
    }
}

// Bind a socket to an address
pub fn bindSocket(socket_fd: c_int, protocol: GooTransportProtocol, address: [*:0]const u8, port: u16) c_int {
    switch (protocol) {
        .Inproc => {
            // In-process doesn't use actual sockets
            return 0;
        },
        .Ipc => {
            // Unix domain socket
            var addr = std.mem.zeroes(c.struct_sockaddr_un);
            addr.sun_family = c.AF_UNIX;
            const path_len = std.mem.len(address);
            if (path_len >= addr.sun_path.len) {
                return -1;
            }
            std.mem.copy(u8, addr.sun_path[0..path_len], address[0..path_len]);

            // Unlink the socket path if it already exists
            _ = c.unlink(address);

            return c.bind(socket_fd, @as(*c.struct_sockaddr, @ptrCast(&addr)), @sizeOf(c.struct_sockaddr_un));
        },
        .Tcp, .Udp => {
            // TCP/UDP socket
            var addr = std.mem.zeroes(c.struct_sockaddr_in);
            addr.sin_family = c.AF_INET;
            addr.sin_port = c.htons(port);

            if (std.mem.eql(u8, std.mem.span(address), "*")) {
                // Bind to all interfaces
                addr.sin_addr.s_addr = c.htonl(c.INADDR_ANY);
            } else {
                // Bind to specific address
                addr.sin_addr.s_addr = c.inet_addr(address);
            }

            return c.bind(socket_fd, @as(*c.struct_sockaddr, @ptrCast(&addr)), @sizeOf(c.struct_sockaddr_in));
        },
        .Pgm, .Epgm, .Vmci => {
            // Not implemented
            return -1;
        },
    }
}

// Connect a socket to an address
pub fn connectSocket(socket_fd: c_int, protocol: GooTransportProtocol, address: [*:0]const u8, port: u16) c_int {
    switch (protocol) {
        .Inproc => {
            // In-process doesn't use actual sockets
            return 0;
        },
        .Ipc => {
            // Unix domain socket
            var addr = std.mem.zeroes(c.struct_sockaddr_un);
            addr.sun_family = c.AF_UNIX;
            const path_len = std.mem.len(address);
            if (path_len >= addr.sun_path.len) {
                return -1;
            }
            std.mem.copy(u8, addr.sun_path[0..path_len], address[0..path_len]);

            return c.connect(socket_fd, @as(*c.struct_sockaddr, @ptrCast(&addr)), @sizeOf(c.struct_sockaddr_un));
        },
        .Tcp, .Udp => {
            // TCP/UDP socket
            var addr = std.mem.zeroes(c.struct_sockaddr_in);
            addr.sin_family = c.AF_INET;
            addr.sin_port = c.htons(port);
            addr.sin_addr.s_addr = c.inet_addr(address);

            return c.connect(socket_fd, @as(*c.struct_sockaddr, @ptrCast(&addr)), @sizeOf(c.struct_sockaddr_in));
        },
        .Pgm, .Epgm, .Vmci => {
            // Not implemented
            return -1;
        },
    }
}

// Start listening on a socket
pub fn listenSocket(socket_fd: c_int) c_int {
    return c.listen(socket_fd, DEFAULT_BACKLOG);
}

// Send data over a socket
pub fn sendSocket(socket_fd: c_int, data: [*]const u8, size: usize, flags: c_int) c_int {
    return @intCast(c.send(socket_fd, data, size, flags));
}

// Server thread function prototype
pub const ServerThreadFn = fn (?*anyopaque) callconv(.C) ?*anyopaque;

// The listener thread function that accepts incoming connections
fn listenerThread(arg: ?*anyopaque) callconv(.C) ?*anyopaque {
    const endpoint = @as(*GooEndpoint, @ptrCast(arg.?));
    const server_socket = endpoint.socket_fd;

    if (server_socket < 0) {
        return null;
    }

    // Set the thread running flag
    endpoint.thread_running = true;

    // Accept loop
    while (endpoint.thread_running) {
        var client_addr: c.struct_sockaddr_storage = undefined;
        var addr_size: c.socklen_t = @sizeOf(c.struct_sockaddr_storage);

        const client_socket = c.accept(server_socket, @as(*c.struct_sockaddr, @ptrCast(&client_addr)), &addr_size);

        if (client_socket < 0) {
            if (c.errno == c.EAGAIN or c.errno == c.EWOULDBLOCK) {
                // No pending connections, sleep a bit
                _ = c.usleep(10000); // 10ms
                continue;
            } else {
                // Error accepting connection
                break;
            }
        }

        // Handle the new connection (this should be expanded based on channel type)
        _ = c.close(client_socket);
    }

    return null;
}

// Start a server endpoint
pub fn startServerEndpoint(endpoint: *GooEndpoint) c_int {
    if (endpoint.socket_fd < 0) {
        return -1;
    }

    // Create a thread for accepting connections
    const pthread_create_result = c.pthread_create(&endpoint.thread, null, listenerThread, endpoint);

    if (pthread_create_result != 0) {
        return -1;
    }

    // Set thread attributes if needed

    return 0;
}

// Stop a server endpoint
pub fn stopServerEndpoint(endpoint: *GooEndpoint) void {
    if (!endpoint.thread_running) {
        return;
    }

    // Signal the thread to stop
    endpoint.thread_running = false;

    // Wait for the thread to finish
    _ = c.pthread_join(endpoint.thread, @ptrCast(@alignCast(&@as(*anyopaque, null))));
}

// Parse an endpoint URL into protocol, address, and port
pub fn parseEndpointUrl(url: [*:0]const u8, protocol: *GooTransportProtocol, address: [*]u8, address_size: usize, port: *u16) bool {
    const url_str = std.mem.span(url);

    // Find the protocol separator
    const protocol_end = std.mem.indexOf(u8, url_str, "://") orelse return false;
    const proto_str = url_str[0..protocol_end];

    // Parse the protocol
    if (std.mem.eql(u8, proto_str, "inproc")) {
        protocol.* = .Inproc;
    } else if (std.mem.eql(u8, proto_str, "ipc")) {
        protocol.* = .Ipc;
    } else if (std.mem.eql(u8, proto_str, "tcp")) {
        protocol.* = .Tcp;
    } else if (std.mem.eql(u8, proto_str, "udp")) {
        protocol.* = .Udp;
    } else if (std.mem.eql(u8, proto_str, "pgm")) {
        protocol.* = .Pgm;
    } else if (std.mem.eql(u8, proto_str, "epgm")) {
        protocol.* = .Epgm;
    } else if (std.mem.eql(u8, proto_str, "vmci")) {
        protocol.* = .Vmci;
    } else {
        return false;
    }

    // Parse the address and port
    const address_start = protocol_end + 3;
    const remaining = url_str[address_start..];

    if (protocol.* == .Ipc or protocol.* == .Inproc) {
        // For IPC and inproc, the path is the address
        if (remaining.len >= address_size) {
            return false;
        }
        @memcpy(address[0..remaining.len], remaining);
        address[remaining.len] = 0;
        port.* = 0;
    } else {
        // For network protocols, parse address:port
        const port_sep = std.mem.indexOf(u8, remaining, ":") orelse return false;
        const addr_part = remaining[0..port_sep];
        const port_part = remaining[port_sep + 1 ..];

        if (addr_part.len >= address_size) {
            return false;
        }

        @memcpy(address[0..addr_part.len], addr_part);
        address[addr_part.len] = 0;

        port.* = std.fmt.parseInt(u16, port_part, 10) catch return false;
    }

    return true;
}

// Exported functions for C interface

// Connect a channel to a remote endpoint
export fn goo_channel_connect(channel: ?*GooChannel, protocol: c_int, address: [*:0]const u8, port: u16) c_int {
    if (channel == null) return -c.EINVAL;

    // Create a new endpoint
    const transport_protocol = @as(GooTransportProtocol, @enumFromInt(protocol));
    const endpoint = createEndpoint(transport_protocol, address, port, false);
    if (endpoint == null) return -c.ENOMEM;

    // Create a socket for the endpoint
    const options = SocketOptions{
        .nonblocking = true,
        .reuse_addr = true,
        .keep_alive = true,
    };
    const sock_fd = createSocket(transport_protocol, false, address, port, options) catch |err| {
        destroyEndpoint(endpoint.?);
        return @intFromError(err);
    };

    endpoint.?.socket_fd = sock_fd;

    // Connect the socket
    if (connectSocket(sock_fd, transport_protocol, address, port) < 0) {
        destroyEndpoint(endpoint.?);
        return -c.ECONNREFUSED;
    }

    // Set the channel as distributed
    channel.?.is_distributed = true;

    // Add the endpoint to the channel's endpoint list
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    endpoint.?.next = channel.?.endpoints;
    channel.?.endpoints = endpoint;

    return 0;
}

// Set a channel's endpoint for binding/listening
export fn goo_channel_set_endpoint(channel: ?*GooChannel, protocol: c_int, address: [*:0]const u8, port: u16) c_int {
    if (channel == null) return -c.EINVAL;

    // Create a new endpoint
    const transport_protocol = @as(GooTransportProtocol, @enumFromInt(protocol));
    const endpoint = createEndpoint(transport_protocol, address, port, true);
    if (endpoint == null) return -c.ENOMEM;

    // Create a socket for the endpoint
    const options = SocketOptions{
        .nonblocking = true,
        .reuse_addr = true,
        .keep_alive = true,
    };
    const sock_fd = createSocket(transport_protocol, true, address, port, options) catch |err| {
        destroyEndpoint(endpoint.?);
        return @intFromError(err);
    };

    endpoint.?.socket_fd = sock_fd;

    // Bind the socket
    if (bindSocket(sock_fd, transport_protocol, address, port) < 0) {
        destroyEndpoint(endpoint.?);
        return -c.EADDRINUSE;
    }

    // For server endpoints, start listening
    if (transport_protocol != .Udp) { // UDP doesn't need listening
        if (listenSocket(sock_fd) < 0) {
            destroyEndpoint(endpoint.?);
            return -c.EOPNOTSUPP;
        }
    }

    // Set the channel as distributed
    channel.?.is_distributed = true;

    // Add the endpoint to the channel's endpoint list
    _ = c.pthread_mutex_lock(&channel.?.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.?.mutex);

    endpoint.?.next = channel.?.endpoints;
    channel.?.endpoints = endpoint;

    return 0;
}

// Disconnect a channel from an endpoint
export fn goo_channel_disconnect(channel: ?*GooChannel, endpoint_url: [*:0]const u8) c_int {
    if (channel == null or endpoint_url[0] == 0) {
        return -c.EINVAL;
    }

    // Find the endpoint in the channel's list
    var prev: ?*GooEndpoint = null;
    var ep = channel.?.endpoints;

    while (ep != null) {
        const ep_url = buildEndpointUrl(ep.?.protocol, ep.?.address, ep.?.port);
        const matches = if (ep_url) |url| std.mem.eql(u8, std.mem.span(url), std.mem.span(endpoint_url)) else false;

        if (ep_url != null) {
            c.free(ep_url);
        }

        if (matches) {
            // Found the endpoint to disconnect
            if (prev == null) {
                channel.?.endpoints = ep.?.next;
            } else {
                prev.?.next = ep.?.next;
            }

            // Stop the server thread if running
            if (ep.?.is_server and ep.?.thread_running) {
                stopServerEndpoint(ep.?);
            }

            // Clean up and free the endpoint
            destroyEndpoint(ep.?);

            // If there are no more endpoints, mark as not distributed
            if (channel.?.endpoints == null) {
                channel.?.is_distributed = false;
            }

            return 0;
        }

        prev = ep;
        ep = ep.?.next;
    }

    // Endpoint not found
    return -c.ENOENT;
}

// Build an endpoint URL from components
fn buildEndpointUrl(protocol: GooTransportProtocol, address: [*:0]const u8, port: u16) ?[*:0]u8 {
    var proto_str: []const u8 = undefined;
    const include_port = switch (protocol) {
        .Inproc, .Ipc => false,
        else => true,
    };

    switch (protocol) {
        .Inproc => proto_str = "inproc",
        .Ipc => proto_str = "ipc",
        .Tcp => proto_str = "tcp",
        .Udp => proto_str = "udp",
        .Pgm => proto_str = "pgm",
        .Epgm => proto_str = "epgm",
        .Vmci => proto_str = "vmci",
    }

    // Allocate a buffer for the URL
    const addr_len = std.mem.len(address);
    const port_str_len: usize = 6; // Maximum length for ":65535\0"
    const url_len = proto_str.len + 3 + addr_len + (if (include_port) port_str_len else 0);

    const url = @as([*:0]u8, @ptrCast(c.malloc(url_len)));
    if (url == null) return null;

    if (include_port) {
        // Format with port
        _ = std.fmt.bufPrint(url[0..url_len], "{s}://{s}:{d}\x00", .{ proto_str, address, port }) catch {
            c.free(url);
            return null;
        };
    } else {
        // Format without port
        _ = std.fmt.bufPrint(url[0..url_len], "{s}://{s}\x00", .{ proto_str, address }) catch {
            c.free(url);
            return null;
        };
    }

    return url;
}

// Create a new message
fn createMessage(data: ?*anyopaque, size: usize) ?*GooMessage {
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
    msg.?.flags = 0;
    msg.?.priority = 0;

    return msg;
}

// Send a message to all endpoints attached to a channel
pub fn sendToEndpoints(channel: *GooChannel, msg: *GooMessage, flags: GooMessageFlags) c_int {
    if (channel.endpoint == null) {
        return -c.ENOTCONN;
    }

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.mutex);

    // Send to the primary endpoint
    const result = sendToSocket(channel.endpoint.?.socket_fd, msg.data, msg.size, @intFromEnum(flags));
    if (result < 0) {
        return result;
    }

    // Success
    return @intCast(result);
}

// Receive a message from any endpoint
pub fn recvFromEndpoints(channel: *GooChannel, out_msg: **GooMessage, flags: GooMessageFlags) c_int {
    if (channel.endpoint == null) {
        return -c.ENOTCONN;
    }

    // Lock the channel
    _ = c.pthread_mutex_lock(&channel.mutex);
    defer _ = c.pthread_mutex_unlock(&channel.mutex);

    // Allocate a temporary buffer for receiving
    const buffer = c.malloc(channel.elem_size);
    if (buffer == null) {
        return -c.ENOMEM;
    }
    defer c.free(buffer);

    // Receive from the primary endpoint
    const result = recvFromSocket(channel.endpoint.?.socket_fd, buffer, channel.elem_size, @intFromEnum(flags));
    if (result < 0) {
        return result;
    }

    // Create a new message with the received data
    const new_msg = createMessage(buffer, @intCast(result));
    if (new_msg == null) {
        return -c.ENOMEM;
    }

    // Set the output pointer - ensure we're dealing with a non-null pointer
    out_msg.* = new_msg;

    return @intCast(result);
}

// Send data to a socket
fn sendToSocket(socket_fd: c_int, data: *anyopaque, size: usize, flags: c_int) c_int {
    const bytes_sent = c.send(socket_fd, data, size, flags);
    if (bytes_sent < 0) {
        return -c.errno;
    }
    return @intCast(bytes_sent);
}

// Receive data from a socket
fn recvFromSocket(socket_fd: c_int, buffer: *anyopaque, size: usize, flags: c_int) c_int {
    const bytes_received = c.recv(socket_fd, buffer, size, flags);
    if (bytes_received < 0) {
        return -c.errno;
    }
    return @intCast(bytes_received);
}

pub fn createSocketTcp(is_server: bool, host: []const u8, port: u16, options: SocketOptions) !c_int {
    var socket_fd = c.socket(c.AF_INET, c.SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std.log.err("Socket creation failed: {s}", .{std.os.strerror(c.errno)});
        return error.SocketCreationFailed;
    }

    // Apply socket options
    if (options.reuse_addr) {
        const value: c_int = 1;
        if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_REUSEADDR, &value, @sizeOf(c_int)) < 0) {
            std.log.err("Failed to set SO_REUSEADDR: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    if (options.keep_alive) {
        const value: c_int = 1;
        if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_KEEPALIVE, &value, @sizeOf(c_int)) < 0) {
            std.log.err("Failed to set SO_KEEPALIVE: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    if (options.no_delay) {
        const value: c_int = 1;
        if (c.setsockopt(socket_fd, c.IPPROTO_TCP, c.TCP_NODELAY, &value, @sizeOf(c_int)) < 0) {
            std.log.err("Failed to set TCP_NODELAY: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    // Set receive timeout
    if (options.receive_timeout_ms > 0) {
        var tv = std.os.timeval{
            .tv_sec = @divTrunc(options.receive_timeout_ms, 1000),
            .tv_usec = @rem(options.receive_timeout_ms, 1000) * 1000,
        };
        if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_RCVTIMEO, &tv, @sizeOf(std.os.timeval)) < 0) {
            std.log.err("Failed to set SO_RCVTIMEO: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    // Set send timeout
    if (options.send_timeout_ms > 0) {
        var tv = std.os.timeval{
            .tv_sec = @divTrunc(options.send_timeout_ms, 1000),
            .tv_usec = @rem(options.send_timeout_ms, 1000) * 1000,
        };
        if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_SNDTIMEO, &tv, @sizeOf(std.os.timeval)) < 0) {
            std.log.err("Failed to set SO_SNDTIMEO: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    // Set non-blocking mode if requested
    if (options.nonblocking) {
        var flags = c.fcntl(socket_fd, c.F_GETFL, 0);
        if (flags < 0) {
            std.log.err("Failed to get socket flags: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
        flags |= c.O_NONBLOCK;
        if (c.fcntl(socket_fd, c.F_SETFL, flags) < 0) {
            std.log.err("Failed to set socket to non-blocking: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    if (is_server) {
        // For server, bind to the specified address and port
        var server_addr: c.sockaddr_in = undefined;
        std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_in));
        server_addr.sin_family = c.AF_INET;
        server_addr.sin_port = c.htons(port);

        if (std.mem.eql(u8, host, "*")) {
            // Bind to all interfaces
            server_addr.sin_addr.s_addr = c.INADDR_ANY;
        } else {
            // Bind to specific interface
            const c_host = @ptrCast([*]const u8, host.ptr);
            if (c.inet_pton(c.AF_INET, c_host, &server_addr.sin_addr) != 1) {
                std.log.err("Invalid address: {s}", .{host});
                _ = c.close(socket_fd);
                return error.InvalidAddress;
            }
        }

        if (c.bind(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @sizeOf(c.sockaddr_in)) < 0) {
            std.log.err("Bind failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.BindFailed;
        }

        if (c.listen(socket_fd, DEFAULT_BACKLOG) < 0) {
            std.log.err("Listen failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.ListenFailed;
        }
    } else {
        // For client, connect to the specified address and port
        var server_addr: c.sockaddr_in = undefined;
        std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_in));
        server_addr.sin_family = c.AF_INET;
        server_addr.sin_port = c.htons(port);

        const c_host = @ptrCast([*]const u8, host.ptr);
        if (c.inet_pton(c.AF_INET, c_host, &server_addr.sin_addr) != 1) {
            std.log.err("Invalid address: {s}", .{host});
            _ = c.close(socket_fd);
            return error.InvalidAddress;
        }

        if (c.connect(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @sizeOf(c.sockaddr_in)) < 0) {
            // For non-blocking sockets, EINPROGRESS is not an error
            if (options.nonblocking and c.errno == c.EINPROGRESS) {
                // Connection is in progress, which is normal for non-blocking sockets
                return socket_fd;
            }
            std.log.err("Connect failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.ConnectFailed;
        }
    }

    return socket_fd;
}

/// Creates a Unix domain socket for IPC communication
pub fn createSocketIpc(is_server: bool, path: []const u8, options: SocketOptions) !c_int {
    var socket_fd = c.socket(c.AF_UNIX, c.SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std.log.err("Socket creation failed: {s}", .{std.os.strerror(c.errno)});
        return error.SocketCreationFailed;
    }

    // Apply socket options
    if (options.reuse_addr) {
        const value: c_int = 1;
        if (c.setsockopt(socket_fd, c.SOL_SOCKET, c.SO_REUSEADDR, &value, @sizeOf(c_int)) < 0) {
            std.log.err("Failed to set SO_REUSEADDR: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    // Set non-blocking mode if requested
    if (options.nonblocking) {
        var flags = c.fcntl(socket_fd, c.F_GETFL, 0);
        if (flags < 0) {
            std.log.err("Failed to get socket flags: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
        flags |= c.O_NONBLOCK;
        if (c.fcntl(socket_fd, c.F_SETFL, flags) < 0) {
            std.log.err("Failed to set socket to non-blocking: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.SocketOptionFailed;
        }
    }

    if (is_server) {
        // For server, bind to the specified path
        var server_addr: c.sockaddr_un = undefined;
        std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_un));
        server_addr.sun_family = c.AF_UNIX;

        // Copy the path to the sun_path field
        if (path.len >= server_addr.sun_path.len) {
            std.log.err("Path too long for Unix domain socket", .{});
            _ = c.close(socket_fd);
            return error.PathTooLong;
        }

        std.mem.copy(u8, @ptrCast([*]u8, &server_addr.sun_path)[0..path.len], path);

        // Remove the socket file if it already exists
        const c_path = @ptrCast([*c]const u8, path.ptr);
        _ = c.unlink(c_path);

        const addr_size = @offsetOf(c.sockaddr_un, "sun_path") + path.len + 1;
        if (c.bind(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @intCast(c.socklen_t, addr_size)) < 0) {
            std.log.err("Bind failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.BindFailed;
        }

        if (c.listen(socket_fd, DEFAULT_BACKLOG) < 0) {
            std.log.err("Listen failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.ListenFailed;
        }
    } else {
        // For client, connect to the specified path
        var server_addr: c.sockaddr_un = undefined;
        std.mem.set(u8, std.mem.asBytes(&server_addr), 0, @sizeOf(c.sockaddr_un));
        server_addr.sun_family = c.AF_UNIX;

        // Copy the path to the sun_path field
        if (path.len >= server_addr.sun_path.len) {
            std.log.err("Path too long for Unix domain socket", .{});
            _ = c.close(socket_fd);
            return error.PathTooLong;
        }

        std.mem.copy(u8, @ptrCast([*]u8, &server_addr.sun_path)[0..path.len], path);

        const addr_size = @offsetOf(c.sockaddr_un, "sun_path") + path.len + 1;
        if (c.connect(socket_fd, @ptrCast(*const c.sockaddr, &server_addr), @intCast(c.socklen_t, addr_size)) < 0) {
            // For non-blocking sockets, EINPROGRESS is not an error
            if (options.nonblocking and c.errno == c.EINPROGRESS) {
                // Connection is in progress, which is normal for non-blocking sockets
                return socket_fd;
            }
            std.log.err("Connect failed: {s}", .{std.os.strerror(c.errno)});
            _ = c.close(socket_fd);
            return error.ConnectFailed;
        }
    }

    return socket_fd;
}

/// Simple message structure for in-process communication
pub const InprocMessage = struct {
    data: []u8,
    size: usize,
    flags: GooMessageFlags,
    next: ?*InprocMessage,

    pub fn create(data: []const u8, flags: GooMessageFlags) !*InprocMessage {
        // Allocate memory for the message and data
        const msg = try std.heap.c_allocator.create(InprocMessage);
        const data_copy = try std.heap.c_allocator.alloc(u8, data.len);
        std.mem.copy(u8, data_copy, data);

        msg.* = InprocMessage{
            .data = data_copy,
            .size = data.len,
            .flags = flags,
            .next = null,
        };

        return msg;
    }

    pub fn destroy(self: *InprocMessage) void {
        // Free the data and the message
        std.heap.c_allocator.free(self.data);
        std.heap.c_allocator.destroy(self);
    }
};

/// Queue for in-process message passing
pub const InprocQueue = struct {
    head: ?*InprocMessage,
    tail: ?*InprocMessage,
    count: usize,
    mutex: c.pthread_mutex_t,
    cond_empty: c.pthread_cond_t,
    cond_full: c.pthread_cond_t,
    capacity: usize,
    closed: bool,

    pub fn init(capacity: usize) !*InprocQueue {
        const queue = try std.heap.c_allocator.create(InprocQueue);

        // Initialize mutex and condition variables
        var mutex_attr: c.pthread_mutexattr_t = undefined;
        _ = c.pthread_mutexattr_init(&mutex_attr);
        _ = c.pthread_mutexattr_settype(&mutex_attr, c.PTHREAD_MUTEX_RECURSIVE);

        var mutex: c.pthread_mutex_t = undefined;
        var cond_empty: c.pthread_cond_t = undefined;
        var cond_full: c.pthread_cond_t = undefined;

        _ = c.pthread_mutex_init(&mutex, &mutex_attr);
        _ = c.pthread_mutexattr_destroy(&mutex_attr);

        _ = c.pthread_cond_init(&cond_empty, null);
        _ = c.pthread_cond_init(&cond_full, null);

        queue.* = InprocQueue{
            .head = null,
            .tail = null,
            .count = 0,
            .mutex = mutex,
            .cond_empty = cond_empty,
            .cond_full = cond_full,
            .capacity = capacity,
            .closed = false,
        };

        return queue;
    }

    pub fn destroy(self: *InprocQueue) void {
        // Free all messages in the queue
        _ = c.pthread_mutex_lock(&self.mutex);
        defer _ = c.pthread_mutex_unlock(&self.mutex);

        var current = self.head;
        while (current) |msg| {
            self.head = msg.next;
            msg.destroy();
            current = self.head;
        }

        // Destroy mutex and condition variables
        _ = c.pthread_cond_destroy(&self.cond_empty);
        _ = c.pthread_cond_destroy(&self.cond_full);
        _ = c.pthread_mutex_destroy(&self.mutex);

        // Free the queue itself
        std.heap.c_allocator.destroy(self);
    }

    pub fn enqueue(self: *InprocQueue, msg: *InprocMessage, blocking: bool, timeout_ms: u32) !bool {
        _ = c.pthread_mutex_lock(&self.mutex);
        defer _ = c.pthread_mutex_unlock(&self.mutex);

        // Check if queue is closed
        if (self.closed) {
            return error.QueueClosed;
        }

        // Check if queue is full
        if (self.count >= self.capacity) {
            if (!blocking) {
                return error.QueueFull;
            }

            // Wait for space to become available
            if (timeout_ms > 0) {
                var ts: c.timespec = undefined;
                _ = c.clock_gettime(c.CLOCK_REALTIME, &ts);

                ts.tv_sec += @divTrunc(timeout_ms, 1000);
                ts.tv_nsec += @intCast(c_long, (@rem(timeout_ms, 1000) * 1000000));
                if (ts.tv_nsec >= 1000000000) {
                    ts.tv_sec += 1;
                    ts.tv_nsec -= 1000000000;
                }

                const wait_result = c.pthread_cond_timedwait(&self.cond_full, &self.mutex, &ts);
                if (wait_result == c.ETIMEDOUT) {
                    return error.Timeout;
                }
            } else {
                _ = c.pthread_cond_wait(&self.cond_full, &self.mutex);
            }

            // Check again if queue is closed after waiting
            if (self.closed) {
                return error.QueueClosed;
            }

            // Check if queue is still full after waiting
            if (self.count >= self.capacity) {
                return error.QueueFull;
            }
        }

        // Add message to the queue
        msg.next = null;
        if (self.tail) |tail| {
            tail.next = msg;
        } else {
            self.head = msg;
        }
        self.tail = msg;
        self.count += 1;

        // Signal that the queue is not empty
        _ = c.pthread_cond_signal(&self.cond_empty);

        return true;
    }

    pub fn dequeue(self: *InprocQueue, blocking: bool, timeout_ms: u32) !?*InprocMessage {
        _ = c.pthread_mutex_lock(&self.mutex);
        defer _ = c.pthread_mutex_unlock(&self.mutex);

        // Check if queue is empty
        if (self.head == null) {
            // If closed and empty, return null
            if (self.closed) {
                return null;
            }

            if (!blocking) {
                return error.QueueEmpty;
            }

            // Wait for a message to arrive
            if (timeout_ms > 0) {
                var ts: c.timespec = undefined;
                _ = c.clock_gettime(c.CLOCK_REALTIME, &ts);

                ts.tv_sec += @divTrunc(timeout_ms, 1000);
                ts.tv_nsec += @intCast(c_long, (@rem(timeout_ms, 1000) * 1000000));
                if (ts.tv_nsec >= 1000000000) {
                    ts.tv_sec += 1;
                    ts.tv_nsec -= 1000000000;
                }

                const wait_result = c.pthread_cond_timedwait(&self.cond_empty, &self.mutex, &ts);
                if (wait_result == c.ETIMEDOUT) {
                    return error.Timeout;
                }
            } else {
                _ = c.pthread_cond_wait(&self.cond_empty, &self.mutex);
            }

            // Check if queue is still empty after waiting
            if (self.head == null) {
                if (self.closed) {
                    return null;
                }
                return error.QueueEmpty;
            }
        }

        // Remove message from the queue
        const msg = self.head.?;
        self.head = msg.next;
        if (self.head == null) {
            self.tail = null;
        }
        self.count -= 1;

        // Signal that the queue is not full
        _ = c.pthread_cond_signal(&self.cond_full);

        return msg;
    }

    pub fn close(self: *InprocQueue) void {
        _ = c.pthread_mutex_lock(&self.mutex);
        defer _ = c.pthread_mutex_unlock(&self.mutex);

        self.closed = true;

        // Signal all waiting consumers and producers
        _ = c.pthread_cond_broadcast(&self.cond_empty);
        _ = c.pthread_cond_broadcast(&self.cond_full);
    }
};

/// Map of named in-process endpoints
var inproc_endpoints = std.StringHashMap(*InprocQueue).init(std.heap.c_allocator);
var inproc_mutex: c.pthread_mutex_t = undefined;
var inproc_initialized = false;

/// Initialize the in-process transport system
pub fn initializeInprocTransport() !void {
    if (inproc_initialized) return;

    var mutex_attr: c.pthread_mutexattr_t = undefined;
    _ = c.pthread_mutexattr_init(&mutex_attr);
    _ = c.pthread_mutexattr_settype(&mutex_attr, c.PTHREAD_MUTEX_RECURSIVE);
    _ = c.pthread_mutex_init(&inproc_mutex, &mutex_attr);
    _ = c.pthread_mutexattr_destroy(&mutex_attr);

    inproc_initialized = true;
}

/// Clean up the in-process transport system
pub fn cleanupInprocTransport() void {
    if (!inproc_initialized) return;

    _ = c.pthread_mutex_lock(&inproc_mutex);
    defer _ = c.pthread_mutex_unlock(&inproc_mutex);

    // Destroy all endpoints
    var it = inproc_endpoints.iterator();
    while (it.next()) |entry| {
        entry.value_ptr.*.destroy();
    }

    inproc_endpoints.deinit();
    _ = c.pthread_mutex_destroy(&inproc_mutex);
    inproc_initialized = false;
}

/// Create or get an in-process endpoint
pub fn getInprocEndpoint(name: []const u8, is_server: bool, capacity: usize) !*InprocQueue {
    if (!inproc_initialized) try initializeInprocTransport();

    _ = c.pthread_mutex_lock(&inproc_mutex);
    defer _ = c.pthread_mutex_unlock(&inproc_mutex);

    // Check if endpoint already exists
    if (inproc_endpoints.get(name)) |endpoint| {
        if (is_server) {
            return error.EndpointAlreadyExists;
        }
        return endpoint;
    }

    // If client and endpoint doesn't exist, return error
    if (!is_server) {
        return error.EndpointNotFound;
    }

    // Create a new endpoint
    const endpoint = try InprocQueue.init(capacity);
    try inproc_endpoints.put(try std.heap.c_allocator.dupe(u8, name), endpoint);

    return endpoint;
}

/// Remove an in-process endpoint
pub fn removeInprocEndpoint(name: []const u8) !void {
    if (!inproc_initialized) return;

    _ = c.pthread_mutex_lock(&inproc_mutex);
    defer _ = c.pthread_mutex_unlock(&inproc_mutex);

    // Check if endpoint exists
    if (inproc_endpoints.get(name)) |endpoint| {
        // Close and destroy the endpoint
        endpoint.close();
        endpoint.destroy();

        // Remove from map
        _ = inproc_endpoints.remove(name);
    }
}

// Create a function to initialize transport
pub fn initializeTransport() !void {
    try initializeInprocTransport();
    // Any other transport initialization would go here
}

// Create a function to clean up transport
pub fn cleanupTransport() void {
    cleanupInprocTransport();
    // Any other transport cleanup would go here
}

// Create a function to open a channel
pub fn openChannel(endpoint: *GooEndpoint, options: *SocketOptions) !void {
    // Create the socket based on the transport protocol
    if (endpoint.protocol == .Inproc) {
        // For in-process communication, set up the queue
        const capacity = 100; // Default capacity, should come from channel options
        endpoint.queue = try getInprocEndpoint(endpoint.address, endpoint.is_server, capacity);
        return;
    }

    // For network transports, create a socket
    endpoint.socket_fd = try createSocket(endpoint.protocol, endpoint.is_server, endpoint.address, endpoint.port, options.*);
}

// Create a function to close a channel
pub fn closeChannel(endpoint: *GooEndpoint) void {
    if (endpoint.protocol == .Inproc) {
        if (endpoint.is_server) {
            // Only servers should remove the endpoint
            removeInprocEndpoint(endpoint.address) catch {};
        }
        return;
    }

    if (endpoint.socket_fd >= 0) {
        _ = c.close(endpoint.socket_fd);
        endpoint.socket_fd = -1;
    }
}

// Create a function to send a message
pub fn sendMessage(endpoint: *GooEndpoint, data: []const u8, flags: GooMessageFlags) !usize {
    if (endpoint.protocol == .Inproc) {
        if (endpoint.queue) |queue| {
            const msg = try InprocMessage.create(data, flags);
            _ = try queue.enqueue(msg, true, 0);
            return data.len;
        }
        return error.QueueNotInitialized;
    }

    if (endpoint.socket_fd < 0) {
        return error.SocketNotConnected;
    }

    var c_flags: c_int = 0;
    if (flags & GooMessageFlags.DontWait != 0) {
        c_flags |= MSG_DONTWAIT;
    }

    const bytes_sent = c.send(endpoint.socket_fd, data.ptr, data.len, c_flags);
    if (bytes_sent < 0) {
        const err = c.errno;
        if (err == c.EAGAIN or err == c.EWOULDBLOCK) {
            return error.WouldBlock;
        }
        std.log.err("Send failed: {s}", .{std.os.strerror(err)});
        return error.SendFailed;
    }

    return @intCast(usize, bytes_sent);
}

// Create a function to receive a message
pub fn receiveMessage(endpoint: *GooEndpoint, buffer: []u8, flags: GooMessageFlags) !usize {
    if (endpoint.protocol == .Inproc) {
        if (endpoint.queue) |queue| {
            const msg = try queue.dequeue(true, 0) orelse return 0;
            defer msg.destroy();

            const copy_len = @min(buffer.len, msg.size);
            std.mem.copy(u8, buffer[0..copy_len], msg.data[0..copy_len]);
            return copy_len;
        }
        return error.QueueNotInitialized;
    }

    if (endpoint.socket_fd < 0) {
        return error.SocketNotConnected;
    }

    var c_flags: c_int = 0;
    if (flags & GooMessageFlags.DontWait != 0) {
        c_flags |= MSG_DONTWAIT;
    }

    const bytes_received = c.recv(endpoint.socket_fd, buffer.ptr, buffer.len, c_flags);
    if (bytes_received < 0) {
        const err = c.errno;
        if (err == c.EAGAIN or err == c.EWOULDBLOCK) {
            return error.WouldBlock;
        }
        std.log.err("Receive failed: {s}", .{std.os.strerror(err)});
        return error.ReceiveFailed;
    }

    return @intCast(usize, bytes_received);
}
