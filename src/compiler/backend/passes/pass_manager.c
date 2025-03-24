#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Vectorize.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>
#include "pass_manager.h"
#include "channel_opt.h"
#include "goroutine_opt.h"
#include "parallel_opt.h"

// Structure to track pass configuration
typedef struct {
    const char* name;
    bool enabled;
    const char* description;
} GooPassConfig;

// Array of pass configurations by group
static GooPassConfig scalar_passes[] = {
    {"instruction-combining", true, "Combine instructions to simpler forms"},
    {"reassociate", true, "Reassociate expressions for better optimization"},
    {"gvn", true, "Global value numbering and redundant load elimination"},
    {"sccp", true, "Sparse conditional constant propagation"},
    {"dce", true, "Dead code elimination"},
    {"simplifycfg", true, "Simplify control flow graph"},
    {NULL, false, NULL}
};

static GooPassConfig loop_passes[] = {
    {"loop-simplify", true, "Simplify loop structures"},
    {"loop-rotate", true, "Rotate loops to expose optimization opportunities"},
    {"loop-unroll", true, "Unroll loops for better performance"},
    {"licm", true, "Loop invariant code motion"},
    {"indvars", true, "Induction variable simplification"},
    {NULL, false, NULL}
};

static GooPassConfig interprocedural_passes[] = {
    {"function-inlining", true, "Inline functions for better optimization"},
    {"global-dce", true, "Global dead code elimination"},
    {"argument-promotion", true, "Promote by-reference arguments to by-value"},
    {"ip-sccp", true, "Interprocedural sparse conditional constant propagation"},
    {NULL, false, NULL}
};

static GooPassConfig vectorization_passes[] = {
    {"slp-vectorize", true, "Vectorize straight-line code"},
    {"loop-vectorize", true, "Vectorize loops for SIMD execution"},
    {NULL, false, NULL}
};

static GooPassConfig custom_passes[] = {
    {"channel-opt", true, "Optimize channel operations"},
    {"goroutine-opt", true, "Optimize goroutine spawning and execution"},
    {"parallel-opt", true, "Optimize parallel execution blocks"},
    {NULL, false, NULL}
};

// Get the pass configuration array for a given group
static GooPassConfig* get_pass_config_for_group(GooPassGroup group) {
    switch (group) {
        case GOO_PASS_GROUP_SCALAR:
            return scalar_passes;
        case GOO_PASS_GROUP_LOOP:
            return loop_passes;
        case GOO_PASS_GROUP_INTERPROCEDURAL:
            return interprocedural_passes;
        case GOO_PASS_GROUP_VECTORIZATION:
            return vectorization_passes;
        case GOO_PASS_GROUP_CUSTOM:
            return custom_passes;
        default:
            return NULL;
    }
}

// Initialize the pass manager system
bool goo_pass_manager_init(void) {
    // Initialize LLVM targets
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();
    
    // Initialize custom passes
    goo_channel_opt_init();
    goo_goroutine_opt_init();
    goo_parallel_opt_init();
    
    return true;
}

// Create a pass manager for module-level optimizations
LLVMPassManagerRef goo_create_module_pass_manager(void) {
    return LLVMCreatePassManager();
}

// Create a pass manager for function-level optimizations
LLVMPassManagerRef goo_create_function_pass_manager(LLVMModuleRef module) {
    return LLVMCreateFunctionPassManagerForModule(module);
}

// Add standard optimization passes based on specified level
bool goo_add_optimization_passes(LLVMPassManagerRef pass_manager, 
                               GooOptimizationLevel level,
                               LLVMTargetMachineRef target_machine) {
    
    // If optimization is disabled, do nothing
    if (level == GOO_OPT_NONE) {
        return true;
    }
    
    // Create a pass manager builder
    LLVMPassManagerBuilderRef builder = LLVMPassManagerBuilderCreate();
    
    // Configure the pass manager builder based on optimization level
    switch (level) {
        case GOO_OPT_BASIC:
            LLVMPassManagerBuilderSetOptLevel(builder, 1);
            LLVMPassManagerBuilderSetSizeLevel(builder, 0);
            break;
            
        case GOO_OPT_MODERATE:
            LLVMPassManagerBuilderSetOptLevel(builder, 2);
            LLVMPassManagerBuilderSetSizeLevel(builder, 0);
            LLVMPassManagerBuilderUseInlinerWithThreshold(builder, 225);
            break;
            
        case GOO_OPT_AGGRESSIVE:
        case GOO_OPT_CUSTOM:  // Custom level includes all aggressive opts
            LLVMPassManagerBuilderSetOptLevel(builder, 3);
            LLVMPassManagerBuilderSetSizeLevel(builder, 0);
            LLVMPassManagerBuilderUseInlinerWithThreshold(builder, 275);
            break;
            
        default:
            break;
    }
    
    // Add target-specific passes
    if (target_machine) {
        LLVMAddAnalysisPasses(target_machine, pass_manager);
    }
    
    // Populate the pass manager with standard optimization passes
    LLVMPassManagerBuilderPopulateModulePassManager(builder, pass_manager);
    
    // Add Goo-specific passes for the custom optimization level
    if (level == GOO_OPT_CUSTOM) {
        // Add custom passes
        goo_add_channel_optimization_pass(pass_manager);
        goo_add_goroutine_optimization_pass(pass_manager);
        goo_add_parallel_optimization_pass(pass_manager);
    }
    
    // Clean up
    LLVMPassManagerBuilderDispose(builder);
    
    return true;
}

