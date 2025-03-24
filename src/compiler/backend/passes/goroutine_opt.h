#ifndef GOO_GOROUTINE_OPT_H
#define GOO_GOROUTINE_OPT_H

#include <stdbool.h>
#include <llvm-c/Core.h>

// Initialize the goroutine optimization pass
bool goo_goroutine_opt_init(void);

// Add the goroutine optimization pass to a pass manager
bool goo_add_goroutine_opt_pass(LLVMPassManagerRef pass_manager);

// Clean up resources used by the goroutine optimization pass
void goo_goroutine_opt_cleanup(void);

// Optimize goroutine spawning by inlining small functions
bool goo_optimize_goroutine_inlining(LLVMModuleRef module);

// Batch similar goroutines to reduce scheduling overhead
bool goo_optimize_goroutine_batching(LLVMModuleRef module);

// Analyze and optimize a goroutine spawn call
bool goo_optimize_goroutine_spawn(LLVMModuleRef module, LLVMValueRef spawn_call);

#endif // GOO_GOROUTINE_OPT_H 