/**
 * @file source_highlight.h
 * @brief Source code highlighting for diagnostics
 * 
 * This file defines functions for highlighting source code in terminal output,
 * making errors and warnings more visually clear to users.
 */

#ifndef GOO_SOURCE_HIGHLIGHT_H
#define GOO_SOURCE_HIGHLIGHT_H

#include <stdbool.h>
#include <stddef.h>
#include "diagnostics.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Settings for source highlighting
 */
typedef struct {
    bool use_color;              // Whether to use colors
    bool show_line_numbers;      // Whether to show line numbers
    bool highlight_full_line;    // Whether to highlight the entire line
    int context_lines;           // Number of context lines to show before/after
    bool unicode;                // Whether to use Unicode box-drawing characters
} GooHighlightOptions;

/**
 * Default highlight options with reasonable settings
 */
GooHighlightOptions goo_highlight_options_default(void);

/**
 * Highlight source code around a diagnostic location
 * 
 * @param diagnostic The diagnostic to highlight
 * @param source_code The complete source code
 * @param options Highlighting options
 * @param output_buffer Buffer to write the highlighted code to
 * @param buffer_size Size of the output buffer
 * @return Number of bytes written to the output buffer
 */
size_t goo_highlight_source(
    const GooDiagnostic* diagnostic,
    const char* source_code,
    const GooHighlightOptions* options,
    char* output_buffer,
    size_t buffer_size
);

/**
 * Highlight a specific region of source code
 * 
 * @param source_code The complete source code
 * @param highlight_line Line number to highlight (1-based)
 * @param highlight_column Column to start highlighting (1-based)
 * @param highlight_length Length of the region to highlight
 * @param options Highlighting options
 * @param output_buffer Buffer to write the highlighted code to
 * @param buffer_size Size of the output buffer
 * @return Number of bytes written to the output buffer
 */
size_t goo_highlight_region(
    const char* source_code,
    unsigned int highlight_line,
    unsigned int highlight_column,
    unsigned int highlight_length,
    const GooHighlightOptions* options,
    char* output_buffer,
    size_t buffer_size
);

/**
 * Print highlighted source code for a diagnostic to stderr
 * 
 * @param diagnostic The diagnostic to highlight
 * @param source_code The complete source code
 * @param options Highlighting options (NULL for defaults)
 */
void goo_print_highlighted_source(
    const GooDiagnostic* diagnostic,
    const char* source_code,
    const GooHighlightOptions* options
);

/**
 * Print highlighted source code for multiple diagnostics to stderr
 * 
 * @param diagnostics Array of diagnostics
 * @param count Number of diagnostics
 * @param source_code The complete source code
 * @param options Highlighting options (NULL for defaults)
 */
void goo_print_highlighted_diagnostics(
    const GooDiagnostic** diagnostics,
    size_t count,
    const char* source_code,
    const GooHighlightOptions* options
);

#ifdef __cplusplus
}
#endif

#endif /* GOO_SOURCE_HIGHLIGHT_H */ 