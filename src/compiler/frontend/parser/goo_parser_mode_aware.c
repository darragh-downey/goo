#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/goo_parser_mode_aware.h"
#include "../include/goo_file_detector.h"
#include "../include/goo_parser.h"

// Extended parser structure with mode information
typedef struct GooModeAwareParser {
    // Base parser data (must be the first field for compatibility with the GooParserHandle cast)
    void* base_parser_data;
    
    // Mode-specific fields
    GooLangMode detected_mode;
    GooLangMode forced_mode;
    bool use_forced_mode;
    GooLangMode default_mode;
    const char* last_error;
} GooModeAwareParser;

// Create a mode-aware parser
GooParserHandle gooModeAwareParserCreate(void) {
    GooModeAwareParser* parser = (GooModeAwareParser*)malloc(sizeof(GooModeAwareParser));
    if (parser == NULL) {
        return NULL;
    }
    
    // Initialize with default values
    parser->base_parser_data = gooParserCreate();
    parser->detected_mode = GOO_LANG_MODE_GOO;
    parser->forced_mode = GOO_LANG_MODE_GOO;
    parser->use_forced_mode = false;
    parser->default_mode = GOO_LANG_MODE_GOO;
    parser->last_error = NULL;
    
    return (GooParserHandle)parser;
}

// Parse source code with automatic language mode detection
GooParserResultCode gooModeAwareParseString(GooParserHandle parser_handle, const char* filename, const char* source) {
    GooModeAwareParser* parser = (GooModeAwareParser*)parser_handle;
    
    // Determine the language mode to use
    GooLangMode mode;
    if (parser->use_forced_mode) {
        mode = parser->forced_mode;
    } else {
        size_t source_len = strlen(source);
        mode = goo_detect_file_mode(filename, source, source_len);
        parser->detected_mode = mode;
    }
    
    // Store the error message before we start parsing
    if (parser->last_error != NULL) {
        free((void*)parser->last_error);
        parser->last_error = NULL;
    }
    
    // Parse according to the determined mode
    GooParserResultCode result;
    
    // Use the base parser with knowledge of the language mode
    // In a real implementation, you would modify the parser behavior based on the mode
    result = gooParserParseString(parser->base_parser_data, source);
    
    // Store the error message if parsing failed
    if (result != GOO_PARSER_SUCCESS) {
        const char* error = gooParserGetError(parser->base_parser_data);
        if (error != NULL) {
            parser->last_error = strdup(error);
        }
    }
    
    // If successfully parsed in Go mode, validate that no Goo extensions are used
    if (result == GOO_PARSER_SUCCESS && mode == GOO_LANG_MODE_GO) {
        GooAstNodeHandle root = gooParserGetAstRoot(parser->base_parser_data);
        if (gooAstNodeUsesExtensions(root)) {
            // Set an error that Go files can't use Goo extensions
            if (parser->last_error != NULL) {
                free((void*)parser->last_error);
            }
            parser->last_error = strdup("Go files cannot use Goo language extensions");
            return GOO_PARSER_ERROR_SYNTAX;
        }
    }
    
    return result;
}

// Parse file with automatic language mode detection
GooParserResultCode gooModeAwareParseFile(GooParserHandle parser_handle, const char* filename) {
    GooModeAwareParser* parser = (GooModeAwareParser*)parser_handle;
    
    // Open the file and read its contents
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        if (parser->last_error != NULL) {
            free((void*)parser->last_error);
        }
        parser->last_error = strdup("Could not open file");
        return GOO_PARSER_ERROR_GENERAL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer for file contents
    char* buffer = (char*)malloc(file_size + 1);
    if (buffer == NULL) {
        fclose(file);
        if (parser->last_error != NULL) {
            free((void*)parser->last_error);
        }
        parser->last_error = strdup("Memory allocation failed");
        return GOO_PARSER_ERROR_OUT_OF_MEMORY;
    }
    
    // Read file contents
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';
    fclose(file);
    
    // Parse the file contents
    GooParserResultCode result = gooModeAwareParseString(parser_handle, filename, buffer);
    
    // Free the buffer
    free(buffer);
    
    return result;
}

// Get the language mode that was detected during parsing
GooLangMode gooParserGetDetectedMode(GooParserHandle parser_handle) {
    GooModeAwareParser* parser = (GooModeAwareParser*)parser_handle;
    return parser->detected_mode;
}

// Set the default language mode for the parser
void gooParserSetDefaultMode(GooParserHandle parser_handle, GooLangMode mode) {
    GooModeAwareParser* parser = (GooModeAwareParser*)parser_handle;
    parser->default_mode = mode;
    goo_set_default_lang_mode(mode); // Update the detector's default mode as well
}

// Force a specific language mode for parsing
void gooParserForceMode(GooParserHandle parser_handle, GooLangMode mode) {
    GooModeAwareParser* parser = (GooModeAwareParser*)parser_handle;
    parser->forced_mode = mode;
    parser->use_forced_mode = true;
}

// Check if an AST node uses Goo language extensions
bool gooAstNodeUsesExtensions(GooAstNodeHandle node) {
    if (node == NULL) {
        return false;
    }
    
    // Get the node type
    GooAstNodeType type = gooAstGetNodeType(node);
    
    // Check if the node type is a Goo extension
    switch (type) {
        // Goo extension nodes
        case GOO_AST_ENUM_DECL:
        case GOO_AST_ENUM_MEMBER:
        case GOO_AST_EXTEND_DECL:
        case GOO_AST_TRAIT_DECL:
        case GOO_AST_MATCH_STMT:
        case GOO_AST_MATCH_CASE:
        case GOO_AST_PATTERN_EXPR:
        case GOO_AST_NULL_COALESCE_EXPR:
        case GOO_AST_OPTIONAL_ACCESS_EXPR:
        case GOO_AST_SAFE_CALL_EXPR:
        case GOO_AST_NULLABLE_TYPE:
            return true;
        
        // For other nodes, check their children
        case GOO_AST_PROGRAM: {
            // Check the program's declarations
            GooAstProgramHandle program = (GooAstProgramHandle)node;
            size_t decl_count = gooAstProgramGetDeclarationCount(program);
            
            for (size_t i = 0; i < decl_count; i++) {
                GooAstNodeHandle decl = gooAstProgramGetDeclaration(program, i);
                if (gooAstNodeUsesExtensions(decl)) {
                    return true;
                }
            }
            
            // Check imports too
            size_t import_count = gooAstProgramGetImportCount(program);
            for (size_t i = 0; i < import_count; i++) {
                GooAstNodeHandle import = gooAstProgramGetImport(program, i);
                if (gooAstNodeUsesExtensions(import)) {
                    return true;
                }
            }
            
            return false;
        }
        
        // For other complex nodes, we'd need to check their children too
        // This is a simplified implementation
        default:
            return false;
    }
} 