#ifndef GOO_SYMBOL_TABLE_H
#define GOO_SYMBOL_TABLE_H

#include <stdbool.h>
#include <llvm-c/Core.h>
#include "ast.h"

// Symbol kinds
typedef enum {
    GOO_SYMBOL_VARIABLE,
    GOO_SYMBOL_FUNCTION,
    GOO_SYMBOL_TYPE,
    GOO_SYMBOL_CHANNEL,
    GOO_SYMBOL_MODULE,
    GOO_SYMBOL_ALLOCATOR
} GooSymbolKind;

// Symbol entry in the symbol table
typedef struct GooSymbol {
    char* name;                // Symbol name
    GooSymbolKind kind;        // Symbol kind
    LLVMValueRef llvm_value;   // LLVM value reference
    GooNode* ast_node;         // Reference to AST node
    LLVMTypeRef llvm_type;     // LLVM type
    struct GooSymbol* next;    // Next symbol in the same scope
} GooSymbol;

// Scope in the symbol table
typedef struct GooScope {
    GooSymbol* symbols;        // Linked list of symbols in this scope
    struct GooScope* parent;   // Parent scope
    bool is_function_scope;    // Whether this is a function scope
} GooScope;

// Symbol table structure
typedef struct SymbolTable {
    GooScope* current_scope;   // Current active scope
    GooScope* global_scope;    // Global scope
} GooSymbolTable;

// Initialize a new symbol table
GooSymbolTable* goo_symbol_table_init();

// Free a symbol table
void goo_symbol_table_free(GooSymbolTable* table);

// Enter a new scope
void goo_symbol_table_enter_scope(GooSymbolTable* table, bool is_function_scope);

// Exit the current scope
void goo_symbol_table_exit_scope(GooSymbolTable* table);

// Add a symbol to the current scope
GooSymbol* goo_symbol_table_add(GooSymbolTable* table, const char* name, GooSymbolKind kind, 
                             LLVMValueRef llvm_value, GooNode* ast_node, LLVMTypeRef llvm_type);

// Look up a symbol by name in the current scope and parent scopes
GooSymbol* goo_symbol_table_lookup(GooSymbolTable* table, const char* name);

// Look up a symbol by name in the current scope only
GooSymbol* goo_symbol_table_lookup_current_scope(GooSymbolTable* table, const char* name);

#endif // GOO_SYMBOL_TABLE_H 