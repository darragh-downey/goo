/**
 * codegen_lang_memory.h
 * 
 * Code generation for language-specific memory operations in Goo.
 * This file defines functions for generating LLVM IR for language-specific memory operations.
 */

#ifndef GOO_CODEGEN_LANG_MEMORY_H
#define GOO_CODEGEN_LANG_MEMORY_H

#include <llvm-c/Core.h>
#include "codegen.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Generate code to create a Goo string.
 * 
 * @param context The code generation context
 * @param str_val The LLVM value representing the string to create
 * @param name The name for the result variable
 * @return The LLVM value representing the created string
 */
LLVMValueRef goo_codegen_string_create(GooCodegenContext* context, LLVMValueRef str_val, const char* name);

/**
 * Generate code to destroy a Goo string.
 * 
 * @param context The code generation context
 * @param str_val The LLVM value representing the string to destroy
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_string_destroy(GooCodegenContext* context, LLVMValueRef str_val);

/**
 * Generate code to create a Goo array.
 * 
 * @param context The code generation context
 * @param element_size_val The LLVM value representing the size of each element
 * @param count_val The LLVM value representing the number of elements
 * @param name The name for the result variable
 * @return The LLVM value representing the created array
 */
LLVMValueRef goo_codegen_array_create(GooCodegenContext* context, LLVMValueRef element_size_val, 
                                    LLVMValueRef count_val, const char* name);

/**
 * Generate code to resize a Goo array.
 * 
 * @param context The code generation context
 * @param array_val The LLVM value representing the array to resize
 * @param new_count_val The LLVM value representing the new number of elements
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_array_resize(GooCodegenContext* context, LLVMValueRef array_val, 
                                     LLVMValueRef new_count_val);

/**
 * Generate code to destroy a Goo array.
 * 
 * @param context The code generation context
 * @param array_val The LLVM value representing the array to destroy
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_array_destroy(GooCodegenContext* context, LLVMValueRef array_val);

/**
 * Generate code to set an element in a Goo array.
 * 
 * @param context The code generation context
 * @param array_val The LLVM value representing the array
 * @param index_val The LLVM value representing the index to set
 * @param value_val The LLVM value representing the value to set
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_array_set(GooCodegenContext* context, LLVMValueRef array_val, 
                                 LLVMValueRef index_val, LLVMValueRef value_val);

/**
 * Generate code to get an element from a Goo array.
 * 
 * @param context The code generation context
 * @param array_val The LLVM value representing the array
 * @param index_val The LLVM value representing the index to get
 * @param value_ptr_val The LLVM value representing the pointer to store the value
 * @return The LLVM value representing the result of the operation
 */
LLVMValueRef goo_codegen_array_get(GooCodegenContext* context, LLVMValueRef array_val, 
                                 LLVMValueRef index_val, LLVMValueRef value_ptr_val);

/**
 * Generate code to get a pointer to an element in a Goo array.
 * 
 * @param context The code generation context
 * @param array_val The LLVM value representing the array
 * @param index_val The LLVM value representing the index to get
 * @param name The name for the result variable
 * @return The LLVM value representing the pointer to the element
 */
LLVMValueRef goo_codegen_array_get_ptr(GooCodegenContext* context, LLVMValueRef array_val, 
                                     LLVMValueRef index_val, const char* name);

/**
 * Create LLVM types for Goo language structures.
 * This must be called before any other language-specific code generation functions.
 * 
 * @param context The code generation context
 */
void goo_codegen_create_lang_types(GooCodegenContext* context);

#ifdef __cplusplus
}
#endif

#endif /* GOO_CODEGEN_LANG_MEMORY_H */ 