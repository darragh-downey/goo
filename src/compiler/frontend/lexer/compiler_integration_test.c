#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lexer_selection.h"

// This program simulates the compiler's interaction with the lexer interface
// It can be used to test the lexer selection interface without modifying 
// the actual compiler code

// Simulated compiler error handling
typedef struct {
    char* message;
    int line;
    int column;
    char* file;
} CompilerError;

typedef struct {
    CompilerError* errors;
    int count;
    int capacity;
} ErrorList;

// Global state
static ErrorList error_list;
static char* current_file = NULL;
static FILE* token_output = NULL;

// Initialize the error list
void init_error_list() {
    error_list.capacity = 10;
    error_list.count = 0;
    error_list.errors = (CompilerError*)malloc(sizeof(CompilerError) * error_list.capacity);
    if (!error_list.errors) {
        fprintf(stderr, "Failed to allocate memory for error list\n");
        exit(1);
    }
}

// Free the error list
void free_error_list() {
    if (!error_list.errors) return;
    
    for (int i = 0; i < error_list.count; i++) {
        free(error_list.errors[i].message);
        free(error_list.errors[i].file);
    }
    
    free(error_list.errors);
    error_list.errors = NULL;
    error_list.count = 0;
    error_list.capacity = 0;
}

// Add an error to the error list
void add_error(const char* message, int line, int column, const char* file) {
    if (!error_list.errors) {
        init_error_list();
    }
    
    // Resize if needed
    if (error_list.count >= error_list.capacity) {
        error_list.capacity *= 2;
        CompilerError* new_errors = (CompilerError*)realloc(error_list.errors, 
                                                          sizeof(CompilerError) * error_list.capacity);
        if (!new_errors) {
            fprintf(stderr, "Failed to reallocate memory for error list\n");
            return;
        }
        error_list.errors = new_errors;
    }
    
    // Add the error
    CompilerError* error = &error_list.errors[error_list.count++];
    error->message = strdup(message);
    error->line = line;
    error->column = column;
    error->file = strdup(file ? file : "unknown");
}

// Print all errors
void print_errors() {
    if (!error_list.errors || error_list.count == 0) {
        printf("No errors\n");
        return;
    }
    
    printf("Errors:\n");
    for (int i = 0; i < error_list.count; i++) {
        CompilerError* error = &error_list.errors[i];
        printf("%s:%d:%d: %s\n", 
               error->file, error->line, error->column, error->message);
    }
}

// Error callback for the lexer
void error_callback(const char* message, int line, int column) {
    add_error(message, line, column, current_file);
}

// Simulate token processing (like what a parser would do)
void process_token(int token_type, int line, int column, const char* value) {
    if (token_output) {
        fprintf(token_output, "%d\t%d\t%d\t%s\n", 
                token_type, line, column, value ? value : "");
    }
}

// External declarations needed for Flex lexer
extern int yylex(void);
extern union {
    int int_val;
    double float_val;
    int bool_val;
    char* string_val;
    void* node;
} yylval;

// Token type definitions from the parser
#define IDENTIFIER 325
#define INT_LITERAL 321
#define FLOAT_LITERAL 322
#define BOOL_LITERAL 323
#define STRING_LITERAL 324
#define RANGE_LITERAL 326

// Read a file into a string
char* read_file_to_string(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* buffer = (char*)malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    // Read file and add null terminator
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';
    
    fclose(file);
    return buffer;
}

