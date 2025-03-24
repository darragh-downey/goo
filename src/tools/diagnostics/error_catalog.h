/**
 * @file error_catalog.h
 * @brief Error catalog system for the Goo compiler
 * 
 * This file defines the error catalog system, which provides detailed
 * explanations for compiler error codes, similar to Rust's --explain feature.
 */

#ifndef GOO_ERROR_CATALOG_H
#define GOO_ERROR_CATALOG_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Error category types
 */
typedef enum {
    GOO_ERROR_CAT_SYNTAX,       // Syntax and parsing errors
    GOO_ERROR_CAT_TYPE,         // Type system errors
    GOO_ERROR_CAT_BORROW,       // Borrow checker errors
    GOO_ERROR_CAT_LIFETIME,     // Lifetime errors
    GOO_ERROR_CAT_COMPILER,     // Compiler internal errors
    GOO_ERROR_CAT_LINKER,       // Linker and linking errors
    GOO_ERROR_CAT_MACRO,        // Macro-related errors
    GOO_ERROR_CAT_ATTRIBUTE,    // Attribute-related errors
    GOO_ERROR_CAT_IO,           // I/O and file errors
    GOO_ERROR_CAT_MISC          // Miscellaneous errors
} GooErrorCategory;

/**
 * Error catalog entry
 */
typedef struct {
    const char* code;           // Error code (e.g., E0001)
    GooErrorCategory category;  // Error category
    const char* short_desc;     // Short description
    const char* explanation;    // Detailed explanation
    const char* example;        // Example code showing the error
    const char* solution;       // Example code showing the solution
} GooErrorCatalogEntry;

/**
 * Error catalog
 */
typedef struct {
    GooErrorCatalogEntry* entries;  // Array of error entries
    size_t count;                   // Number of entries
    size_t capacity;                // Capacity of entries array
} GooErrorCatalog;

/**
 * Initialize the error catalog system
 */
bool goo_error_catalog_init(void);

/**
 * Clean up the error catalog system
 */
void goo_error_catalog_cleanup(void);

/**
 * Look up an error code in the catalog
 * 
 * @param code The error code to look up
 * @return The error entry, or NULL if not found
 */
const GooErrorCatalogEntry* goo_error_catalog_lookup(const char* code);

/**
 * Print the explanation for an error code
 * 
 * @param code The error code to explain
 * @return true if the code was found and explained, false otherwise
 */
bool goo_error_catalog_explain(const char* code);

/**
 * Register a new error code in the catalog
 * 
 * @param code The error code (e.g., E0001)
 * @param category The error category
 * @param short_desc Short description
 * @param explanation Detailed explanation
 * @param example Example code showing the error
 * @param solution Example code showing the solution
 * @return true if registered successfully, false otherwise
 */
bool goo_error_catalog_register(
    const char* code,
    GooErrorCategory category,
    const char* short_desc,
    const char* explanation,
    const char* example,
    const char* solution
);

/**
 * Get the number of registered error codes
 */
size_t goo_error_catalog_count(void);

/**
 * Get all error codes in a category
 * 
 * @param category The category to filter by
 * @param codes Array to store the codes
 * @param capacity Capacity of the codes array
 * @return Number of codes found
 */
size_t goo_error_catalog_get_by_category(
    GooErrorCategory category,
    const char** codes,
    size_t capacity
);

/**
 * Get a string representation of an error category
 */
const char* goo_error_category_to_string(GooErrorCategory category);

#ifdef __cplusplus
}
#endif

#endif /* GOO_ERROR_CATALOG_H */ 