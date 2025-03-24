/**
 * @file error_catalog.c
 * @brief Implementation of the error catalog system
 */

#include "error_catalog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global error catalog
static GooErrorCatalog* global_catalog = NULL;

// Initial capacity for the catalog
#define INITIAL_CATALOG_CAPACITY 100

// Private helper functions
static char* duplicate_string(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, str, len + 1);
    }
    return result;
}

static void ensure_capacity(GooErrorCatalog* catalog) {
    if (catalog->count >= catalog->capacity) {
        size_t new_capacity = catalog->capacity * 2;
        GooErrorCatalogEntry* new_entries = (GooErrorCatalogEntry*)realloc(
            catalog->entries, 
            new_capacity * sizeof(GooErrorCatalogEntry)
        );
        
        if (new_entries) {
            catalog->entries = new_entries;
            catalog->capacity = new_capacity;
        }
    }
}

// Public API implementation

bool goo_error_catalog_init(void) {
    // Already initialized?
    if (global_catalog) {
        return true;
    }
    
    // Allocate catalog
    global_catalog = (GooErrorCatalog*)malloc(sizeof(GooErrorCatalog));
    if (!global_catalog) {
        return false;
    }
    
    // Initialize entries array
    global_catalog->capacity = INITIAL_CATALOG_CAPACITY;
    global_catalog->entries = (GooErrorCatalogEntry*)malloc(
        global_catalog->capacity * sizeof(GooErrorCatalogEntry)
    );
    
    if (!global_catalog->entries) {
        free(global_catalog);
        global_catalog = NULL;
        return false;
    }
    
    global_catalog->count = 0;
    
    // Register some common errors
    
    // Syntax errors
    goo_error_catalog_register(
        "E0001",
        GOO_ERROR_CAT_SYNTAX,
        "unexpected token",
        "This error occurs when the parser encounters a token that doesn't match\n"
        "the expected syntax at the current position.\n\n"
        "Check for missing punctuation, mismatched brackets or parentheses,\n"
        "or other syntax errors in the code.",
        "fn main() {\n"
        "    let x = 5\n"    // Missing semicolon
        "    println(x);\n"
        "}",
        "fn main() {\n"
        "    let x = 5;\n"   // Added semicolon
        "    println(x);\n"
        "}"
    );
    
    goo_error_catalog_register(
        "E0002",
        GOO_ERROR_CAT_SYNTAX,
        "unterminated string literal",
        "This error occurs when a string literal is not properly terminated with\n"
        "a closing quote character.\n\n"
        "Make sure all string literals have matching opening and closing quotes.",
        "fn main() {\n"
        "    let message = \"Hello, world;\n"  // Missing closing quote
        "    println(message);\n"
        "}",
        "fn main() {\n"
        "    let message = \"Hello, world\";\n"  // Added closing quote
        "    println(message);\n"
        "}"
    );
    
    // Type errors
    goo_error_catalog_register(
        "E0101",
        GOO_ERROR_CAT_TYPE,
        "mismatched types",
        "This error occurs when a value of one type is used where a value of a\n"
        "different type is expected.\n\n"
        "To fix this error, make sure the types match, or add an explicit conversion\n"
        "if the language allows it.",
        "fn main() {\n"
        "    let x: int = \"hello\";\n"  // String assigned to int
        "}",
        "fn main() {\n"
        "    let x: int = 42;\n"        // Correct type
        "    // Or with conversion:\n"
        "    let y: int = to_int(\"42\");\n"
        "}"
    );
    
    goo_error_catalog_register(
        "E0102",
        GOO_ERROR_CAT_TYPE,
        "undefined variable",
        "This error occurs when trying to use a variable that hasn't been declared\n"
        "or is out of scope at the current position in the code.\n\n"
        "Check for typos in variable names, or make sure the variable is declared\n"
        "before it's used.",
        "fn main() {\n"
        "    println(x);\n"  // x is not defined
        "}",
        "fn main() {\n"
        "    let x = 10;\n"  // Declare x before using it
        "    println(x);\n"
        "}"
    );
    
    // Borrow checker errors
    goo_error_catalog_register(
        "E0201",
        GOO_ERROR_CAT_BORROW,
        "cannot borrow as mutable because it is also borrowed as immutable",
        "This error occurs when trying to borrow a value as mutable while it's already\n"
        "borrowed as immutable.\n\n"
        "In Goo, you can have either one mutable reference or any number of immutable\n"
        "references to a value, but not both at the same time.",
        "fn main() {\n"
        "    let mut v = [1, 2, 3];\n"
        "    let r1 = &v;\n"        // Immutable borrow
        "    let r2 = &mut v;\n"    // Error: mutable borrow while immutable borrow exists
        "    println(\"{}, {}\", r1[0], r2[0]);\n"
        "}",
        "fn main() {\n"
        "    let mut v = [1, 2, 3];\n"
        "    {\n"
        "        let r1 = &v;\n"    // Immutable borrow in inner scope
        "        println(\"{}\", r1[0]);\n"
        "    }  // Immutable borrow ends here\n"
        "    let r2 = &mut v;\n"    // Now we can borrow as mutable
        "    println(\"{}\", r2[0]);\n"
        "}"
    );
    
    // Success!
    return true;
}

