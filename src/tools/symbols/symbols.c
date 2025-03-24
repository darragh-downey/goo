/**
 * @file symbols.c
 * @brief Implementation of the Goo symbol management
 */

#include "symbols.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Initial capacity for symbol tables
#define INITIAL_CAPACITY 256

// Forward declaration of internal functions
static bool read_file_to_string(const char* filename, char** content);
static bool is_identifier_char(char c);
static bool parse_tokens_for_symbols(GooSymbolTable* table, const char* filename, const char* source);
static void add_symbol_to_table(GooSymbolTable* table, GooSymbol* symbol);

/**
 * Create a new symbol table
 */
GooSymbolTable* goo_symbol_table_new(void) {
    GooSymbolTable* table = (GooSymbolTable*)malloc(sizeof(GooSymbolTable));
    if (!table) {
        return NULL;
    }
    
    table->capacity = INITIAL_CAPACITY;
    table->symbol_count = 0;
    table->file_count = 0;
    
    table->symbols = (GooSymbol**)malloc(sizeof(GooSymbol*) * table->capacity);
    if (!table->symbols) {
        free(table);
        return NULL;
    }
    
    table->files = (char**)malloc(sizeof(char*) * INITIAL_CAPACITY);
    if (!table->files) {
        free(table->symbols);
        free(table);
        return NULL;
    }
    
    return table;
}

/**
 * Free a symbol table and all symbols within it
 */
void goo_symbol_table_free(GooSymbolTable* table) {
    if (!table) {
        return;
    }
    
    // Free all symbols
    for (unsigned int i = 0; i < table->symbol_count; i++) {
        goo_symbol_free(table->symbols[i]);
    }
    
    // Free all file paths
    for (unsigned int i = 0; i < table->file_count; i++) {
        free(table->files[i]);
    }
    
    free(table->symbols);
    free(table->files);
    free(table);
}

/**
 * Extract symbols from a source file and add them to the table
 */
bool goo_extract_symbols(GooSymbolTable* table, const char* filename, const char* source) {
    if (!table || !filename) {
        return false;
    }
    
    // Read the file if source is not provided
    char* file_content = NULL;
    bool free_source = false;
    
    if (!source) {
        if (!read_file_to_string(filename, &file_content)) {
            return false;
        }
        source = file_content;
        free_source = true;
    }
    
    // Add file to the list if not already present
    bool file_found = false;
    for (unsigned int i = 0; i < table->file_count; i++) {
        if (strcmp(table->files[i], filename) == 0) {
            file_found = true;
            break;
        }
    }
    
    if (!file_found) {
        // Check if we need to resize
        if (table->file_count >= table->capacity) {
            unsigned int new_capacity = table->capacity * 2;
            char** new_files = (char**)realloc(table->files, sizeof(char*) * new_capacity);
            if (!new_files) {
                if (free_source) {
                    free(file_content);
                }
                return false;
            }
            table->files = new_files;
        }
        
        // Add the file
        table->files[table->file_count] = strdup(filename);
        if (!table->files[table->file_count]) {
            if (free_source) {
                free(file_content);
            }
            return false;
        }
        table->file_count++;
    }
    
    // Parse the source code to extract symbols
    bool result = parse_tokens_for_symbols(table, filename, source);
    
    // Free source if we allocated it
    if (free_source) {
        free(file_content);
    }
    
    return result;
}

/**
 * Find a symbol by name in the symbol table
 */
GooSymbol* goo_find_symbol(const GooSymbolTable* table, const char* name, const char* parent) {
    if (!table || !name) {
        return NULL;
    }
    
    for (unsigned int i = 0; i < table->symbol_count; i++) {
        if (strcmp(table->symbols[i]->name, name) == 0) {
            // If parent is specified, check if it matches
            if (parent) {
                if (table->symbols[i]->parent_name && strcmp(table->symbols[i]->parent_name, parent) == 0) {
                    return table->symbols[i];
                }
            } else {
                return table->symbols[i];
            }
        }
    }
    
    return NULL;
}

/**
 * Find symbols by prefix for completion
 */
