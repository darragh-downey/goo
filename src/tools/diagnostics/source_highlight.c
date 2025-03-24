/**
 * @file source_highlight.c
 * @brief Implementation of source code highlighting for diagnostics
 */

#include "source_highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// ANSI color codes
#define COLOR_RESET         "\x1b[0m"
#define COLOR_RED           "\x1b[31m"
#define COLOR_GREEN         "\x1b[32m"
#define COLOR_YELLOW        "\x1b[33m"
#define COLOR_BLUE          "\x1b[34m"
#define COLOR_MAGENTA       "\x1b[35m"
#define COLOR_CYAN          "\x1b[36m"
#define COLOR_BOLD          "\x1b[1m"
#define COLOR_DIM           "\x1b[2m"
#define COLOR_UNDERLINE     "\x1b[4m"
#define COLOR_BG_RED        "\x1b[41m"
#define COLOR_BG_GREEN      "\x1b[42m"
#define COLOR_BG_YELLOW     "\x1b[43m"
#define COLOR_BG_BLUE       "\x1b[44m"
#define COLOR_BG_MAGENTA    "\x1b[45m"
#define COLOR_BG_CYAN       "\x1b[46m"

// Box drawing characters
#define UNICODE_HORIZONTAL          "─"
#define UNICODE_VERTICAL            "│"
#define UNICODE_DOWN_RIGHT          "┌"
#define UNICODE_DOWN_LEFT           "┐"
#define UNICODE_UP_RIGHT            "└"
#define UNICODE_UP_LEFT             "┘"
#define UNICODE_VERTICAL_RIGHT      "├"
#define UNICODE_VERTICAL_LEFT       "┤"
#define UNICODE_ARROW_RIGHT         "→"
#define UNICODE_ARROW_LEFT          "←"

// ASCII alternatives for box drawing
#define ASCII_HORIZONTAL            "-"
#define ASCII_VERTICAL              "|"
#define ASCII_DOWN_RIGHT            "+"
#define ASCII_DOWN_LEFT             "+"
#define ASCII_UP_RIGHT              "+"
#define ASCII_UP_LEFT               "+"
#define ASCII_VERTICAL_RIGHT        "+"
#define ASCII_VERTICAL_LEFT         "+"
#define ASCII_ARROW_RIGHT           ">"
#define ASCII_ARROW_LEFT            "<"

// Maximum line length for highlighting
#define MAX_LINE_LENGTH 1024

// Forward declarations of private functions
static char** split_lines(const char* source, size_t* line_count);
static void free_line_array(char** lines, size_t count);
static size_t count_digits(unsigned int num);
static size_t find_line_start(const char* source, unsigned int line);
static size_t find_line_end(const char* source, size_t start);
static size_t write_error_pointer(
    char* buffer, 
    size_t buffer_size, 
    unsigned int column, 
    unsigned int length, 
    unsigned int line_number_width,
    const GooHighlightOptions* options
);

// Implementation of public functions

GooHighlightOptions goo_highlight_options_default(void) {
    GooHighlightOptions options;
    options.use_color = true;
    options.show_line_numbers = true;
    options.highlight_full_line = false;
    options.context_lines = 3;
    options.unicode = true;
    
    return options;
}

size_t goo_highlight_source(
    const GooDiagnostic* diagnostic,
    const char* source_code,
    const GooHighlightOptions* options,
    char* output_buffer,
    size_t buffer_size
) {
    if (!diagnostic || !source_code || !output_buffer || buffer_size == 0) {
        return 0;
    }
    
    return goo_highlight_region(
        source_code,
        diagnostic->primary_location.line,
        diagnostic->primary_location.column,
        diagnostic->primary_location.length,
        options,
        output_buffer,
        buffer_size
    );
}

