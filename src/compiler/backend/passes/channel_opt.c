#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Scalar.h>
#include "channel_opt.h"

// Private channel analysis data structure
typedef struct {
    LLVMValueRef channel;         // The channel value
    LLVMValueRef creator;         // Function that created the channel
    LLVMValueRef* users;          // Functions that use the channel
    unsigned user_count;          // Number of users
    bool is_local;                // True if channel is only used in one function
    bool has_multiple_senders;    // True if multiple functions send to this channel
    bool has_multiple_receivers;  // True if multiple functions receive from this channel
    int optimal_buffer_size;      // Calculated optimal buffer size based on usage
    bool can_batch;               // True if operations can be batched
} GooChannelAnalysis;

// List of channel analyses
static GooChannelAnalysis* g_channel_analyses = NULL;
static unsigned g_channel_analysis_count = 0;
static unsigned g_channel_analysis_capacity = 0;

// Initialize the channel optimization pass
bool goo_channel_opt_init(void) {
    // Initialize the channel analysis list
    g_channel_analysis_capacity = 16;
    g_channel_analyses = malloc(g_channel_analysis_capacity * sizeof(GooChannelAnalysis));
    if (!g_channel_analyses) {
        return false;
    }
    
    g_channel_analysis_count = 0;
    
    return true;
}

// Clean up resources used by the channel optimization pass
void goo_channel_opt_cleanup(void) {
    // Free the channel analysis list
    if (g_channel_analyses) {
        for (unsigned i = 0; i < g_channel_analysis_count; i++) {
            free(g_channel_analyses[i].users);
        }
        
        free(g_channel_analyses);
        g_channel_analyses = NULL;
        g_channel_analysis_count = 0;
        g_channel_analysis_capacity = 0;
    }
}

// Add a channel analysis to the list
static bool add_channel_analysis(GooChannelAnalysis analysis) {
    // Check if we need to resize the list
    if (g_channel_analysis_count >= g_channel_analysis_capacity) {
        unsigned new_capacity = g_channel_analysis_capacity * 2;
        GooChannelAnalysis* new_analyses = realloc(g_channel_analyses, 
                                                 new_capacity * sizeof(GooChannelAnalysis));
        if (!new_analyses) {
            return false;
        }
        
        g_channel_analyses = new_analyses;
        g_channel_analysis_capacity = new_capacity;
    }
    
    // Add the analysis to the list
    g_channel_analyses[g_channel_analysis_count++] = analysis;
    
    return true;
}

// LLVM callback function for analyzing channel operations
static int analyze_channel_operation(LLVMValueRef value, void* data) {
    // Skip non-instructions
    if (!LLVMIsAInstruction(value)) {
        return 0;
    }
    
    // Check if this is a call instruction
    if (LLVMGetInstructionOpcode(value) != LLVMCall) {
        return 0;
    }
    
    // Get the called function
    LLVMValueRef called_function = LLVMGetCalledValue(value);
    if (!called_function) {
        return 0;
    }
    
    // Check if this is a channel operation
    const char* func_name = LLVMGetValueName(called_function);
    if (!func_name) {
        return 0;
    }
    
    // Check if this is a channel creation, send, or receive operation
    bool is_channel_create = strstr(func_name, "goo_channel_create") != NULL;
    bool is_channel_send = strstr(func_name, "goo_channel_send") != NULL || 
                           strstr(func_name, "goo_distributed_channel_send") != NULL;
    bool is_channel_recv = strstr(func_name, "goo_channel_recv") != NULL;
    
    if (!is_channel_create && !is_channel_send && !is_channel_recv) {
        return 0;
    }
    
    // Get the channel argument
    LLVMValueRef channel = NULL;
    if (is_channel_create) {
        // The result of the call is the channel
        channel = value;
    } else {
        // The first argument is the channel
        channel = LLVMGetOperand(value, 0);
    }
    
    // Get the parent function
    LLVMValueRef parent_function = LLVMGetInstructionParent(value);
    if (!parent_function) {
        parent_function = LLVMGetBasicBlockParent(LLVMGetInstructionParent(value));
    }
    
    // Create or update channel analysis
    for (unsigned i = 0; i < g_channel_analysis_count; i++) {
        if (g_channel_analyses[i].channel == channel) {
            // Update existing analysis
            
            // Check if this is a new user
            bool found_user = false;
            for (unsigned j = 0; j < g_channel_analyses[i].user_count; j++) {
                if (g_channel_analyses[i].users[j] == parent_function) {
                    found_user = true;
                    break;
                }
            }
            
            if (!found_user) {
                // Add the new user
                unsigned new_count = g_channel_analyses[i].user_count + 1;
                LLVMValueRef* new_users = realloc(g_channel_analyses[i].users, 
                                               new_count * sizeof(LLVMValueRef));
                if (!new_users) {
                    return 0;
                }
                
                new_users[g_channel_analyses[i].user_count] = parent_function;
                g_channel_analyses[i].users = new_users;
                g_channel_analyses[i].user_count = new_count;
                
                // Update local status
                g_channel_analyses[i].is_local = (new_count <= 1);
            }
            
            // Update sender/receiver status
            if (is_channel_send) {
                g_channel_analyses[i].has_multiple_senders = 
                    (g_channel_analyses[i].has_multiple_senders || 
                     g_channel_analyses[i].creator != parent_function);
            } else if (is_channel_recv) {
                g_channel_analyses[i].has_multiple_receivers = 
                    (g_channel_analyses[i].has_multiple_receivers || 
                     g_channel_analyses[i].creator != parent_function);
            }
            
            return 0;
        }
    }
    
    // Create a new analysis for this channel
    if (is_channel_create) {
        GooChannelAnalysis analysis = {0};
        analysis.channel = channel;
        analysis.creator = parent_function;
        analysis.user_count = 1;
        analysis.users = malloc(sizeof(LLVMValueRef));
        if (!analysis.users) {
            return 0;
        }
        
        analysis.users[0] = parent_function;
        analysis.is_local = true;
        analysis.has_multiple_senders = false;
        analysis.has_multiple_receivers = false;
        analysis.optimal_buffer_size = -1;  // Not calculated yet
        analysis.can_batch = false;         // Not determined yet
        
        add_channel_analysis(analysis);
    }
    
    return 0;
}

