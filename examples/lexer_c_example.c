#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/goo_lexer.h"

// Helper function to get value string based on token type
const char* get_token_value_str(const GooToken* token) {
    static char buffer[128];
    
    if (token->has_string_value && token->string_value != NULL) {
        snprintf(buffer, sizeof(buffer), "\"%.*s\"", 
                 token->string_length > 100 ? 100 : token->string_length, 
                 token->string_value);
        return buffer;
    }
    
    switch (token->token_type) {
        case GOO_TOKEN_INT_LITERAL:
            snprintf(buffer, sizeof(buffer), "%lld", token->int_value);
            break;
        case GOO_TOKEN_FLOAT_LITERAL:
            snprintf(buffer, sizeof(buffer), "%g", token->float_value);
            break;
        case GOO_TOKEN_TRUE:
        case GOO_TOKEN_FALSE:
            return token->bool_value ? "true" : "false";
        default:
            return "";
    }
    
    return buffer;
}

int main(int argc, char* argv[]) {
    // Default source code example
    const char* source_code = 
        "package main\n"
        "\n"
        "import \"std\"\n"
        "\n"
        "func fibonacci(n: int) -> int {\n"
        "    if n <= 1 {\n"
        "        return n\n"
        "    }\n"
        "    return fibonacci(n-1) + fibonacci(n-2)\n"
        "}\n"
        "\n"
        "func main() {\n"
        "    // Print first 10 Fibonacci numbers\n"
        "    for i := 0; i < 10; i += 1 {\n"
        "        std.println(fibonacci(i))\n"
        "    }\n"
        "}\n";
    
    // If a file is specified as argument, use that instead
    if (argc > 1) {
        FILE* file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
            return 1;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);
        
        // Allocate memory for file content
        char* file_content = (char*)malloc(file_size + 1);
        if (!file_content) {
            fprintf(stderr, "Error: Memory allocation failed\n");
            fclose(file);
            return 1;
        }
        
        // Read file content
        size_t read_size = fread(file_content, 1, file_size, file);
        file_content[read_size] = '\0';
        fclose(file);
        
        // Use file content as source code
        source_code = file_content;
    }
    
    printf("Source code:\n%s\n\n", source_code);
    
    // Initialize lexer
    GooLexerHandle lexer = goo_lexer_init(source_code, (unsigned int)strlen(source_code));
    if (!lexer) {
        fprintf(stderr, "Error: Failed to initialize lexer\n");
        if (argc > 1) {
            free((void*)source_code);
        }
        return 1;
    }
    
    // Print header for token table
    printf("%-20s %-15s %-15s %-7s %-7s\n", "Token Type", "Value", "Name", "Line", "Column");
    printf("----------------------------------------------------------------------------------\n");
    
    // Process all tokens
    GooToken token;
    int token_count = 0;
    
    while (goo_lexer_next_token(lexer, &token)) {
        token_count++;
        
        // Get token type name
        const char* type_name = goo_token_type_name(token.token_type);
        
        // Get token value as string
        const char* value_str = get_token_value_str(&token);
        
        // Print token information
        printf("%-20s %-15s %-15s %5u %7u\n", 
               type_name, 
               value_str, 
               token.has_string_value ? token.string_value : "", 
               token.line, 
               token.column);
        
        // Break on EOF
        if (token.token_type == GOO_TOKEN_EOF) {
            break;
        }
        
        // Check for errors
        if (token.token_type == GOO_TOKEN_ERROR) {
            fprintf(stderr, "Lexer error at line %u, column %u: %s\n", 
                    token.line, token.column, 
                    token.has_string_value ? token.string_value : "Unknown error");
        }
    }
    
    printf("\nTotal tokens: %d\n", token_count);
    
    // Clean up
    goo_lexer_destroy(lexer);
    goo_lexer_cleanup();
    
    if (argc > 1) {
        free((void*)source_code);
    }
    
    return 0;
} 