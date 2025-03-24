/**
 * @file goo_parser.h
 * @brief C API for the Goo language parser
 */

#ifndef GOO_PARSER_H
#define GOO_PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "goo_lexer.h"

#ifdef __cplusplus
extern "C" {
#endif

// AST node types
typedef enum {
    GOO_AST_PROGRAM,
    GOO_AST_PACKAGE_DECL,
    GOO_AST_IMPORT_DECL,
    GOO_AST_FUNCTION_DECL,
    GOO_AST_METHOD_DECL,
    GOO_AST_VAR_DECL,
    GOO_AST_CONST_DECL,
    GOO_AST_TYPE_DECL,
    GOO_AST_STRUCT_TYPE,
    GOO_AST_INTERFACE_TYPE,
    GOO_AST_ARRAY_TYPE,
    GOO_AST_SLICE_TYPE,
    GOO_AST_MAP_TYPE,
    GOO_AST_CHAN_TYPE,
    GOO_AST_FUNCTION_TYPE,
    GOO_AST_FIELD_DECL,
    GOO_AST_GENERIC_PARAM,      // Go 1.18+
    GOO_AST_GENERIC_CONSTRAINTS, // Go 1.18+
    
    // Statements
    GOO_AST_BLOCK,
    GOO_AST_IF_STMT,
    GOO_AST_FOR_STMT,
    GOO_AST_RANGE_STMT,
    GOO_AST_RETURN_STMT,
    GOO_AST_BREAK_STMT,
    GOO_AST_CONTINUE_STMT,
    GOO_AST_GOTO_STMT,
    GOO_AST_SWITCH_STMT,
    GOO_AST_CASE_CLAUSE,
    GOO_AST_DEFAULT_CLAUSE,
    GOO_AST_SELECT_STMT,
    GOO_AST_COMM_CLAUSE,
    GOO_AST_LABEL_STMT,
    GOO_AST_ASSIGNMENT_STMT,
    GOO_AST_EXPR_STMT,
    GOO_AST_SEND_STMT,
    GOO_AST_INC_STMT,
    GOO_AST_DEC_STMT,
    GOO_AST_DEFER_STMT,
    GOO_AST_GO_STMT,
    
    // Expressions
    GOO_AST_CALL_EXPR,
    GOO_AST_SELECTOR_EXPR,      // a.b
    GOO_AST_INDEX_EXPR,         // a[i]
    GOO_AST_SLICE_EXPR,         // a[i:j:k]
    GOO_AST_CHAN_RECV_EXPR,     // <-ch
    GOO_AST_TYPE_ASSERT_EXPR,   // x.(type)
    GOO_AST_IDENTIFIER,
    GOO_AST_INT_LITERAL,
    GOO_AST_FLOAT_LITERAL,
    GOO_AST_IMAGINARY_LITERAL,
    GOO_AST_CHAR_LITERAL,
    GOO_AST_STRING_LITERAL,
    GOO_AST_RAW_STRING_LITERAL,
    GOO_AST_BOOL_LITERAL,
    GOO_AST_NIL_LITERAL,
    GOO_AST_COMPOSITE_LIT,      // struct/array/slice/map literal
    GOO_AST_FUNCTION_LIT,       // func(){...}
    GOO_AST_KEY_VALUE_EXPR,     // key: value
    GOO_AST_PREFIX_EXPR,
    GOO_AST_INFIX_EXPR,
    GOO_AST_TYPE_EXPR,
    GOO_AST_ELLIPSIS_EXPR,      // ...x
    
    // Goo extensions
    GOO_AST_ENUM_DECL,
    GOO_AST_ENUM_MEMBER,
    GOO_AST_EXTEND_DECL,
    GOO_AST_TRAIT_DECL,
    GOO_AST_MATCH_STMT,
    GOO_AST_MATCH_CASE,
    GOO_AST_PATTERN_EXPR,
    GOO_AST_NULL_COALESCE_EXPR, // a ?? b
    GOO_AST_OPTIONAL_ACCESS_EXPR, // a?.b
    GOO_AST_SAFE_CALL_EXPR,     // a?(b)
    GOO_AST_NULLABLE_TYPE       // int?
} GooAstNodeType;

// Parser result codes
typedef enum {
    GOO_PARSER_SUCCESS = 0,
    GOO_PARSER_ERROR_SYNTAX = 1,
    GOO_PARSER_ERROR_UNEXPECTED_TOKEN = 2,
    GOO_PARSER_ERROR_EXPECTED_TOKEN = 3,
    GOO_PARSER_ERROR_INVALID_STATEMENT = 4,
    GOO_PARSER_ERROR_INVALID_EXPRESSION = 5,
    GOO_PARSER_ERROR_OUT_OF_MEMORY = 6,
    GOO_PARSER_ERROR_GENERAL = 7
} GooParserResultCode;

// Opaque handles
typedef void* GooParserHandle;
typedef void* GooAstNodeHandle;
typedef void* GooAstProgramHandle;

// Position in source code
typedef struct {
    size_t line;
    size_t column;
} GooSourcePosition;

// Create a new parser
GooParserHandle gooParserCreate(void);

// Parse source code into an AST
GooParserResultCode gooParserParseString(GooParserHandle parser, const char* source);

// Parse source code from a file into an AST
GooParserResultCode gooParserParseFile(GooParserHandle parser, const char* filename);

// Get the AST root node
GooAstNodeHandle gooParserGetAstRoot(GooParserHandle parser);

// Get the program node
GooAstProgramHandle gooParserGetProgram(GooParserHandle parser);

// Get error information
const char* gooParserGetError(GooParserHandle parser);
GooSourcePosition gooParserGetErrorPosition(GooParserHandle parser);

// Get node type
GooAstNodeType gooAstGetNodeType(GooAstNodeHandle node);

// Get node position
GooSourcePosition gooAstGetStartPosition(GooAstNodeHandle node);
GooSourcePosition gooAstGetEndPosition(GooAstNodeHandle node);

// Get program information
size_t gooAstProgramGetImportCount(GooAstProgramHandle program);
GooAstNodeHandle gooAstProgramGetImport(GooAstProgramHandle program, size_t index);
size_t gooAstProgramGetDeclarationCount(GooAstProgramHandle program);
GooAstNodeHandle gooAstProgramGetDeclaration(GooAstProgramHandle program, size_t index);
GooAstNodeHandle gooAstProgramGetPackageDeclaration(GooAstProgramHandle program);

// Clean up resources
void gooParserDestroy(GooParserHandle parser);

#ifdef __cplusplus
}
#endif

#endif // GOO_PARSER_H 