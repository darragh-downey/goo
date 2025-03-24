#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#include "goo_runtime.h"
#include "goo_distributed.h"
#include "messaging/goo_pgm.h"

// ===== Endpoint Parsing =====

// Parse an endpoint URL (protocol://address:port)
GooEndpoint* goo_endpoint_parse(const char* endpoint_url) {
    if (!endpoint_url) return NULL;
    
    GooEndpoint* endpoint = (GooEndpoint*)malloc(sizeof(GooEndpoint));
    if (!endpoint) {
        perror("Failed to allocate memory for endpoint");
        return NULL;
    }
    
    // Initialize default values
    endpoint->protocol = GOO_PROTOCOL_INPROC;
    endpoint->address = NULL;
    endpoint->port = 0;
    endpoint->is_server = false;
    endpoint->socket_fd = -1;
    endpoint->thread_running = false;
    
    // Make a copy of the endpoint URL for parsing
    char* url_copy = strdup(endpoint_url);
    if (!url_copy) {
        perror("Failed to allocate memory for endpoint URL copy");
        free(endpoint);
        return NULL;
    }
    
    // Parse protocol
    char* protocol_str = url_copy;
    char* address_str = strstr(url_copy, "://");
    
    if (!address_str) {
        fprintf(stderr, "Invalid endpoint URL format: %s\n", endpoint_url);
        free(url_copy);
        free(endpoint);
        return NULL;
    }
    
    // Null-terminate the protocol string
    *address_str = '\0';
    address_str += 3; // Skip over "://"
    
    // Determine the protocol
    if (strcmp(protocol_str, "inproc") == 0) {
        endpoint->protocol = GOO_PROTOCOL_INPROC;
    } else if (strcmp(protocol_str, "tcp") == 0) {
        endpoint->protocol = GOO_PROTOCOL_TCP;
    } else if (strcmp(protocol_str, "udp") == 0) {
        endpoint->protocol = GOO_PROTOCOL_UDP;
    } else if (strcmp(protocol_str, "ipc") == 0) {
        endpoint->protocol = GOO_PROTOCOL_IPC;
    } else if (strcmp(protocol_str, "pgm") == 0) {
        endpoint->protocol = GOO_PROTOCOL_PGM;
    } else if (strcmp(protocol_str, "epgm") == 0) {
        endpoint->protocol = GOO_PROTOCOL_EPGM;
    } else {
        fprintf(stderr, "Unsupported protocol: %s\n", protocol_str);
        free(url_copy);
        free(endpoint);
        return NULL;
    }
    
    // Parse address and port
    char* port_str = strrchr(address_str, ':');
    if (!port_str && endpoint->protocol != GOO_PROTOCOL_INPROC && 
        endpoint->protocol != GOO_PROTOCOL_IPC) {
        fprintf(stderr, "Invalid endpoint URL format (missing port): %s\n", endpoint_url);
        free(url_copy);
        free(endpoint);
        return NULL;
    }
    
    // Check if this is a server endpoint (address starts with *)
    if (address_str[0] == '*' && address_str[1] == ':') {
        endpoint->is_server = true;
        // Skip the * and :
        port_str = address_str + 1;
    }
    
    if (port_str) {
        // Null-terminate the address string
        *port_str = '\0';
        port_str++; // Skip over ":"
        
        // Parse the port
        endpoint->port = atoi(port_str);
        if (endpoint->port <= 0 || endpoint->port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", port_str);
            free(url_copy);
            free(endpoint);
            return NULL;
        }
    }
    
    // Copy the address
    if (endpoint->is_server) {
        endpoint->address = strdup("0.0.0.0"); // Listen on all interfaces
    } else {
        endpoint->address = strdup(address_str);
    }
    
    if (!endpoint->address) {
        perror("Failed to allocate memory for endpoint address");
        free(url_copy);
        free(endpoint);
        return NULL;
    }
    
    free(url_copy);
    return endpoint;
}

// Free an endpoint
void goo_endpoint_free(GooEndpoint* endpoint) {
    if (!endpoint) return;
    
    if (endpoint->socket_fd >= 0) {
        close(endpoint->socket_fd);
    }
    
    free(endpoint->address);
    free(endpoint);
}

// ===== Socket Setup =====

// Create a socket for a TCP server endpoint
static int create_tcp_server_socket(GooEndpoint* endpoint) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create TCP server socket");
        return -1;
    }
    
    // Allow address reuse
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Failed to set socket options");
        close(sockfd);
        return -1;
    }
    
    // Bind to the address and port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(endpoint->port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind server socket");
        close(sockfd);
        return -1;
    }
    
    // Start listening
    if (listen(sockfd, 10) < 0) {
        perror("Failed to listen on server socket");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Create a socket for a TCP client endpoint
static int create_tcp_client_socket(GooEndpoint* endpoint) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create TCP client socket");
        return -1;
    }
    
    // Resolve the address
    struct hostent* server = gethostbyname(endpoint->address);
    if (server == NULL) {
        fprintf(stderr, "Failed to resolve host: %s\n", endpoint->address);
        close(sockfd);
        return -1;
    }
    
    // Connect to the server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(endpoint->port);
    
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to connect to server");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Create a socket for a UDP endpoint
static int create_udp_socket(GooEndpoint* endpoint) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Failed to create UDP socket");
        return -1;
    }
    
    // For server endpoints, bind to the port
    if (endpoint->is_server) {
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(endpoint->port);
        
        if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Failed to bind UDP socket");
            close(sockfd);
            return -1;
        }
    }
    
    return sockfd;
}