int goo_find_symbols_by_prefix(
    const GooSymbolTable* table,
    const char* prefix,
    const char* scope_name,
    int max_results,
    GooSymbol** results
) {
    if (!table || !prefix || !results || max_results <= 0) {
        return 0;
    }
    
    int count = 0;
    size_t prefix_len = strlen(prefix);
    
    // First, look for symbols in the current scope if specified
    if (scope_name) {
        GooSymbol* scope = goo_find_symbol(table, scope_name, NULL);
        if (scope) {
            for (unsigned int i = 0; i < scope->child_count && count < max_results; i++) {
                if (strncmp(scope->children[i]->name, prefix, prefix_len) == 0) {
                    results[count++] = scope->children[i];
                }
            }
        }
    }
    
    // Then look for global symbols or symbols in parent scopes
    for (unsigned int i = 0; i < table->symbol_count && count < max_results; i++) {
        // Skip symbols that are not in global scope if scope_name was provided
        if (scope_name && table->symbols[i]->parent_name) {
            continue;
        }
        
        if (strncmp(table->symbols[i]->name, prefix, prefix_len) == 0) {
            results[count++] = table->symbols[i];
        }
    }
    
    return count;
}

/**
 * Find all symbols of a given type
 */
int goo_find_symbols_by_type(
    const GooSymbolTable* table,
    GooSymbolType type,
    int max_results,
    GooSymbol** results
) {
    if (!table || !results || max_results <= 0) {
        return 0;
    }
    
    int count = 0;
    
    for (unsigned int i = 0; i < table->symbol_count && count < max_results; i++) {
        if (table->symbols[i]->type == type) {
            results[count++] = table->symbols[i];
        }
    }
    
    return count;
}

/**
 * Find symbols at a specific position in a file
 */
GooSymbol* goo_find_symbol_at_position(
    const GooSymbolTable* table,
    const char* filename,
    unsigned int line,
    unsigned int column
) {
    if (!table || !filename) {
        return NULL;
    }
    
    // Find the symbol whose definition contains the given position
    for (unsigned int i = 0; i < table->symbol_count; i++) {
        if (strcmp(table->symbols[i]->definition.file, filename) == 0 &&
            table->symbols[i]->definition.line == line) {
            
            unsigned int symbol_start = table->symbols[i]->definition.column;
            unsigned int symbol_end = symbol_start + table->symbols[i]->definition.length;
            
            if (column >= symbol_start && column < symbol_end) {
                return table->symbols[i];
            }
        }
        
        // Check references too
        for (unsigned int j = 0; j < table->symbols[i]->reference_count; j++) {
            if (strcmp(table->symbols[i]->references[j].file, filename) == 0 &&
                table->symbols[i]->references[j].line == line) {
                
                unsigned int ref_start = table->symbols[i]->references[j].column;
                unsigned int ref_end = ref_start + table->symbols[i]->references[j].length;
                
                if (column >= ref_start && column < ref_end) {
                    return table->symbols[i];
                }
            }
        }
    }
    
    return NULL;
}

/**
 * Find all references to a symbol
 */
int goo_find_references(
    const GooSymbolTable* table,
    const GooSymbol* symbol,
    int max_results,
    GooSymbolLocation* results
) {
    if (!table || !symbol || !results || max_results <= 0) {
        return 0;
    }
    
    int count = 0;
    
    // Add the symbol definition itself
    if (count < max_results) {
        results[count++] = symbol->definition;
    }
    
    // Add all references
    for (unsigned int i = 0; i < symbol->reference_count && count < max_results; i++) {
        results[count++] = symbol->references[i];
    }
    
    return count;
}

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
) {
    if (!name || !file) {
        return NULL;
    }
    
    GooSymbol* symbol = (GooSymbol*)malloc(sizeof(GooSymbol));
    if (!symbol) {
        return NULL;
    }
    
    // Initialize all fields
    symbol->name = strdup(name);
    if (!symbol->name) {
        free(symbol);
        return NULL;
    }
    
    symbol->type = type;
    symbol->visibility = visibility;
    
    symbol->definition.file = strdup(file);
    if (!symbol->definition.file) {
        free(symbol->name);
        free(symbol);
        return NULL;
    }
    
    symbol->definition.line = line;
    symbol->definition.column = column;
    symbol->definition.length = length;
    
    symbol->parent_name = NULL;
    symbol->parent = NULL;
    symbol->type_name = NULL;
    symbol->documentation = NULL;
    
    symbol->references = NULL;
    symbol->reference_count = 0;
    
    symbol->children = NULL;
    symbol->child_count = 0;
    
    return symbol;
}

/**
 * Free a symbol and all its children
 */
void goo_symbol_free(GooSymbol* symbol) {
    if (!symbol) {
        return;
    }
    
    free(symbol->name);
    free((void*)symbol->definition.file); // Cast away const
    free(symbol->parent_name);
    free(symbol->type_name);
    free(symbol->documentation);
    
    // Free references
    if (symbol->references) {
        for (unsigned int i = 0; i < symbol->reference_count; i++) {
            free((void*)symbol->references[i].file); // Cast away const
        }
        free(symbol->references);
    }
    
    // Free children
    if (symbol->children) {
        for (unsigned int i = 0; i < symbol->child_count; i++) {
            goo_symbol_free(symbol->children[i]);
        }
        free(symbol->children);
    }
    
    free(symbol);
}

