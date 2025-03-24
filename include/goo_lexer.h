/**
 * @file goo_lexer.h
 * @brief C API for the Goo language lexer implemented in Zig
 */

#ifndef GOO_LEXER_H
#define GOO_LEXER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque handle to a lexer instance.
 */
typedef struct GooLexer* GooLexerHandle;

/**
 * Token type definitions - must match the enum in the Zig code.
 */
enum GooTokenType {
    // Special tokens
    GOO_TOKEN_ERROR = 0,
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
    
    // Keywords - control flow
    GOO_TOKEN_IF,
    GOO_TOKEN_ELSE,
    GOO_TOKEN_FOR,
    GOO_TOKEN_RETURN,
    
    // Keywords - declarations
    GOO_TOKEN_PACKAGE,
    GOO_TOKEN_IMPORT,
    GOO_TOKEN_FUNC,
    GOO_TOKEN_VAR,
    GOO_TOKEN_CONST,
    GOO_TOKEN_MODULE,
    
    // Keywords - parallel execution
    GOO_TOKEN_GO,
    GOO_TOKEN_PARALLEL,
    GOO_TOKEN_CHAN,
    GOO_TOKEN_SUPERVISE,
    GOO_TOKEN_COMPTIME,
    GOO_TOKEN_BUILD,
    
    // Keywords - memory management
    GOO_TOKEN_ALLOCATOR,
    GOO_TOKEN_ALLOC,
    GOO_TOKEN_FREE,
    GOO_TOKEN_SCOPE,
    GOO_TOKEN_HEAP,
    GOO_TOKEN_ARENA,
    GOO_TOKEN_FIXED,
    GOO_TOKEN_POOL,
    GOO_TOKEN_BUMP,
    GOO_TOKEN_CUSTOM,
    
    // Keywords - error handling
    GOO_TOKEN_TRY,
    GOO_TOKEN_RECOVER,
    GOO_TOKEN_SUPER,
    
    // Keywords - memory safety
    GOO_TOKEN_SAFE,
    GOO_TOKEN_UNSAFE,
    
    // Keywords - runtime mode
    GOO_TOKEN_KERNEL,
    GOO_TOKEN_USER,
    
    // Keywords - access control
    GOO_TOKEN_CAP,
    GOO_TOKEN_SHARED,
    GOO_TOKEN_PRIVATE,
    GOO_TOKEN_REFLECT,
    
    // Keywords - vectorization
    GOO_TOKEN_SIMD,
    GOO_TOKEN_VECTOR,
    GOO_TOKEN_ALIGNED,
    GOO_TOKEN_MASK,
    GOO_TOKEN_FUSED,
    GOO_TOKEN_AUTO,
    GOO_TOKEN_ARCH,
    
    // Keywords - types
    GOO_TOKEN_TRUE,
    GOO_TOKEN_FALSE,
    GOO_TOKEN_INT_TYPE,
    GOO_TOKEN_INT8_TYPE,
    GOO_TOKEN_INT16_TYPE,
    GOO_TOKEN_INT32_TYPE,
    GOO_TOKEN_INT64_TYPE,
    GOO_TOKEN_UINT_TYPE,
    GOO_TOKEN_FLOAT32_TYPE,
    GOO_TOKEN_FLOAT64_TYPE,
    GOO_TOKEN_BOOL_TYPE,
    GOO_TOKEN_STRING_TYPE,
    
    // Keywords - channel patterns
    GOO_TOKEN_PUB,
    GOO_TOKEN_SUB,
    GOO_TOKEN_PUSH,
    GOO_TOKEN_PULL,
    GOO_TOKEN_REQ,
    GOO_TOKEN_REP,
    GOO_TOKEN_DEALER,
    GOO_TOKEN_ROUTER,
    GOO_TOKEN_PAIR,
};

/**
 * C-compatible token structure.
 */
typedef struct {
    int token_type;              // Type of token (see GooTokenType enum)
    unsigned int line;           // Line number (1-based)
    unsigned int column;         // Column number (1-based)
    bool has_string_value;       // Whether the token has a string value
    char* string_value;          // String value (if has_string_value is true)
    unsigned int string_length;  // Length of string value
    long long int_value;         // Integer value (for numeric literals)
    double float_value;          // Float value (for float literals)
    bool bool_value;             // Boolean value (for true/false literals)
} GooToken;

/**
 * Initialize a new lexer with the given source code.
 * 
 * @param source The source code to lex.
 * @param source_len The length of the source code.
 * @return A handle to the lexer, or NULL if initialization failed.
 */
GooLexerHandle goo_lexer_init(const char* source, unsigned int source_len);

/**
 * Free resources associated with the lexer.
 * 
 * @param handle The lexer handle.
 */
void goo_lexer_destroy(GooLexerHandle handle);

/**
 * Get the next token from the lexer.
 * 
 * @param handle The lexer handle.
 * @param token_out Pointer to a GooToken structure to fill with the token data.
 * @return true if a token was retrieved, false on error or end of input.
 */
bool goo_lexer_next_token(GooLexerHandle handle, GooToken* token_out);

/**
 * Get the token type name as a string.
 * 
 * @param token_type The token type.
 * @return A string representing the token type. The caller must not free this string.
 */
const char* goo_token_type_name(int token_type);

/**
 * Clean up global resources when the library is unloaded.
 */
void goo_lexer_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* GOO_LEXER_H */ 