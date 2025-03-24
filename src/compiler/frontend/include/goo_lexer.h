#ifndef GOO_LEXER_H
#define GOO_LEXER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Token types
typedef enum {
    GOO_TOKEN_ERROR,
    GOO_TOKEN_EOF,
    
    // Operators
    GOO_TOKEN_PLUS,
    GOO_TOKEN_MINUS,
    GOO_TOKEN_ASTERISK,
    GOO_TOKEN_SLASH,
    GOO_TOKEN_PERCENT,
    GOO_TOKEN_ASSIGN,
    GOO_TOKEN_PLUS_ASSIGN,
    GOO_TOKEN_MINUS_ASSIGN,
    GOO_TOKEN_ASTERISK_ASSIGN,
    GOO_TOKEN_SLASH_ASSIGN,
    GOO_TOKEN_BANG,
    GOO_TOKEN_EQ,
    GOO_TOKEN_NEQ,
    GOO_TOKEN_LT,
    GOO_TOKEN_GT,
    GOO_TOKEN_LEQ,
    GOO_TOKEN_GEQ,
    GOO_TOKEN_AND,
    GOO_TOKEN_OR,
    GOO_TOKEN_BITAND,
    GOO_TOKEN_BITOR,
    GOO_TOKEN_BITXOR,
    GOO_TOKEN_BITNOT,
    GOO_TOKEN_LSHIFT,
    GOO_TOKEN_RSHIFT,
    GOO_TOKEN_DOT,
    GOO_TOKEN_RANGE,
    GOO_TOKEN_ARROW,
    GOO_TOKEN_DECLARE_ASSIGN,
    
    // Delimiters
    GOO_TOKEN_COMMA,
    GOO_TOKEN_SEMICOLON,
    GOO_TOKEN_COLON,
    GOO_TOKEN_LPAREN,
    GOO_TOKEN_RPAREN,
    GOO_TOKEN_LBRACE,
    GOO_TOKEN_RBRACE,
    GOO_TOKEN_LBRACKET,
    GOO_TOKEN_RBRACKET,
    
    // Literals
    GOO_TOKEN_IDENTIFIER,
    GOO_TOKEN_INT_LITERAL,
    GOO_TOKEN_FLOAT_LITERAL,
    GOO_TOKEN_STRING_LITERAL,
    
    // Keywords
    GOO_TOKEN_IF,
    GOO_TOKEN_ELSE,
    GOO_TOKEN_FOR,
    GOO_TOKEN_RETURN,
    GOO_TOKEN_PACKAGE,
    GOO_TOKEN_IMPORT,
    GOO_TOKEN_FUNC,
    GOO_TOKEN_VAR,
    GOO_TOKEN_CONST,
    GOO_TOKEN_TRUE,
    GOO_TOKEN_FALSE
} GooTokenType;

// Token value union
typedef union {
    const char* string_val;
    int64_t integer_val;
    double float_val;
    bool boolean_val;
} GooTokenValue;

// Token struct
typedef struct {
    GooTokenType token_type;
    GooTokenValue value;
    size_t line;
    size_t column;
} GooToken;

// Opaque lexer handle
typedef struct GooLexer* GooLexerHandle;

// Create a new lexer
GooLexerHandle goo_lexer_init(const char* source, size_t length);

// Get the next token
bool goo_lexer_next_token(GooLexerHandle lexer, GooToken* token);

// Get a string representation of a token type
const char* goo_token_type_name(GooTokenType type);

// Destroy a lexer
void goo_lexer_destroy(GooLexerHandle lexer);

// Clean up global resources
void goo_lexer_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_LEXER_H 