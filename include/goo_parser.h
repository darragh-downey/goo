/**
 * @file goo_parser.h
 * @brief C API for the Goo language parser
 */

#ifndef GOO_PARSER_H
#define GOO_PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Goo Parser C API
 * This header provides the interface to the Zig-based parser
 * for the Goo programming language
 */

// Opaque handles for parser-related objects
typedef void* GooParserHandle;
typedef void* GooASTHandle;
typedef void* GooASTNodeHandle;

// Token types (matching lexer token types)
typedef enum {
    GOO_TOKEN_ILLEGAL = 0,
    GOO_TOKEN_EOF = 1,
    GOO_TOKEN_IDENT = 2,
    GOO_TOKEN_INT = 3,
    GOO_TOKEN_FLOAT = 4,
    GOO_TOKEN_STRING = 5,
    GOO_TOKEN_COMMENT = 6,
    GOO_TOKEN_SEMICOLON = 7,
    GOO_TOKEN_COLON = 8,
    // ... other token types will be defined here
} GooTokenType;

// Parser error information
typedef struct {
    unsigned int line;
    unsigned int column;
    char* message;
} GooParserError;

// AST node types
typedef enum {
    GOO_NODE_PROGRAM = 0,
    GOO_NODE_FUNC_DECL = 1,
    GOO_NODE_VAR_DECL = 2,
    GOO_NODE_CONST_DECL = 3,
    GOO_NODE_BLOCK = 4,
    GOO_NODE_IF_STMT = 5,
    GOO_NODE_FOR_STMT = 6,
    GOO_NODE_RETURN_STMT = 7,
    GOO_NODE_EXPR_STMT = 8,
    GOO_NODE_CALL_EXPR = 9,
    GOO_NODE_IDENT = 10,
    GOO_NODE_INT_LIT = 11,
    GOO_NODE_FLOAT_LIT = 12,
    GOO_NODE_STRING_LIT = 13,
    GOO_NODE_BOOL_LIT = 14,
    // ... other node types will be defined here
} GooASTNodeType;

/**
 * Initialize a new parser with the given source code
 * 
 * @param source Pointer to source code string
 * @param source_len Length of the source code
 * @return Handle to the parser, or NULL on failure
 */
GooParserHandle goo_parser_init(const char* source, unsigned int source_len);

/**
 * Free resources associated with the parser
 * 
 * @param handle The parser handle
 */
void goo_parser_destroy(GooParserHandle handle);

/**
 * Parse the source code and return an AST handle
 * 
 * @param handle The parser handle
 * @return AST handle, or NULL if parsing failed
 */
GooASTHandle goo_parser_parse(GooParserHandle handle);

/**
 * Get the number of parsing errors
 * 
 * @param handle The parser handle
 * @return The number of errors
 */
unsigned int goo_parser_error_count(GooParserHandle handle);

/**
 * Get an error by index
 * 
 * @param handle The parser handle
 * @param index The error index
 * @param error_out Pointer to store error info
 * @return true if successful, false if index is out of range
 */
bool goo_parser_get_error(GooParserHandle handle, unsigned int index, GooParserError* error_out);

/**
 * Free resources associated with an AST
 * 
 * @param ast_handle The AST handle
 */
void goo_ast_destroy(GooASTHandle ast_handle);

/**
 * Print the AST structure to stdout
 * 
 * @param ast_handle The AST handle
 */
void goo_ast_print(GooASTHandle ast_handle);

/**
 * Get the type of an AST node
 * 
 * @param node_handle The node handle
 * @return The node type
 */
GooASTNodeType goo_ast_node_type(GooASTNodeHandle node_handle);

/**
 * Get the root node of an AST
 * 
 * @param ast_handle The AST handle
 * @return The root node handle
 */
GooASTNodeHandle goo_ast_root(GooASTHandle ast_handle);

/**
 * Clean up global resources when done with the parser
 */
void goo_parser_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* GOO_PARSER_H */ 