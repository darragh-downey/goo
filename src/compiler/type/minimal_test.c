#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../tools/diagnostics/diagnostics.h"
#include "../../tools/diagnostics/diagnostics_module.h"

int main(int argc, char** argv) {
    // Initialize the diagnostics system
    GooDiagnosticContext* ctx = goo_init_diagnostics(argc, argv);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize diagnostics system\n");
        return 1;
    }
    
    // Create source location
    const char* filename = "test.goo";
    unsigned int line = 10;
    unsigned int column = 5;
    unsigned int length = 15;
    const char* message = "This is a test error message";
    
    // Instead of creating a complex diagnostic, let's use the higher level API
    goo_diagnostics_report_error(
        ctx,
        filename,
        NULL, // No source code
        line,
        column,
        length,
        message
    );
    
    // Print all diagnostics
    goo_diag_print_all(ctx);
    
    // Print a summary
    printf("Diagnostics summary: %d errors, %d warnings\n", 
           goo_diag_error_count(ctx),
           goo_diag_warning_count(ctx));
    
    // Clean up
    goo_cleanup_diagnostics(ctx);
    
    return 0;
} 