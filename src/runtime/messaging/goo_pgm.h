/**
 * goo_pgm.h
 * 
 * Header file for PGM (Pragmatic General Multicast) implementation
 * for the Goo messaging system.
 */

#ifndef GOO_PGM_H
#define GOO_PGM_H

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include "messaging/goo_channels.h"

// PGM socket options
typedef struct GooPGMOptions {
    uint32_t send_window_size;      // Send window size in bytes
    uint32_t recv_window_size;      // Receive window size in bytes
    uint16_t max_tpdu;              // Maximum Transport Protocol Data Unit size
    uint32_t txw_secs;              // Transmit window in seconds
    uint32_t rxw_secs;              // Receive window in seconds
    uint32_t peer_expiry;           // Peer timeout in milliseconds
    uint32_t spmr_expiry;           // SPM request expiry time in milliseconds
    uint32_t nak_bo_ivl;            // NAK back-off interval in milliseconds
    uint32_t nak_rpt_ivl;           // NAK repeat interval in milliseconds
    uint32_t nak_rdata_ivl;         // NAK RDATA interval in milliseconds
    uint32_t nak_data_retries;      // NAK DATA retries
    uint32_t nak_ncf_retries;       // NAK NCF retries
    bool use_fec;                   // Use Forward Error Correction
    uint8_t fec_k;                  // FEC k parameter
    uint8_t fec_n;                  // FEC n parameter
} GooPGMOptions;

// PGM statistics
typedef struct GooPGMStats {
    uint64_t data_bytes_sent;       // Total data bytes sent
    uint64_t data_bytes_received;   // Total data bytes received
    uint64_t nak_packets_sent;      // NAK packets sent
    uint64_t nak_packets_received;  // NAK packets received
    uint64_t packets_recovered;     // Packets recovered
    uint64_t packets_lost;          // Packets lost and not recovered
    uint64_t packets_retransmitted; // Packets retransmitted
} GooPGMStats;

// Initialize PGM library
bool goo_pgm_init(void);

// Clean up PGM library
void goo_pgm_cleanup(void);

// Create a PGM sender socket
int goo_pgm_create_sender(const char* address, uint16_t port, GooPGMOptions* options);

// Create a PGM receiver socket
int goo_pgm_create_receiver(const char* address, uint16_t port, GooPGMOptions* options);

// Send data over a PGM connection
bool goo_pgm_send(int socket_fd, const void* data, size_t size);

// Receive data from a PGM connection
ssize_t goo_pgm_receive(int socket_fd, void* buffer, size_t buffer_size, int timeout_ms);

// Close a PGM socket
void goo_pgm_close(int socket_fd);

// Get the default PGM options
GooPGMOptions goo_pgm_default_options(void);

// Get PGM statistics for a socket
bool goo_pgm_get_stats(int socket_fd, GooPGMStats* stats);

// Initialize an endpoint with PGM
bool goo_endpoint_init_pgm(GooEndpoint* endpoint, bool is_epgm);

#endif // GOO_PGM_H 