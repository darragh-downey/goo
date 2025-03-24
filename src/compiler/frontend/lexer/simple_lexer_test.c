#include <stdio.h>
#include "token_definitions.h"
#include "lexer_selection.h"

int main(int argc, char *argv[]) {
    const char *test_string = "let x = 10;";
    
    if (argc > 1) {
        test_string = argv[1];
    }
    
    // Initialize lexer with the test string
    GooLexer *lexer = lexer_init_string(test_string);
    if (!lexer) {
        printf("Failed to initialize lexer\n");
        return 1;
    }
    
    printf("Tokenizing: '%s'\n", test_string);
    printf("Tokens:\n");
    
    // Process all tokens
    int token_count = 0;
    GooToken token;
    do {
        token = lexer_next_token(lexer);
        token_count++;
        
        printf("  Token %d: type=%d, line=%d, col=%d, literal='%s'\n", 
               token_count, token.type, token.line, token.column, 
               token.literal ? token.literal : "");
        
        if (token.has_value) {
            switch (token.type) {
                case INT_LITERAL:
                    printf("    value=%lld (int)\n", (long long)token.value.int_value);
                    break;
                case FLOAT_LITERAL:
                    printf("    value=%f (float)\n", token.value.float_value);
                    break;
                case BOOL_LITERAL:
                    printf("    value=%s (bool)\n", token.value.bool_value ? "true" : "false");
                    break;
                case STRING_LITERAL:
                    printf("    value=\"%s\" (string)\n", token.value.string_value);
                    break;
            }
        }
    } while (token.type != 0);  // Continue until EOF
    
    // Clean up
    lexer_free(lexer);
    
    printf("Total tokens: %d\n", token_count - 1);  // Subtract 1 for EOF token
    
    return 0;
} 