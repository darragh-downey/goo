#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/un.h>
#include <netdb.h>

#include "goo_channels.h"
#include "goo_pgm.h"

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

// Transport endpoint
typedef struct {
    GooTransportProtocol protocol;
    int socket;
    struct sockaddr_storage addr;
    socklen_t addr_len;
    bool is_bound;
    bool is_connected;
    pthread_mutex_t mutex;
    char* endpoint_str;
} GooTransportEndpoint;

// Create a new transport endpoint
GooTransportEndpoint* goo_transport_create(GooTransportProtocol protocol) {
    GooTransportEndpoint* endpoint = (GooTransportEndpoint*)malloc(sizeof(GooTransportEndpoint));
    if (!endpoint) {
        return NULL;
    }
    
    endpoint->protocol = protocol;
    endpoint->socket = -1;
    endpoint->is_bound = false;
    endpoint->is_connected = false;
    endpoint->endpoint_str = NULL;
    
    if (pthread_mutex_init(&endpoint->mutex, NULL) != 0) {
        free(endpoint);
        return NULL;
    }
    
    // Initialize the socket based on protocol
    switch (protocol) {
        case GOO_PROTO_INPROC:
            // In-process doesn't need a socket
            endpoint->socket = 0;
            break;
            
        case GOO_PROTO_IPC:
            endpoint->socket = socket(AF_UNIX, SOCK_STREAM, 0);
            break;
            
        case GOO_PROTO_TCP:
            endpoint->socket = socket(AF_INET, SOCK_STREAM, 0);
            break;
            
        case GOO_PROTO_UDP:
            endpoint->socket = socket(AF_INET, SOCK_DGRAM, 0);
            break;
            
        case GOO_PROTO_PGM:
        case GOO_PROTO_EPGM:
            endpoint->socket = goo_pgm_socket_create(protocol == GOO_PROTO_EPGM);
            break;
            
        case GOO_PROTO_VMCI:
            // VMCI not implemented yet
            free(endpoint);
            return NULL;
            
        default:
            free(endpoint);
            return NULL;
    }
    
    if (endpoint->socket < 0 && protocol != GOO_PROTO_INPROC) {
        pthread_mutex_destroy(&endpoint->mutex);
        free(endpoint);
        return NULL;
    }
    
    return endpoint;
}

// Destroy a transport endpoint
void goo_transport_destroy(GooTransportEndpoint* endpoint) {
    if (!endpoint) return;
    
    pthread_mutex_lock(&endpoint->mutex);
    
    if (endpoint->socket >= 0 && endpoint->protocol != GOO_PROTO_INPROC) {
        close(endpoint->socket);
    }
    
    if (endpoint->endpoint_str) {
        free(endpoint->endpoint_str);
    }
    
    pthread_mutex_unlock(&endpoint->mutex);
    pthread_mutex_destroy(&endpoint->mutex);
    
    free(endpoint);
}

