#include <stdlib.h>
#include <string.h>
#include "context.h"

// Initialize a new context with default settings
GooContext* goo_context_init() {
    GooContext* context = (GooContext*)malloc(sizeof(GooContext));
    if (!context) return NULL;
    
    // Set default values
    context->mode = GOO_MODE_COMPILE;
    context->optimize = true;
    context->module_name = "goo_module";
    context->input_file = NULL;
    context->output_file = NULL;
    context->target_triple = NULL;
    context->cpu = NULL;
    context->features = NULL;
    context->opt_level = 2;
    context->debug = false;
    context->emit_llvm = false;
    
    return context;
}

// Free a context
void goo_context_free(GooContext* context) {
    if (!context) return;
    
    // Free the strings that we own
    // Note: Some strings are owned by the caller and should not be freed
    
    free(context);
}

// Set the compilation mode
void goo_context_set_mode(GooContext* context, GooCompilationMode mode) {
    if (!context) return;
    
    context->mode = mode;
}

// Set the optimization level
void goo_context_set_opt_level(GooContext* context, int level) {
    if (!context) return;
    
    context->opt_level = level;
    context->optimize = (level > 0);
}

// Set the target triple
void goo_context_set_target_triple(GooContext* context, const char* triple) {
    if (!context) return;
    
    context->target_triple = triple;
}

// Set the target CPU
void goo_context_set_target_cpu(GooContext* context, const char* cpu) {
    if (!context) return;
    
    context->cpu = cpu;
}

// Set the target features
void goo_context_set_target_features(GooContext* context, const char* features) {
    if (!context) return;
    
    context->features = features;
}

// Set the input file
void goo_context_set_input_file(GooContext* context, const char* file) {
    if (!context) return;
    
    context->input_file = file;
}

// Set the output file
void goo_context_set_output_file(GooContext* context, const char* file) {
    if (!context) return;
    
    context->output_file = file;
}

// Set the module name
void goo_context_set_module_name(GooContext* context, const char* name) {
    if (!context) return;
    
    context->module_name = name;
}

// Set whether to emit LLVM IR
void goo_context_set_emit_llvm(GooContext* context, bool emit) {
    if (!context) return;
    
    context->emit_llvm = emit;
}

// Set whether to generate debug info
void goo_context_set_debug(GooContext* context, bool debug) {
    if (!context) return;
    
    context->debug = debug;
} 