// Process a file using the selected lexer, simulating compiler behavior
int process_file(const char* filename, const char* token_output_file, int debug_mode) {
    // Set current file for error reporting
    if (current_file) {
        free(current_file);
    }
    current_file = strdup(filename);
    
    // Open token output file if specified
    if (token_output_file) {
        token_output = fopen(token_output_file, "w");
        if (!token_output) {
            fprintf(stderr, "Error: Could not open token output file: %s\n", token_output_file);
            return 1;
        }
    }
    
    // Set error callback
    lexer_set_error_callback(error_callback);
    
    // Set debug mode
    lexer_set_debug(debug_mode);
    
    printf("Processing file: %s\n", filename);
    
    // Read file content
    char* source = read_file_to_string(filename);
    if (!source) {
        fprintf(stderr, "Error: Could not read file: %s\n", filename);
        return 1;
    }
    
    // Set source string for lexer
    lexer_set_string(source);
    free(source);
    
    // Timing
    clock_t start = clock();
    int token_count = 0;
    
    // Process tokens
#ifdef USE_ZIG_LEXER
    // For Zig lexer, use the direct C API through lexer_selection interface
    GooLexer* lexer = lexer_get_current();
    if (lexer) {
        GooToken token;
        while (1) {
            token = goo_lexer_next_token(lexer);
            token_count++;
            
            // Get string value based on token type
            const char* string_value = NULL;
            switch (token.token_type) {
                case IDENTIFIER:
                case STRING_LITERAL:
                case RANGE_LITERAL:
                    string_value = token.string_value;
                    break;
                default:
                    // Other token types don't have string values
                    break;
            }
            
            // Process the token
            process_token(token.token_type, token.line, token.column, string_value);
            
            // Free token resources
            goo_token_free(token);
            
            // Stop at EOF
            if (token.token_type == 0) { // EOF
                break;
            }
        }
    }
#else
    // For Flex lexer, use yylex
    while (1) {
        int token_type = yylex();
        token_count++;
        
        if (token_type == 0) { // EOF
            process_token(0, lexer_get_line(), lexer_get_column(), NULL);
            break;
        }
        
        // Get string value based on token type
        const char* string_value = NULL;
        switch (token_type) {
            case IDENTIFIER:
            case STRING_LITERAL:
            case RANGE_LITERAL:
                string_value = yylval.string_val;
                break;
            default:
                // Other token types don't have string values
                break;
        }
        
        // Process the token
        process_token(token_type, lexer_get_line(), lexer_get_column(), string_value);
        
        // Free string values
        if (token_type == IDENTIFIER || token_type == STRING_LITERAL || token_type == RANGE_LITERAL) {
            if (yylval.string_val) {
                free(yylval.string_val);
                yylval.string_val = NULL;
            }
        }
    }
#endif
    
    // Timing
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    // Print statistics
    printf("Processed %d tokens in %.3f seconds\n", token_count - 1, time_taken);
    
    // Clean up
    lexer_cleanup();
    
    if (token_output) {
        fclose(token_output);
        token_output = NULL;
    }
    
    return 0;
}

void print_usage(const char* program_name) {
    printf("Usage: %s [options] <file1> [file2 ...]\n", program_name);
    printf("Options:\n");
    printf("  --debug               Enable debug output\n");
    printf("  --output-tokens=FILE  Output tokens to FILE\n");
    printf("  --zig                 Force Zig lexer (default if compiled with USE_ZIG_LEXER)\n");
    printf("  --flex                Force Flex lexer\n");
    printf("  --help                Show this help message\n");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse command line arguments
    int debug_mode = 0;
    char* token_output_file = NULL;
    int i;
    
    // First pass: Process options
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strncmp(argv[i], "--output-tokens=", 16) == 0) {
            token_output_file = argv[i] + 16;
        } else if (strcmp(argv[i], "--zig") == 0) {
            putenv("USE_ZIG_LEXER=1");
        } else if (strcmp(argv[i], "--flex") == 0) {
            putenv("USE_ZIG_LEXER=0");
        } else if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            // Not an option, must be a filename
            break;
        }
    }
    
    // Initialize error handling
    init_error_list();
    
    // Second pass: Process files
    int result = 0;
    for (; i < argc; i++) {
        result = process_file(argv[i], token_output_file, debug_mode);
        if (result != 0) {
            fprintf(stderr, "Error processing file: %s\n", argv[i]);
            break;
        }
    }
    
    // Print any errors
    print_errors();
    
    // Clean up
    free_error_list();
    if (current_file) {
        free(current_file);
    }
    
    return result;
} 