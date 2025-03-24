/**
 * codegen_scope.c
 * 
 * Implementation of code generation for scope-based resource management in Goo.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <llvm-c/Core.h>
#include "codegen.h"
#include "codegen_scope.h"
#include "codegen_memory.h"
#include "../../include/scope/scope.h"

/**
 * Generate code to enter a new scope.
 */
LLVMValueRef goo_codegen_scope_enter(GooCodegenContext* context) {
    if (!context) return NULL;
    
    // Get the scope_enter function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_scope_enter");
    if (!func) return NULL;
    
    // Call the function
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, NULL, 0, "scope_enter");
}

/**
 * Generate code to exit the current scope.
 */
LLVMValueRef goo_codegen_scope_exit(GooCodegenContext* context) {
    if (!context) return NULL;
    
    // Get the scope_exit function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_scope_exit");
    if (!func) return NULL;
    
    // Call the function
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, NULL, 0, "scope_exit");
}

/**
 * Generate code to register a cleanup function for memory.
 */
LLVMValueRef goo_codegen_scope_register_memory_cleanup(GooCodegenContext* context, 
                                                    LLVMValueRef ptr_val, 
                                                    LLVMValueRef size_val, 
                                                    LLVMValueRef alignment_val) {
    if (!context || !ptr_val || !size_val) return NULL;
    
    // Get the register function
    LLVMValueRef func;
    LLVMValueRef args[3];
    
    if (alignment_val) {
        // Use aligned memory cleanup
        func = goo_codegen_get_function(context, "goo_scope_register_memory_cleanup");
        if (!func) return NULL;
        
        args[0] = ptr_val;
        args[1] = size_val;
        args[2] = alignment_val;
        
        return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, "register_memory_cleanup");
    } else {
        // Use regular memory cleanup with zero alignment
        func = goo_codegen_get_function(context, "goo_scope_register_memory_cleanup");
        if (!func) return NULL;
        
        args[0] = ptr_val;
        args[1] = size_val;
        args[2] = LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, false);
        
        return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, "register_memory_cleanup");
    }
}

/**
 * Generate code to register a cleanup function for a resource.
 */
LLVMValueRef goo_codegen_scope_register_resource_cleanup(GooCodegenContext* context, 
                                                      LLVMValueRef resource_val, 
                                                      LLVMValueRef cleanup_fn_val) {
    if (!context || !resource_val || !cleanup_fn_val) return NULL;
    
    // Get the register function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_scope_register_resource_cleanup");
    if (!func) return NULL;
    
    LLVMValueRef args[2] = { resource_val, cleanup_fn_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, "register_resource_cleanup");
}

/**
 * Generate code to register a raw cleanup function.
 */
LLVMValueRef goo_codegen_scope_register_cleanup(GooCodegenContext* context, 
                                             LLVMValueRef data_val, 
                                             LLVMValueRef cleanup_fn_val) {
    if (!context || !data_val || !cleanup_fn_val) return NULL;
    
    // Get the register function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_scope_register_cleanup");
    if (!func) return NULL;
    
    LLVMValueRef args[2] = { data_val, cleanup_fn_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, "register_cleanup");
}

/**
 * Generate a scope with an automatic cleanup for memory.
 * This creates a scope, allocates memory, and sets up cleanup.
 */
LLVMValueRef goo_codegen_scope_auto_memory(GooCodegenContext* context, LLVMValueRef size_val, const char* name) {
    if (!context || !size_val) return NULL;
    
    // Enter a new scope
    LLVMValueRef scope_enter = goo_codegen_scope_enter(context);
    if (!scope_enter) return NULL;
    
    // Check the result of scope_enter
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
    LLVMBasicBlockRef success_block = LLVMAppendBasicBlockInContext(context->context, 
                                                                   LLVMGetBasicBlockParent(current_block), 
                                                                   "scope_enter_success");
    LLVMBasicBlockRef fail_block = LLVMAppendBasicBlockInContext(context->context, 
                                                               LLVMGetBasicBlockParent(current_block), 
                                                               "scope_enter_fail");
    LLVMBasicBlockRef end_block = LLVMAppendBasicBlockInContext(context->context, 
                                                              LLVMGetBasicBlockParent(current_block), 
                                                              "scope_enter_end");
    
    // Create the conditional branch
    LLVMBuildCondBr(context->builder, scope_enter, success_block, fail_block);
    
    // Generate code for success case
    LLVMPositionBuilderAtEnd(context->builder, success_block);
    
    // Allocate memory
    LLVMValueRef mem_ptr = goo_codegen_memory_alloc(context, size_val, name);
    if (!mem_ptr) {
        // If allocation fails, just exit the scope and return NULL
        goo_codegen_scope_exit(context);
        LLVMBuildBr(context->builder, fail_block);
    } else {
        // Register the memory for cleanup
        goo_codegen_scope_register_memory_cleanup(context, mem_ptr, size_val, NULL);
        
        // Return the allocated memory
        LLVMBuildBr(context->builder, end_block);
    }
    
    // Generate code for failure case
    LLVMPositionBuilderAtEnd(context->builder, fail_block);
    LLVMValueRef null_ptr = LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0));
    LLVMBuildBr(context->builder, end_block);
    
    // Generate code for the end block
    LLVMPositionBuilderAtEnd(context->builder, end_block);
    
    // Create a PHI node for the result
    LLVMValueRef result = LLVMBuildPhi(context->builder, 
                                      LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), 
                                      "auto_mem_result");
    
    // Add incoming values to the PHI node
    LLVMValueRef values[2] = { mem_ptr, null_ptr };
    LLVMBasicBlockRef blocks[2] = { success_block, fail_block };
    LLVMAddIncoming(result, values, blocks, 2);
    
    return result;
} 