/**
 * @file formatter.c
 * @brief Implementation of the Goo code formatter
 */

#include "formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward declarations of internal functions
static bool read_file_to_string(const char* filename, char** content, size_t* length);
static bool write_string_to_file(const char* filename, const char* content, size_t length);
static int format_line(const char* line, const GooFormatterOptions* options, char* output, size_t output_size);
static bool is_comment_line(const char* line);
static void trim_trailing_whitespace(char* line);
static int count_leading_spaces(const char* line);
static char* indent_line(const char* line, int indent_level, const GooFormatterOptions* options);

// Set of keywords after which a brace likely appears
static const char* brace_keywords[] = {
    "fn", "struct", "enum", "impl", "if", "else", "while", "for", "match", NULL
};

/**
 * Initialize default formatting options
 */
GooFormatterOptions goo_formatter_default_options(void) {
    GooFormatterOptions options;
    options.tab_width = 4;
    options.use_tabs = false;
    options.max_width = 100;
    options.format_comments = true;
    options.reflow_comments = true;
    options.align_comments = true;
    options.brace_style_same_line = true;
    options.spaces_around_operators = true;
    options.compact_array_init = false;
    
    return options;
}

/**
 * Format source code using the specified options
 */
int goo_format_source(
    const char* source,
    const GooFormatterOptions* options,
    char* output_buffer,
    size_t buffer_size
) {
    if (!source || !options || !output_buffer || buffer_size == 0) {
        return -1;
    }
    
    // Copy source to ensure it's null-terminated
    size_t source_len = strlen(source);
    char* source_copy = (char*)malloc(source_len + 1);
    if (!source_copy) {
        return -1;
    }
    
    strcpy(source_copy, source);
    
    // Split the source into lines
    const int MAX_LINES = 50000;
    char* lines[MAX_LINES];
    int line_count = 0;
    
    char* line = strtok(source_copy, "\n");
    while (line && line_count < MAX_LINES) {
        lines[line_count++] = line;
        line = strtok(NULL, "\n");
    }
    
    // Format each line
    size_t total_written = 0;
    int indent_level = 0;
    
    for (int i = 0; i < line_count; i++) {
        // Adjust indentation based on braces
        char* trimmed = lines[i];
        while (*trimmed && isspace(*trimmed)) trimmed++;
        
        // Decrease indent if the line starts with a closing brace
        if (*trimmed == '}') {
            indent_level = indent_level > 0 ? indent_level - 1 : 0;
        }
        
        // Format the line with appropriate indentation
        char* indented = indent_line(lines[i], indent_level, options);
        if (!indented) {
            free(source_copy);
            return -1;
        }
        
        // Apply additional formatting rules
        char formatted[1024];
        int written = format_line(indented, options, formatted, sizeof(formatted));
        free(indented);
        
        if (written < 0) {
            free(source_copy);
            return -1;
        }
        
        // Increase indent if the line ends with an opening brace
        size_t len = strlen(formatted);
        if (len > 0 && formatted[len - 1] == '{') {
            indent_level++;
        }
        
        // Copy to output buffer
        if (total_written + written + 2 > buffer_size) {
            // Not enough space
            free(source_copy);
            return -1;
        }
        
        // Add the formatted line and a newline
        strcpy(output_buffer + total_written, formatted);
        total_written += written;
        
        // Add newline unless it's the last line
        if (i < line_count - 1) {
            output_buffer[total_written++] = '\n';
        }
    }
    
    // Ensure null termination
    output_buffer[total_written] = '\0';
    
    free(source_copy);
    return (int)total_written;
}

/**
 * Format a source file in place
 */
bool goo_format_file(
    const char* filename,
    const GooFormatterOptions* options
) {
    // Read file content
    char* content = NULL;
    size_t length = 0;
    
    if (!read_file_to_string(filename, &content, &length)) {
        return false;
    }
    
    // Allocate output buffer
    char* formatted = (char*)malloc(length * 2); // Assume formatted code won't be more than twice as large
    if (!formatted) {
        free(content);
        return false;
    }
    
    // Format the source
    int result = goo_format_source(content, options, formatted, length * 2);
    
    free(content);
    
    if (result < 0) {
        free(formatted);
        return false;
    }
    
    // Write formatted content back to file
    bool success = write_string_to_file(filename, formatted, result);
    
    free(formatted);
    return success;
}