/**
 * Add a reference to a symbol
 */
bool goo_symbol_add_reference(
    GooSymbol* symbol,
    const char* file,
    unsigned int line,
    unsigned int column,
    unsigned int length
) {
    if (!symbol || !file) {
        return false;
    }
    
    // Resize references array if needed
    if (symbol->reference_count == 0) {
        symbol->references = (GooSymbolLocation*)malloc(sizeof(GooSymbolLocation));
        if (!symbol->references) {
            return false;
        }
    } else {
        GooSymbolLocation* new_refs = (GooSymbolLocation*)realloc(
            symbol->references,
            sizeof(GooSymbolLocation) * (symbol->reference_count + 1)
        );
        if (!new_refs) {
            return false;
        }
        symbol->references = new_refs;
    }
    
    // Add the reference
    symbol->references[symbol->reference_count].file = strdup(file);
    if (!symbol->references[symbol->reference_count].file) {
        return false;
    }
    
    symbol->references[symbol->reference_count].line = line;
    symbol->references[symbol->reference_count].column = column;
    symbol->references[symbol->reference_count].length = length;
    
    symbol->reference_count++;
    return true;
}

/**
 * Add a child symbol to a parent symbol
 */
bool goo_symbol_add_child(
    GooSymbol* parent,
    GooSymbol* child
) {
    if (!parent || !child) {
        return false;
    }
    
    // Resize children array if needed
    if (parent->child_count == 0) {
        parent->children = (GooSymbol**)malloc(sizeof(GooSymbol*));
        if (!parent->children) {
            return false;
        }
    } else {
        GooSymbol** new_children = (GooSymbol**)realloc(
            parent->children,
            sizeof(GooSymbol*) * (parent->child_count + 1)
        );
        if (!new_children) {
            return false;
        }
        parent->children = new_children;
    }
    
    // Add the child
    parent->children[parent->child_count++] = child;
    
    // Set the parent-child relationship
    child->parent = parent;
    child->parent_name = strdup(parent->name);
    
    return true;
}

/**
 * Get the string representation of a symbol type
 */
const char* goo_symbol_type_to_string(GooSymbolType type) {
    switch (type) {
        case GOO_SYMBOL_FUNCTION: return "function";
        case GOO_SYMBOL_METHOD: return "method";
        case GOO_SYMBOL_STRUCT: return "struct";
        case GOO_SYMBOL_ENUM: return "enum";
        case GOO_SYMBOL_VARIABLE: return "variable";
        case GOO_SYMBOL_CONSTANT: return "constant";
        case GOO_SYMBOL_PARAMETER: return "parameter";
        case GOO_SYMBOL_TYPE_ALIAS: return "type_alias";
        case GOO_SYMBOL_TRAIT: return "trait";
        case GOO_SYMBOL_MODULE: return "module";
        case GOO_SYMBOL_IMPORT: return "import";
        default: return "unknown";
    }
}

/**
 * Get the string representation of a symbol visibility
 */
const char* goo_symbol_visibility_to_string(GooSymbolVisibility visibility) {
    switch (visibility) {
        case GOO_VISIBILITY_PUBLIC: return "public";
        case GOO_VISIBILITY_PRIVATE: return "private";
        case GOO_VISIBILITY_INTERNAL: return "internal";
        case GOO_VISIBILITY_LOCAL: return "local";
        default: return "unknown";
    }
}

// Internal helper functions

/**
 * Add a symbol to the symbol table
 */
static void add_symbol_to_table(GooSymbolTable* table, GooSymbol* symbol) {
    if (!table || !symbol) {
        return;
    }
    
    // Check if we need to resize
    if (table->symbol_count >= table->capacity) {
        unsigned int new_capacity = table->capacity * 2;
        GooSymbol** new_symbols = (GooSymbol**)realloc(
            table->symbols,
            sizeof(GooSymbol*) * new_capacity
        );
        if (!new_symbols) {
            return;
        }
        table->symbols = new_symbols;
        table->capacity = new_capacity;
    }
    
    // Add the symbol
    table->symbols[table->symbol_count++] = symbol;
}

/**
 * Read a file into a string
 */
static bool read_file_to_string(const char* filename, char** content) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return false;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    *content = (char*)malloc(size + 1);
    if (!*content) {
        fclose(file);
        return false;
    }
    
    // Read file content
    size_t read_size = fread(*content, 1, size, file);
    fclose(file);
    
    if (read_size != (size_t)size) {
        free(*content);
        return false;
    }
    
    // Null-terminate the string
    (*content)[size] = '\0';
    
    return true;
}