// Bind to an address
bool goo_transport_bind(GooTransportEndpoint* endpoint, const char* address, int port) {
    if (!endpoint) return false;
    
    pthread_mutex_lock(&endpoint->mutex);
    
    bool success = false;
    
    switch (endpoint->protocol) {
        case GOO_PROTO_INPROC: {
            // For in-process, we just store the identifier
            if (endpoint->endpoint_str) free(endpoint->endpoint_str);
            endpoint->endpoint_str = strdup(address);
            success = (endpoint->endpoint_str != NULL);
            break;
        }
        
        case GOO_PROTO_IPC: {
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, address, sizeof(addr.sun_path) - 1);
            
            // Remove existing socket file if it exists
            unlink(address);
            
            if (bind(endpoint->socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                listen(endpoint->socket, 10);
                memcpy(&endpoint->addr, &addr, sizeof(addr));
                endpoint->addr_len = sizeof(addr);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(address);
                
                success = true;
            }
            break;
        }
        
        case GOO_PROTO_TCP:
        case GOO_PROTO_UDP: {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            
            // If address is "*", bind to any address
            if (strcmp(address, "*") == 0) {
                addr.sin_addr.s_addr = INADDR_ANY;
            } else {
                inet_pton(AF_INET, address, &addr.sin_addr);
            }
            
            if (bind(endpoint->socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                if (endpoint->protocol == GOO_PROTO_TCP) {
                    listen(endpoint->socket, 10);
                }
                
                memcpy(&endpoint->addr, &addr, sizeof(addr));
                endpoint->addr_len = sizeof(addr);
                
                // Format endpoint string
                char endpoint_str[128];
                snprintf(endpoint_str, sizeof(endpoint_str), "%s://%s:%d", 
                         endpoint->protocol == GOO_PROTO_TCP ? "tcp" : "udp",
                         address, port);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(endpoint_str);
                
                success = true;
            }
            break;
        }
        
        case GOO_PROTO_PGM:
        case GOO_PROTO_EPGM: {
            success = goo_pgm_socket_bind(endpoint->socket, address, port);
            
            if (success) {
                // Format endpoint string
                char endpoint_str[128];
                snprintf(endpoint_str, sizeof(endpoint_str), "%s://%s:%d", 
                         endpoint->protocol == GOO_PROTO_PGM ? "pgm" : "epgm",
                         address, port);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(endpoint_str);
            }
            break;
        }
        
        default:
            break;
    }
    
    if (success) {
        endpoint->is_bound = true;
    }
    
    pthread_mutex_unlock(&endpoint->mutex);
    return success;
}

// Connect to an address
bool goo_transport_connect(GooTransportEndpoint* endpoint, const char* address, int port) {
    if (!endpoint) return false;
    
    pthread_mutex_lock(&endpoint->mutex);
    
    bool success = false;
    
    switch (endpoint->protocol) {
        case GOO_PROTO_INPROC: {
            // For in-process, we just store the identifier
            if (endpoint->endpoint_str) free(endpoint->endpoint_str);
            endpoint->endpoint_str = strdup(address);
            success = (endpoint->endpoint_str != NULL);
            break;
        }
        
        case GOO_PROTO_IPC: {
            struct sockaddr_un addr;
            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, address, sizeof(addr.sun_path) - 1);
            
            if (connect(endpoint->socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                memcpy(&endpoint->addr, &addr, sizeof(addr));
                endpoint->addr_len = sizeof(addr);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(address);
                
                success = true;
            }
            break;
        }
        
        case GOO_PROTO_TCP:
        case GOO_PROTO_UDP: {
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            
            // Resolve hostname to IP if needed
            struct hostent *he = gethostbyname(address);
            if (he) {
                memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
            } else {
                inet_pton(AF_INET, address, &addr.sin_addr);
            }
            
            if (connect(endpoint->socket, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
                memcpy(&endpoint->addr, &addr, sizeof(addr));
                endpoint->addr_len = sizeof(addr);
                
                // Format endpoint string
                char endpoint_str[128];
                snprintf(endpoint_str, sizeof(endpoint_str), "%s://%s:%d", 
                         endpoint->protocol == GOO_PROTO_TCP ? "tcp" : "udp",
                         address, port);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(endpoint_str);
                
                success = true;
            }
            break;
        }
        
        case GOO_PROTO_PGM:
        case GOO_PROTO_EPGM: {
            success = goo_pgm_socket_connect(endpoint->socket, address, port);
            
            if (success) {
                // Format endpoint string
                char endpoint_str[128];
                snprintf(endpoint_str, sizeof(endpoint_str), "%s://%s:%d", 
                         endpoint->protocol == GOO_PROTO_PGM ? "pgm" : "epgm",
                         address, port);
                
                if (endpoint->endpoint_str) free(endpoint->endpoint_str);
                endpoint->endpoint_str = strdup(endpoint_str);
            }
            break;
        }
        
        default:
            break;
    }
    
    if (success) {
        endpoint->is_connected = true;
    }
    
    pthread_mutex_unlock(&endpoint->mutex);
    return success;
}

// Send data through the transport
int goo_transport_send(GooTransportEndpoint* endpoint, const void* data, size_t size) {
    if (!endpoint || !data || size <= 0) return -1;
    
    pthread_mutex_lock(&endpoint->mutex);
    
    int sent = -1;
    
    switch (endpoint->protocol) {
        case GOO_PROTO_INPROC:
            // In-process sends should be handled by the channel directly
            sent = (int)size;
            break;
            
        case GOO_PROTO_IPC:
        case GOO_PROTO_TCP:
            sent = send(endpoint->socket, data, size, 0);
            break;
            
        case GOO_PROTO_UDP:
            if (endpoint->is_connected) {
                sent = send(endpoint->socket, data, size, 0);
            } else if (endpoint->is_bound) {
                // For UDP servers, we need a destination address
                // This would be set on a per-message basis
                sent = -1;
            }
            break;
            
        case GOO_PROTO_PGM:
        case GOO_PROTO_EPGM:
            sent = goo_pgm_socket_send(endpoint->socket, data, size);
            break;
            
        default:
            break;
    }
    
    pthread_mutex_unlock(&endpoint->mutex);
    return sent;
}

// Receive data from the transport
int goo_transport_recv(GooTransportEndpoint* endpoint, void* data, size_t size) {
    if (!endpoint || !data || size <= 0) return -1;
    
    pthread_mutex_lock(&endpoint->mutex);
    
    int received = -1;
    
    switch (endpoint->protocol) {
        case GOO_PROTO_INPROC:
            // In-process receives should be handled by the channel directly
            received = -1;
            break;
            
        case GOO_PROTO_IPC:
        case GOO_PROTO_TCP:
        case GOO_PROTO_UDP:
            received = recv(endpoint->socket, data, size, 0);
            break;
            
        case GOO_PROTO_PGM:
        case GOO_PROTO_EPGM:
            received = goo_pgm_socket_recv(endpoint->socket, data, size);
            break;
            
        default:
            break;
    }
    
    pthread_mutex_unlock(&endpoint->mutex);
    return received;
}

// Parse an endpoint string into protocol, address and port
bool goo_transport_parse_endpoint(const char* endpoint_str, 
                                 GooTransportProtocol* protocol_out,
                                 char* address_out, size_t address_size,
                                 int* port_out) {
    if (!endpoint_str || !protocol_out || !address_out || !port_out) {
        return false;
    }
    
    // Parse protocol
    const char* proto_end = strstr(endpoint_str, "://");
    if (!proto_end) {
        return false;
    }
    
    size_t proto_len = proto_end - endpoint_str;
    if (strncmp(endpoint_str, "inproc", proto_len) == 0) {
        *protocol_out = GOO_PROTO_INPROC;
    } else if (strncmp(endpoint_str, "ipc", proto_len) == 0) {
        *protocol_out = GOO_PROTO_IPC;
    } else if (strncmp(endpoint_str, "tcp", proto_len) == 0) {
        *protocol_out = GOO_PROTO_TCP;
    } else if (strncmp(endpoint_str, "udp", proto_len) == 0) {
        *protocol_out = GOO_PROTO_UDP;
    } else if (strncmp(endpoint_str, "pgm", proto_len) == 0) {
        *protocol_out = GOO_PROTO_PGM;
    } else if (strncmp(endpoint_str, "epgm", proto_len) == 0) {
        *protocol_out = GOO_PROTO_EPGM;
    } else if (strncmp(endpoint_str, "vmci", proto_len) == 0) {
        *protocol_out = GOO_PROTO_VMCI;
    } else {
        return false;
    }
    
    // Move past "://"
    const char* addr_start = proto_end + 3;
    
    // For inproc and ipc, the rest is just the path/identifier
    if (*protocol_out == GOO_PROTO_INPROC || *protocol_out == GOO_PROTO_IPC) {
        strncpy(address_out, addr_start, address_size - 1);
        address_out[address_size - 1] = '\0';
        *port_out = 0;
        return true;
    }
    
    // For other protocols, parse address and port
    const char* port_start = strrchr(addr_start, ':');
    if (!port_start) {
        return false;
    }
    
    // Copy address
    size_t addr_len = port_start - addr_start;
    if (addr_len >= address_size) {
        addr_len = address_size - 1;
    }
    strncpy(address_out, addr_start, addr_len);
    address_out[addr_len] = '\0';
    
    // Parse port
    *port_out = atoi(port_start + 1);
    
    return true;
}

// Get the endpoint string
const char* goo_transport_get_endpoint_string(GooTransportEndpoint* endpoint) {
    if (!endpoint) return NULL;
    return endpoint->endpoint_str;
}

// Set non-blocking mode
bool goo_transport_set_nonblocking(GooTransportEndpoint* endpoint, bool nonblocking) {
    if (!endpoint || endpoint->protocol == GOO_PROTO_INPROC) {
        return false;
    }
    
    int flags = fcntl(endpoint->socket, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    return fcntl(endpoint->socket, F_SETFL, flags) != -1;
}

// Set timeout for operations
bool goo_transport_set_timeout(GooTransportEndpoint* endpoint, int timeout_ms) {
    if (!endpoint || endpoint->protocol == GOO_PROTO_INPROC) {
        return false;
    }
    
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    return (setsockopt(endpoint->socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0 &&
            setsockopt(endpoint->socket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == 0);
} 