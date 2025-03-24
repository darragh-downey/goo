/**
 * @file goo_lsp_server.h
 * @brief Header file for the Goo Language Server Protocol (LSP) server
 *
 * This file defines the public interface for the Goo LSP server, which
 * implements the Language Server Protocol to provide language features
 * like diagnostics, code completion, and more to IDEs that support LSP.
 * 
 * @copyright Copyright (c) 2023, Goo Language Authors
 * @license MIT
 */

#ifndef GOO_LSP_SERVER_H
#define GOO_LSP_SERVER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * Forward declarations to avoid including the JSON library in the header
 */
struct json_object;

/**
 * @brief Configuration options for the LSP server
 */
typedef struct {
    /** Whether to enable diagnostic reporting */
    bool enable_diagnostics;
    
    /** Whether to enable hover information */
    bool enable_hover;
    
    /** Whether to enable code completion */
    bool enable_completion;
    
    /** Whether to enable go-to-definition */
    bool enable_definition;
    
    /** Whether to enable find references */
    bool enable_references;
    
    /** Whether to enable document formatting */
    bool enable_formatting;
    
    /** Whether to enable document symbols */
    bool enable_symbols;
    
    /** Whether to enable document highlighting */
    bool enable_highlight;
    
    /** Whether to enable rename symbol */
    bool enable_rename;
    
    /** Whether to enable signature help */
    bool enable_signature_help;
    
    /** Path to the Goo standard library */
    char* std_lib_path;
    
    /** Enable verbose logging */
    bool verbose;
} GooLspServerConfig;

/**
 * @brief The Goo LSP server instance
 */
typedef struct GooLspServer GooLspServer;

/**
 * @brief Create a new LSP server instance
 * 
 * @param config The server configuration options
 * @return A pointer to the new server instance, or NULL on failure
 */
GooLspServer* goo_lsp_server_create(const GooLspServerConfig* config);

/**
 * @brief Initialize the LSP server
 * 
 * @param server The server instance
 * @param init_params The initialization parameters from the client
 * @return A JSON object containing the server capabilities, or NULL on error
 */
struct json_object* goo_lsp_server_initialize(
    GooLspServer* server,
    const struct json_object* init_params
);

/**
 * @brief Clean up and destroy an LSP server instance
 * 
 * @param server The server instance to destroy
 */
void goo_lsp_server_destroy(GooLspServer* server);

/**
 * @brief Start the LSP server's main loop
 * 
 * This function starts the server's main loop, which reads messages
 * from standard input and writes responses to standard output.
 * 
 * @param server The server instance
 * @return true if the server was shut down gracefully, false on error
 */
bool goo_lsp_server_start(GooLspServer* server);

/**
 * @brief Process a single LSP message
 * 
 * @param server The server instance
 * @param message The message to process
 * @return true if the message was processed successfully, false on error
 */
bool goo_lsp_server_process_message(
    GooLspServer* server,
    const struct json_object* message
);

/**
 * @brief Run the LSP server
 * 
 * This is a convenience function that creates a server with default
 * configuration, initializes it, and starts the main loop.
 * 
 * @param argc The argument count
 * @param argv The argument values
 * @return 0 on success, non-zero on error
 */
int goo_lsp_server_run(int argc, char** argv);

/**
 * @brief Shut down the LSP server
 * 
 * @param server The server instance
 * @return A JSON object containing the result, or NULL on error
 */
struct json_object* goo_lsp_server_shutdown(GooLspServer* server);

/**
 * @brief Update the LSP server configuration
 * 
 * @param server The server instance
 * @param config The new configuration options
 * @return true if the configuration was updated successfully, false on error
 */
bool goo_lsp_server_update_config(
    GooLspServer* server,
    const GooLspServerConfig* config
);

#endif /* GOO_LSP_SERVER_H */ 