/**
 * Format a source file, writing the result to a different file
 */
bool goo_format_file_to(
    const char* input_filename,
    const char* output_filename,
    const GooFormatterOptions* options
) {
    // Read file content
    char* content = NULL;
    size_t length = 0;
    
    if (!read_file_to_string(input_filename, &content, &length)) {
        return false;
    }
    
    // Allocate output buffer
    char* formatted = (char*)malloc(length * 2); // Assume formatted code won't be more than twice as large
    if (!formatted) {
        free(content);
        return false;
    }
    
    // Format the source
    int result = goo_format_source(content, options, formatted, length * 2);
    
    free(content);
    
    if (result < 0) {
        free(formatted);
        return false;
    }
    
    // Write formatted content to the output file
    bool success = write_string_to_file(output_filename, formatted, result);
    
    free(formatted);
    return success;
}

/**
 * Check if a file needs formatting
 */
bool goo_file_needs_formatting(
    const char* filename,
    const GooFormatterOptions* options
) {
    // Read file content
    char* content = NULL;
    size_t length = 0;
    
    if (!read_file_to_string(filename, &content, &length)) {
        return false;
    }
    
    // Format the content in a temporary buffer
    char* formatted = (char*)malloc(length * 2);
    if (!formatted) {
        free(content);
        return false;
    }
    
    int result = goo_format_source(content, options, formatted, length * 2);
    
    if (result < 0) {
        free(content);
        free(formatted);
        return false;
    }
    
    // Compare original and formatted content
    bool needs_formatting = (result != length || memcmp(content, formatted, length) != 0);
    
    free(content);
    free(formatted);
    
    return needs_formatting;
}

/**
 * Process command-line arguments to configure formatter options
 */
bool goo_formatter_process_args(
    int argc,
    char** argv,
    GooFormatterOptions* options
) {
    if (!options) {
        return false;
    }
    
    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--tab-width") == 0 && i + 1 < argc) {
            options->tab_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--use-tabs") == 0) {
            options->use_tabs = true;
        } else if (strcmp(argv[i], "--no-tabs") == 0) {
            options->use_tabs = false;
        } else if (strcmp(argv[i], "--max-width") == 0 && i + 1 < argc) {
            options->max_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-format-comments") == 0) {
            options->format_comments = false;
        } else if (strcmp(argv[i], "--no-reflow-comments") == 0) {
            options->reflow_comments = false;
        } else if (strcmp(argv[i], "--no-align-comments") == 0) {
            options->align_comments = false;
        } else if (strcmp(argv[i], "--brace-new-line") == 0) {
            options->brace_style_same_line = false;
        } else if (strcmp(argv[i], "--brace-same-line") == 0) {
            options->brace_style_same_line = true;
        } else if (strcmp(argv[i], "--no-spaces-operators") == 0) {
            options->spaces_around_operators = false;
        } else if (strcmp(argv[i], "--compact-arrays") == 0) {
            options->compact_array_init = true;
        }
    }
    
    return true;
}

// Internal helper functions

/**
 * Read a file into a string buffer
 */
static bool read_file_to_string(const char* filename, char** content, size_t* length) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    *content = (char*)malloc(size + 1);
    if (!*content) {
        fclose(file);
        return false;
    }
    
    // Read file content
    size_t read_size = fread(*content, 1, size, file);
    fclose(file);
    
    if (read_size != (size_t)size) {
        free(*content);
        *content = NULL;
        return false;
    }
    
    // Null-terminate the string
    (*content)[size] = '\0';
    *length = size;
    
    return true;
}

/**
 * Write a string to a file
 */
static bool write_string_to_file(const char* filename, const char* content, size_t length) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        return false;
    }
    
    size_t written = fwrite(content, 1, length, file);
    fclose(file);
    
    return written == length;
}

/**
 * Format a single line of code
 */
