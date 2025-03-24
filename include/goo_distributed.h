#ifndef GOO_DISTRIBUTED_H
#define GOO_DISTRIBUTED_H

#include <stdbool.h>
#include "goo_runtime.h"

// Protocol types for endpoints
typedef enum {
    GOO_PROTOCOL_INPROC = 0,  // In-process
    GOO_PROTOCOL_TCP,         // TCP/IP
    GOO_PROTOCOL_UDP,         // UDP
    GOO_PROTOCOL_IPC,         // Inter-process communication
    GOO_PROTOCOL_PGM,         // Pragmatic General Multicast
    GOO_PROTOCOL_EPGM         // Encapsulated PGM
} GooProtocol;

// Endpoint structure
typedef struct {
    GooProtocol protocol;
    char* address;
    int port;
    bool is_server;
    int socket_fd;
    bool thread_running;
} GooEndpoint;

// Parse an endpoint URL (protocol://address:port)
GooEndpoint* goo_endpoint_parse(const char* endpoint_url);

// Free an endpoint
void goo_endpoint_free(GooEndpoint* endpoint);

// Initialize the socket for an endpoint
bool goo_endpoint_init_socket(GooEndpoint* endpoint);

// Set up the endpoint for a channel
bool goo_channel_set_endpoint(GooChannel* channel, const char* endpoint_url);

// Send data to a remote endpoint
bool goo_channel_send_to_endpoint(GooChannel* channel, void* data);

// Enhanced channel send function that also sends to the endpoint
bool goo_distributed_channel_send(GooChannel* channel, void* data);

#endif // GOO_DISTRIBUTED_H 