// Create a socket for an IPC endpoint
static int create_ipc_socket(GooEndpoint* endpoint) {
    // IPC is not implemented in this example
    // Would use Unix domain sockets for local IPC
    fprintf(stderr, "IPC protocol not implemented yet\n");
    return -1;
}

// Initialize the socket for an endpoint
bool goo_endpoint_init_socket(GooEndpoint* endpoint) {
    if (!endpoint) return false;
    
    switch (endpoint->protocol) {
        case GOO_PROTOCOL_TCP:
            if (endpoint->is_server) {
                endpoint->socket_fd = create_tcp_server_socket(endpoint);
            } else {
                endpoint->socket_fd = create_tcp_client_socket(endpoint);
            }
            break;
            
        case GOO_PROTOCOL_UDP:
            endpoint->socket_fd = create_udp_socket(endpoint);
            break;
            
        case GOO_PROTOCOL_IPC:
            endpoint->socket_fd = create_ipc_socket(endpoint);
            break;
            
        case GOO_PROTOCOL_PGM:
            return goo_endpoint_init_pgm(endpoint, false);
            
        case GOO_PROTOCOL_EPGM:
            return goo_endpoint_init_pgm(endpoint, true);
            
        case GOO_PROTOCOL_INPROC:
            // In-process channels don't need sockets
            return true;
            
        default:
            fprintf(stderr, "Unsupported protocol\n");
            return false;
    }
    
    return endpoint->socket_fd >= 0;
}

// ===== Channel I/O =====

// Structure for a message packet
typedef struct {
    uint32_t size;      // Size of the data
    char data[];        // Flexible array member for the data
} GooMessagePacket;

// Thread function for TCP server
static void* tcp_server_thread(void* arg) {
    GooChannel* channel = (GooChannel*)arg;
    GooEndpoint* endpoint = (GooEndpoint*)channel->endpoint;
    
    endpoint->thread_running = true;
    
    while (true) {
        // Accept a new connection
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(endpoint->socket_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EINTR) continue; // Interrupted, try again
            perror("Failed to accept connection");
            break;
        }
        
        // Handle the connection - read data and put it in the channel
        uint32_t size;
        if (recv(client_fd, &size, sizeof(size), 0) != sizeof(size)) {
            perror("Failed to read message size");
            close(client_fd);
            continue;
        }
        
        // Convert from network byte order
        size = ntohl(size);
        
        // Allocate buffer for the message
        void* buffer = malloc(channel->element_size);
        if (!buffer) {
            perror("Failed to allocate memory for message");
            close(client_fd);
            continue;
        }
        
        // Read the message data
        if (recv(client_fd, buffer, size, 0) != size) {
            perror("Failed to read message data");
            free(buffer);
            close(client_fd);
            continue;
        }
        
        // Send data to the channel
        if (!goo_channel_send(channel, buffer)) {
            fprintf(stderr, "Failed to send data to channel\n");
            free(buffer);
            close(client_fd);
            continue;
        }
        
        free(buffer);
        close(client_fd);
    }
    
    endpoint->thread_running = false;
    return NULL;
}

// Thread function for UDP server
static void* udp_server_thread(void* arg) {
    GooChannel* channel = (GooChannel*)arg;
    GooEndpoint* endpoint = (GooEndpoint*)channel->endpoint;
    
    endpoint->thread_running = true;
    
    while (true) {
        // Allocate buffer for the message
        void* buffer = malloc(channel->element_size);
        if (!buffer) {
            perror("Failed to allocate memory for message");
            continue;
        }
        
        // Receive data
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        ssize_t received = recvfrom(endpoint->socket_fd, buffer, channel->element_size, 
                                 0, (struct sockaddr*)&client_addr, &client_len);
        
        if (received < 0) {
            if (errno == EINTR) {
                free(buffer);
                continue; // Interrupted, try again
            }
            perror("Failed to receive UDP data");
            free(buffer);
            break;
        }
        
        // Send data to the channel
        if (!goo_channel_send(channel, buffer)) {
            fprintf(stderr, "Failed to send data to channel\n");
            free(buffer);
            continue;
        }
        
        free(buffer);
    }
    
    endpoint->thread_running = false;
    return NULL;
}

