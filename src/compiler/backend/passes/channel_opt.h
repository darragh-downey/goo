#ifndef GOO_CHANNEL_OPT_H
#define GOO_CHANNEL_OPT_H

#include <stdbool.h>
#include <llvm-c/Core.h>

// Initialize the channel optimization pass
bool goo_channel_opt_init(void);

// Add the channel optimization pass to a pass manager
bool goo_add_channel_opt_pass(LLVMPassManagerRef pass_manager);

// Clean up resources used by the channel optimization pass
void goo_channel_opt_cleanup(void);

// Optimize a specific channel operation in the module
bool goo_optimize_channel_op(LLVMModuleRef module, LLVMValueRef channel_op);

// Perform fast-path optimization for local channels
bool goo_optimize_local_channels(LLVMModuleRef module);

// Optimize buffer sizes based on usage patterns
bool goo_optimize_channel_buffers(LLVMModuleRef module);

// Batch sequential channel operations
bool goo_optimize_channel_batching(LLVMModuleRef module);

#endif // GOO_CHANNEL_OPT_H 