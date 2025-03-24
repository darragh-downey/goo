/**
 * @file goo_lsp_main.c
 * @brief Main entry point for the Goo Language Server Protocol server
 *
 * This file contains the main function for running the LSP server as a
 * standalone executable.
 *
 * @copyright Copyright (c) 2023, Goo Language Authors
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "goo_lsp_server.h"

// Print usage information
static void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h, --help                Display this help message\n");
    fprintf(stderr, "  -v, --verbose             Enable verbose logging\n");
    fprintf(stderr, "  -s, --std-lib PATH        Path to the Goo standard library\n");
    fprintf(stderr, "  --no-diagnostics          Disable diagnostic reporting\n");
    fprintf(stderr, "  --no-hover                Disable hover information\n");
    fprintf(stderr, "  --no-completion           Disable code completion\n");
    fprintf(stderr, "  --no-definition           Disable go-to-definition\n");
    fprintf(stderr, "  --no-references           Disable find references\n");
    fprintf(stderr, "  --no-formatting           Disable document formatting\n");
    fprintf(stderr, "  --no-symbols              Disable document symbols\n");
    fprintf(stderr, "  --no-highlight            Disable document highlighting\n");
    fprintf(stderr, "  --no-rename               Disable rename symbol\n");
    fprintf(stderr, "  --no-signature-help       Disable signature help\n");
    fprintf(stderr, "  --version                 Display version information\n");
}

// Print version information
static void print_version() {
    fprintf(stderr, "Goo Language Server Protocol Server v0.1.0\n");
    fprintf(stderr, "Copyright (c) 2023, Goo Language Authors\n");
    fprintf(stderr, "Licensed under MIT\n");
}

int main(int argc, char** argv) {
    // Default configuration
    GooLspServerConfig config = {
        .enable_diagnostics = true,
        .enable_hover = true,
        .enable_completion = true,
        .enable_definition = true,
        .enable_references = true,
        .enable_formatting = true,
        .enable_symbols = true,
        .enable_highlight = true,
        .enable_rename = true,
        .enable_signature_help = true,
        .std_lib_path = NULL,
        .verbose = false
    };
    
    // Long options
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"std-lib", required_argument, 0, 's'},
        {"no-diagnostics", no_argument, 0, 1000},
        {"no-hover", no_argument, 0, 1001},
        {"no-completion", no_argument, 0, 1002},
        {"no-definition", no_argument, 0, 1003},
        {"no-references", no_argument, 0, 1004},
        {"no-formatting", no_argument, 0, 1005},
        {"no-symbols", no_argument, 0, 1006},
        {"no-highlight", no_argument, 0, 1007},
        {"no-rename", no_argument, 0, 1008},
        {"no-signature-help", no_argument, 0, 1009},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };
    
    // Parse command line arguments
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "hvs:V", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            case 'v':
                config.verbose = true;
                break;
                
            case 's':
                config.std_lib_path = strdup(optarg);
                break;
                
            case 'V':
                print_version();
                return 0;
                
            case 1000:
                config.enable_diagnostics = false;
                break;
                
            case 1001:
                config.enable_hover = false;
                break;
                
            case 1002:
                config.enable_completion = false;
                break;
                
            case 1003:
                config.enable_definition = false;
                break;
                
            case 1004:
                config.enable_references = false;
                break;
                
            case 1005:
                config.enable_formatting = false;
                break;
                
            case 1006:
                config.enable_symbols = false;
                break;
                
            case 1007:
                config.enable_highlight = false;
                break;
                
            case 1008:
                config.enable_rename = false;
                break;
                
            case 1009:
                config.enable_signature_help = false;
                break;
                
            case '?':
                // getopt_long already printed an error message
                return 1;
                
            default:
                abort();
        }
    }
    
    // Create and run the server
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