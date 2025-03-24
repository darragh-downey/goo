/**
 * codegen_memory.h
 * 
 * Memory management functions for the Goo compiler's code generation.
 * This file defines the interface for generating memory management code.
 */

#ifndef GOO_CODEGEN_MEMORY_H
#define GOO_CODEGEN_MEMORY_H

#include <llvm-c/Core.h>
#include "codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate code to initialize the memory system.
 * 
 * @param context The code generation context
 * @return The LLVM value representing the result of the initialization
 */
LLVMValueRef goo_codegen_memory_init(GooCodegenContext* context);

/**
 * Generate code to clean up the memory system.
 * 
 * @param context The code generation context
 * @return The LLVM value representing the result of the cleanup
 */
LLVMValueRef goo_codegen_memory_cleanup(GooCodegenContext* context);

/**
 * Generate code to allocate memory using the Goo allocator.
 * 
 * @param context The code generation context
 * @param size_val The LLVM value representing the size to allocate
 * @param name The name for the result variable
 * @return The LLVM value representing the allocated memory
 */
LLVMValueRef goo_codegen_memory_alloc(GooCodegenContext* context, LLVMValueRef size_val, const char* name);

/**
 * Generate code to free memory allocated with the Goo allocator.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to free
 * @param size_val The LLVM value representing the size of the allocation
 * @return The LLVM value representing the result of the free operation
 */
LLVMValueRef goo_codegen_memory_free(GooCodegenContext* context, LLVMValueRef ptr_val, LLVMValueRef size_val);

/**
 * Generate code to reallocate memory using the Goo allocator.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to reallocate
 * @param old_size_val The LLVM value representing the old size
 * @param new_size_val The LLVM value representing the new size
 * @param name The name for the result variable
 * @return The LLVM value representing the reallocated memory
 */
LLVMValueRef goo_codegen_memory_realloc(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                      LLVMValueRef old_size_val, LLVMValueRef new_size_val, 
                                      const char* name);

/**
 * Generate code to allocate aligned memory.
 * 
 * @param context The code generation context
 * @param size_val The LLVM value representing the size to allocate
 * @param alignment_val The LLVM value representing the alignment
 * @param name The name for the result variable
 * @return The LLVM value representing the allocated memory
 */
LLVMValueRef goo_codegen_memory_alloc_aligned(GooCodegenContext* context, LLVMValueRef size_val, 
                                           LLVMValueRef alignment_val, const char* name);

/**
 * Generate code to free aligned memory.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to free
 * @param size_val The LLVM value representing the size of the allocation
 * @param alignment_val The LLVM value representing the alignment
 * @return The LLVM value representing the result of the free operation
 */
LLVMValueRef goo_codegen_memory_free_aligned(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                          LLVMValueRef size_val, LLVMValueRef alignment_val);

/**
 * Generate code to reallocate aligned memory.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to reallocate
 * @param old_size_val The LLVM value representing the old size
 * @param new_size_val The LLVM value representing the new size
 * @param alignment_val The LLVM value representing the alignment
 * @param name The name for the result variable
 * @return The LLVM value representing the reallocated memory
 */
LLVMValueRef goo_codegen_memory_realloc_aligned(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                             LLVMValueRef old_size_val, LLVMValueRef new_size_val, 
                                             LLVMValueRef alignment_val, const char* name);

/**
 * Generate code to allocate memory with an error handler.
 * This function will generate code that panics if allocation fails.
 * 
 * @param context The code generation context
 * @param size_val The LLVM value representing the size to allocate
 * @param name The name for the result variable
 * @return The LLVM value representing the allocated memory
 */
LLVMValueRef goo_codegen_memory_alloc_or_panic(GooCodegenContext* context, LLVMValueRef size_val, 
                                             const char* name);

/**
 * Generate code for automatically managed memory allocation.
 * This allocates memory that will be automatically freed when the scope exits.
 * 
 * @param context The code generation context
 * @param size_val The LLVM value representing the size to allocate
 * @param name The name for the result variable
 * @return The LLVM value representing the allocated memory
 */
LLVMValueRef goo_codegen_memory_auto_alloc(GooCodegenContext* context, LLVMValueRef size_val, 
                                         const char* name);

/**
 * Generate code to register a cleanup function for automatically managed memory.
 * 
 * @param context The code generation context
 * @param ptr_val The LLVM value representing the pointer to free
 * @param size_val The LLVM value representing the size of the allocation
 * @return The LLVM value representing the result of the registration
 */
LLVMValueRef goo_codegen_memory_auto_cleanup(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                          LLVMValueRef size_val);

#ifdef __cplusplus
}
#endif

#endif /* GOO_CODEGEN_MEMORY_H */ 