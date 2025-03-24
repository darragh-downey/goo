#ifndef GOO_CONTEXT_H
#define GOO_CONTEXT_H

#include <stdbool.h>

// Compilation mode
typedef enum {
    GOO_MODE_COMPILE, // Compile to object file or executable
    GOO_MODE_JIT,     // Just-in-time compilation and execution
    GOO_MODE_INTERPRET // Interpret the AST directly (not implemented)
} GooCompilationMode;

// Context for the Goo language
typedef struct {
    GooCompilationMode mode;    // Compilation mode
    bool optimize;              // Whether to optimize the code
    const char* module_name;    // Name of the current module
    const char* input_file;     // Name of the input file
    const char* output_file;    // Name of the output file
    const char* target_triple;  // Target triple for LLVM
    const char* cpu;            // Target CPU for LLVM
    const char* features;       // Target features for LLVM
    int opt_level;              // Optimization level
    bool debug;                 // Whether to generate debug info
    bool emit_llvm;             // Whether to emit LLVM IR
} GooContext;

// Initialize a new context with default settings
GooContext* goo_context_init();

// Free a context
void goo_context_free(GooContext* context);

// Set the compilation mode
void goo_context_set_mode(GooContext* context, GooCompilationMode mode);

// Set the optimization level
void goo_context_set_opt_level(GooContext* context, int level);

// Set the target triple
void goo_context_set_target_triple(GooContext* context, const char* triple);

// Set the target CPU
void goo_context_set_target_cpu(GooContext* context, const char* cpu);

// Set the target features
void goo_context_set_target_features(GooContext* context, const char* features);

// Set the input file
void goo_context_set_input_file(GooContext* context, const char* file);

// Set the output file
void goo_context_set_output_file(GooContext* context, const char* file);

// Set the module name
void goo_context_set_module_name(GooContext* context, const char* name);

// Set whether to emit LLVM IR
void goo_context_set_emit_llvm(GooContext* context, bool emit);

// Set whether to generate debug info
void goo_context_set_debug(GooContext* context, bool debug);

#endif // GOO_CONTEXT_H 