size_t goo_highlight_region(
    const char* source_code,
    unsigned int highlight_line,
    unsigned int highlight_column,
    unsigned int highlight_length,
    const GooHighlightOptions* options,
    char* output_buffer,
    size_t buffer_size
) {
    if (!source_code || !output_buffer || buffer_size == 0) {
        return 0;
    }
    
    // Use default options if none provided
    GooHighlightOptions default_options;
    if (!options) {
        default_options = goo_highlight_options_default();
        options = &default_options;
    }
    
    // Split the source into lines
    size_t line_count = 0;
    char** lines = split_lines(source_code, &line_count);
    if (!lines) {
        return 0;
    }
    
    // Calculate the range of lines to show
    unsigned int first_line = highlight_line - options->context_lines;
    if (first_line < 1) {
        first_line = 1;
    }
    
    unsigned int last_line = highlight_line + options->context_lines;
    if (last_line > line_count) {
        last_line = line_count;
    }
    
    // Calculate the width needed for line numbers
    unsigned int line_number_width = count_digits(last_line);
    
    // Buffer for a single line
    char line_buffer[MAX_LINE_LENGTH];
    
    // Buffer position
    size_t pos = 0;
    
    // Write lines to the output buffer
    for (unsigned int i = first_line; i <= last_line; i++) {
        if (pos >= buffer_size - 1) {
            break;
        }
        
        // Get the line (0-based index)
        const char* line = lines[i - 1];
        
        // Format line number with padding
        if (options->show_line_numbers) {
            int written = snprintf(line_buffer, sizeof(line_buffer), 
                                  "%s%*u %s%s",
                                  (options->use_color ? COLOR_DIM : ""),
                                  line_number_width, i,
                                  (options->use_color ? COLOR_RESET : ""),
                                  (options->unicode ? UNICODE_VERTICAL : ASCII_VERTICAL));
            
            if (written > 0) {
                size_t to_copy = (written < buffer_size - pos - 1) ? written : buffer_size - pos - 1;
                memcpy(output_buffer + pos, line_buffer, to_copy);
                pos += to_copy;
            }
        }
        
        // Determine if this is the highlighted line
        bool is_highlight_line = (i == highlight_line);
        
        // Write the line content
        if (is_highlight_line && options->highlight_full_line && options->use_color) {
            int written = snprintf(line_buffer, sizeof(line_buffer), " %s%s%s\n",
                                  COLOR_BG_RED, line, COLOR_RESET);
            
            if (written > 0) {
                size_t to_copy = (written < buffer_size - pos - 1) ? written : buffer_size - pos - 1;
                memcpy(output_buffer + pos, line_buffer, to_copy);
                pos += to_copy;
            }
        } else {
            int written = snprintf(line_buffer, sizeof(line_buffer), " %s\n", line);
            if (written > 0) {
                size_t to_copy = (written < buffer_size - pos - 1) ? written : buffer_size - pos - 1;
                memcpy(output_buffer + pos, line_buffer, to_copy);
                pos += to_copy;
            }
        }
        
        // Add the error pointer line if this is the highlighted line
        if (is_highlight_line) {
            size_t written = write_error_pointer(
                output_buffer + pos, buffer_size - pos,
                highlight_column, highlight_length, 
                line_number_width, options
            );
            
            pos += written;
        }
    }
    
    // Ensure null termination
    if (pos < buffer_size) {
        output_buffer[pos] = '\0';
    } else {
        output_buffer[buffer_size - 1] = '\0';
    }
    
    // Clean up
    free_line_array(lines, line_count);
    
    return pos;
}

void goo_print_highlighted_source(
    const GooDiagnostic* diagnostic,
    const char* source_code,
    const GooHighlightOptions* options
) {
    if (!diagnostic || !source_code) {
        return;
    }
    
    // Use default options if none provided
    GooHighlightOptions default_options;
    if (!options) {
        default_options = goo_highlight_options_default();
        options = &default_options;
    }
    
    // Buffer for the highlighted source
    char buffer[4096];  // Reasonably sized buffer for most cases
    
    // Highlight the source
    size_t length = goo_highlight_source(diagnostic, source_code, options, buffer, sizeof(buffer));
    
    // Print it
    if (length > 0) {
        fprintf(stderr, "%s", buffer);
    }
}