// Analyze the module for channel operations
static bool analyze_channels(LLVMModuleRef module) {
    // Clear any existing analyses
    goo_channel_opt_cleanup();
    goo_channel_opt_init();
    
    // Iterate through all functions in the module
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function) {
        // Iterate through all basic blocks in the function
        LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(function);
        while (block) {
            // Iterate through all instructions in the basic block
            LLVMValueRef instruction = LLVMGetFirstInstruction(block);
            while (instruction) {
                analyze_channel_operation(instruction, NULL);
                instruction = LLVMGetNextInstruction(instruction);
            }
            
            block = LLVMGetNextBasicBlock(block);
        }
        
        function = LLVMGetNextFunction(function);
    }
    
    return true;
}

// Perform fast-path optimization for local channels
bool goo_optimize_local_channels(LLVMModuleRef module) {
    // First analyze all channels in the module
    analyze_channels(module);
    
    // Look for local channels that can be optimized
    for (unsigned i = 0; i < g_channel_analysis_count; i++) {
        if (g_channel_analyses[i].is_local) {
            // This channel is only used in one function - optimize it
            
            // Get the channel creation call
            LLVMValueRef channel = g_channel_analyses[i].channel;
            
            // Replace uses of goo_channel_send with direct store operations
            // Replace uses of goo_channel_recv with direct load operations
            // This eliminates the synchronization overhead
            
            // Find all uses of the channel
            LLVMUseRef use = LLVMGetFirstUse(channel);
            while (use) {
                LLVMValueRef user = LLVMGetUser(use);
                
                // Check if this is a channel send or receive
                if (LLVMIsACallInst(user)) {
                    LLVMValueRef called_value = LLVMGetCalledValue(user);
                    if (called_value) {
                        const char* func_name = LLVMGetValueName(called_value);
                        if (func_name) {
                            if (strstr(func_name, "goo_channel_send") != NULL) {
                                // Replace with optimized send operation
                                // ...
                            } else if (strstr(func_name, "goo_channel_recv") != NULL) {
                                // Replace with optimized receive operation
                                // ...
                            }
                        }
                    }
                }
                
                use = LLVMGetNextUse(use);
            }
        }
    }
    
    return true;
}

// Optimize buffer sizes based on usage patterns
bool goo_optimize_channel_buffers(LLVMModuleRef module) {
    // First analyze all channels in the module
    analyze_channels(module);
    
    // Calculate optimal buffer sizes for each channel
    for (unsigned i = 0; i < g_channel_analysis_count; i++) {
        // Heuristic: If there are multiple senders and one receiver,
        // or one sender and multiple receivers, a larger buffer might help
        if ((g_channel_analyses[i].has_multiple_senders && !g_channel_analyses[i].has_multiple_receivers) ||
            (!g_channel_analyses[i].has_multiple_senders && g_channel_analyses[i].has_multiple_receivers)) {
            
            // Find the channel creation call
            if (LLVMIsACallInst(g_channel_analyses[i].channel)) {
                // Check if the buffer size argument is a constant
                LLVMValueRef buffer_size_arg = LLVMGetOperand(g_channel_analyses[i].channel, 1);
                if (LLVMIsAConstant(buffer_size_arg)) {
                    int current_buffer_size = LLVMConstIntGetSExtValue(buffer_size_arg);
                    
                    // Calculate a better buffer size based on usage patterns
                    int optimal_buffer_size = current_buffer_size;
                    
                    // If the buffer is small or non-existent, suggest a larger one
                    if (current_buffer_size < 4) {
                        optimal_buffer_size = 16;  // A reasonable default
                    }
                    
                    // Update the channel analysis
                    g_channel_analyses[i].optimal_buffer_size = optimal_buffer_size;
                    
                    // Replace the buffer size argument if it's different
                    if (optimal_buffer_size != current_buffer_size) {
                        LLVMContextRef context = LLVMGetModuleContext(module);
                        LLVMValueRef new_buffer_size = LLVMConstInt(LLVMInt64TypeInContext(context), 
                                                                optimal_buffer_size, 0);
                        
                        // Replace the argument in the call
                        LLVMSetOperand(g_channel_analyses[i].channel, 1, new_buffer_size);
                    }
                }
            }
        }
    }
    
    return true;
}

