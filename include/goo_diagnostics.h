/**
 * @file goo_diagnostics.h
 * @brief Main include file for the Goo compiler diagnostics system
 */

#ifndef GOO_DIAGNOSTICS_MAIN_H
#define GOO_DIAGNOSTICS_MAIN_H

#include "../src/tools/diagnostics/diagnostics.h"
#include "../src/tools/diagnostics/error_catalog.h"
#include "../src/tools/diagnostics/source_highlight.h"
#include "../src/tools/diagnostics/diagnostics_module.h"

/**
 * @brief Initialize the diagnostics system for the Goo compiler
 * 
 * This is a convenience function that initializes the entire diagnostics
 * system with default settings.
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return A pointer to the initialized diagnostic context, or NULL on failure
 */
GooDiagnosticContext* goo_init_diagnostics(int argc, char** argv);

/**
 * @brief Clean up and free resources used by the diagnostics system
 * 
 * @param ctx The diagnostic context to clean up
 */
void goo_cleanup_diagnostics(GooDiagnosticContext* ctx);

#endif /* GOO_DIAGNOSTICS_MAIN_H */ 