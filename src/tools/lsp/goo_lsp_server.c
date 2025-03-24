/**
 * @file goo_lsp_server.c
 * @brief Implementation of the Language Server Protocol server for the Goo language
 *
 * @copyright Copyright (c) 2023, Goo Language Authors
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <json-c/json.h>

#include "goo_lsp_server.h"
#include "goo_lsp_protocol.h"

// Default configuration values
#define DEFAULT_CONFIG_INIT { \
    .enable_diagnostics = true, \
    .enable_hover = true, \
    .enable_completion = true, \
    .enable_definition = true, \
    .enable_references = true, \
    .enable_formatting = true, \
    .enable_symbols = true, \
    .enable_highlight = true, \
    .enable_rename = true, \
    .enable_signature_help = true, \
    .std_lib_path = NULL, \
    .verbose = false \
}

// Internal server structure
struct GooLspServer {
    GooLspServerConfig config;
    bool running;
    bool initialized;
    bool shutdown_requested;
    char* root_uri;
    pthread_mutex_t mutex;
    int request_id;
    
    // Document storage
    struct json_object* documents;  // Map of document URI to content
    
    // Capabilities
    struct json_object* server_capabilities;
    struct json_object* client_capabilities;
};

// Internal logging function
static void lsp_log(GooLspServer* server, const char* format, ...) {
    if (!server || !server->config.verbose) {
        return;
    }
    
    FILE* log_file = fopen("goo-lsp-server.log", "a");
    if (!log_file) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fclose(log_file);
}

// Helper function to create a JSON-RPC response
static struct json_object* create_response(int id, struct json_object* result) {
    struct json_object* response = json_object_new_object();
    
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", json_object_new_int(id));
    
    if (result) {
        json_object_object_add(response, "result", result);
    } else {
        json_object_object_add(response, "result", json_object_new_object());
    }
    
    return response;
}

// Helper function to create a JSON-RPC error response
static struct json_object* create_error_response(int id, int code, const char* message) {
    struct json_object* response = json_object_new_object();
    struct json_object* error = json_object_new_object();
    
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", json_object_new_int(id));
    
    json_object_object_add(error, "code", json_object_new_int(code));
    json_object_object_add(error, "message", json_object_new_string(message));
    
    json_object_object_add(response, "error", error);
    
    return response;
}

// Helper function to create a notification (no id)
static struct json_object* create_notification(const char* method, struct json_object* params) {
    struct json_object* notification = json_object_new_object();
    
    json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(notification, "method", json_object_new_string(method));
    
    if (params) {
        json_object_object_add(notification, "params", params);
    }
    
    return notification;
}

// Create server capabilities JSON object
static struct json_object* create_server_capabilities(const GooLspServerConfig* config) {
    struct json_object* capabilities = json_object_new_object();
    
    // TextDocumentSync
    struct json_object* text_document_sync = json_object_new_object();
    json_object_object_add(text_document_sync, "openClose", json_object_new_boolean(true));
    json_object_object_add(text_document_sync, "change", json_object_new_int(2)); // Incremental
    json_object_object_add(text_document_sync, "willSave", json_object_new_boolean(true));
    json_object_object_add(text_document_sync, "willSaveWaitUntil", json_object_new_boolean(true));
    json_object_object_add(text_document_sync, "save", json_object_new_boolean(true));
    json_object_object_add(capabilities, "textDocumentSync", text_document_sync);
    
    // CompletionProvider
    if (config->enable_completion) {
        struct json_object* completion = json_object_new_object();
        json_object_object_add(completion, "resolveProvider", json_object_new_boolean(true));
        
        struct json_object* trigger_chars = json_object_new_array();
        json_object_array_add(trigger_chars, json_object_new_string("."));
        json_object_array_add(trigger_chars, json_object_new_string(":"));
        json_object_array_add(trigger_chars, json_object_new_string("("));
        json_object_object_add(completion, "triggerCharacters", trigger_chars);
        
        json_object_object_add(capabilities, "completionProvider", completion);
    }
    
    // HoverProvider
    if (config->enable_hover) {
        json_object_object_add(capabilities, "hoverProvider", json_object_new_boolean(true));
    }
    
    // SignatureHelpProvider
    if (config->enable_signature_help) {
        struct json_object* signature = json_object_new_object();
        
        struct json_object* trigger_chars = json_object_new_array();
        json_object_array_add(trigger_chars, json_object_new_string("("));
        json_object_array_add(trigger_chars, json_object_new_string(","));
        json_object_object_add(signature, "triggerCharacters", trigger_chars);
        
        json_object_object_add(capabilities, "signatureHelpProvider", signature);
    }
    
    // DefinitionProvider
    if (config->enable_definition) {
        json_object_object_add(capabilities, "definitionProvider", json_object_new_boolean(true));
    }
    
    // ReferencesProvider
    if (config->enable_references) {
        json_object_object_add(capabilities, "referencesProvider", json_object_new_boolean(true));
    }
    
    // DocumentHighlightProvider
    if (config->enable_highlight) {
        json_object_object_add(capabilities, "documentHighlightProvider", json_object_new_boolean(true));
    }
    
    // DocumentSymbolProvider
    if (config->enable_symbols) {
        json_object_object_add(capabilities, "documentSymbolProvider", json_object_new_boolean(true));
    }
    
    // WorkspaceSymbolProvider
    if (config->enable_symbols) {
        json_object_object_add(capabilities, "workspaceSymbolProvider", json_object_new_boolean(true));
    }
    
    // CodeActionProvider
    if (config->enable_diagnostics) {
        json_object_object_add(capabilities, "codeActionProvider", json_object_new_boolean(true));
    }
    
    // DocumentFormattingProvider
    if (config->enable_formatting) {
        json_object_object_add(capabilities, "documentFormattingProvider", json_object_new_boolean(true));
    }
    
    // RenameProvider
    if (config->enable_rename) {
        json_object_object_add(capabilities, "renameProvider", json_object_new_boolean(true));
    }
    
    return capabilities;
}

// Handle initialize request
static struct json_object* handle_initialize(GooLspServer* server, struct json_object* params) {
    // Extract root URI if present
    struct json_object* root_uri_obj;
    if (json_object_object_get_ex(params, "rootUri", &root_uri_obj)) {
        const char* root_uri = json_object_get_string(root_uri_obj);
        if (root_uri) {
            free(server->root_uri);
            server->root_uri = strdup(root_uri);
        }
    }
    
    // Extract client capabilities if present
    struct json_object* capabilities_obj;
    if (json_object_object_get_ex(params, "capabilities", &capabilities_obj)) {
        if (server->client_capabilities) {
            json_object_put(server->client_capabilities);
        }
        server->client_capabilities = json_object_get(capabilities_obj);
    }
    
    // Create initialize result
    struct json_object* result = json_object_new_object();
    
    // Set server information
    struct json_object* server_info = json_object_new_object();
    json_object_object_add(server_info, "name", json_object_new_string("Goo Language Server"));
    json_object_object_add(server_info, "version", json_object_new_string("0.1.0"));
    json_object_object_add(result, "serverInfo", server_info);
    
    // Set capabilities
    if (server->server_capabilities) {
        json_object_put(server->server_capabilities);
    }
    server->server_capabilities = create_server_capabilities(&server->config);
    json_object_object_add(result, "capabilities", json_object_get(server->server_capabilities));
    
    server->initialized = true;
    
    return result;
}

// Handle initialized notification
static void handle_initialized(GooLspServer* server) {
    lsp_log(server, "Server initialized with root URI: %s", 
            server->root_uri ? server->root_uri : "(none)");
    
    // Here we could register additional capabilities or send initial notifications
}

// Handle text document open notification
static void handle_text_document_did_open(GooLspServer* server, struct json_object* params) {
    struct json_object* text_document;
    if (!json_object_object_get_ex(params, "textDocument", &text_document)) {
        return;
    }
    
    struct json_object* uri_obj;
    struct json_object* text_obj;
    
    if (!json_object_object_get_ex(text_document, "uri", &uri_obj) ||
        !json_object_object_get_ex(text_document, "text", &text_obj)) {
        return;
    }
    
    const char* uri = json_object_get_string(uri_obj);
    const char* text = json_object_get_string(text_obj);
    
    lsp_log(server, "Document opened: %s", uri);
    
    // Add document to storage
    json_object_object_add(server->documents, uri, json_object_new_string(text));
    
    // If diagnostics are enabled, send diagnostics
    if (server->config.enable_diagnostics) {
        // Parse the document and compute diagnostics
        // For now, just send an empty diagnostics list
        struct json_object* params = json_object_new_object();
        json_object_object_add(params, "uri", json_object_new_string(uri));
        json_object_object_add(params, "diagnostics", json_object_new_array());
        
        struct json_object* notification = create_notification("textDocument/publishDiagnostics", params);
        
        // Send notification (this would normally go through the proper output channel)
        const char* json_text = json_object_to_json_string(notification);
        lsp_log(server, "Publishing diagnostics: %s", json_text);
        
        json_object_put(notification);
    }
}

// Handle text document change notification
static void handle_text_document_did_change(GooLspServer* server, struct json_object* params) {
    struct json_object* text_document;
    struct json_object* content_changes;
    
    if (!json_object_object_get_ex(params, "textDocument", &text_document) ||
        !json_object_object_get_ex(params, "contentChanges", &content_changes)) {
        return;
    }
    
    struct json_object* uri_obj;
    if (!json_object_object_get_ex(text_document, "uri", &uri_obj)) {
        return;
    }
    
    const char* uri = json_object_get_string(uri_obj);
    
    // Get the existing document
    struct json_object* doc_content;
    if (!json_object_object_get_ex(server->documents, uri, &doc_content)) {
        lsp_log(server, "Document not found for change: %s", uri);
        return;
    }
    
    // For simplicity, we'll assume full document sync and just take the first change
    if (json_object_array_length(content_changes) > 0) {
        struct json_object* change = json_object_array_get_idx(content_changes, 0);
        struct json_object* text_obj;
        
        if (json_object_object_get_ex(change, "text", &text_obj)) {
            const char* new_text = json_object_get_string(text_obj);
            
            // Update document in storage
            json_object_object_add(server->documents, uri, json_object_new_string(new_text));
            
            lsp_log(server, "Document updated: %s", uri);
            
            // If diagnostics are enabled, recompute and send diagnostics
            if (server->config.enable_diagnostics) {
                // In a real implementation, parse the document and compute diagnostics here
                // For now, just send an empty diagnostics list
                struct json_object* diag_params = json_object_new_object();
                json_object_object_add(diag_params, "uri", json_object_new_string(uri));
                json_object_object_add(diag_params, "diagnostics", json_object_new_array());
                
                struct json_object* notification = create_notification("textDocument/publishDiagnostics", diag_params);
                
                // Send notification (this would normally go through the proper output channel)
                const char* json_text = json_object_to_json_string(notification);
                lsp_log(server, "Publishing diagnostics after change: %s", json_text);
                
                json_object_put(notification);
            }
        }
    }
}

// Handle text document close notification
static void handle_text_document_did_close(GooLspServer* server, struct json_object* params) {
    struct json_object* text_document;
    if (!json_object_object_get_ex(params, "textDocument", &text_document)) {
        return;
    }
    
    struct json_object* uri_obj;
    if (!json_object_object_get_ex(text_document, "uri", &uri_obj)) {
        return;
    }
    
    const char* uri = json_object_get_string(uri_obj);
    
    // Remove document from storage
    json_object_object_del(server->documents, uri);
    
    lsp_log(server, "Document closed: %s", uri);
    
    // If diagnostics are enabled, clear diagnostics
    if (server->config.enable_diagnostics) {
        struct json_object* diag_params = json_object_new_object();
        json_object_object_add(diag_params, "uri", json_object_new_string(uri));
        json_object_object_add(diag_params, "diagnostics", json_object_new_array());
        
        struct json_object* notification = create_notification("textDocument/publishDiagnostics", diag_params);
        
        // Send notification (this would normally go through the proper output channel)
        const char* json_text = json_object_to_json_string(notification);
        lsp_log(server, "Clearing diagnostics: %s", json_text);
        
        json_object_put(notification);
    }
}

// Process a single LSP message
static bool process_message_internal(GooLspServer* server, const struct json_object* message, struct json_object** response) {
    if (!server || !message || !response) {
        return false;
    }
    
    *response = NULL;
    
    // Get method and parameters
    struct json_object* method_obj;
    struct json_object* params_obj;
    struct json_object* id_obj;
    
    if (!json_object_object_get_ex(message, "method", &method_obj)) {
        return false; // Not a valid request or notification
    }
    
    const char* method = json_object_get_string(method_obj);
    
    json_object_object_get_ex(message, "params", &params_obj);
    json_object_object_get_ex(message, "id", &id_obj);
    
    int id = id_obj ? json_object_get_int(id_obj) : -1;
    
    lsp_log(server, "Received message: %s (id: %d)", method, id);
    
    // Handle requests that return a response
    if (id_obj) {
        // Initialize request
        if (strcmp(method, "initialize") == 0) {
            if (!params_obj) {
                *response = create_error_response(id, -32602, "Invalid params");
                return true;
            }
            struct json_object* result = handle_initialize(server, params_obj);
            *response = create_response(id, result);
            return true;
        }
        
        // Ensure server is initialized
        if (!server->initialized && strcmp(method, "shutdown") != 0) {
            *response = create_error_response(id, -32002, "Server not yet initialized");
            return true;
        }
        
        // Shutdown request
        if (strcmp(method, "shutdown") == 0) {
            server->shutdown_requested = true;
            *response = create_response(id, json_object_new_object());
            return true;
        }
        
        // Handle other requests...
        // Add implementation for other request handlers here
        
        // For now, we'll just return a generic "not implemented" error
        *response = create_error_response(id, -32001, "Method not implemented");
        return true;
    }
    
    // Handle notifications that don't return a response
    
    // Exit notification
    if (strcmp(method, "exit") == 0) {
        server->running = false;
        return true;
    }
    
    // Initialized notification
    if (strcmp(method, "initialized") == 0) {
        handle_initialized(server);
        return true;
    }
    
    // Document open notification
    if (strcmp(method, "textDocument/didOpen") == 0) {
        if (params_obj) {
            handle_text_document_did_open(server, params_obj);
        }
        return true;
    }
    
    // Document change notification
    if (strcmp(method, "textDocument/didChange") == 0) {
        if (params_obj) {
            handle_text_document_did_change(server, params_obj);
        }
        return true;
    }
    
    // Document close notification
    if (strcmp(method, "textDocument/didClose") == 0) {
        if (params_obj) {
            handle_text_document_did_close(server, params_obj);
        }
        return true;
    }
    
    // Other notifications can be ignored
    return true;
}

// Read a message from stdin
static char* read_message_from_stdin() {
    // Read the Content-Length header
    char header[4096];
    int content_length = -1;
    
    while (content_length == -1) {
        if (!fgets(header, sizeof(header), stdin)) {
            if (feof(stdin)) {
                return NULL; // End of input
            }
            continue;
        }
        
        // Remove trailing newline
        char* newline = strchr(header, '\n');
        if (newline) {
            *newline = '\0';
        }
        
        // Skip empty lines
        if (header[0] == '\0') {
            continue;
        }
        
        // Check for Content-Length header
        if (strncmp(header, "Content-Length: ", 16) == 0) {
            content_length = atoi(header + 16);
        }
        
        // If we've found the content length, read until we hit the empty line
        if (content_length != -1) {
            while (fgets(header, sizeof(header), stdin)) {
                if (header[0] == '\r' || header[0] == '\n') {
                    break;
                }
            }
        }
    }
    
    // Read the content
    if (content_length <= 0) {
        return NULL;
    }
    
    char* content = (char*)malloc(content_length + 1);
    if (!content) {
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, content_length, stdin);
    if (bytes_read != (size_t)content_length) {
        free(content);
        return NULL;
    }
    
    content[content_length] = '\0';
    return content;
}

// Write a message to stdout
static void write_message_to_stdout(const char* message) {
    if (!message) {
        return;
    }
    
    size_t length = strlen(message);
    
    // Write the header
    fprintf(stdout, "Content-Length: %zu\r\n\r\n", length);
    
    // Write the content
    fwrite(message, 1, length, stdout);
    fflush(stdout);
}

// Public functions

GooLspServer* goo_lsp_server_create(const GooLspServerConfig* config) {
    GooLspServerConfig default_config = DEFAULT_CONFIG_INIT;
    
    if (!config) {
        config = &default_config;
    }
    
    GooLspServer* server = (GooLspServer*)malloc(sizeof(GooLspServer));
    if (!server) {
        return NULL;
    }
    
    // Initialize the server with the provided config
    memcpy(&server->config, config, sizeof(GooLspServerConfig));
    server->running = false;
    server->initialized = false;
    server->shutdown_requested = false;
    server->root_uri = NULL;
    pthread_mutex_init(&server->mutex, NULL);
    server->request_id = 1;
    
    // Initialize document storage
    server->documents = json_object_new_object();
    
    // Initialize capabilities
    server->server_capabilities = NULL;
    server->client_capabilities = NULL;
    
    return server;
}

struct json_object* goo_lsp_server_initialize(GooLspServer* server, const struct json_object* init_params) {
    if (!server || !init_params) {
        return NULL;
    }
    
    pthread_mutex_lock(&server->mutex);
    struct json_object* result = handle_initialize(server, (struct json_object*)init_params);
    pthread_mutex_unlock(&server->mutex);
    
    return result;
}

void goo_lsp_server_destroy(GooLspServer* server) {
    if (!server) {
        return;
    }
    
    // Clean up resources
    free(server->root_uri);
    
    if (server->documents) {
        json_object_put(server->documents);
    }
    
    if (server->server_capabilities) {
        json_object_put(server->server_capabilities);
    }
    
    if (server->client_capabilities) {
        json_object_put(server->client_capabilities);
    }
    
    pthread_mutex_destroy(&server->mutex);
    
    free(server);
}

bool goo_lsp_server_start(GooLspServer* server) {
    if (!server) {
        return false;
    }
    
    server->running = true;
    lsp_log(server, "Server starting...");
    
    // Main loop
    while (server->running) {
        // Read a message from stdin
        char* message_text = read_message_from_stdin();
        if (!message_text) {
            break;
        }
        
        // Parse the message
        struct json_object* message = json_tokener_parse(message_text);
        free(message_text);
        
        if (!message) {
            continue;
        }
        
        // Process the message
        struct json_object* response = NULL;
        pthread_mutex_lock(&server->mutex);
        bool success = process_message_internal(server, message, &response);
        pthread_mutex_unlock(&server->mutex);
        
        json_object_put(message);
        
        if (!success) {
            continue;
        }
        
        // Write the response to stdout if there is one
        if (response) {
            const char* response_text = json_object_to_json_string(response);
            write_message_to_stdout(response_text);
            json_object_put(response);
        }
    }
    
    return server->shutdown_requested;
}

bool goo_lsp_server_process_message(GooLspServer* server, const struct json_object* message) {
    if (!server || !message) {
        return false;
    }
    
    struct json_object* response = NULL;
    pthread_mutex_lock(&server->mutex);
    bool success = process_message_internal(server, message, &response);
    pthread_mutex_unlock(&server->mutex);
    
    if (response) {
        const char* response_text = json_object_to_json_string(response);
        write_message_to_stdout(response_text);
        json_object_put(response);
    }
    
    return success;
}

int goo_lsp_server_run(int argc, char** argv) {
    // Parse command line arguments
    bool verbose = false;
    char* std_lib_path = NULL;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0 || strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else if (strcmp(argv[i], "--std-lib") == 0 || strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                std_lib_path = argv[++i];
            }
        }
    }
    
    // Create server with configuration from command line
    GooLspServerConfig config = DEFAULT_CONFIG_INIT;
    config.verbose = verbose;
    if (std_lib_path) {
        config.std_lib_path = strdup(std_lib_path);
    }
    
    GooLspServer* server = goo_lsp_server_create(&config);
    if (!server) {
        fprintf(stderr, "Failed to create LSP server\n");
        free(config.std_lib_path);
        return 1;
    }
    
    // Start the server
    bool success = goo_lsp_server_start(server);
    
    // Clean up
    goo_lsp_server_destroy(server);
    free(config.std_lib_path);
    
    return success ? 0 : 1;
}

struct json_object* goo_lsp_server_shutdown(GooLspServer* server) {
    if (!server) {
        return NULL;
    }
    
    server->shutdown_requested = true;
    lsp_log(server, "Server shutting down...");
    
    return json_object_new_object(); // Empty result
}

bool goo_lsp_server_update_config(GooLspServer* server, const GooLspServerConfig* config) {
    if (!server || !config) {
        return false;
    }
    
    pthread_mutex_lock(&server->mutex);
    
    // Update the configuration
    bool enable_diagnostics = server->config.enable_diagnostics;
    memcpy(&server->config, config, sizeof(GooLspServerConfig));
    
    // If diagnostics were enabled or disabled, update all documents
    if (enable_diagnostics != config->enable_diagnostics) {
        // Handle diagnostics enabling/disabling
        if (!config->enable_diagnostics) {
            // Clear all diagnostics
            json_object_object_foreach(server->documents, uri, content) {
                struct json_object* params = json_object_new_object();
                json_object_object_add(params, "uri", json_object_new_string(uri));
                json_object_object_add(params, "diagnostics", json_object_new_array());
                
                struct json_object* notification = create_notification("textDocument/publishDiagnostics", params);
                
                const char* json_text = json_object_to_json_string(notification);
                lsp_log(server, "Clearing diagnostics: %s", json_text);
                
                json_object_put(notification);
            }
        } else {
            // Compute diagnostics for all documents
            // This would be implemented in a real server
        }
    }
    
    pthread_mutex_unlock(&server->mutex);
    
    return true;
} 