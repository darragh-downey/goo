/**
 * @file goo_tools.c
 * @brief Implementation of unified Goo compiler tools functionality
 */

#include "../include/goo_tools.h"
#include <stdio.h>
#include <stdlib.h>

// Global tool contexts
static GooDiagnosticContext* global_diag_context = NULL;
static GooLintContext* global_lint_context = NULL;
static GooSymbolTable* global_symbol_table = NULL;

/**
 * Global initialization for all compiler tools
 */
bool goo_tools_init(int argc, char** argv) {
    // Initialize diagnostics
    global_diag_context = goo_init_diagnostics(argc, argv);
    if (!global_diag_context) {
        return false;
    }
    
    // Initialize linter (if available)
    #ifdef GOO_LINTER_H
    global_lint_context = goo_lint_context_new();
    if (!global_lint_context) {
        goo_cleanup_diagnostics(global_diag_context);
        return false;
    }
    #endif
    
    // Initialize symbol table
    #ifdef GOO_SYMBOLS_H
    global_symbol_table = goo_symbol_table_new();
    if (!global_symbol_table) {
        #ifdef GOO_LINTER_H
        goo_lint_context_free(global_lint_context);
        #endif
        goo_cleanup_diagnostics(global_diag_context);
        return false;
    }
    #endif
    
    return true;
}

/**
 * Global cleanup for all compiler tools
 */
void goo_tools_cleanup(void) {
    // Clean up diagnostics
    if (global_diag_context) {
        goo_cleanup_diagnostics(global_diag_context);
        global_diag_context = NULL;
    }
    
    // Clean up linter
    #ifdef GOO_LINTER_H
    if (global_lint_context) {
        goo_lint_context_free(global_lint_context);
        global_lint_context = NULL;
    }
    #endif
    
    // Clean up symbol table
    #ifdef GOO_SYMBOLS_H
    if (global_symbol_table) {
        goo_symbol_table_free(global_symbol_table);
        global_symbol_table = NULL;
    }
    #endif
} 