// Add channel optimization pass
bool goo_add_channel_optimization_pass(LLVMPassManagerRef pass_manager) {
    // Check if the pass is enabled
    if (!custom_passes[0].enabled) {
        return true;
    }
    
    // Add the channel optimization pass
    return goo_add_channel_opt_pass(pass_manager);
}

// Add goroutine optimization pass
bool goo_add_goroutine_optimization_pass(LLVMPassManagerRef pass_manager) {
    // Check if the pass is enabled
    if (!custom_passes[1].enabled) {
        return true;
    }
    
    // Add the goroutine optimization pass
    return goo_add_goroutine_opt_pass(pass_manager);
}

// Add parallel optimization pass
bool goo_add_parallel_optimization_pass(LLVMPassManagerRef pass_manager) {
    // Check if the pass is enabled
    if (!custom_passes[2].enabled) {
        return true;
    }
    
    // Add the parallel optimization pass
    return goo_add_parallel_opt_pass(pass_manager);
}

// Run optimization passes on a module
bool goo_run_module_optimizations(GooCodegenContext* context, GooOptimizationLevel level) {
    if (!context || !context->module) {
        return false;
    }
    
    // Create a module pass manager
    LLVMPassManagerRef pass_manager = goo_create_module_pass_manager();
    
    // Add optimization passes
    goo_add_optimization_passes(pass_manager, level, context->target_machine);
    
    // Run the passes
    LLVMRunPassManager(pass_manager, context->module);
    
    // Clean up
    LLVMDisposePassManager(pass_manager);
    
    return true;
}

// Run optimization passes on a function
bool goo_run_function_optimizations(GooCodegenContext* context, 
                                  LLVMValueRef function,
                                  GooOptimizationLevel level) {
    if (!context || !context->module || !function) {
        return false;
    }
    
    // Create a function pass manager
    LLVMPassManagerRef pass_manager = goo_create_function_pass_manager(context->module);
    
    // Add optimization passes
    goo_add_optimization_passes(pass_manager, level, context->target_machine);
    
    // Initialize the pass manager
    LLVMInitializeFunctionPassManager(pass_manager);
    
    // Run the passes on the function
    LLVMRunFunctionPassManager(pass_manager, function);
    
    // Finalize the pass manager
    LLVMFinalizeFunctionPassManager(pass_manager);
    
    // Clean up
    LLVMDisposePassManager(pass_manager);
    
    return true;
}

// Configure which passes are enabled/disabled
bool goo_configure_pass(GooPassGroup group, const char* pass_name, bool enabled) {
    GooPassConfig* config = get_pass_config_for_group(group);
    if (!config || !pass_name) {
        return false;
    }
    
    // Find the pass with the given name
    for (int i = 0; config[i].name != NULL; i++) {
        if (strcmp(config[i].name, pass_name) == 0) {
            config[i].enabled = enabled;
            return true;
        }
    }
    
    return false;  // Pass not found
}

// Get description of available passes
const char* goo_get_pass_description(GooPassGroup group, const char* pass_name) {
    GooPassConfig* config = get_pass_config_for_group(group);
    if (!config || !pass_name) {
        return NULL;
    }
    
    // Find the pass with the given name
    for (int i = 0; config[i].name != NULL; i++) {
        if (strcmp(config[i].name, pass_name) == 0) {
            return config[i].description;
        }
    }
    
    return NULL;  // Pass not found
}

// Clean up pass manager resources
void goo_pass_manager_cleanup(void) {
    // Clean up custom passes
    goo_channel_opt_cleanup();
    goo_goroutine_opt_cleanup();
    goo_parallel_opt_cleanup();
} 