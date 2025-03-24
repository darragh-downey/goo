#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>
#include <llvm-c/Target.h>
#include "ast.h"
#include "parser.h"
#include "lexer.h"
#include "codegen.h"
#include "context.h"

// Parse a file and return the AST
static GooAst* parse_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", filename);
        return NULL;
    }
    
    // Initialize the lexer
    GooLexerState* lexer = goo_lexer_init(file);
    if (!lexer) {
        fprintf(stderr, "Failed to initialize lexer\n");
        fclose(file);
        return NULL;
    }
    
    // Parse the file
    GooAst* ast = goo_parse(lexer);
    
    // Clean up
    goo_lexer_free(lexer);
    fclose(file);
    
    return ast;
}

int main(int argc, char *argv[]) {
    // Initialize the context
    GooContext* goo_ctx = goo_context_init();
    if (!goo_ctx) {
        fprintf(stderr, "Failed to initialize Goo context\n");
        return 1;
    }
    
    // Parse command line arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file> [options]\n", argv[0]);
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "  -o <output_file>    Specify output file (default: derived from input file)\n");
        fprintf(stderr, "  -O<level>           Set optimization level (0-3, default: 2)\n");
        fprintf(stderr, "  -emit-llvm          Emit LLVM IR in addition to object code\n");
        fprintf(stderr, "  -S                  Emit assembly instead of object code\n");
        fprintf(stderr, "  -target <triple>    Specify target triple\n");
        fprintf(stderr, "  -cpu <cpu>          Specify target CPU\n");
        fprintf(stderr, "  -features <features> Specify target features\n");
        fprintf(stderr, "  -interpret          Run in interpretation mode\n");
        fprintf(stderr, "  -jit                Run in JIT mode\n");
        goo_context_free(goo_ctx);
        return 1;
    }
    
    // Set default values
    const char* input_file = argv[1];
    char* output_file = NULL;
    int opt_level = 2;
    bool emit_llvm = false;
    bool emit_assembly = false;
    
    // Configure the context with defaults
    goo_context_set_mode(goo_ctx, GOO_MODE_COMPILE);
    goo_context_set_input_file(goo_ctx, input_file);
    
    // Generate default output filename (input file with .o extension)
    size_t input_len = strlen(input_file);
    output_file = (char*)malloc(input_len + 3); // +3 for .o and null terminator
    if (!output_file) {
        fprintf(stderr, "Failed to allocate memory for output filename\n");
        goo_context_free(goo_ctx);
        return 1;
    }
    strcpy(output_file, input_file);
    char* dot = strrchr(output_file, '.');
    if (dot) *dot = '\0';
    strcat(output_file, ".o");
    
    // Parse other command line arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            free(output_file);
            output_file = strdup(argv[++i]);
        } else if (strncmp(argv[i], "-O", 2) == 0) {
            opt_level = atoi(argv[i] + 2);
            if (opt_level < 0 || opt_level > 3) {
                fprintf(stderr, "Invalid optimization level: %d (must be 0-3)\n", opt_level);
                free(output_file);
                goo_context_free(goo_ctx);
                return 1;
            }
        } else if (strcmp(argv[i], "-emit-llvm") == 0) {
            emit_llvm = true;
        } else if (strcmp(argv[i], "-S") == 0) {
            emit_assembly = true;
            // Update output filename to .s extension if using default
            if (output_file == strdup(input_file)) {
                char* dot = strrchr(output_file, '.');
                if (dot) *dot = '\0';
                strcat(output_file, ".s");
            }
        } else if (strcmp(argv[i], "-target") == 0 && i + 1 < argc) {
            goo_context_set_target_triple(goo_ctx, argv[++i]);
        } else if (strcmp(argv[i], "-cpu") == 0 && i + 1 < argc) {
            goo_context_set_target_cpu(goo_ctx, argv[++i]);
        } else if (strcmp(argv[i], "-features") == 0 && i + 1 < argc) {
            goo_context_set_target_features(goo_ctx, argv[++i]);
        } else if (strcmp(argv[i], "-interpret") == 0) {
            goo_context_set_mode(goo_ctx, GOO_MODE_INTERPRET);
        } else if (strcmp(argv[i], "-jit") == 0) {
            goo_context_set_mode(goo_ctx, GOO_MODE_JIT);
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            free(output_file);
            goo_context_free(goo_ctx);
            return 1;
        }
    }
    
    // Set the remaining context values
    goo_context_set_output_file(goo_ctx, output_file);
    goo_context_set_opt_level(goo_ctx, opt_level);
    goo_context_set_emit_llvm(goo_ctx, emit_llvm);
    
    // Extract module name from input file
    char* module_name = strdup(input_file);
    char* slash = strrchr(module_name, '/');
    if (slash) {
        memmove(module_name, slash + 1, strlen(slash + 1) + 1);
    }
    char* dot = strrchr(module_name, '.');
    if (dot) *dot = '\0';
    goo_context_set_module_name(goo_ctx, module_name);
    free(module_name);
    
    // Parse the input file to get the AST
    GooAst* ast = parse_file(goo_ctx->input_file);
    if (!ast) {
        fprintf(stderr, "Failed to parse input file: %s\n", goo_ctx->input_file);
        free(output_file);
        goo_context_free(goo_ctx);
        return 1;
    }
    
    // Initialize the code generator
    GooCodegenContext* codegen_ctx = goo_codegen_init(ast, goo_ctx, goo_ctx->module_name);
    if (!codegen_ctx) {
        fprintf(stderr, "Failed to initialize code generator\n");
        free_ast(ast);
        free(output_file);
        goo_context_free(goo_ctx);
        return 1;
    }
    
    // Generate optimized code
    if (!goo_codegen_generate_optimized(codegen_ctx)) {
        fprintf(stderr, "Failed to generate optimized code\n");
        goo_codegen_free(codegen_ctx);
        free_ast(ast);
        free(output_file);
        goo_context_free(goo_ctx);
        return 1;
    }
    
    // Emit LLVM IR if requested
    if (emit_llvm) {
        if (!goo_codegen_emit_llvm(codegen_ctx)) {
            fprintf(stderr, "Failed to emit LLVM IR\n");
            // Continue anyway as this is not fatal
        }
    }
    
    // Emit assembly file if requested
    if (emit_assembly) {
        if (!goo_codegen_generate_assembly_file(codegen_ctx, output_file)) {
            fprintf(stderr, "Failed to generate assembly file: %s\n", output_file);
            goo_codegen_free(codegen_ctx);
            free_ast(ast);
            free(output_file);
            goo_context_free(goo_ctx);
            return 1;
        }
    } else {
        // Emit code based on context settings (object file by default)
        if (!goo_codegen_emit(codegen_ctx)) {
            fprintf(stderr, "Failed to emit code\n");
            goo_codegen_free(codegen_ctx);
            free_ast(ast);
            free(output_file);
            goo_context_free(goo_ctx);
            return 1;
        }
    }
    
    // Clean up
    goo_codegen_free(codegen_ctx);
    free_ast(ast);
    free(output_file);
    goo_context_free(goo_ctx);
    
    printf("Compilation successful: %s -> %s\n", input_file, goo_ctx->output_file);
    return 0;
} 