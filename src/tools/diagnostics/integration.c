/**
 * @file integration.c
 * @brief Example of integrating the diagnostics system into the main compiler
 */

#include "diagnostics_module.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Mock compiler context (simplified)
typedef struct {
    // Core compiler fields
    const char* input_file;
    const char* output_file;
    bool debug_mode;
    int optimization_level;
    
    // Diagnostics integration
    GooDiagnosticContext* diag_context;
    char* source_code;           // Full source for diagnostics
} GooCompilerContext;

// Create a new compiler context
GooCompilerContext* create_compiler_context() {
    GooCompilerContext* ctx = (GooCompilerContext*)malloc(sizeof(GooCompilerContext));
    if (!ctx) {
        return NULL;
    }
    
    // Initialize with defaults
    ctx->input_file = NULL;
    ctx->output_file = NULL;
    ctx->debug_mode = false;
    ctx->optimization_level = 0;
    ctx->diag_context = NULL;
    ctx->source_code = NULL;
    
    return ctx;
}

// Free the compiler context
void free_compiler_context(GooCompilerContext* ctx) {
    if (!ctx) {
        return;
    }
    
    // Clean up diagnostics context
    if (ctx->diag_context) {
        goo_diagnostics_cleanup(ctx->diag_context);
    }
    
    // Free source code
    free(ctx->source_code);
    
    // Free the context itself
    free(ctx);
}

// Initialize diagnostics in the compiler context
bool init_compiler_diagnostics(GooCompilerContext* ctx, int argc, char** argv) {
    if (!ctx) {
        return false;
    }
    
    // Check if we need to handle the --explain flag
    if (goo_diagnostics_handle_explain(argc, argv)) {
        // Exit early if --explain was handled
        return false;
    }
    
    // Set up diagnostics configuration
    GooDiagnosticsConfig config = goo_diagnostics_default_config();
    
    // Process diagnostic-related command-line arguments
    goo_diagnostics_process_args(argc, argv, &config);
    
    // Initialize the diagnostics system
    ctx->diag_context = goo_diagnostics_init(&config);
    if (!ctx->diag_context) {
        fprintf(stderr, "Failed to initialize diagnostics system\n");
        return false;
    }
    
    return true;
}

// Read input file into memory for diagnostics
bool read_source_file(GooCompilerContext* ctx) {
    if (!ctx || !ctx->input_file) {
        return false;
    }
    
    // Open the file
    FILE* file = fopen(ctx->input_file, "rb");
    if (!file) {
        goo_diagnostics_report_error(ctx->diag_context, 
                                    ctx->input_file, NULL, 0, 0, 0, 
                                    "cannot open file '%s': %s", 
                                    ctx->input_file, strerror(errno));
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory
    ctx->source_code = (char*)malloc(size + 1);
    if (!ctx->source_code) {
        fclose(file);
        goo_diagnostics_report_error(ctx->diag_context, 
                                    ctx->input_file, NULL, 0, 0, 0, 
                                    "cannot allocate memory for source code");
        return false;
    }
    
    // Read file content
    size_t read = fread(ctx->source_code, 1, size, file);
    ctx->source_code[read] = '\0';
    
    // Close the file
    fclose(file);
    
    return true;
}

// Example of lexer reporting an error
void lexer_report_error(GooCompilerContext* ctx, 
                       unsigned int line, unsigned int column, unsigned int length,
                       const char* message, ...) {
    if (!ctx || !ctx->diag_context) {
        return;
    }
    
    // Format the message
    char formatted_message[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);
    
    // Report the error
    goo_diagnostics_report_error(ctx->diag_context, 
                               ctx->input_file, ctx->source_code,
                               line, column, length,
                               formatted_message);
}

// Example of parser reporting an error with multiple related notes
void parser_report_complex_error(GooCompilerContext* ctx, 
                                unsigned int line, unsigned int column, unsigned int length,
                                const char* message) {
    if (!ctx || !ctx->diag_context) {
        return;
    }
    
    // Create the main diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_ERROR,
        ctx->input_file,
        line, column, length,
        message
    );
    
    if (!diag) {
        return;
    }
    
    // Example: Add related notes for complex errors
    // (In real code, these would point to actual relevant code locations)
    goo_diag_add_child(
        diag, GOO_DIAG_NOTE,
        ctx->input_file, line - 2, 1, 10,
        "variable first declared here"
    );
    
    goo_diag_add_child(
        diag, GOO_DIAG_NOTE,
        ctx->input_file, line, column, length,
        "invalid use here"
    );
    
    // Add a suggestion for fixing the error
    goo_diag_add_suggestion(
        diag,
        ctx->input_file, line, column, length,
        "try using the correct type",
        "let value: string = \"text\";",
        GOO_APPLICABILITY_MACHINE_APPLICABLE
    );
    
    // Set error code and explanation
    goo_diag_set_code(
        diag, "E0101",
        "Mismatched types error"
    );
    
    // Emit the diagnostic
    goo_diag_emit(ctx->diag_context, diag);
    
    // Print source highlighting
    GooHighlightOptions options = goo_highlight_options_default();
    goo_print_highlighted_source(diag, ctx->source_code, &options);
}

// Example of type checker reporting a warning
void type_checker_report_warning(GooCompilerContext* ctx, 
                                unsigned int line, unsigned int column, unsigned int length,
                                const char* message, ...) {
    if (!ctx || !ctx->diag_context) {
        return;
    }
    
    // Format the message
    char formatted_message[1024];
    va_list args;
    va_start(args, message);
    vsnprintf(formatted_message, sizeof(formatted_message), message, args);
    va_end(args);
    
    // Report the warning
    goo_diagnostics_report_warning(ctx->diag_context, 
                                 ctx->input_file, ctx->source_code,
                                 line, column, length,
                                 formatted_message);
}

// Example of how to check for errors and show summary
bool compile_and_check_errors(GooCompilerContext* ctx) {
    // Do compilation work...
    
    // Example: Report some errors and warnings
    lexer_report_error(ctx, 10, 5, 3, "unexpected token '%s'", "@@");
    parser_report_complex_error(ctx, 15, 10, 8, "mismatched types");
    type_checker_report_warning(ctx, 20, 12, 6, "unused variable '%s'", "result");
    
    // Print diagnostics summary
    goo_diagnostics_print_summary(ctx->diag_context, ctx->source_code);
    
    // Check if compilation should be aborted
    if (goo_diagnostics_should_abort(ctx->diag_context)) {
        fprintf(stderr, "Compilation aborted due to errors\n");
        return false;
    }
    
    return true;
}

// Example main function showing integration
int main(int argc, char** argv) {
    // Create compiler context
    GooCompilerContext* ctx = create_compiler_context();
    if (!ctx) {
        fprintf(stderr, "Failed to create compiler context\n");
        return 1;
    }
    
    // Set up input/output files (normally from command line)
    ctx->input_file = "example.goo";
    ctx->output_file = "example";
    
    // Initialize diagnostics system
    if (!init_compiler_diagnostics(ctx, argc, argv)) {
        free_compiler_context(ctx);
        return 1;
    }
    
    // Read source file
    if (!read_source_file(ctx)) {
        free_compiler_context(ctx);
        return 1;
    }
    
    // Compile and check for errors
    bool success = compile_and_check_errors(ctx);
    
    // Clean up
    free_compiler_context(ctx);
    
    return success ? 0 : 1;
} 