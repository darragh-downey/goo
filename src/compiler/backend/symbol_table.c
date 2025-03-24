#include <stdlib.h>
#include <string.h>
#include "symbol_table.h"

// Initialize a new symbol table
GooSymbolTable* goo_symbol_table_init() {
    GooSymbolTable* table = (GooSymbolTable*)malloc(sizeof(GooSymbolTable));
    if (!table) return NULL;

    // Create global scope
    GooScope* global_scope = (GooScope*)malloc(sizeof(GooScope));
    if (!global_scope) {
        free(table);
        return NULL;
    }

    global_scope->symbols = NULL;
    global_scope->parent = NULL;
    global_scope->is_function_scope = false;

    table->global_scope = global_scope;
    table->current_scope = global_scope;

    return table;
}

// Free all symbols in a scope
static void free_scope_symbols(GooSymbol* symbol) {
    GooSymbol* current = symbol;
    GooSymbol* next;

    while (current) {
        next = current->next;
        free(current->name);
        free(current);
        current = next;
    }
}

// Free a scope and all its symbols
static void free_scope(GooScope* scope) {
    if (!scope) return;
    
    free_scope_symbols(scope->symbols);
    free(scope);
}

// Free a symbol table
void goo_symbol_table_free(GooSymbolTable* table) {
    if (!table) return;

    // Free all scopes
    GooScope* current = table->current_scope;
    GooScope* parent;

    // Navigate up to the global scope
    while (current && current != table->global_scope) {
        parent = current->parent;
        free_scope(current);
        current = parent;
    }

    // Free the global scope
    free_scope(table->global_scope);
    
    // Free the table itself
    free(table);
}

// Enter a new scope
void goo_symbol_table_enter_scope(GooSymbolTable* table, bool is_function_scope) {
    if (!table) return;

    // Create new scope
    GooScope* new_scope = (GooScope*)malloc(sizeof(GooScope));
    if (!new_scope) return;

    new_scope->symbols = NULL;
    new_scope->parent = table->current_scope;
    new_scope->is_function_scope = is_function_scope;

    // Set as current scope
    table->current_scope = new_scope;
}

// Exit the current scope
void goo_symbol_table_exit_scope(GooSymbolTable* table) {
    if (!table || !table->current_scope || table->current_scope == table->global_scope) {
        return;  // Cannot exit global scope
    }

    GooScope* old_scope = table->current_scope;
    table->current_scope = old_scope->parent;
    
    free_scope(old_scope);
}

// Add a symbol to the current scope
GooSymbol* goo_symbol_table_add(GooSymbolTable* table, const char* name, GooSymbolKind kind, 
                             LLVMValueRef llvm_value, GooNode* ast_node, LLVMTypeRef llvm_type) {
    if (!table || !table->current_scope || !name) return NULL;

    // Check if symbol already exists in the current scope
    GooSymbol* existing = goo_symbol_table_lookup_current_scope(table, name);
    if (existing) return existing;  // Symbol already exists

    // Create new symbol
    GooSymbol* symbol = (GooSymbol*)malloc(sizeof(GooSymbol));
    if (!symbol) return NULL;

    symbol->name = strdup(name);
    if (!symbol->name) {
        free(symbol);
        return NULL;
    }

    symbol->kind = kind;
    symbol->llvm_value = llvm_value;
    symbol->ast_node = ast_node;
    symbol->llvm_type = llvm_type;
    
    // Add to the beginning of the linked list
    symbol->next = table->current_scope->symbols;
    table->current_scope->symbols = symbol;

    return symbol;
}

// Look up a symbol by name in the current scope only
GooSymbol* goo_symbol_table_lookup_current_scope(GooSymbolTable* table, const char* name) {
    if (!table || !table->current_scope || !name) return NULL;

    GooSymbol* current = table->current_scope->symbols;

    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;  // Found the symbol
        }
        current = current->next;
    }

    return NULL;  // Symbol not found in current scope
}

// Look up a symbol by name in the current scope and parent scopes
GooSymbol* goo_symbol_table_lookup(GooSymbolTable* table, const char* name) {
    if (!table || !name) return NULL;

    GooScope* scope = table->current_scope;

    while (scope) {
        GooSymbol* current = scope->symbols;

        while (current) {
            if (strcmp(current->name, name) == 0) {
                return current;  // Found the symbol
            }
            current = current->next;
        }

        scope = scope->parent;  // Move up to the parent scope
    }

    return NULL;  // Symbol not found in any scope
} 