void goo_error_catalog_cleanup(void) {
    if (!global_catalog) {
        return;
    }
    
    // Free all entry strings
    for (size_t i = 0; i < global_catalog->count; i++) {
        GooErrorCatalogEntry* entry = &global_catalog->entries[i];
        
        free((void*)entry->code);
        free((void*)entry->short_desc);
        free((void*)entry->explanation);
        free((void*)entry->example);
        free((void*)entry->solution);
    }
    
    // Free the entries array and catalog
    free(global_catalog->entries);
    free(global_catalog);
    global_catalog = NULL;
}

const GooErrorCatalogEntry* goo_error_catalog_lookup(const char* code) {
    if (!global_catalog || !code) {
        return NULL;
    }
    
    // Linear search for now (could use hash table for larger catalogs)
    for (size_t i = 0; i < global_catalog->count; i++) {
        if (strcmp(global_catalog->entries[i].code, code) == 0) {
            return &global_catalog->entries[i];
        }
    }
    
    return NULL;
}

bool goo_error_catalog_explain(const char* code) {
    const GooErrorCatalogEntry* entry = goo_error_catalog_lookup(code);
    if (!entry) {
        printf("Error code %s not found in the catalog.\n", code);
        return false;
    }
    
    // Print the error explanation
    printf("Error[%s]: %s\n", entry->code, entry->short_desc);
    printf("Category: %s\n\n", goo_error_category_to_string(entry->category));
    
    printf("Explanation:\n%s\n\n", entry->explanation);
    
    printf("Example of incorrect code:\n");
    printf("```\n%s\n```\n\n", entry->example);
    
    printf("Example of corrected code:\n");
    printf("```\n%s\n```\n", entry->solution);
    
    return true;
}

bool goo_error_catalog_register(
    const char* code,
    GooErrorCategory category,
    const char* short_desc,
    const char* explanation,
    const char* example,
    const char* solution
) {
    if (!global_catalog || !code || !short_desc) {
        return false;
    }
    
    // Check if code already exists
    for (size_t i = 0; i < global_catalog->count; i++) {
        if (strcmp(global_catalog->entries[i].code, code) == 0) {
            // Update existing entry
            GooErrorCatalogEntry* entry = &global_catalog->entries[i];
            
            free((void*)entry->short_desc);
            free((void*)entry->explanation);
            free((void*)entry->example);
            free((void*)entry->solution);
            
            entry->category = category;
            entry->short_desc = duplicate_string(short_desc);
            entry->explanation = duplicate_string(explanation);
            entry->example = duplicate_string(example);
            entry->solution = duplicate_string(solution);
            
            return (entry->short_desc != NULL);
        }
    }
    
    // Ensure capacity for new entry
    ensure_capacity(global_catalog);
    
    // Add new entry
    GooErrorCatalogEntry* entry = &global_catalog->entries[global_catalog->count];
    
    entry->code = duplicate_string(code);
    entry->category = category;
    entry->short_desc = duplicate_string(short_desc);
    entry->explanation = duplicate_string(explanation);
    entry->example = duplicate_string(example);
    entry->solution = duplicate_string(solution);
    
    // Check if all strings were duplicated successfully
    if (entry->code && entry->short_desc) {
        global_catalog->count++;
        return true;
    }
    
    // Clean up on failure
    free((void*)entry->code);
    free((void*)entry->short_desc);
    free((void*)entry->explanation);
    free((void*)entry->example);
    free((void*)entry->solution);
    
    return false;
}

size_t goo_error_catalog_count(void) {
    if (!global_catalog) {
        return 0;
    }
    
    return global_catalog->count;
}

size_t goo_error_catalog_get_by_category(
    GooErrorCategory category,
    const char** codes,
    size_t capacity
) {
    if (!global_catalog || !codes || capacity == 0) {
        return 0;
    }
    
    size_t count = 0;
    
    for (size_t i = 0; i < global_catalog->count && count < capacity; i++) {
        if (global_catalog->entries[i].category == category) {
            codes[count++] = global_catalog->entries[i].code;
        }
    }
    
    return count;
}

const char* goo_error_category_to_string(GooErrorCategory category) {
    switch (category) {
        case GOO_ERROR_CAT_SYNTAX:
            return "Syntax";
        case GOO_ERROR_CAT_TYPE:
            return "Type System";
        case GOO_ERROR_CAT_BORROW:
            return "Borrow Checker";
        case GOO_ERROR_CAT_LIFETIME:
            return "Lifetime";
        case GOO_ERROR_CAT_COMPILER:
            return "Compiler";
        case GOO_ERROR_CAT_LINKER:
            return "Linker";
        case GOO_ERROR_CAT_MACRO:
            return "Macro";
        case GOO_ERROR_CAT_ATTRIBUTE:
            return "Attribute";
        case GOO_ERROR_CAT_IO:
            return "I/O";
        case GOO_ERROR_CAT_MISC:
            return "Miscellaneous";
        default:
            return "Unknown";
    }
} 