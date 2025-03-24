/**
 * codegen_scope.h
 * 
 * Code generation for scope-based resource management in Goo.
 * This file defines functions for generating LLVM IR for scope management.
 */

#ifndef GOO_CODEGEN_SCOPE_H
#define GOO_CODEGEN_SCOPE_H

#include <llvm-c/Core.h>
#include "codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate code to enter a new scope.
 * 
 * @param context The code generation context
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_scope_enter(GooCodegenContext* context);

/**
 * Generate code to exit the current scope.
 * 
 * @param context The code generation context
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_scope_exit(GooCodegenContext* context);

/**
 * Generate code to register a cleanup function for memory.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to clean up
 * @param size_val The LLVM value representing the size of the memory
 * @param alignment_val The LLVM value representing the alignment, or NULL for default
 * @return The LLVM value representing the result of the registration
 */
LLVMValueRef goo_codegen_scope_register_memory_cleanup(GooCodegenContext* context, 
                                                    LLVMValueRef ptr_val, 
                                                    LLVMValueRef size_val, 
                                                    LLVMValueRef alignment_val);

/**
 * Generate code to register a cleanup function for a resource.
 * 
 * @param context The code generation context
 * @param resource_val The LLVM value representing the resource to clean up
 * @param cleanup_fn_val The LLVM value representing the cleanup function
 * @return The LLVM value representing the result of the registration
 */
LLVMValueRef goo_codegen_scope_register_resource_cleanup(GooCodegenContext* context, 
                                                      LLVMValueRef resource_val, 
                                                      LLVMValueRef cleanup_fn_val);

/**
 * Generate code to register a raw cleanup function.
 * 
 * @param context The code generation context
 * @param data_val The LLVM value representing the data to pass to the cleanup function
 * @param cleanup_fn_val The LLVM value representing the cleanup function
 * @return The LLVM value representing the result of the registration
 */
LLVMValueRef goo_codegen_scope_register_cleanup(GooCodegenContext* context, 
                                             LLVMValueRef data_val, 
                                             LLVMValueRef cleanup_fn_val);

/**
 * Generate a scope with an automatic cleanup for memory.
 * This creates a scope, allocates memory, and sets up cleanup.
 * 
 * @param context The code generation context
 * @param size_val The LLVM value representing the size to allocate
 * @param name The name for the result variable
 * @return The LLVM value representing the allocated memory
 */
LLVMValueRef goo_codegen_scope_auto_memory(GooCodegenContext* context, 
                                        LLVMValueRef size_val, 
                                        const char* name);

#ifdef __cplusplus
}
#endif

#endif /* GOO_CODEGEN_SCOPE_H */ 