static int format_line(const char* line, const GooFormatterOptions* options, char* output, size_t output_size) {
    if (!line || !options || !output || output_size == 0) {
        return -1;
    }
    
    // First, copy the line
    strncpy(output, line, output_size - 1);
    output[output_size - 1] = '\0';
    
    // Apply formatting rules
    
    // 1. Trim trailing whitespace
    trim_trailing_whitespace(output);
    
    // 2. Adjust spacing around operators if enabled
    if (options->spaces_around_operators) {
        // Simple implementation of spacing around operators
        // A full implementation would use lexer tokens
        char temp[1024];
        char* src = output;
        char* dst = temp;
        
        while (*src && dst - temp < (int)sizeof(temp) - 3) {
            if (strchr("+-*/%=<>&|^!", *src) && *(src+1) != '=' && *(src-1) != ' ' && dst > temp) {
                *dst++ = ' ';
                *dst++ = *src++;
                
                // Add space after operator unless there's already one
                if (*src && *src != ' ' && strchr("([{", *src) == NULL) {
                    *dst++ = ' ';
                }
            } else {
                *dst++ = *src++;
            }
        }
        
        *dst = '\0';
        
        // Copy back if changed
        if (strcmp(temp, output) != 0) {
            strncpy(output, temp, output_size - 1);
            output[output_size - 1] = '\0';
        }
    }
    
    // 3. Handle brace style
    // This is simplified and would need to be enhanced with lexer integration
    if (!options->brace_style_same_line) {
        // If the line ends with {, check if it should be on its own line
        int len = strlen(output);
        if (len > 0 && output[len - 1] == '{') {
            // Check if the line starts with a keyword that should have { on its own line
            bool move_brace = false;
            for (const char** keyword = brace_keywords; *keyword; keyword++) {
                char* word_pos = strstr(output, *keyword);
                if (word_pos && (word_pos == output || !isalnum(*(word_pos-1)))) {
                    move_brace = true;
                    break;
                }
            }
            
            if (move_brace) {
                // Remove the brace and trailing spaces
                while (len > 0 && (output[len - 1] == '{' || isspace(output[len - 1]))) {
                    output[--len] = '\0';
                }
                
                // The brace will be added on the next line by the caller
            }
        }
    }
    
    return strlen(output);
}

/**
 * Check if a line is a comment
 */
static bool is_comment_line(const char* line) {
    while (*line && isspace(*line)) line++;
    return *line == '/' && (*(line + 1) == '/' || *(line + 1) == '*');
}

/**
 * Trim trailing whitespace from a line
 */
static void trim_trailing_whitespace(char* line) {
    int len = strlen(line);
    while (len > 0 && isspace(line[len - 1])) {
        line[--len] = '\0';
    }
}

/**
 * Count the number of leading spaces in a line
 */
static int count_leading_spaces(const char* line) {
    int count = 0;
    while (*line && isspace(*line)) {
        if (*line == '\t') {
            count += 4; // Assume tabs are 4 spaces
        } else {
            count++;
        }
        line++;
    }
    return count;
}

/**
 * Indent a line based on indent level and formatting options
 */
static char* indent_line(const char* line, int indent_level, const GooFormatterOptions* options) {
    // Skip leading whitespace
    const char* content = line;
    while (*content && isspace(*content)) content++;
    
    // Don't indent empty lines
    if (*content == '\0') {
        char* result = (char*)malloc(1);
        if (result) {
            result[0] = '\0';
        }
        return result;
    }
    
    // Calculate indentation
    int indent_size = indent_level * options->tab_width;
    
    // Allocate memory for the indented line
    char* indented = (char*)malloc(strlen(content) + indent_size + 1);
    if (!indented) {
        return NULL;
    }
    
    // Add indentation
    int pos = 0;
    if (options->use_tabs) {
        for (int i = 0; i < indent_level; i++) {
            indented[pos++] = '\t';
        }
    } else {
        for (int i = 0; i < indent_size; i++) {
            indented[pos++] = ' ';
        }
    }
    
    // Add the content
    strcpy(indented + pos, content);
    
    return indented;
} 