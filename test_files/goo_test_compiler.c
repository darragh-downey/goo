#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goo_memory.h"
#include "goo_lexer.h"
#include "goo_parser.h"

/**
 * Test file for Goo compiler frontend components (lexer and parser)
 * This file demonstrates the C API integration with the Zig-based lexer and parser
 */

// Sample Goo code for testing
const char* test_source = 
    "// This is a comment\n"
    "func main() {\n"
    "    let x = 42;\n"
    "    let y = 3.14;\n"
    "    let message = \"Hello, Goo!\";\n"
    "    \n"
    "    if x > 40 {\n"
    "        println(message);\n"
    "    }\n"
    "    \n"
    "    for i in 0..5 {\n"
    "        println(i);\n"
    "    }\n"
    "}\n";

// Helper function to print token information
void print_token(GooToken* token) {
    const char* token_type_name = goo_token_type_name(token->token_type);
    printf("Token: %-15s Line: %3d Column: %3d ", token_type_name, token->line, token->column);
    
    if (token->has_string_value && token->string_value != NULL) {
        printf("Value: \"%.*s\"", token->string_length, token->string_value);
    } else if (token->token_type == GOO_TOKEN_INT) {
        printf("Value: %lld", token->int_value);
    } else if (token->token_type == GOO_TOKEN_FLOAT) {
        printf("Value: %f", token->float_value);
    }
    
    printf("\n");
}

// Function to test lexer functionality
void test_lexer(void) {
    printf("=== Testing Goo Lexer ===\n\n");
    printf("Source code:\n%s\n", test_source);
    
    // Initialize the lexer
    size_t source_len = strlen(test_source);
    GooLexerHandle lexer = goo_lexer_init(test_source, (unsigned int)source_len);
    
    if (lexer == NULL) {
        printf("Failed to initialize lexer\n");
        return;
    }
    
    // Tokenize the source code
    printf("\nTokens:\n");
    GooToken token;
    int token_count = 0;
    
    while (goo_lexer_next_token(lexer, &token)) {
        print_token(&token);
        token_count++;
        
        if (token.token_type == GOO_TOKEN_EOF) {
            break;
        }
    }
    
    printf("\nFound %d tokens\n", token_count);
    
    // Clean up
    goo_lexer_destroy(lexer);
}

// Function to test parser functionality
void test_parser(void) {
    printf("\n=== Testing Goo Parser ===\n\n");
    
    // Initialize the parser
    size_t source_len = strlen(test_source);
    GooParserHandle parser = goo_parser_init(test_source, (unsigned int)source_len);
    
    if (parser == NULL) {
        printf("Failed to initialize parser\n");
        return;
    }
    
    // Parse the source code
    GooASTHandle ast = goo_parser_parse(parser);
    
    if (ast == NULL) {
        printf("Parsing failed\n");
        
        // Get and print parsing errors
        unsigned int error_count = goo_parser_error_count(parser);
        printf("Found %u parsing errors:\n", error_count);
        
        for (unsigned int i = 0; i < error_count; i++) {
            GooParserError error;
            if (goo_parser_get_error(parser, i, &error)) {
                printf("Error %u: %s (at line %u, column %u)\n", 
                       i + 1, error.message, error.line, error.column);
            }
        }
    } else {
        printf("Successfully parsed source code!\n");
        
        // Print AST structure
        printf("\nAST Structure:\n");
        goo_ast_print(ast);
        
        // Clean up AST
        goo_ast_destroy(ast);
    }
    
    // Clean up
    goo_parser_destroy(parser);
}

// Main test function
int main(int argc, char** argv) {
    printf("Goo Compiler Frontend Test\n\n");
    
    // Initialize memory system (needed for compiler components)
    if (!goo_memory_init()) {
        printf("Failed to initialize memory subsystem\n");
        return 1;
    }
    
    // Run lexer test
    test_lexer();
    
    // Run parser test
    test_parser();
    
    // Clean up memory system
    goo_memory_cleanup();
    
    // Clean up global resources
    goo_lexer_cleanup();
    goo_parser_cleanup();
    
    printf("\nCompiler frontend tests completed\n");
    return 0;
} 