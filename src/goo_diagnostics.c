/**
 * @file goo_diagnostics.c
 * @brief Implementation of convenience functions for the Goo compiler diagnostics system
 */

#include "../include/goo_diagnostics.h"
#include "tools/diagnostics/diagnostics_module.h"

/**
 * Initialize the diagnostics system for the Goo compiler
 */
GooDiagnosticContext* goo_init_diagnostics(int argc, char** argv) {
    // Check if we need to handle the --explain flag
    if (goo_diagnostics_handle_explain(argc, argv)) {
        // Exit early - the explanation was handled
        return NULL;
    }
    
    // Set up the configuration with defaults
    GooDiagnosticsConfig config = goo_diagnostics_default_config();
    
    // Process command line arguments to configure diagnostics
    goo_diagnostics_process_args(argc, argv, &config);
    
    // Initialize the diagnostics system
    return goo_diagnostics_init(&config);
}

/**
 * Clean up and free resources used by the diagnostics system
 */
void goo_cleanup_diagnostics(GooDiagnosticContext* ctx) {
    if (ctx != NULL) {
        goo_diagnostics_cleanup(ctx);
    }
} 