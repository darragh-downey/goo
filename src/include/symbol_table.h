/**
 * symbol_table.h
 * 
 * Symbol table implementation for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_SRC_SYMBOL_TABLE_H
#define GOO_SRC_SYMBOL_TABLE_H

#include <stdbool.h>
#include <stdint.h>
#include "type_table.h"

// Forward declarations
typedef struct GooSymbol GooSymbol;
typedef struct GooSymbolTable GooSymbolTable;
typedef struct GooScope GooScope;

// Symbol kinds
typedef enum {
    GOO_SYMBOL_UNKNOWN,
    GOO_SYMBOL_VARIABLE,       // Variable
    GOO_SYMBOL_CONSTANT,       // Constant
    GOO_SYMBOL_FUNCTION,       // Function
    GOO_SYMBOL_PARAMETER,      // Function parameter
    GOO_SYMBOL_TYPE,           // Type definition
    GOO_SYMBOL_MODULE,         // Module
    GOO_SYMBOL_PACKAGE,        // Package
    GOO_SYMBOL_IMPORT,         // Import
    GOO_SYMBOL_FIELD,          // Struct field
    GOO_SYMBOL_METHOD,         // Method
    GOO_SYMBOL_LABEL           // Label
} GooSymbolKind;

// Symbol flags
typedef enum {
    GOO_SYMBOL_FLAG_NONE = 0,
    GOO_SYMBOL_FLAG_USED = 1,             // Symbol is used
    GOO_SYMBOL_FLAG_DEFINED = 2,          // Symbol is defined
    GOO_SYMBOL_FLAG_EXPORTED = 4,         // Symbol is exported (public)
    GOO_SYMBOL_FLAG_CONSTANT = 8,         // Symbol is a constant
    GOO_SYMBOL_FLAG_MUTABLE = 16,         // Symbol is mutable
    GOO_SYMBOL_FLAG_SAFE = 32,            // Symbol is marked as safe
    GOO_SYMBOL_FLAG_UNSAFE = 64,          // Symbol is marked as unsafe
    GOO_SYMBOL_FLAG_COMPTIME = 128,       // Symbol is compile-time
    GOO_SYMBOL_FLAG_RUNTIME = 256,        // Symbol is runtime
    GOO_SYMBOL_FLAG_BUILTIN = 512,        // Symbol is built-in
    GOO_SYMBOL_FLAG_IMPLICIT = 1024       // Symbol is implicitly declared
} GooSymbolFlag;

// Symbol structure
struct GooSymbol {
    char* name;                // Symbol name
    GooSymbolKind kind;        // Symbol kind
    int flags;                 // Symbol flags
    GooType* type;             // Symbol type
    GooAstNode* node;          // AST node for this symbol
    GooSymbol* next;           // Next symbol in current scope
};

// Scope structure
struct GooScope {
    GooSymbol* symbols;        // Symbols in this scope
    GooScope* parent;          // Parent scope
    int level;                 // Nesting level (0 for global)
    bool is_function_scope;    // Whether this is a function scope
    bool is_loop_scope;        // Whether this is a loop scope
};

// Symbol table structure
struct GooSymbolTable {
    GooScope* current_scope;   // Current scope
    GooScope* global_scope;    // Global scope
    int scope_count;           // Number of scopes
};

/**
 * Create a new symbol table
 * 
 * @return A new symbol table
 */
GooSymbolTable* goo_symbol_table_create(void);

/**
 * Free a symbol table and all symbols in it
 * 
 * @param table The symbol table to free
 */
void goo_symbol_table_free(GooSymbolTable* table);

/**
 * Push a new scope onto the stack
 * 
 * @param table The symbol table
 */
void goo_symbol_table_push_scope(GooSymbolTable* table);

/**
 * Push a function scope onto the stack
 * 
 * @param table The symbol table
 */
void goo_symbol_table_push_function_scope(GooSymbolTable* table);

/**
 * Push a loop scope onto the stack
 * 
 * @param table The symbol table
 */
void goo_symbol_table_push_loop_scope(GooSymbolTable* table);

/**
 * Pop the current scope from the stack
 * 
 * @param table The symbol table
 */
void goo_symbol_table_pop_scope(GooSymbolTable* table);

/**
 * Create a new symbol
 * 
 * @param name The symbol name
 * @param type The symbol type
 * @param kind The symbol kind
 * @return A new symbol
 */
GooSymbol* goo_symbol_create(const char* name, GooType* type, GooSymbolKind kind);

/**
 * Add a symbol to the current scope
 * 
 * @param table The symbol table
 * @param symbol The symbol to add
 * @return true if added successfully, false if already exists in current scope
 */
bool goo_symbol_table_add(GooSymbolTable* table, GooSymbol* symbol);

/**
 * Lookup a symbol in the current scope and parent scopes
 * 
 * @param table The symbol table
 * @param name The symbol name
 * @return The symbol or NULL if not found
 */
GooSymbol* goo_symbol_table_lookup(GooSymbolTable* table, const char* name);

/**
 * Lookup a symbol in the current scope only
 * 
 * @param table The symbol table
 * @param name The symbol name
 * @return The symbol or NULL if not found
 */
GooSymbol* goo_symbol_table_lookup_current_scope(GooSymbolTable* table, const char* name);

/**
 * Look up a symbol in the global scope
 * 
 * @param table The symbol table
 * @param name The symbol name
 * @return The symbol or NULL if not found
 */
GooSymbol* goo_symbol_table_lookup_global(GooSymbolTable* table, const char* name);

/**
 * Get the current function scope
 * 
 * @param table The symbol table
 * @return The function scope or NULL if not in a function
 */
GooScope* goo_symbol_table_get_function_scope(GooSymbolTable* table);

/**
 * Check if currently in a loop scope
 * 
 * @param table The symbol table
 * @return true if in a loop scope, false otherwise
 */
bool goo_symbol_table_in_loop(GooSymbolTable* table);

/**
 * Marks a symbol as used
 * 
 * @param symbol The symbol to mark
 */
void goo_symbol_mark_used(GooSymbol* symbol);

/**
 * Marks a symbol as defined
 * 
 * @param symbol The symbol to mark
 */
void goo_symbol_mark_defined(GooSymbol* symbol);

/**
 * Marks a symbol as exported (public)
 * 
 * @param symbol The symbol to mark
 */
void goo_symbol_mark_exported(GooSymbol* symbol);

/**
 * Checks if a symbol has a specific flag
 * 
 * @param symbol The symbol to check
 * @param flag The flag to check for
 * @return true if the symbol has the flag, false otherwise
 */
bool goo_symbol_has_flag(GooSymbol* symbol, GooSymbolFlag flag);

/**
 * Sets a flag on a symbol
 * 
 * @param symbol The symbol to modify
 * @param flag The flag to set
 */
void goo_symbol_set_flag(GooSymbol* symbol, GooSymbolFlag flag);

/**
 * Clears a flag on a symbol
 * 
 * @param symbol The symbol to modify
 * @param flag The flag to clear
 */
void goo_symbol_clear_flag(GooSymbol* symbol, GooSymbolFlag flag);

#endif /* GOO_SRC_SYMBOL_TABLE_H */ 