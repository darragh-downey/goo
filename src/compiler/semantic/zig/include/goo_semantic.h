#ifndef GOO_SEMANTIC_H
#define GOO_SEMANTIC_H

#include <stdint.h>
#include "goo_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opaque handle for the semantic analyzer
 */
typedef struct GooSemantic* GooSemanticHandle;

/**
 * Error codes for semantic analysis
 */
typedef enum {
    GOO_SEMANTIC_SUCCESS = 0,
    GOO_SEMANTIC_TYPE_MISMATCH = 1,
    GOO_SEMANTIC_UNDEFINED_VARIABLE = 2,
    GOO_SEMANTIC_UNDEFINED_FUNCTION = 3,
    GOO_SEMANTIC_UNDEFINED_TYPE = 4,
    GOO_SEMANTIC_INVALID_ASSIGNMENT = 5,
    GOO_SEMANTIC_INVALID_OPERATION = 6,
    GOO_SEMANTIC_INVALID_FUNCTION_CALL = 7,
    GOO_SEMANTIC_GENERAL_ERROR = 8,
} GooSemanticErrorCode;

/**
 * Type ID enum
 */
typedef enum {
    GOO_TYPE_VOID = 0,
    GOO_TYPE_BOOL = 1,
    GOO_TYPE_INT = 2,
    GOO_TYPE_FLOAT = 3,
    GOO_TYPE_STRING = 4,
    GOO_TYPE_ARRAY = 5,
    GOO_TYPE_STRUCT = 6,
    GOO_TYPE_FUNCTION = 7,
    GOO_TYPE_CUSTOM = 8,
    GOO_TYPE_ERROR = 9,
} GooTypeId;

/**
 * Create a new semantic analyzer instance
 *
 * @return Handle to the semantic analyzer, or NULL on failure
 */
GooSemanticHandle gooSemanticCreate(void);

/**
 * Destroy a semantic analyzer instance
 *
 * @param handle Handle to the semantic analyzer instance
 */
void gooSemanticDestroy(GooSemanticHandle handle);

/**
 * Run semantic analysis on an AST
 *
 * @param handle Handle to the semantic analyzer instance
 * @param parser_handle Handle to the parser containing the AST to analyze
 * @return Error code indicating success or failure
 */
GooSemanticErrorCode gooSemanticAnalyze(GooSemanticHandle handle, GooParserHandle parser_handle);

/**
 * Get the error message from the last semantic operation
 *
 * @param handle Handle to the semantic analyzer instance
 * @return Error message, or "No error" if no error occurred
 */
const char* gooSemanticGetErrorMessage(GooSemanticHandle handle);

/**
 * Get the number of semantic errors
 *
 * @param handle Handle to the semantic analyzer instance
 * @return Number of semantic errors
 */
int gooSemanticGetErrorCount(GooSemanticHandle handle);

/**
 * Get a semantic error by index
 *
 * @param handle Handle to the semantic analyzer instance
 * @param index Index of the error to retrieve
 * @return Error message
 */
const char* gooSemanticGetError(GooSemanticHandle handle, int index);

#ifdef __cplusplus
}
#endif

#endif /* GOO_SEMANTIC_H */ 