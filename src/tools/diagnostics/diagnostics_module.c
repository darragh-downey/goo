/**
 * @file diagnostics_module.c
 * @brief Implementation of the central diagnostics module
 */

#include "diagnostics_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

GooDiagnosticsConfig goo_diagnostics_default_config(void) {
    GooDiagnosticsConfig config;
    config.enable_colors = true;
    config.json_output = false;
    config.treat_warnings_as_errors = false;
    config.error_limit = 0;  // No limit
    config.context_lines = 3;
    config.unicode = true;
    config.machine_applicable_only = false;
    
    return config;
}

GooDiagnosticContext* goo_diagnostics_init(const GooDiagnosticsConfig* config) {
    // Initialize the error catalog
    if (!goo_error_catalog_init()) {
        fprintf(stderr, "Failed to initialize error catalog\n");
        return NULL;
    }
    
    // Create diagnostic context
    GooDiagnosticContext* context = goo_diag_context_new();
    if (!context) {
        goo_error_catalog_cleanup();
        return NULL;
    }
    
    // Apply configuration if provided
    if (config) {
        context->colored_output = config->enable_colors;
        context->json_output = config->json_output;
        context->treat_warnings_as_errors = config->treat_warnings_as_errors;
        context->error_limit = config->error_limit;
    }
    
    return context;
}

void goo_diagnostics_cleanup(GooDiagnosticContext* context) {
    if (context) {
        goo_diag_context_free(context);
    }
    
    goo_error_catalog_cleanup();
}

bool goo_diagnostics_process_args(int argc, char** argv, GooDiagnosticsConfig* config) {
    if (!config) {
        return false;
    }
    
    // Process command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--color=never") == 0 || 
            strcmp(argv[i], "--no-color") == 0) {
            config->enable_colors = false;
        }
        else if (strcmp(argv[i], "--color=always") == 0) {
            config->enable_colors = true;
        }
        else if (strcmp(argv[i], "--json") == 0) {
            config->json_output = true;
        }
        else if (strcmp(argv[i], "-Werror") == 0 || 
                 strcmp(argv[i], "--warnings-as-errors") == 0) {
            config->treat_warnings_as_errors = true;
        }
        else if (strncmp(argv[i], "--error-limit=", 14) == 0) {
            config->error_limit = atoi(argv[i] + 14);
        }
        else if (strncmp(argv[i], "--context-lines=", 16) == 0) {
            config->context_lines = atoi(argv[i] + 16);
        }
        else if (strcmp(argv[i], "--no-unicode") == 0) {
            config->unicode = false;
        }
        else if (strcmp(argv[i], "--machine-fixes-only") == 0) {
            config->machine_applicable_only = true;
        }
    }
    
    return true;
}

bool goo_diagnostics_handle_explain(int argc, char** argv) {
    // Check for --explain argument
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--explain") == 0) {
            const char* code = argv[i + 1];
            
            // Initialize error catalog
            if (!goo_error_catalog_init()) {
                fprintf(stderr, "Failed to initialize error catalog\n");
                return true;
            }
            
            // Explain the error code
            bool result = goo_error_catalog_explain(code);
            
            // Clean up
            goo_error_catalog_cleanup();
            
            if (!result) {
                fprintf(stderr, "Error code '%s' not found in the catalog.\n", code);
            }
            
            // Return true to indicate we handled this flag
            return true;
        }
    }
    
    // --explain flag not found
    return false;
}

void goo_diagnostics_report_error(
    GooDiagnosticContext* context,
    const char* filename,
    const char* source,
    unsigned int line,
    unsigned int column,
    unsigned int length,
    const char* message,
    ...
) {
    if (!context || !filename || !message) {
        return;
    }
    
    // Format the message
    char formatted_message[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);
    
    // Create diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_ERROR,
        filename,
        line,
        column,
        length,
        formatted_message
    );
    
    if (!diag) {
        return;
    }
    
    // Emit the diagnostic
    goo_diag_emit(context, diag);
    
    // If source is provided, print source highlighting
    if (source) {
        GooHighlightOptions options = goo_highlight_options_default();
        goo_print_highlighted_source(diag, source, &options);
    }
}

void goo_diagnostics_report_warning(
    GooDiagnosticContext* context,
    const char* filename,
    const char* source,
    unsigned int line,
    unsigned int column,
    unsigned int length,
    const char* message,
    ...
) {
    if (!context || !filename || !message) {
        return;
    }
    
    // Format the message
    char formatted_message[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);
    
    // Create diagnostic
    GooDiagnosticLevel level = context->treat_warnings_as_errors ?
                                GOO_DIAG_ERROR : GOO_DIAG_WARNING;
    
    GooDiagnostic* diag = goo_diag_new(
        level,
        filename,
        line,
        column,
        length,
        formatted_message
    );
    
    if (!diag) {
        return;
    }
    
    // Emit the diagnostic
    goo_diag_emit(context, diag);
    
    // If source is provided, print source highlighting
    if (source) {
        GooHighlightOptions options = goo_highlight_options_default();
        goo_print_highlighted_source(diag, source, &options);
    }
}

void goo_diagnostics_print_summary(
    GooDiagnosticContext* context,
    const char* source
) {
    if (!context) {
        return;
    }
    
    // If JSON output is enabled, print all diagnostics as JSON
    if (context->json_output) {
        goo_diag_print_json(context);
        return;
    }
    
    // Otherwise, print normal summary
    if (context->error_count > 0 || context->warning_count > 0) {
        fprintf(stderr, "Summary: ");
        
        if (context->error_count > 0) {
            fprintf(stderr, "%d %s", 
                context->error_count, 
                context->error_count == 1 ? "error" : "errors");
                
            if (context->warning_count > 0) {
                fprintf(stderr, ", ");
            }
        }
        
        if (context->warning_count > 0) {
            fprintf(stderr, "%d %s", 
                context->warning_count, 
                context->warning_count == 1 ? "warning" : "warnings");
        }
        
        fprintf(stderr, "\n");
    }
}

bool goo_diagnostics_should_abort(GooDiagnosticContext* context) {
    if (!context) {
        return true;  // Be conservative if context is NULL
    }
    
    return context->error_count > 0;
} 