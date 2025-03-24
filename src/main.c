#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goo.h"
#include "ast.h"
#include "codegen.h"
#include "goo_runtime.h"
#include "goo_distributed.h"

// Compiler context
struct GooContext {
    int verbose;                  // Verbose output flag
    char* output_file;            // Output file name
    bool optimize;                // Optimization flag
    GooCompilationMode mode;      // Compilation mode
    
    // Runtime options
    bool enable_distributed;      // Enable distributed channels
    int thread_pool_size;         // Thread pool size for goroutines
    GooSupervisionPolicy supervision_policy;  // Default supervision policy
    char* runtime_lib_path;       // Path to the runtime library
    bool debug_mode;              // Generate debug information
};

// Print usage information
static void print_usage(const char* program_name) {
    printf("Goo Programming Language Compiler v%d.%d.%d\n", 
           GOO_VERSION_MAJOR, GOO_VERSION_MINOR, GOO_VERSION_PATCH);
    printf("Usage: %s [options] command [files...]\n", program_name);
    printf("\nCommands:\n");
    printf("  run <file>        Compile and run the specified file\n");
    printf("  build <file>      Compile the specified file using compile-time build options\n");
    printf("  test <file>       Run tests in the specified file\n");
    printf("  interpret <file>  Interpret the specified file without compilation\n");
    printf("  jit <file>        JIT compile and run the specified file\n");
    printf("\nOptions:\n");
    printf("  -o <file>         Specify output file\n");
    printf("  -v                Enable verbose output\n");
    printf("  -O                Enable optimization\n");
    printf("  -d                Enable distributed channels\n");
    printf("  -t <num>          Set thread pool size (default: 4)\n");
    printf("  -s <policy>       Set supervision policy (one-for-one, one-for-all, rest-for-one)\n");
    printf("  -r <path>         Specify runtime library path\n");
    printf("  -g                Generate debug information\n");
}

// Initialize the compiler context
GooContext* goo_init(void) {
    GooContext* ctx = malloc(sizeof(GooContext));
    if (!ctx) {
        fprintf(stderr, "Error: Failed to allocate memory for compiler context\n");
        return NULL;
    }
    
    ctx->verbose = 0;
    ctx->output_file = NULL;
    ctx->optimize = false;
    ctx->mode = GOO_MODE_COMPILE;
    
    // Initialize runtime options with defaults
    ctx->enable_distributed = false;
    ctx->thread_pool_size = 4;  // Default to 4 threads
    ctx->supervision_policy = GOO_SUPERVISE_ONE_FOR_ONE;  // Default policy
    ctx->runtime_lib_path = strdup("./lib");  // Default runtime path
    ctx->debug_mode = false;
    
    return ctx;
}

// Parse command-line arguments
static int parse_args(GooContext* ctx, int argc, char** argv) {
    int i = 1;
    
    // Parse options
    while (i < argc && argv[i][0] == '-') {
        if (strcmp(argv[i], "-v") == 0) {
            ctx->verbose = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing output file after -o\n");
                return -1;
            }
            ctx->output_file = strdup(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-O") == 0) {
            ctx->optimize = true;
        } else if (strcmp(argv[i], "-d") == 0) {
            ctx->enable_distributed = true;
        } else if (strcmp(argv[i], "-t") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing thread count after -t\n");
                return -1;
            }
            ctx->thread_pool_size = atoi(argv[i + 1]);
            if (ctx->thread_pool_size <= 0) {
                fprintf(stderr, "Error: Invalid thread count: %s\n", argv[i + 1]);
                return -1;
            }
            i++;
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing supervision policy after -s\n");
                return -1;
            }
            if (strcmp(argv[i + 1], "one-for-one") == 0) {
                ctx->supervision_policy = GOO_SUPERVISE_ONE_FOR_ONE;
            } else if (strcmp(argv[i + 1], "one-for-all") == 0) {
                ctx->supervision_policy = GOO_SUPERVISE_ONE_FOR_ALL;
            } else if (strcmp(argv[i + 1], "rest-for-one") == 0) {
                ctx->supervision_policy = GOO_SUPERVISE_REST_FOR_ONE;
            } else {
                fprintf(stderr, "Error: Unknown supervision policy: %s\n", argv[i + 1]);
                return -1;
            }
            i++;
        } else if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: Missing runtime path after -r\n");
                return -1;
            }
            free(ctx->runtime_lib_path);
            ctx->runtime_lib_path = strdup(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-g") == 0) {
            ctx->debug_mode = true;
        } else {
            fprintf(stderr, "Error: Unknown option: %s\n", argv[i]);
            return -1;
        }
        i++;
    }
    
    // Check if command is provided
    if (i >= argc) {
        fprintf(stderr, "Error: No command specified\n");
        return -1;
    }
    
    // Parse command
    if (strcmp(argv[i], "run") == 0) {
        ctx->mode = GOO_MODE_RUN;
    } else if (strcmp(argv[i], "build") == 0) {
        ctx->mode = GOO_MODE_BUILD;
    } else if (strcmp(argv[i], "test") == 0) {
        ctx->mode = GOO_MODE_TEST;
    } else if (strcmp(argv[i], "interpret") == 0) {
        ctx->mode = GOO_MODE_INTERPRET;
    } else if (strcmp(argv[i], "jit") == 0) {
        ctx->mode = GOO_MODE_JIT;
    } else {
        fprintf(stderr, "Error: Unknown command: %s\n", argv[i]);
        return -1;
    }
    i++;
    
    // Check if file is provided
    if (i >= argc) {
        fprintf(stderr, "Error: No input file specified\n");
        return -1;
    }
    
    return i; // Return index of the first file argument
}

