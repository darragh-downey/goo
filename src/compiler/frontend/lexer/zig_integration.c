#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/goo_lexer.h"
#include "lexer_selection.h"

// External declarations for parser integration
extern int yyparse(void);
extern FILE *yyin;
extern int yydebug;

// Token definitions from the parser
#define IDENTIFIER 325
#define INT_LITERAL 321
#define FLOAT_LITERAL 322
#define BOOL_LITERAL 323
#define STRING_LITERAL 324
#define RANGE_LITERAL 326

// Bison YYSTYPE union for semantic values
extern union {
    int int_val;
    double float_val;
    int bool_val;
    char* string_val;
    void* node;   // Generic pointer for AST nodes
} yylval;

// For location tracking
extern int yylineno;
typedef struct {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;
extern YYLTYPE yylloc;

// Global variables for token consumption
static int token_consumed = 1;
static GooToken current_token;  // Current token being processed

// Implementation of yylex for Bison integration
int yylex(void) {
    // Return the current token if not consumed
    if (!token_consumed) {
        token_consumed = 1;
        return current_token.token_type;
    }
    
    // Clean up previous token
    goo_token_free(current_token);
    
    // Get current lexer
    GooLexer *lexer = lexer_get_current();
    if (lexer == NULL) {
        fprintf(stderr, "Lexer not initialized\n");
        return 0; // EOF
    }
    
    // Get the next token
    current_token = goo_lexer_next_token(lexer);
    token_consumed = 1;
    
    // Update position tracking
    lexer_update_position(current_token.line, current_token.column);
    
    // Update yylval based on token type
    switch (current_token.token_type) {
        case IDENTIFIER:
        case STRING_LITERAL:
        case RANGE_LITERAL:
            if (current_token.string_value) {
                yylval.string_val = strdup(current_token.string_value);
            } else {
                yylval.string_val = NULL;
            }
            break;
            
        case INT_LITERAL:
            yylval.int_val = (int)current_token.int_value;
            break;
            
        case FLOAT_LITERAL:
            yylval.float_val = current_token.float_value;
            break;
            
        case BOOL_LITERAL:
            yylval.bool_val = current_token.bool_value;
            break;
    }
    
    // Update location information for Bison
    if (yylineno) {
        yylineno = current_token.line;
    }
    
    yylloc.first_line = current_token.line;
    yylloc.first_column = current_token.column;
    yylloc.last_line = current_token.line;
    yylloc.last_column = current_token.column;
    
    // Store the token for error reporting
    lexer_set_token(current_token);
    
    // For debugging
#ifdef LEXER_DEBUG
    printf("Token: %s (%d) at line %d, column %d\n",
           goo_token_get_name(current_token.token_type),
           current_token.token_type,
           current_token.line,
           current_token.column);
#endif
    
    return current_token.token_type;
}

// Error handling for parser (may be defined elsewhere)
void yyerror(const char *s) {
    lexer_error_at(lexer_get_line(), lexer_get_column(), s);
}

#ifdef ZIG_INTEGRATION_MAIN
// Main function for standalone testing
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file> [--debug]\n", argv[0]);
        return 1;
    }
    
    // Check for debug flag
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            lexer_set_debug(1);
        }
    }
    
    // Open input file
    FILE *file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }
    
    // Initialize lexer
    lexer_set_file(file);
    fclose(file);
    
    // Parse the input
    printf("Starting parse...\n");
    int result = yyparse();
    printf("Parse %s. Result code: %d\n", 
           result == 0 ? "successful" : "failed", result);
    
    // Clean up
    lexer_cleanup();
    
    return result;
}

// Tokenize a file and print all tokens
void tokenize_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open file: %s\n", filename);
        return;
    }
    
    // Initialize lexer
    lexer_set_file(file);
    fclose(file);
    
    // Get lexer
    GooLexer *lexer = lexer_get_current();
    if (!lexer) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return;
    }
    
    printf("Tokens in %s:\n", filename);
    printf("--------------------\n");
    
    int token_count = 0;
    GooToken token;
    
    while (1) {
        token = goo_lexer_next_token(lexer);
        token_count++;
        
        printf("%3d: %-15s at line %3d, column %3d", 
               token_count, 
               goo_token_get_name(token.token_type),
               token.line, 
               token.column);
        
        // Print token value based on type
        switch (token.token_type) {
            case INT_LITERAL:
                printf("  Value: %ld\n", token.int_value);
                break;
                
            case FLOAT_LITERAL:
                printf("  Value: %f\n", token.float_value);
                break;
                
            case BOOL_LITERAL:
                printf("  Value: %s\n", token.bool_value ? "true" : "false");
                break;
                
            case STRING_LITERAL:
            case IDENTIFIER:
            case RANGE_LITERAL:
                if (token.string_value) {
                    printf("  Value: \"%s\"\n", token.string_value);
                } else {
                    printf("\n");
                }
                break;
                
            default:
                printf("\n");
                break;
        }
        
        // Free token resources
        goo_token_free(token);
        
        // Stop at EOF
        if (token.token_type == 0) { // EOF
            break;
        }
    }
    
    printf("--------------------\n");
    printf("Total tokens: %d\n", token_count);
    
    // Clean up
    lexer_cleanup();
}
#endif 