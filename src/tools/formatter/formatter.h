/**
 * @file formatter.h
 * @brief Code formatting utility for the Goo programming language
 * 
 * This file defines the interfaces for automatic code formatting in the Goo
 * compiler, inspired by tools like gofmt and rustfmt.
 */

#ifndef GOO_FORMATTER_H
#define GOO_FORMATTER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Configuration options for code formatting
 */
typedef struct {
    int tab_width;                 // Number of spaces per tab (default: 4)
    bool use_tabs;                 // Use tabs instead of spaces (default: false)
    int max_width;                 // Maximum line width (default: 100)
    bool format_comments;          // Format comments (default: true)
    bool reflow_comments;          // Reflow comments to fit max_width (default: true)
    bool align_comments;           // Align consecutive line comments (default: true)
    bool brace_style_same_line;    // Open braces on same line (default: true)
    bool spaces_around_operators;  // Add spaces around operators (default: true)
    bool compact_array_init;       // Compact array initializations (default: false)
} GooFormatterOptions;

/**
 * Initialize default formatting options
 */
GooFormatterOptions goo_formatter_default_options(void);

/**
 * Format source code using the specified options
 * 
 * @param source The source code to format
 * @param options Formatting options
 * @param output_buffer Buffer to store the formatted code
 * @param buffer_size Size of the output buffer
 * @return Number of bytes written to the output buffer, or -1 on error
 */
int goo_format_source(
    const char* source,
    const GooFormatterOptions* options,
    char* output_buffer,
    size_t buffer_size
);

/**
 * Format a source file in place
 * 
 * @param filename Path to the file to format
 * @param options Formatting options
 * @return true on success, false on error
 */
bool goo_format_file(
    const char* filename,
    const GooFormatterOptions* options
);

/**
 * Format a source file, writing the result to a different file
 * 
 * @param input_filename Path to the input file
 * @param output_filename Path to the output file
 * @param options Formatting options
 * @return true on success, false on error
 */
bool goo_format_file_to(
    const char* input_filename,
    const char* output_filename,
    const GooFormatterOptions* options
);

/**
 * Check if a file needs formatting
 * 
 * @param filename Path to the file to check
 * @param options Formatting options
 * @return true if the file would be changed by formatting, false otherwise
 */
bool goo_file_needs_formatting(
    const char* filename,
    const GooFormatterOptions* options
);

/**
 * Process command-line arguments to configure formatter options
 * 
 * @param argc Command line argument count
 * @param argv Command line arguments
 * @param options Options to configure
 * @return true if processing succeeded, false on error
 */
bool goo_formatter_process_args(
    int argc,
    char** argv,
    GooFormatterOptions* options
);

#ifdef __cplusplus
}
#endif

#endif /* GOO_FORMATTER_H */ 