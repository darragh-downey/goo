/**
 * @file goo_tools.h
 * @brief Unified header for Goo compiler tools
 *
 * This file includes all of the available Goo compiler tools
 * for easy access from the main compiler and other utilities.
 */

#ifndef GOO_TOOLS_H
#define GOO_TOOLS_H

// Diagnostics System
#include "goo_diagnostics.h"

// Code Formatter
#include "../src/tools/formatter/formatter.h"

// Symbol Management
#include "../src/tools/symbols/symbols.h"

// Linter
#include "../src/tools/linter/linter.h"

/**
 * Global initialization for all compiler tools
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @return true on success, false on failure
 */
bool goo_tools_init(int argc, char** argv);

/**
 * Global cleanup for all compiler tools
 */
void goo_tools_cleanup(void);

#endif /* GOO_TOOLS_H */ 