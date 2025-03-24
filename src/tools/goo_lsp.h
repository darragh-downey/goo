#ifndef GOO_LSP_H
#define GOO_LSP_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Forward declarations
typedef struct GooLSPServer GooLSPServer;
typedef struct GooLSPDocument GooLSPDocument;
typedef struct GooLSPSymbol GooLSPSymbol;
typedef struct GooLSPDiagnostic GooLSPDiagnostic;
typedef struct GooLSPRequest GooLSPRequest;
typedef struct GooLSPResponse GooLSPResponse;
typedef struct GooLSPPosition GooLSPPosition;
typedef struct GooLSPRange GooLSPRange;
typedef struct GooLSPCompletionItem GooLSPCompletionItem;
typedef struct GooLSPLocation GooLSPLocation;
typedef struct GooLSPCodeAction GooLSPCodeAction;

// Symbol kinds
typedef enum {
    GOO_SYMBOL_FILE = 1,
    GOO_SYMBOL_MODULE = 2,
    GOO_SYMBOL_NAMESPACE = 3,
    GOO_SYMBOL_PACKAGE = 4,
    GOO_SYMBOL_CLASS = 5,
    GOO_SYMBOL_METHOD = 6,
    GOO_SYMBOL_PROPERTY = 7,
    GOO_SYMBOL_FIELD = 8,
    GOO_SYMBOL_CONSTRUCTOR = 9,
    GOO_SYMBOL_ENUM = 10,
    GOO_SYMBOL_INTERFACE = 11,
    GOO_SYMBOL_FUNCTION = 12,
    GOO_SYMBOL_VARIABLE = 13,
    GOO_SYMBOL_CONSTANT = 14,
    GOO_SYMBOL_STRING = 15,
    GOO_SYMBOL_NUMBER = 16,
    GOO_SYMBOL_BOOLEAN = 17,
    GOO_SYMBOL_ARRAY = 18,
    GOO_SYMBOL_OBJECT = 19,
    GOO_SYMBOL_KEY = 20,
    GOO_SYMBOL_NULL = 21,
    GOO_SYMBOL_ENUM_MEMBER = 22,
    GOO_SYMBOL_STRUCT = 23,
    GOO_SYMBOL_EVENT = 24,
    GOO_SYMBOL_OPERATOR = 25,
    GOO_SYMBOL_TYPE_PARAMETER = 26
} GooSymbolKind;

// Diagnostic severity
typedef enum {
    GOO_DIAGNOSTIC_ERROR = 1,
    GOO_DIAGNOSTIC_WARNING = 2,
    GOO_DIAGNOSTIC_INFORMATION = 3,
    GOO_DIAGNOSTIC_HINT = 4
} GooDiagnosticSeverity;

// LSP server configuration
typedef struct {
    char* workspace_root;        // Root directory of the workspace
    char* log_file;              // Log file for LSP server
    int log_level;               // 0=off, 1=error, 2=warn, 3=info, 4=debug
    bool enable_snippets;        // Enable code snippets in completion
    bool enable_diagnostics;     // Enable diagnostics
    bool enable_formatting;      // Enable document formatting
    bool enable_signature_help;  // Enable signature help
    bool enable_hover;           // Enable hover information
    bool enable_references;      // Enable finding references
    bool enable_implementation;  // Enable finding implementations
    bool enable_definition;      // Enable go to definition
    bool enable_type_definition; // Enable go to type definition
    bool enable_code_action;     // Enable code actions
    bool enable_workspace_symbols; // Enable workspace symbols
    int max_completion_items;    // Maximum number of completion items
    int max_symbols;             // Maximum number of symbols to return
} GooLSPConfig;

// Position in a document
struct GooLSPPosition {
    int line;        // Line number (0-based)
    int character;   // Character offset (0-based, UTF-16 code units)
};

// Range in a document
struct GooLSPRange {
    GooLSPPosition start;
    GooLSPPosition end;
};

// Location in workspace
struct GooLSPLocation {
    char* uri;       // Document URI
    GooLSPRange range;
};

// Symbol information
struct GooLSPSymbol {
    char* name;            // Symbol name
    GooSymbolKind kind;    // Symbol kind
    GooLSPLocation location; // Symbol location
    char* container_name;  // Container name
};

// Diagnostic information
struct GooLSPDiagnostic {
    GooLSPRange range;              // The range to which this diagnostic applies
    GooDiagnosticSeverity severity; // The severity of this diagnostic
    char* code;                    // A code for this diagnostic
    char* source;                  // The source of this diagnostic
    char* message;                 // The diagnostic message
};

