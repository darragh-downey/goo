/**
 * @file goo_diagnostics.c
 * @brief Implementation of the main Goo diagnostics interface
 */

#include "diagnostics_module.h"
#include <stdlib.h>

GooDiagnosticContext* goo_init_diagnostics(int argc, char** argv) {
    // Create default configuration
    GooDiagnosticsConfig config = goo_diagnostics_default_config();
    
    // Process command line arguments to update configuration
    if (!goo_diagnostics_process_args(argc, argv, &config)) {
        return NULL;
    }
    
    // Check if we're just explaining an error code
    if (goo_diagnostics_handle_explain(argc, argv)) {
        // Exit early if we handled an --explain flag
        exit(0);
    }
    
    // Initialize the diagnostics system with the configuration
    return goo_diagnostics_init(&config);
}

void goo_cleanup_diagnostics(GooDiagnosticContext* ctx) {
    goo_diagnostics_cleanup(ctx);
} 