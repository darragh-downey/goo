/**
 * @file symbols.h
 * @brief Symbol management for the Goo compiler
 * 
 * This module handles symbol extraction, indexing, and lookup to
 * support features like code navigation, completion, and refactoring.
 */

#ifndef GOO_SYMBOLS_H
#define GOO_SYMBOLS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Symbol type enum
 */
typedef enum {
    GOO_SYMBOL_UNKNOWN,
    GOO_SYMBOL_FUNCTION,
    GOO_SYMBOL_METHOD,
    GOO_SYMBOL_STRUCT,
    GOO_SYMBOL_ENUM,
    GOO_SYMBOL_VARIABLE,
    GOO_SYMBOL_CONSTANT,
    GOO_SYMBOL_PARAMETER,
    GOO_SYMBOL_TYPE_ALIAS,
    GOO_SYMBOL_TRAIT,
    GOO_SYMBOL_MODULE,
    GOO_SYMBOL_IMPORT
} GooSymbolType;

/**
 * Symbol visibility scope
 */
typedef enum {
    GOO_VISIBILITY_UNKNOWN,
    GOO_VISIBILITY_PUBLIC,
    GOO_VISIBILITY_PRIVATE,
    GOO_VISIBILITY_INTERNAL,
    GOO_VISIBILITY_LOCAL
} GooSymbolVisibility;

/**
 * Symbol source location
 */
typedef struct {
    const char* file;
    unsigned int line;
    unsigned int column;
    unsigned int length;
} GooSymbolLocation;

/**
 * Symbol definition
 */
typedef struct GooSymbol {
    char* name;
    GooSymbolType type;
    GooSymbolVisibility visibility;
    GooSymbolLocation definition;
    
    char* parent_name;          // Name of parent scope (function, struct, etc.)
    struct GooSymbol* parent;   // Pointer to parent symbol, if available
    
    char* type_name;            // Type name for variables, return type for functions
    char* documentation;        // Documentation comments
    
    // References
    GooSymbolLocation* references;
    unsigned int reference_count;
    
    // Child symbols (members of structs, etc.)
    struct GooSymbol** children;
    unsigned int child_count;
} GooSymbol;

/**
 * Symbol table for a project
 */
typedef struct {
    GooSymbol** symbols;
    unsigned int symbol_count;
    unsigned int capacity;
    
    char** files;               // List of files in the project
    unsigned int file_count;
} GooSymbolTable;

/**
 * Create a new symbol table
 */
GooSymbolTable* goo_symbol_table_new(void);

/**
 * Free a symbol table and all symbols within it
 */
void goo_symbol_table_free(GooSymbolTable* table);

/**
 * Extract symbols from a source file and add them to the table
 * 
 * @param table Symbol table to add symbols to
 * @param filename Source file path
 * @param source Source code (if NULL, file will be read from disk)
 * @return true on success, false on error
 */
bool goo_extract_symbols(GooSymbolTable* table, const char* filename, const char* source);

/**
 * Find a symbol by name in the symbol table
 * 
 * @param table Symbol table to search in
 * @param name Symbol name to find
 * @param parent Parent symbol name (can be NULL)
 * @return Found symbol or NULL if not found
 */
GooSymbol* goo_find_symbol(const GooSymbolTable* table, const char* name, const char* parent);

/**
 * Find symbols by prefix for completion
 * 
 * @param table Symbol table to search in
 * @param prefix Symbol name prefix
 * @param scope_name Current scope (can be NULL)
 * @param max_results Maximum number of results to return
 * @param results Array to fill with matching symbols
 * @return Number of matching symbols found
 */
int goo_find_symbols_by_prefix(
    const GooSymbolTable* table,
    const char* prefix,
    const char* scope_name,
    int max_results,
    GooSymbol** results
);

/**
 * Find all symbols of a given type
 * 
 * @param table Symbol table to search in
 * @param type Symbol type to find
 * @param max_results Maximum number of results to return
 * @param results Array to fill with matching symbols
 * @return Number of matching symbols found
 */
int goo_find_symbols_by_type(
    const GooSymbolTable* table,
    GooSymbolType type,
    int max_results,
    GooSymbol** results
);

/**
 * Find symbols at a specific position in a file
 * 
 * @param table Symbol table to search in
 * @param filename Source file path
 * @param line Line number (1-based)
 * @param column Column number (1-based) 
 * @return Found symbol or NULL if not found
 */
GooSymbol* goo_find_symbol_at_position(
    const GooSymbolTable* table,
    const char* filename,
    unsigned int line,
    unsigned int column
);

/**
 * Find all references to a symbol
 * 
 * @param table Symbol table to search in
 * @param symbol Symbol to find references for
 * @param max_results Maximum number of results to return
 * @param results Array to fill with reference locations
 * @return Number of references found
 */
int goo_find_references(
    const GooSymbolTable* table,
    const GooSymbol* symbol,
    int max_results,
    GooSymbolLocation* results
);

/**
 * Create a new symbol
 */
GooSymbol* goo_symbol_new(
    const char* name,
    GooSymbolType type,
    GooSymbolVisibility visibility,
    const char* file,
    unsigned int line,
    unsigned int column,
    unsigned int length
);

/**
 * Free a symbol and all its children
 */
void goo_symbol_free(GooSymbol* symbol);

/**
 * Add a reference to a symbol
 */
bool goo_symbol_add_reference(
    GooSymbol* symbol,
    const char* file,
    unsigned int line,
    unsigned int column,
    unsigned int length
);

/**
 * Add a child symbol to a parent symbol
 */
bool goo_symbol_add_child(
    GooSymbol* parent,
    GooSymbol* child
);

/**
 * Get the string representation of a symbol type
 */
const char* goo_symbol_type_to_string(GooSymbolType type);

/**
 * Get the string representation of a symbol visibility
 */
const char* goo_symbol_visibility_to_string(GooSymbolVisibility visibility);

#ifdef __cplusplus
}
#endif

#endif /* GOO_SYMBOLS_H */ 