// Completion item
struct GooLSPCompletionItem {
    char* label;            // The label of this completion item
    GooSymbolKind kind;     // The kind of this completion item
    char* detail;           // Additional details about the item
    char* documentation;    // Documentation for the item
    char* insert_text;      // Text to insert
    char* filter_text;      // Text to filter against
    bool preselect;         // Whether this item should be preselected
    int sort_text;          // Sort order (lower is higher priority)
};

// Code action
struct GooLSPCodeAction {
    char* title;            // The title of this code action
    char* kind;             // The kind of this code action
    GooLSPDiagnostic* diagnostics; // Related diagnostics
    int diagnostic_count;   // Number of related diagnostics
    char* edit_text;        // Text edit to apply
    GooLSPRange edit_range; // Range to edit
};

// === LSP Server API ===

// Create a new LSP server
GooLSPServer* goo_lsp_server_create(GooLSPConfig* config);

// Destroy an LSP server
void goo_lsp_server_destroy(GooLSPServer* server);

// Start the LSP server
bool goo_lsp_server_start(GooLSPServer* server);

// Stop the LSP server
void goo_lsp_server_stop(GooLSPServer* server);

// Process an LSP request
bool goo_lsp_server_process_request(GooLSPServer* server, GooLSPRequest* request, GooLSPResponse* response);

// Handle stdin/stdout directly for standard LSP communication
bool goo_lsp_server_run_stdio(GooLSPServer* server);

// Run the server with custom I/O functions
bool goo_lsp_server_run_custom(GooLSPServer* server, 
                              char* (*read_func)(void* context), 
                              void (*write_func)(void* context, const char* data),
                              void* context);

// === Document Management ===

// Open a document
GooLSPDocument* goo_lsp_document_open(GooLSPServer* server, const char* uri, const char* language_id, int version, const char* text);

// Update a document
bool goo_lsp_document_update(GooLSPServer* server, const char* uri, int version, const char* text);

// Close a document
bool goo_lsp_document_close(GooLSPServer* server, const char* uri);

// Get document by URI
GooLSPDocument* goo_lsp_get_document(GooLSPServer* server, const char* uri);

// === Language Features ===

// Get completion items at position
GooLSPCompletionItem* goo_lsp_get_completion(GooLSPServer* server, const char* uri, GooLSPPosition position, int* count);

// Get hover information at position
char* goo_lsp_get_hover(GooLSPServer* server, const char* uri, GooLSPPosition position);

// Get signature help at position
char* goo_lsp_get_signature_help(GooLSPServer* server, const char* uri, GooLSPPosition position);

// Go to definition
GooLSPLocation* goo_lsp_get_definition(GooLSPServer* server, const char* uri, GooLSPPosition position);

// Find references
GooLSPLocation* goo_lsp_find_references(GooLSPServer* server, const char* uri, GooLSPPosition position, bool include_declaration, int* count);

// Document symbols
GooLSPSymbol* goo_lsp_document_symbols(GooLSPServer* server, const char* uri, int* count);

// Workspace symbols
GooLSPSymbol* goo_lsp_workspace_symbols(GooLSPServer* server, const char* query, int* count);

// Format document
char* goo_lsp_format_document(GooLSPServer* server, const char* uri);

// Format range
char* goo_lsp_format_range(GooLSPServer* server, const char* uri, GooLSPRange range);

// Get document diagnostics
GooLSPDiagnostic* goo_lsp_get_diagnostics(GooLSPServer* server, const char* uri, int* count);

// Get code actions
GooLSPCodeAction* goo_lsp_get_code_actions(GooLSPServer* server, const char* uri, GooLSPRange range, int* count);

// === Configuration and Workspace ===

// Initialize workspace
bool goo_lsp_initialize_workspace(GooLSPServer* server, const char* root_uri);

// Index workspace
bool goo_lsp_index_workspace(GooLSPServer* server);

// Update configuration
bool goo_lsp_update_config(GooLSPServer* server, GooLSPConfig* config);

// Add additional include paths
bool goo_lsp_add_include_path(GooLSPServer* server, const char* path);

// === Utility Functions ===

// Convert URI to file path
char* goo_lsp_uri_to_path(const char* uri);

// Convert file path to URI
char* goo_lsp_path_to_uri(const char* path);

// Position utilities
bool goo_lsp_position_less_than(GooLSPPosition pos1, GooLSPPosition pos2);
bool goo_lsp_position_less_equal(GooLSPPosition pos1, GooLSPPosition pos2);
bool goo_lsp_position_equal(GooLSPPosition pos1, GooLSPPosition pos2);

// Range utilities
bool goo_lsp_range_contains_position(GooLSPRange range, GooLSPPosition position);
bool goo_lsp_range_overlaps(GooLSPRange range1, GooLSPRange range2);

#endif // GOO_LSP_H 