#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/compiler/frontend/parser/zig/include/goo_parser.h"

// Helper function to get string representation of AST node type
const char* get_node_type_str(GooAstNodeType type) {
    switch (type) {
        case GOO_AST_PROGRAM: return "Program";
        case GOO_AST_PACKAGE_DECL: return "Package Declaration";
        case GOO_AST_IMPORT_DECL: return "Import Declaration";
        case GOO_AST_FUNCTION_DECL: return "Function Declaration";
        case GOO_AST_PARAMETER: return "Parameter";
        case GOO_AST_VAR_DECL: return "Variable Declaration";
        case GOO_AST_CONST_DECL: return "Constant Declaration";
        case GOO_AST_TYPE_DECL: return "Type Declaration";
        case GOO_AST_TYPE_EXPR: return "Type Expression";
        case GOO_AST_BLOCK: return "Block";
        case GOO_AST_IF_STMT: return "If Statement";
        case GOO_AST_FOR_STMT: return "For Statement";
        case GOO_AST_RETURN_STMT: return "Return Statement";
        case GOO_AST_EXPR_STMT: return "Expression Statement";
        case GOO_AST_CALL_EXPR: return "Call Expression";
        case GOO_AST_IDENTIFIER: return "Identifier";
        case GOO_AST_INT_LITERAL: return "Integer Literal";
        case GOO_AST_FLOAT_LITERAL: return "Float Literal";
        case GOO_AST_STRING_LITERAL: return "String Literal";
        case GOO_AST_BOOL_LITERAL: return "Boolean Literal";
        case GOO_AST_PREFIX_EXPR: return "Prefix Expression";
        case GOO_AST_INFIX_EXPR: return "Infix Expression";
        default: return "Unknown Node Type";
    }
}

// Helper function to get string representation of error code
const char* get_error_str(GooParserErrorCode code) {
    switch (code) {
        case GOO_PARSER_SUCCESS: return "Success";
        case GOO_PARSER_UNEXPECTED_TOKEN: return "Unexpected Token";
        case GOO_PARSER_MISSING_TOKEN: return "Missing Token";
        case GOO_PARSER_INVALID_SYNTAX: return "Invalid Syntax";
        case GOO_PARSER_OUT_OF_MEMORY: return "Out of Memory";
        case GOO_PARSER_NOT_IMPLEMENTED: return "Not Implemented";
        case GOO_PARSER_UNKNOWN_ERROR: return "Unknown Error";
        default: return "Undefined Error";
    }
}

int main(int argc, char* argv[]) {
    // Default source code if no file is provided
    const char* source_code = 
        "package example;\n"
        "import \"std/io\";\n"
        "\n"
        "func fibonacci(n: int): int {\n"
        "    if n <= 1 {\n"
        "        return n;\n"
        "    }\n"
        "    return fibonacci(n - 1) + fibonacci(n - 2);\n"
        "}\n"
        "\n"
        "func main() {\n"
        "    for i := 0; i < 10; i = i + 1 {\n"
        "        io.println(fibonacci(i));\n"
        "    }\n"
        "}\n";

    // If a file is provided, read from it instead
    if (argc > 1) {
        FILE* file = fopen(argv[1], "r");
        if (!file) {
            fprintf(stderr, "Error: Could not open file '%s'\n", argv[1]);
            return 1;
        }

        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Allocate buffer for file contents
        char* buffer = (char*)malloc(file_size + 1);
        if (!buffer) {
            fprintf(stderr, "Error: Out of memory\n");
            fclose(file);
            return 1;
        }

        // Read file contents
        size_t bytes_read = fread(buffer, 1, file_size, file);
        buffer[bytes_read] = '\0';
        fclose(file);

        // Use file contents as source
        source_code = buffer;
    }

    printf("=== Goo Parser C API Example ===\n\n");
    
    // Initialize the parser
    printf("Initializing parser...\n");
    GooParserHandle parser = goo_parser_init(source_code);
    if (!parser) {
        fprintf(stderr, "Error: Failed to initialize parser\n");
        return 1;
    }

    // Parse the program
    printf("Parsing program...\n");
    GooParserErrorCode result = goo_parser_parse_program(parser);
    
    if (result != GOO_PARSER_SUCCESS) {
        fprintf(stderr, "Parser error: %s\n", get_error_str(result));
        const char* error_msg = goo_parser_get_error(parser);
        if (error_msg) {
            fprintf(stderr, "Error details: %s\n", error_msg);
        }
        goo_parser_destroy(parser);
        if (argc > 1) {
            free((void*)source_code);
        }
        return 1;
    }

    printf("Parsing completed successfully.\n\n");

    // Get the AST root
    GooAstNodeHandle root = goo_parser_get_ast_root(parser);
    if (root) {
        GooAstNodeType root_type = goo_ast_get_node_type(root);
        printf("AST root type: %s\n", get_node_type_str(root_type));
        
        // Note: Further AST traversal would be implemented here
        // For now, just print a note that it's not implemented yet
        printf("AST traversal not yet implemented.\n");
    } else {
        printf("No AST root node available.\n");
    }

    // Clean up
    printf("\nCleaning up...\n");
    goo_parser_destroy(parser);
    
    // Free the buffer if we read from a file
    if (argc > 1) {
        free((void*)source_code);
    }

    printf("Done.\n");
    return 0;
} 