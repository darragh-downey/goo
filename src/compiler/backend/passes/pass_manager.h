#ifndef GOO_PASS_MANAGER_H
#define GOO_PASS_MANAGER_H

#include <stdbool.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Scalar.h>
#include <llvm-c/Transforms/Vectorize.h>
#include <llvm-c/Transforms/IPO.h>
#include "../../include/codegen.h"

// Optimization level definitions
typedef enum {
    GOO_OPT_NONE = 0,     // -O0: No optimization
    GOO_OPT_BASIC = 1,    // -O1: Basic optimizations
    GOO_OPT_MODERATE = 2, // -O2: Moderate optimizations
    GOO_OPT_AGGRESSIVE = 3, // -O3: Aggressive optimizations
    GOO_OPT_CUSTOM = 4    // -O4: Goo-specific optimizations
} GooOptimizationLevel;

// Pass group definitions
typedef enum {
    GOO_PASS_GROUP_SCALAR,      // Scalar transformations
    GOO_PASS_GROUP_LOOP,        // Loop transformations
    GOO_PASS_GROUP_INTERPROCEDURAL, // Interprocedural optimizations
    GOO_PASS_GROUP_VECTORIZATION,   // SIMD/vectorization
    GOO_PASS_GROUP_CUSTOM       // Goo-specific optimizations
} GooPassGroup;

// Initialize the pass manager system
bool goo_pass_manager_init(void);

// Create a pass manager for module-level optimizations
LLVMPassManagerRef goo_create_module_pass_manager(void);

// Create a pass manager for function-level optimizations
LLVMPassManagerRef goo_create_function_pass_manager(LLVMModuleRef module);

// Add optimization passes based on specified level
bool goo_add_optimization_passes(LLVMPassManagerRef pass_manager, 
                                GooOptimizationLevel level,
                                LLVMTargetMachineRef target_machine);

// Add Goo-specific optimization passes
bool goo_add_channel_optimization_pass(LLVMPassManagerRef pass_manager);
bool goo_add_goroutine_optimization_pass(LLVMPassManagerRef pass_manager);
bool goo_add_parallel_optimization_pass(LLVMPassManagerRef pass_manager);

// Run optimization passes on a module
bool goo_run_module_optimizations(GooCodegenContext* context, GooOptimizationLevel level);

// Run optimization passes on a function
bool goo_run_function_optimizations(GooCodegenContext* context, 
                                  LLVMValueRef function,
                                  GooOptimizationLevel level);

// Configure which passes are enabled/disabled
bool goo_configure_pass(GooPassGroup group, const char* pass_name, bool enabled);

// Get description of available passes
const char* goo_get_pass_description(GooPassGroup group, const char* pass_name);

// Clean up pass manager resources
void goo_pass_manager_cleanup(void);

#endif // GOO_PASS_MANAGER_H 