// Batch sequential channel operations
bool goo_optimize_channel_batching(LLVMModuleRef module) {
    // First analyze all channels in the module
    analyze_channels(module);
    
    // Look for patterns where multiple sends or receives on the same channel
    // occur sequentially in the same basic block
    
    // Iterate through all functions in the module
    LLVMValueRef function = LLVMGetFirstFunction(module);
    while (function) {
        // Iterate through all basic blocks in the function
        LLVMBasicBlockRef block = LLVMGetFirstBasicBlock(function);
        while (block) {
            // Look for sequential channel operations
            LLVMValueRef prev_channel = NULL;
            LLVMValueRef prev_op = NULL;
            int sequence_length = 0;
            
            // Iterate through all instructions in the basic block
            LLVMValueRef instruction = LLVMGetFirstInstruction(block);
            while (instruction) {
                // Check if this is a channel operation
                if (LLVMGetInstructionOpcode(instruction) == LLVMCall) {
                    LLVMValueRef called_function = LLVMGetCalledValue(instruction);
                    if (called_function) {
                        const char* func_name = LLVMGetValueName(called_function);
                        if (func_name) {
                            bool is_channel_send = strstr(func_name, "goo_channel_send") != NULL;
                            bool is_channel_recv = strstr(func_name, "goo_channel_recv") != NULL;
                            
                            if (is_channel_send || is_channel_recv) {
                                // Get the channel argument
                                LLVMValueRef channel = LLVMGetOperand(instruction, 0);
                                
                                // Check if this is a continuation of a sequence
                                if (prev_channel && channel == prev_channel) {
                                    sequence_length++;
                                } else {
                                    // If we had a sequence, optimize it
                                    if (sequence_length >= 3) {
                                        // We found a sequence of at least 3 operations on the same channel
                                        // Replace with a batched operation
                                        // ...
                                    }
                                    
                                    // Start a new sequence
                                    prev_channel = channel;
                                    prev_op = instruction;
                                    sequence_length = 1;
                                }
                            } else {
                                // Not a channel operation, reset the sequence
                                if (sequence_length >= 3) {
                                    // We found a sequence of at least 3 operations on the same channel
                                    // Replace with a batched operation
                                    // ...
                                }
                                
                                prev_channel = NULL;
                                prev_op = NULL;
                                sequence_length = 0;
                            }
                        }
                    }
                } else {
                    // Not a call instruction, reset the sequence
                    if (sequence_length >= 3) {
                        // We found a sequence of at least 3 operations on the same channel
                        // Replace with a batched operation
                        // ...
                    }
                    
                    prev_channel = NULL;
                    prev_op = NULL;
                    sequence_length = 0;
                }
                
                instruction = LLVMGetNextInstruction(instruction);
            }
            
            // Check if we ended the block with a sequence
            if (sequence_length >= 3) {
                // We found a sequence of at least 3 operations on the same channel
                // Replace with a batched operation
                // ...
            }
            
            block = LLVMGetNextBasicBlock(block);
        }
        
        function = LLVMGetNextFunction(function);
    }
    
    return true;
}

// Optimize a specific channel operation in the module
bool goo_optimize_channel_op(LLVMModuleRef module, LLVMValueRef channel_op) {
    // Determine the type of channel operation
    if (!LLVMIsACallInst(channel_op)) {
        return false;
    }
    
    LLVMValueRef called_function = LLVMGetCalledValue(channel_op);
    if (!called_function) {
        return false;
    }
    
    const char* func_name = LLVMGetValueName(called_function);
    if (!func_name) {
        return false;
    }
    
    bool is_channel_create = strstr(func_name, "goo_channel_create") != NULL;
    bool is_channel_send = strstr(func_name, "goo_channel_send") != NULL;
    bool is_channel_recv = strstr(func_name, "goo_channel_recv") != NULL;
    
    if (!is_channel_create && !is_channel_send && !is_channel_recv) {
        return false;
    }
    
    // Apply different optimizations based on the operation type
    if (is_channel_create) {
        // Optimize channel creation
        // ...
    } else if (is_channel_send) {
        // Optimize channel send
        // ...
    } else if (is_channel_recv) {
        // Optimize channel receive
        // ...
    }
    
    return true;
}

// Add the channel optimization pass to a pass manager
bool goo_add_channel_opt_pass(LLVMPassManagerRef pass_manager) {
    // Create a module pass that applies all channel optimizations
    // Note: LLVM-C doesn't provide direct access to create custom passes,
    // so we would need to use a callback function that applies our optimizations
    
    // In a real implementation, we would register a custom pass using the LLVM C++ API
    // and then expose it through the C API. For now, we'll just return true.
    return true;
} 