#ifndef GOO_LEXER_H
#define GOO_LEXER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Token type enumeration
typedef enum {
    // Basic token types
    TOKEN_ERROR = 0,
    TOKEN_EOF = 1,
    TOKEN_IDENTIFIER = 2,
    
    // Literals
    TOKEN_INT_LITERAL = 3,
    TOKEN_FLOAT_LITERAL = 4,
    TOKEN_STRING_LITERAL = 5,
    TOKEN_BOOL_LITERAL = 6,
    TOKEN_RANGE_LITERAL = 7,
    
    // Operators
    TOKEN_PLUS = 10,
    TOKEN_MINUS = 11,
    TOKEN_ASTERISK = 12,
    TOKEN_SLASH = 13,
    TOKEN_PERCENT = 14,
    TOKEN_EQUAL = 15,
    TOKEN_NOT_EQUAL = 16,
    TOKEN_LESS_THAN = 17,
    TOKEN_GREATER_THAN = 18,
    TOKEN_LESS_EQUAL = 19,
    TOKEN_GREATER_EQUAL = 20,
    TOKEN_ASSIGN = 21,
    TOKEN_PLUS_ASSIGN = 22,
    TOKEN_MINUS_ASSIGN = 23,
    TOKEN_ASTERISK_ASSIGN = 24,
    TOKEN_SLASH_ASSIGN = 25,
    TOKEN_LOGICAL_AND = 26,
    TOKEN_LOGICAL_OR = 27,
    TOKEN_LOGICAL_NOT = 28,
    TOKEN_BITWISE_AND = 29,
    TOKEN_BITWISE_OR = 30,
    TOKEN_BITWISE_XOR = 31,
    TOKEN_BITWISE_NOT = 32,
    TOKEN_SHIFT_LEFT = 33,
    TOKEN_SHIFT_RIGHT = 34,
    
    // Punctuation
    TOKEN_COMMA = 40,
    TOKEN_SEMICOLON = 41,
    TOKEN_COLON = 42,
    TOKEN_DOT = 43,
    TOKEN_LPAREN = 44,
    TOKEN_RPAREN = 45,
    TOKEN_LBRACE = 46,
    TOKEN_RBRACE = 47,
    TOKEN_LBRACKET = 48,
    TOKEN_RBRACKET = 49,
    TOKEN_RANGE = 50,
    TOKEN_ARROW = 51,
    
    // Keywords
    TOKEN_FUNCTION = 60,
    TOKEN_LET = 61,
    TOKEN_CONST = 62,
    TOKEN_IF = 63,
    TOKEN_ELSE = 64,
    TOKEN_RETURN = 65,
    TOKEN_WHILE = 66,
    TOKEN_FOR = 67,
    TOKEN_TRUE = 68,
    TOKEN_FALSE = 69,
    TOKEN_IN = 70,
    TOKEN_BREAK = 71,
    TOKEN_CONTINUE = 72,
    TOKEN_NULL = 73,
    TOKEN_IMPORT = 74,
    TOKEN_AS = 75,
    TOKEN_TYPE = 76,
    TOKEN_STRUCT = 77,
    TOKEN_ENUM = 78,
    TOKEN_INTERFACE = 79,
    TOKEN_IMPLEMENT = 80,
    TOKEN_MATCH = 81,
    TOKEN_PUBLIC = 82,
    TOKEN_PRIVATE = 83,
    TOKEN_TRY = 84,
    TOKEN_CATCH = 85,
    TOKEN_THROW = 86,
    TOKEN_DEFER = 87,
    TOKEN_ASYNC = 88,
    TOKEN_AWAIT = 89,
    TOKEN_USING = 90
    
} GooTokenType;

// Range value structure
typedef struct {
    int64_t start;
    int64_t end;
    int inclusive;  // 0 = false, 1 = true
} GooRangeValue;

// Token value union
typedef union {
    int64_t int_value;
    double float_value;
    int bool_value;          // 0 = false, 1 = true
    const char* string_value;
    GooRangeValue range_value;
} GooTokenValue;

// Token structure
typedef struct {
    int type;
    int line;
    int column;
    const char* literal;
    int has_value;           // 0 = no value, 1 = has value
    GooTokenValue value;
} GooToken;

// Lexer structure
typedef struct GooLexer GooLexer;

// Initialize a lexer with a string
GooLexer* init_lexer(const char* input);

// Get the next token from the lexer
GooToken next_token(void);

// Free the lexer
void free_lexer(void);

// Reset the string buffer
void reset_buffer(void);

// Clean up all resources
void cleanup(void);

// Tokenize a file and print tokens
int tokenize_file(const char* filename);

#ifdef __cplusplus
}
#endif

#endif // GOO_LEXER_H 