void goo_print_highlighted_diagnostics(
    const GooDiagnostic** diagnostics,
    size_t count,
    const char* source_code,
    const GooHighlightOptions* options
) {
    if (!diagnostics || !source_code || count == 0) {
        return;
    }
    
    // Use default options if none provided
    GooHighlightOptions default_options;
    if (!options) {
        default_options = goo_highlight_options_default();
        options = &default_options;
    }
    
    // Print each diagnostic
    for (size_t i = 0; i < count; i++) {
        const GooDiagnostic* diagnostic = diagnostics[i];
        
        // Print the diagnostic message first
        if (diagnostic->primary_location.filename) {
            const char* level_str;
            const char* level_color = "";
            const char* reset = "";
            
            switch (diagnostic->level) {
                case GOO_DIAG_ERROR:
                    level_str = "error";
                    if (options->use_color) {
                        level_color = COLOR_BOLD COLOR_RED;
                        reset = COLOR_RESET;
                    }
                    break;
                    
                case GOO_DIAG_WARNING:
                    level_str = "warning";
                    if (options->use_color) {
                        level_color = COLOR_BOLD COLOR_YELLOW;
                        reset = COLOR_RESET;
                    }
                    break;
                    
                case GOO_DIAG_NOTE:
                    level_str = "note";
                    if (options->use_color) {
                        level_color = COLOR_BOLD COLOR_BLUE;
                        reset = COLOR_RESET;
                    }
                    break;
                    
                case GOO_DIAG_HELP:
                    level_str = "help";
                    if (options->use_color) {
                        level_color = COLOR_BOLD COLOR_CYAN;
                        reset = COLOR_RESET;
                    }
                    break;
                    
                case GOO_DIAG_ICE:
                    level_str = "internal compiler error";
                    if (options->use_color) {
                        level_color = COLOR_BOLD COLOR_RED;
                        reset = COLOR_RESET;
                    }
                    break;
                    
                default:
                    level_str = "unknown";
                    break;
            }
            
            fprintf(stderr, "%s:%u:%u: %s%s%s: %s\n", 
                    diagnostic->primary_location.filename, 
                    diagnostic->primary_location.line, 
                    diagnostic->primary_location.column,
                    level_color, level_str, reset,
                    diagnostic->message);
        } else {
            fprintf(stderr, "%s: %s\n", 
                    diagnostic->level == GOO_DIAG_ERROR ? "error" : 
                    diagnostic->level == GOO_DIAG_WARNING ? "warning" : 
                    diagnostic->level == GOO_DIAG_NOTE ? "note" : 
                    diagnostic->level == GOO_DIAG_HELP ? "help" : "ice",
                    diagnostic->message);
        }
        
        // Print the highlighted source
        goo_print_highlighted_source(diagnostic, source_code, options);
        
        // Print suggestions if any
        for (size_t j = 0; j < diagnostic->suggestions_count; j++) {
            GooSuggestion* suggestion = diagnostic->suggestions[j];
            
            // Print the suggestion message
            fprintf(stderr, "%shelp%s: %s\n", 
                   options->use_color ? COLOR_BOLD COLOR_CYAN : "",
                   options->use_color ? COLOR_RESET : "",
                   suggestion->message);
            
            // Print the suggested replacement if it exists
            if (suggestion->suggested_replacement) {
                fprintf(stderr, "%s\n", suggestion->suggested_replacement);
            }
        }
        
        // Print separator between diagnostics
        fprintf(stderr, "\n");
    }
}

// Private helper functions