// Clean up the compiler context
void goo_cleanup(GooContext* ctx) {
    if (!ctx) return;
    
    free(ctx->output_file);
    free(ctx->runtime_lib_path);
    free(ctx);
}

// Configure the code generation context with runtime options
static void configure_codegen_runtime(GooCodegenContext* codegen_ctx, GooContext* ctx) {
    if (!codegen_ctx || !ctx) return;
    
    // Set runtime options
    codegen_ctx->enable_distributed = ctx->enable_distributed;
    codegen_ctx->thread_pool_size = ctx->thread_pool_size;
    codegen_ctx->supervision_policy = ctx->supervision_policy;
    codegen_ctx->debug_mode = ctx->debug_mode;
    
    if (ctx->runtime_lib_path) {
        codegen_ctx->runtime_lib_path = strdup(ctx->runtime_lib_path);
    }
}

// Main function
int main(int argc, char** argv) {
    int result = 0;
    
    // Check if arguments are provided
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Initialize compiler context
    GooContext* ctx = goo_init();
    if (!ctx) {
        return 1;
    }
    
    // Parse command-line arguments
    int file_index = parse_args(ctx, argc, argv);
    if (file_index < 0) {
        print_usage(argv[0]);
        goo_cleanup(ctx);
        return 1;
    }
    
    // Process each input file
    for (int i = file_index; i < argc; i++) {
        // Parse the file
        if (ctx->verbose) {
            printf("Parsing file: %s\n", argv[i]);
        }
        
        GooAst* ast = goo_parse_file(argv[i]);
        if (!ast) {
            fprintf(stderr, "Error: Failed to parse file: %s\n", argv[i]);
            result = 1;
            continue;
        }
        
        // Generate code
        if (ctx->verbose) {
            printf("Generating code for: %s\n", argv[i]);
        }
        
        GooCodegenContext* codegen_ctx = goo_codegen_init(ctx->mode, ctx->optimize);
        if (!codegen_ctx) {
            fprintf(stderr, "Error: Failed to initialize code generation\n");
            goo_ast_free(ast);
            result = 1;
            continue;
        }
        
        // Configure runtime options
        configure_codegen_runtime(codegen_ctx, ctx);
        
        // Generate code from the AST
        if (!goo_codegen_generate(codegen_ctx, ast->root)) {
            fprintf(stderr, "Error: Failed to generate code for: %s\n", argv[i]);
            goo_codegen_free(codegen_ctx);
            goo_ast_free(ast);
            result = 1;
            continue;
        }
        
        // Output file handling
        const char* output_file = ctx->output_file;
        if (!output_file) {
            // Default output file name based on input file and mode
            char* base_name = strdup(argv[i]);
            char* dot = strrchr(base_name, '.');
            if (dot) *dot = '\0';
            
            char output_buffer[256];
            if (ctx->mode == GOO_MODE_COMPILE || ctx->mode == GOO_MODE_BUILD) {
                snprintf(output_buffer, sizeof(output_buffer), "%s.o", base_name);
            } else {
                snprintf(output_buffer, sizeof(output_buffer), "%s", base_name);
            }
            output_file = output_buffer;
            free(base_name);
        }
        
        // Emit code based on compilation mode
        if (ctx->mode == GOO_MODE_COMPILE || ctx->mode == GOO_MODE_BUILD) {
            if (ctx->verbose) {
                printf("Emitting object file: %s\n", output_file);
            }
            
            if (!goo_codegen_emit(codegen_ctx, output_file)) {
                fprintf(stderr, "Error: Failed to emit code to: %s\n", output_file);
                result = 1;
            }
        } else if (ctx->mode == GOO_MODE_RUN) {
            if (ctx->verbose) {
                printf("Executing: %s\n", argv[i]);
            }
            
            if (!goo_codegen_run_jit(codegen_ctx)) {
                fprintf(stderr, "Error: Execution failed\n");
                result = 1;
            }
        } else if (ctx->mode == GOO_MODE_JIT) {
            if (ctx->verbose) {
                printf("JIT compiling and executing: %s\n", argv[i]);
            }
            
            if (!goo_codegen_run_jit(codegen_ctx)) {
                fprintf(stderr, "Error: JIT execution failed\n");
                result = 1;
            }
        } else if (ctx->mode == GOO_MODE_INTERPRET) {
            if (ctx->verbose) {
                printf("Interpreting: %s\n", argv[i]);
            }
            
            if (!goo_codegen_interpret(codegen_ctx)) {
                fprintf(stderr, "Error: Interpretation failed\n");
                result = 1;
            }
        } else if (ctx->mode == GOO_MODE_TEST) {
            if (ctx->verbose) {
                printf("Running tests in: %s\n", argv[i]);
            }
            
            // Run tests using JIT compilation
            if (!goo_codegen_run_jit(codegen_ctx)) {
                fprintf(stderr, "Error: Test execution failed\n");
                result = 1;
            }
        }
        
        // Clean up
        goo_codegen_free(codegen_ctx);
        goo_ast_free(ast);
    }
    
    // Clean up compiler context
    goo_cleanup(ctx);
    
    return result;
} 