/**
 * Check if a character is valid in an identifier
 */
static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

/**
 * Parse source code tokens to extract symbols
 * This is a very basic implementation that would need to be replaced 
 * with actual parser integration
 */
static bool parse_tokens_for_symbols(GooSymbolTable* table, const char* filename, const char* source) {
    // This is a simplified implementation for demonstration purposes.
    // In a real compiler, this would use the actual parser to extract symbols.
    
    const char* keywords[] = {
        "fn", "struct", "enum", "impl", "const", "let", "type", "trait", "module", "import", NULL
    };
    
    GooSymbolType keyword_types[] = {
        GOO_SYMBOL_FUNCTION,
        GOO_SYMBOL_STRUCT,
        GOO_SYMBOL_ENUM,
        GOO_SYMBOL_METHOD, // impl
        GOO_SYMBOL_CONSTANT,
        GOO_SYMBOL_VARIABLE,
        GOO_SYMBOL_TYPE_ALIAS,
        GOO_SYMBOL_TRAIT,
        GOO_SYMBOL_MODULE,
        GOO_SYMBOL_IMPORT
    };
    
    // Current line and column
    unsigned int line = 1;
    unsigned int column = 1;
    
    // Stack of parent scopes
    GooSymbol* scope_stack[256];
    int scope_depth = 0;
    
    const char* p = source;
    while (*p) {
        // Skip whitespace
        if (isspace(*p)) {
            if (*p == '\n') {
                line++;
                column = 1;
            } else {
                column++;
            }
            p++;
            continue;
        }
        
        // Skip comments
        if (*p == '/' && *(p + 1) == '/') {
            // Single-line comment
            p += 2;
            column += 2;
            while (*p && *p != '\n') {
                p++;
                column++;
            }
            continue;
        }
        
        if (*p == '/' && *(p + 1) == '*') {
            // Multi-line comment
            p += 2;
            column += 2;
            while (*p && !(*p == '*' && *(p + 1) == '/')) {
                if (*p == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                p++;
            }
            if (*p) {
                p += 2; // Skip */
                column += 2;
            }
            continue;
        }
        
        // Track scope with braces
        if (*p == '{') {
            column++;
            p++;
            continue;
        }
        
        if (*p == '}') {
            if (scope_depth > 0) {
                scope_depth--;
            }
            column++;
            p++;
            continue;
        }
        
        // Check for keywords
        for (int i = 0; keywords[i]; i++) {
            size_t len = strlen(keywords[i]);
            if (strncmp(p, keywords[i], len) == 0 && 
                (p[len] == ' ' || p[len] == '\t' || p[len] == '\n')) {
                
                // Found a keyword, skip it
                p += len;
                column += len;
                
                // Skip spaces after keyword
                while (*p && isspace(*p) && *p != '\n') {
                    p++;
                    column++;
                }
                
                // Get the identifier name
                const char* name_start = p;
                unsigned int name_col = column;
                while (*p && is_identifier_char(*p)) {
                    p++;
                    column++;
                }
                
                if (p > name_start) {
                    // Create a symbol
                    char* name = (char*)malloc(p - name_start + 1);
                    if (name) {
                        strncpy(name, name_start, p - name_start);
                        name[p - name_start] = '\0';
                        
                        GooSymbol* symbol = goo_symbol_new(
                            name,
                            keyword_types[i],
                            GOO_VISIBILITY_PUBLIC, // Default visibility
                            filename,
                            line,
                            name_col,
                            p - name_start
                        );
                        
                        free(name);
                        
                        if (symbol) {
                            // Add to current scope if we have one
                            if (scope_depth > 0 && scope_stack[scope_depth - 1]) {
                                goo_symbol_add_child(scope_stack[scope_depth - 1], symbol);
                            }
                            
                            // Add to table
                            add_symbol_to_table(table, symbol);
                            
                            // If this is a scope-defining symbol, add to scope stack
                            if (keyword_types[i] == GOO_SYMBOL_FUNCTION ||
                                keyword_types[i] == GOO_SYMBOL_STRUCT ||
                                keyword_types[i] == GOO_SYMBOL_ENUM ||
                                keyword_types[i] == GOO_SYMBOL_TRAIT) {
                                
                                if (scope_depth < 256) {
                                    scope_stack[scope_depth++] = symbol;
                                }
                            }
                        }
                    }
                }
                
                break;
            }
        }
        
        // Default: just advance
        column++;
        p++;
    }
    
    return true;
} 