static char** split_lines(const char* source, size_t* line_count) {
    if (!source || !line_count) {
        return NULL;
    }
    
    // Count the number of lines
    size_t count = 1;  // At least one line
    for (const char* p = source; *p; p++) {
        if (*p == '\n') {
            count++;
        }
    }
    
    // Allocate the array of line pointers
    char** lines = (char**)malloc(count * sizeof(char*));
    if (!lines) {
        return NULL;
    }
    
    // Copy each line
    const char* start = source;
    size_t line_index = 0;
    
    for (const char* p = source; ; p++) {
        if (*p == '\n' || *p == '\0') {
            // Calculate line length (excluding newline)
            size_t length = p - start;
            
            // Allocate and copy the line
            char* line = (char*)malloc(length + 1);
            if (!line) {
                // Clean up already allocated lines
                for (size_t i = 0; i < line_index; i++) {
                    free(lines[i]);
                }
                free(lines);
                return NULL;
            }
            
            memcpy(line, start, length);
            line[length] = '\0';
            
            lines[line_index++] = line;
            
            // Stop if we reached the end of the string
            if (*p == '\0') {
                break;
            }
            
            // Move to the next line
            start = p + 1;
        }
    }
    
    *line_count = count;
    return lines;
}

static void free_line_array(char** lines, size_t count) {
    if (!lines) {
        return;
    }
    
    for (size_t i = 0; i < count; i++) {
        free(lines[i]);
    }
    
    free(lines);
}

static size_t count_digits(unsigned int num) {
    if (num == 0) {
        return 1;
    }
    
    size_t count = 0;
    while (num > 0) {
        count++;
        num /= 10;
    }
    
    return count;
}

static size_t find_line_start(const char* source, unsigned int line) {
    if (!source || line == 0) {
        return 0;
    }
    
    size_t pos = 0;
    unsigned int current_line = 1;
    
    while (source[pos] && current_line < line) {
        if (source[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    return pos;
}

static size_t find_line_end(const char* source, size_t start) {
    if (!source) {
        return start;
    }
    
    size_t pos = start;
    while (source[pos] && source[pos] != '\n') {
        pos++;
    }
    
    return pos;
}

static size_t write_error_pointer(
    char* buffer, 
    size_t buffer_size, 
    unsigned int column, 
    unsigned int length, 
    unsigned int line_number_width,
    const GooHighlightOptions* options
) {
    if (!buffer || buffer_size < 4) {
        return 0;
    }
    
    // Adjust column to be 0-based for easier calculation
    column = (column > 0) ? column - 1 : 0;
    
    // Ensure length is at least 1
    length = (length > 0) ? length : 1;
    
    // Buffer position
    size_t pos = 0;
    
    // Add padding for line number
    if (options->show_line_numbers) {
        for (unsigned int i = 0; i < line_number_width + 1; i++) {
            if (pos >= buffer_size - 1) {
                break;
            }
            buffer[pos++] = ' ';
        }
        
        // Add the vertical line
        if (pos < buffer_size - 1) {
            buffer[pos++] = ' ';
        }
    }
    
    // Add space padding up to the column
    for (unsigned int i = 0; i < column; i++) {
        if (pos >= buffer_size - 1) {
            break;
        }
        buffer[pos++] = ' ';
    }
    
    // Add the error indicator (^^^^^)
    if (options->use_color) {
        // Add color code for red
        const char* color_code = COLOR_BOLD COLOR_RED;
        size_t color_len = strlen(color_code);
        
        if (pos + color_len < buffer_size - 1) {
            memcpy(buffer + pos, color_code, color_len);
            pos += color_len;
        }
    }
    
    // Add caret symbols for the underline
    for (unsigned int i = 0; i < length; i++) {
        if (pos >= buffer_size - 1) {
            break;
        }
        buffer[pos++] = '^';
    }
    
    if (options->use_color) {
        // Add reset code
        const char* reset_code = COLOR_RESET;
        size_t reset_len = strlen(reset_code);
        
        if (pos + reset_len < buffer_size - 1) {
            memcpy(buffer + pos, reset_code, reset_len);
            pos += reset_len;
        }
    }
    
    // Add newline
    if (pos < buffer_size - 1) {
        buffer[pos++] = '\n';
    }
    
    // Ensure null termination
    if (pos < buffer_size) {
        buffer[pos] = '\0';
    } else {
        buffer[buffer_size - 1] = '\0';
    }
    
    return pos;
} 