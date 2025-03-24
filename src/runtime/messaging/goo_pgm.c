/**
 * goo_pgm.c
 * 
 * Implementation of PGM (Pragmatic General Multicast) transport protocol
 * for the Goo messaging system.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include "messaging/goo_pgm.h"

// PGM protocol constants
#define PGM_DEFAULT_PORT 7500
#define PGM_MAX_TPDU 1500
#define PGM_DEFAULT_SEND_WINDOW (8 * 1024 * 1024)  // 8MB
#define PGM_DEFAULT_RECV_WINDOW (16 * 1024 * 1024) // 16MB

// PGM socket structure
typedef struct GooPGMSocket {
    int socket_fd;                  // Socket file descriptor
    struct sockaddr_in addr;        // Socket address
    bool is_sender;                 // Is this a sender socket?
    bool is_epgm;                   // Is this EPGM (encapsulated PGM)?
    GooPGMOptions options;          // Socket options
    GooPGMStats stats;              // Socket statistics
    pthread_mutex_t mutex;          // Mutex for thread safety
    bool initialized;               // Is the socket initialized?
} GooPGMSocket;

// Global state
static bool pgm_initialized = false;
static pthread_mutex_t pgm_init_mutex = PTHREAD_MUTEX_INITIALIZER;

// Map of socket file descriptors to PGM socket structures
static GooPGMSocket* pgm_sockets[FD_SETSIZE];

// Initialize the PGM library
bool goo_pgm_init(void) {
    pthread_mutex_lock(&pgm_init_mutex);
    
    if (!pgm_initialized) {
        // Initialize socket map
        memset(pgm_sockets, 0, sizeof(pgm_sockets));
        
        // Set global state
        pgm_initialized = true;
    }
    
    pthread_mutex_unlock(&pgm_init_mutex);
    return true;
}

// Clean up the PGM library
void goo_pgm_cleanup(void) {
    pthread_mutex_lock(&pgm_init_mutex);
    
    if (pgm_initialized) {
        // Close any open sockets
        for (int i = 0; i < FD_SETSIZE; i++) {
            if (pgm_sockets[i]) {
                goo_pgm_close(i);
            }
        }
        
        // Reset global state
        pgm_initialized = false;
    }
    
    pthread_mutex_unlock(&pgm_init_mutex);
}

// Get default PGM options
GooPGMOptions goo_pgm_default_options(void) {
    GooPGMOptions options;
    memset(&options, 0, sizeof(options));
    
    options.send_window_size = PGM_DEFAULT_SEND_WINDOW;
    options.recv_window_size = PGM_DEFAULT_RECV_WINDOW;
    options.max_tpdu = PGM_MAX_TPDU;
    options.txw_secs = 30;      // 30 seconds transmit window
    options.rxw_secs = 30;      // 30 seconds receive window
    options.peer_expiry = 300000; // 5 minutes
    options.spmr_expiry = 250;  // 250ms
    options.nak_bo_ivl = 50;    // 50ms
    options.nak_rpt_ivl = 200;  // 200ms
    options.nak_rdata_ivl = 500; // 500ms
    options.nak_data_retries = 5;
    options.nak_ncf_retries = 2;
    options.use_fec = false;
    options.fec_k = 8;
    options.fec_n = 10;
    
    return options;
}

// Create a PGM sender
int goo_pgm_create_sender(const char* address, uint16_t port, GooPGMOptions* options) {
    if (!pgm_initialized && !goo_pgm_init()) {
        return -1;
    }
    
    // Get a free socket slot
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        perror("Failed to create UDP socket for PGM");
        return -1;
    }
    
    // Create PGM socket structure
    GooPGMSocket* pgm_socket = (GooPGMSocket*)malloc(sizeof(GooPGMSocket));
    if (!pgm_socket) {
        close(socket_fd);
        return -1;
    }
    
    // Initialize the structure
    memset(pgm_socket, 0, sizeof(GooPGMSocket));
    pgm_socket->socket_fd = socket_fd;
    pgm_socket->is_sender = true;
    pgm_socket->is_epgm = false; // We'll set this elsewhere if needed
    
    // Copy options
    if (options) {
        pgm_socket->options = *options;
    } else {
        pgm_socket->options = goo_pgm_default_options();
    }
    
    // Initialize mutex
    pthread_mutex_init(&pgm_socket->mutex, NULL);
    
    // Set up the address
    memset(&pgm_socket->addr, 0, sizeof(pgm_socket->addr));
    pgm_socket->addr.sin_family = AF_INET;
    pgm_socket->addr.sin_port = htons(port ? port : PGM_DEFAULT_PORT);
    
    if (address && strcmp(address, "*") != 0) {
        // Use specified address
        if (inet_pton(AF_INET, address, &pgm_socket->addr.sin_addr) <= 0) {
            perror("Invalid address for PGM sender");
            free(pgm_socket);
            close(socket_fd);
            return -1;
        }
    } else {
        // Use any address
        pgm_socket->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    
    // Set socket options
    int optval = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Failed to set SO_REUSEADDR on PGM socket");
        free(pgm_socket);
        close(socket_fd);
        return -1;
    }
    
    // Enable multicast
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = pgm_socket->addr.sin_addr.s_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Failed to join multicast group");
        free(pgm_socket);
        close(socket_fd);
        return -1;
    }
    
    // Set the socket to non-blocking mode
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Add to socket map
    pgm_sockets[socket_fd] = pgm_socket;
    pgm_socket->initialized = true;
    
    return socket_fd;
}

// Create a PGM receiver
int goo_pgm_create_receiver(const char* address, uint16_t port, GooPGMOptions* options) {
    if (!pgm_initialized && !goo_pgm_init()) {
        return -1;
    }
    
    // Get a free socket slot
    int socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd < 0) {
        perror("Failed to create UDP socket for PGM");
        return -1;
    }
    
    // Create PGM socket structure
    GooPGMSocket* pgm_socket = (GooPGMSocket*)malloc(sizeof(GooPGMSocket));
    if (!pgm_socket) {
        close(socket_fd);
        return -1;
    }
    
    // Initialize the structure
    memset(pgm_socket, 0, sizeof(GooPGMSocket));
    pgm_socket->socket_fd = socket_fd;
    pgm_socket->is_sender = false;
    pgm_socket->is_epgm = false; // We'll set this elsewhere if needed
    
    // Copy options
    if (options) {
        pgm_socket->options = *options;
    } else {
        pgm_socket->options = goo_pgm_default_options();
    }
    
    // Initialize mutex
    pthread_mutex_init(&pgm_socket->mutex, NULL);
    
    // Set up the address
    memset(&pgm_socket->addr, 0, sizeof(pgm_socket->addr));
    pgm_socket->addr.sin_family = AF_INET;
    pgm_socket->addr.sin_port = htons(port ? port : PGM_DEFAULT_PORT);
    
    if (address && strcmp(address, "*") != 0) {
        // Use specified address
        if (inet_pton(AF_INET, address, &pgm_socket->addr.sin_addr) <= 0) {
            perror("Invalid address for PGM receiver");
            free(pgm_socket);
            close(socket_fd);
            return -1;
        }
    } else {
        // Use any address
        pgm_socket->addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    
    // Set socket options
    int optval = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("Failed to set SO_REUSEADDR on PGM socket");
        free(pgm_socket);
        close(socket_fd);
        return -1;
    }
    
    // Bind the socket
    if (bind(socket_fd, (struct sockaddr*)&pgm_socket->addr, sizeof(pgm_socket->addr)) < 0) {
        perror("Failed to bind PGM socket");
        free(pgm_socket);
        close(socket_fd);
        return -1;
    }
    
    // Enable multicast
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = pgm_socket->addr.sin_addr.s_addr;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    
    if (setsockopt(socket_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("Failed to join multicast group");
        free(pgm_socket);
        close(socket_fd);
        return -1;
    }
    
    // Set the socket to non-blocking mode
    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Add to socket map
    pgm_sockets[socket_fd] = pgm_socket;
    pgm_socket->initialized = true;
    
    return socket_fd;
}

// Send data over a PGM connection
bool goo_pgm_send(int socket_fd, const void* data, size_t size) {
    if (socket_fd < 0 || socket_fd >= FD_SETSIZE || !pgm_sockets[socket_fd]) {
        fprintf(stderr, "Invalid PGM socket\n");
        return false;
    }
    
    GooPGMSocket* pgm_socket = pgm_sockets[socket_fd];
    
    // Check if this is a sender socket
    if (!pgm_socket->is_sender) {
        fprintf(stderr, "Cannot send data on a receiver socket\n");
        return false;
    }
    
    // Lock the socket
    pthread_mutex_lock(&pgm_socket->mutex);
    
    // Send the data
    ssize_t sent = sendto(pgm_socket->socket_fd, data, size, 0,
                        (struct sockaddr*)&pgm_socket->addr, sizeof(pgm_socket->addr));
    
    // Update statistics
    if (sent > 0) {
        pgm_socket->stats.data_bytes_sent += sent;
    }
    
    pthread_mutex_unlock(&pgm_socket->mutex);
    
    return (sent == size);
}

// Receive data from a PGM connection
ssize_t goo_pgm_receive(int socket_fd, void* buffer, size_t buffer_size, int timeout_ms) {
    if (socket_fd < 0 || socket_fd >= FD_SETSIZE || !pgm_sockets[socket_fd]) {
        fprintf(stderr, "Invalid PGM socket\n");
        return -1;
    }
    
    GooPGMSocket* pgm_socket = pgm_sockets[socket_fd];
    
    // Check if this is a receiver socket
    if (pgm_socket->is_sender) {
        fprintf(stderr, "Cannot receive data on a sender socket\n");
        return -1;
    }
    
    // Set up for select()
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(pgm_socket->socket_fd, &readfds);
    
    // Set timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    // Wait for data
    int result = select(pgm_socket->socket_fd + 1, &readfds, NULL, NULL,
                      timeout_ms >= 0 ? &tv : NULL);
    
    if (result < 0) {
        perror("Error in select() for PGM receive");
        return -1;
    }
    
    if (result == 0) {
        // Timeout
        return 0;
    }
    
    // Data is available, receive it
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    // Lock the socket
    pthread_mutex_lock(&pgm_socket->mutex);
    
    ssize_t received = recvfrom(pgm_socket->socket_fd, buffer, buffer_size, 0,
                             (struct sockaddr*)&from_addr, &from_len);
    
    if (received > 0) {
        // Update statistics
        pgm_socket->stats.data_bytes_received += received;
    }
    
    pthread_mutex_unlock(&pgm_socket->mutex);
    
    return received;
}

// Close a PGM socket
void goo_pgm_close(int socket_fd) {
    if (socket_fd < 0 || socket_fd >= FD_SETSIZE || !pgm_sockets[socket_fd]) {
        return;
    }
    
    GooPGMSocket* pgm_socket = pgm_sockets[socket_fd];
    
    // Lock the socket
    pthread_mutex_lock(&pgm_socket->mutex);
    
    // Close the socket
    close(pgm_socket->socket_fd);
    
    // Free the structure
    pthread_mutex_unlock(&pgm_socket->mutex);
    pthread_mutex_destroy(&pgm_socket->mutex);
    
    // Remove from socket map
    pgm_sockets[socket_fd] = NULL;
    free(pgm_socket);
}

// Get PGM statistics for a socket
bool goo_pgm_get_stats(int socket_fd, GooPGMStats* stats) {
    if (socket_fd < 0 || socket_fd >= FD_SETSIZE || !pgm_sockets[socket_fd] || !stats) {
        return false;
    }
    
    GooPGMSocket* pgm_socket = pgm_sockets[socket_fd];
    
    // Lock the socket
    pthread_mutex_lock(&pgm_socket->mutex);
    
    // Copy statistics
    *stats = pgm_socket->stats;
    
    pthread_mutex_unlock(&pgm_socket->mutex);
    
    return true;
}

// Initialize an endpoint with PGM
bool goo_endpoint_init_pgm(GooEndpoint* endpoint, bool is_epgm) {
    if (!endpoint) return false;
    
    GooPGMOptions options = goo_pgm_default_options();
    
    // Create the appropriate socket based on server/client role
    int socket_fd;
    if (endpoint->is_server) {
        socket_fd = goo_pgm_create_receiver(endpoint->address, endpoint->port, &options);
    } else {
        socket_fd = goo_pgm_create_sender(endpoint->address, endpoint->port, &options);
    }
    
    if (socket_fd < 0) {
        return false;
    }
    
    // Set the socket in the endpoint
    endpoint->socket_fd = socket_fd;
    
    // Set EPGM flag if needed
    if (is_epgm) {
        GooPGMSocket* pgm_socket = pgm_sockets[socket_fd];
        if (pgm_socket) {
            pgm_socket->is_epgm = true;
        }
    }
    
    return true;
} 