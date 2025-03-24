#ifndef GOO_TRANSPORT_H
#define GOO_TRANSPORT_H

#include <stdlib.h>
#include <stdbool.h>

// Transport protocols
typedef enum {
    GOO_PROTO_INPROC = 0,  // In-process communication
    GOO_PROTO_IPC,         // Inter-process communication
    GOO_PROTO_TCP,         // TCP/IP
    GOO_PROTO_UDP,         // UDP
    GOO_PROTO_PGM,         // Pragmatic General Multicast
    GOO_PROTO_EPGM,        // Encapsulated PGM
    GOO_PROTO_VMCI         // Virtual Machine Communication Interface
} GooTransportProtocol;

// Opaque transport endpoint
typedef struct GooTransportEndpoint GooTransportEndpoint;

// Create a new transport endpoint
GooTransportEndpoint* goo_transport_create(GooTransportProtocol protocol);

// Destroy a transport endpoint
void goo_transport_destroy(GooTransportEndpoint* endpoint);

// Bind to an address
bool goo_transport_bind(GooTransportEndpoint* endpoint, const char* address, int port);

// Connect to an address
bool goo_transport_connect(GooTransportEndpoint* endpoint, const char* address, int port);

// Send data through the transport
int goo_transport_send(GooTransportEndpoint* endpoint, const void* data, size_t size);

// Receive data from the transport
int goo_transport_recv(GooTransportEndpoint* endpoint, void* data, size_t size);

// Parse an endpoint string into protocol, address and port
bool goo_transport_parse_endpoint(const char* endpoint_str, 
                                 GooTransportProtocol* protocol_out,
                                 char* address_out, size_t address_size,
                                 int* port_out);

// Get the endpoint string
const char* goo_transport_get_endpoint_string(GooTransportEndpoint* endpoint);

// Set non-blocking mode
bool goo_transport_set_nonblocking(GooTransportEndpoint* endpoint, bool nonblocking);

// Set timeout for operations
bool goo_transport_set_timeout(GooTransportEndpoint* endpoint, int timeout_ms);

#endif // GOO_TRANSPORT_H 