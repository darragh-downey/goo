/**
 * @file goo_fmt.c
 * @brief Goo source code formatter CLI tool
 */

#include "formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Forward declarations
static void print_usage(void);
static void format_directory(const char* dir_path, const GooFormatterOptions* options, bool recursive);
static bool is_goo_file(const char* filename);

int main(int argc, char** argv) {
    // Default options
    GooFormatterOptions options = goo_formatter_default_options();
    
    // Flag for recursively processing directories
    bool recursive = false;
    
    // Flag for check-only mode (don't modify files)
    bool check_only = false;
    
    // Process arguments
    if (!goo_formatter_process_args(argc, argv, &options)) {
        fprintf(stderr, "Error processing arguments\n");
        return 1;
    }
    
    // Parse command-line flags
    int i;
    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
                print_usage();
                return 0;
            } else if (strcmp(argv[i], "--recursive") == 0 || strcmp(argv[i], "-r") == 0) {
                recursive = true;
            } else if (strcmp(argv[i], "--check") == 0 || strcmp(argv[i], "-c") == 0) {
                check_only = true;
            } else if (strcmp(argv[i], "--tab-width") == 0 || 
                       strcmp(argv[i], "--max-width") == 0) {
                // Skip the option and its argument, which were handled by goo_formatter_process_args
                i++;
            }
            // Other formatting options were handled by goo_formatter_process_args
        } else {
            // Found a non-flag argument (should be a file or directory)
            break;
        }
    }
    
    // If no files or directories were specified, print usage
    if (i >= argc) {
        fprintf(stderr, "Error: No files or directories specified\n");
        print_usage();
        return 1;
    }
    
    // Process each file or directory
    int file_count = 0;
    int error_count = 0;
    int formatted_count = 0;
    
    for (; i < argc; i++) {
        struct stat st;
        
        if (stat(argv[i], &st) != 0) {
            fprintf(stderr, "Error: Cannot access '%s'\n", argv[i]);
            error_count++;
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // Process directory
            format_directory(argv[i], &options, recursive);
        } else if (S_ISREG(st.st_mode)) {
            // Process single file
            file_count++;
            
            // Only process .goo files
            if (!is_goo_file(argv[i])) {
                fprintf(stderr, "Skipping non-Goo file: %s\n", argv[i]);
                continue;
            }
            
            if (check_only) {
                // Check if file needs formatting
                if (goo_file_needs_formatting(argv[i], &options)) {
                    printf("%s needs formatting\n", argv[i]);
                    formatted_count++;
                    error_count++; // In check mode, a file needing formatting is an error
                }
            } else {
                // Format the file
                printf("Formatting %s... ", argv[i]);
                if (goo_format_file(argv[i], &options)) {
                    printf("done\n");
                    formatted_count++;
                } else {
                    printf("failed\n");
                    error_count++;
                }
            }
        } else {
            fprintf(stderr, "Error: '%s' is not a regular file or directory\n", argv[i]);
            error_count++;
        }
    }
    
    // Print summary
    if (check_only) {
        printf("\nFound %d file(s), %d would be reformatted\n", file_count, formatted_count);
        return error_count > 0 ? 1 : 0;
    } else {
        printf("\nFormatted %d file(s), %d error(s)\n", formatted_count, error_count);
        return error_count > 0 ? 1 : 0;
    }
}

/**
 * Print usage information
 */
static void print_usage(void) {
    printf("Usage: goo-fmt [options] file1 [file2 ...]\n");
    printf("Format Goo source files.\n\n");
    printf("Options:\n");
    printf("  -h, --help               Display this help and exit\n");
    printf("  -r, --recursive          Recursively process directories\n");
    printf("  -c, --check              Check if files need formatting, don't modify them\n");
    printf("  --tab-width N            Set tab width to N spaces (default: 4)\n");
    printf("  --use-tabs               Use tabs for indentation instead of spaces\n");
    printf("  --no-tabs                Use spaces for indentation (default)\n");
    printf("  --max-width N            Set maximum line width to N (default: 100)\n");
    printf("  --no-format-comments     Don't format comments\n");
    printf("  --no-reflow-comments     Don't reflow comments to fit max width\n");
    printf("  --no-align-comments      Don't align consecutive line comments\n");
    printf("  --brace-new-line         Put open braces on a new line\n");
    printf("  --brace-same-line        Put open braces on the same line (default)\n");
    printf("  --no-spaces-operators    Don't put spaces around operators\n");
    printf("  --compact-arrays         Use compact formatting for array initializations\n");
}

/**
 * Format all Goo files in a directory
 */
static void format_directory(const char* dir_path, const GooFormatterOptions* options, bool recursive) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "Error: Cannot open directory '%s'\n", dir_path);
        return;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) != 0) {
            fprintf(stderr, "Error: Cannot access '%s'\n", path);
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // Recursively process subdirectories if requested
            if (recursive) {
                format_directory(path, options, recursive);
            }
        } else if (S_ISREG(st.st_mode) && is_goo_file(entry->d_name)) {
            // Format Goo files
            printf("Formatting %s... ", path);
            if (goo_format_file(path, options)) {
                printf("done\n");
            } else {
                printf("failed\n");
            }
        }
    }
    
    closedir(dir);
}

/**
 * Check if a file is a Goo source file
 */
static bool is_goo_file(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) {
        return false;
    }
    
    return strcmp(ext, ".goo") == 0;
} 