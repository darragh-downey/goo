#ifndef GOO_PARSER_H
#define GOO_PARSER_H

#include <stdint.h>
#include "../../include/goo_lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle to the parser
typedef struct GooParser* GooParserHandle;

// Opaque handle to an AST node
typedef struct GooAstNode* GooAstNodeHandle;

// Error codes for the parser
typedef enum {
    GOO_PARSER_SUCCESS = 0,
    GOO_PARSER_UNEXPECTED_TOKEN = 1,
    GOO_PARSER_MISSING_TOKEN = 2,
    GOO_PARSER_INVALID_SYNTAX = 3,
    GOO_PARSER_OUT_OF_MEMORY = 4,
    GOO_PARSER_NOT_IMPLEMENTED = 5,
    GOO_PARSER_UNKNOWN_ERROR = 6
} GooParserErrorCode;

// AST node types
typedef enum {
    GOO_AST_PROGRAM = 0,
    GOO_AST_PACKAGE_DECL = 1,
    GOO_AST_IMPORT_DECL = 2,
    GOO_AST_FUNCTION_DECL = 3,
    GOO_AST_PARAMETER = 4,
    GOO_AST_VAR_DECL = 5,
    GOO_AST_CONST_DECL = 6,
    GOO_AST_TYPE_DECL = 7,
    GOO_AST_TYPE_EXPR = 8,
    GOO_AST_BLOCK = 9,
    GOO_AST_IF_STMT = 10,
    GOO_AST_FOR_STMT = 11,
    GOO_AST_RETURN_STMT = 12,
    GOO_AST_EXPR_STMT = 13,
    GOO_AST_CALL_EXPR = 14,
    GOO_AST_IDENTIFIER = 15,
    GOO_AST_INT_LITERAL = 16,
    GOO_AST_FLOAT_LITERAL = 17,
    GOO_AST_STRING_LITERAL = 18,
    GOO_AST_BOOL_LITERAL = 19,
    GOO_AST_PREFIX_EXPR = 20,
    GOO_AST_INFIX_EXPR = 21,
} GooAstNodeType;

// Initialize a parser for the given source code
GooParserHandle goo_parser_init(const char* source_code);

// Destroy the parser and free resources
void goo_parser_destroy(GooParserHandle parser);

// Parse the program
GooParserErrorCode goo_parser_parse_program(GooParserHandle parser);

// Get the current error message
const char* goo_parser_get_error(GooParserHandle parser);

// Get the root AST node after parsing
GooAstNodeHandle goo_parser_get_ast_root(GooParserHandle parser);

// Get the type of an AST node
GooAstNodeType goo_ast_get_node_type(GooAstNodeHandle node);

// Clean up global resources when the library is unloaded
void goo_parser_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // GOO_PARSER_H 