// Thread function for PGM server
static void* pgm_server_thread(void* arg) {
    GooChannel* channel = (GooChannel*)arg;
    GooEndpoint* endpoint = (GooEndpoint*)channel->endpoint;
    
    endpoint->thread_running = true;
    
    // Buffer for received data
    void* buffer = malloc(channel->element_size);
    if (!buffer) {
        perror("Failed to allocate buffer for PGM server");
        endpoint->thread_running = false;
        return NULL;
    }
    
    while (true) {
        // Receive data with a 100ms timeout
        ssize_t received = goo_pgm_receive(endpoint->socket_fd, buffer, channel->element_size, 100);
        
        if (received < 0) {
            // Error occurred
            perror("Error receiving from PGM socket");
            break;
        }
        
        if (received == 0) {
            // Timeout, continue polling
            continue;
        }
        
        // Got data, send it to the channel
        if (!goo_channel_send(channel, buffer)) {
            fprintf(stderr, "Failed to send data to channel\n");
            continue;
        }
    }
    
    free(buffer);
    endpoint->thread_running = false;
    return NULL;
}

// Set up the endpoint for a channel
bool goo_channel_set_endpoint(GooChannel* channel, const char* endpoint_url) {
    if (!channel || !endpoint_url) return false;
    
    // Parse the endpoint URL
    GooEndpoint* endpoint = goo_endpoint_parse(endpoint_url);
    if (!endpoint) {
        fprintf(stderr, "Failed to parse endpoint URL: %s\n", endpoint_url);
        return false;
    }
    
    // Initialize the socket
    if (!goo_endpoint_init_socket(endpoint)) {
        fprintf(stderr, "Failed to initialize socket for endpoint: %s\n", endpoint_url);
        goo_endpoint_free(endpoint);
        return false;
    }
    
    // Store the endpoint in the channel
    channel->endpoint = endpoint;
    
    // For server endpoints, start a listener thread
    if (endpoint->is_server) {
        pthread_t thread;
        if (endpoint->protocol == GOO_PROTOCOL_TCP) {
            if (pthread_create(&thread, NULL, tcp_server_thread, channel) != 0) {
                perror("Failed to create TCP server thread");
                goo_endpoint_free(endpoint);
                channel->endpoint = NULL;
                return false;
            }
        } else if (endpoint->protocol == GOO_PROTOCOL_UDP) {
            if (pthread_create(&thread, NULL, udp_server_thread, channel) != 0) {
                perror("Failed to create UDP server thread");
                goo_endpoint_free(endpoint);
                channel->endpoint = NULL;
                return false;
            }
        } else if (endpoint->protocol == GOO_PROTOCOL_PGM || 
                  endpoint->protocol == GOO_PROTOCOL_EPGM) {
            if (pthread_create(&thread, NULL, pgm_server_thread, channel) != 0) {
                perror("Failed to create PGM server thread");
                goo_endpoint_free(endpoint);
                channel->endpoint = NULL;
                return false;
            }
        }
        pthread_detach(thread); // Don't need to join this thread
    }
    
    return true;
}

// Send data to a remote endpoint
bool goo_channel_send_to_endpoint(GooChannel* channel, void* data) {
    if (!channel || !data || !channel->endpoint) return false;
    
    GooEndpoint* endpoint = (GooEndpoint*)channel->endpoint;
    
    // Only client endpoints can send data
    if (endpoint->is_server) return false;
    
    switch (endpoint->protocol) {
        case GOO_PROTOCOL_TCP: {
            // Send the size first
            uint32_t size = htonl(channel->element_size);
            if (send(endpoint->socket_fd, &size, sizeof(size), 0) != sizeof(size)) {
                perror("Failed to send message size");
                return false;
            }
            
            // Send the data
            if (send(endpoint->socket_fd, data, channel->element_size, 0) != channel->element_size) {
                perror("Failed to send message data");
                return false;
            }
            
            return true;
        }
        
        case GOO_PROTOCOL_UDP: {
            // Resolve the address
            struct hostent* server = gethostbyname(endpoint->address);
            if (server == NULL) {
                fprintf(stderr, "Failed to resolve host: %s\n", endpoint->address);
                return false;
            }
            
            // Set up the server address
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
            server_addr.sin_port = htons(endpoint->port);
            
            // Send the data
            if (sendto(endpoint->socket_fd, data, channel->element_size, 0,
                    (struct sockaddr*)&server_addr, sizeof(server_addr)) != channel->element_size) {
                perror("Failed to send UDP data");
                return false;
            }
            
            return true;
        }
        
        case GOO_PROTOCOL_PGM:
        case GOO_PROTOCOL_EPGM:
            // Use the PGM send function
            return goo_pgm_send(endpoint->socket_fd, data, channel->element_size);
            
        default:
            fprintf(stderr, "Unsupported protocol for sending data\n");
            return false;
    }
}

// Enhanced channel send function that also sends to the endpoint
bool goo_distributed_channel_send(GooChannel* channel, void* data) {
    if (!channel || !data) return false;
    
    // Send to the local channel
    if (!goo_channel_send(channel, data)) {
        return false;
    }
    
    // If this channel has an endpoint, send to it as well
    if (channel->endpoint && !((GooEndpoint*)channel->endpoint)->is_server) {
        return goo_channel_send_to_endpoint(channel, data);
    }
    
    return true;
} 