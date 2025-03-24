/**
 * codegen_lang_memory.c
 * 
 * Code generation for language-specific memory operations in Goo.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <llvm-c/Core.h>
#include "codegen.h"
#include "codegen_memory.h"
#include "codegen_lang_memory.h"

/**
 * Generate code to create a Goo string.
 */
LLVMValueRef goo_codegen_string_create(GooCodegenContext* context, LLVMValueRef str_val, const char* name) {
    if (!context || !str_val) return NULL;
    
    // Get the string_create function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_string_create");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[1] = { str_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 1, name ? name : "str_create");
}

/**
 * Generate code to destroy a Goo string.
 */
LLVMValueRef goo_codegen_string_destroy(GooCodegenContext* context, LLVMValueRef str_val) {
    if (!context || !str_val) return NULL;
    
    // Get the string_destroy function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_string_destroy");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[1] = { str_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 1, "str_destroy");
}

/**
 * Generate code to create a Goo array.
 */
LLVMValueRef goo_codegen_array_create(GooCodegenContext* context, LLVMValueRef element_size_val, 
                                    LLVMValueRef count_val, const char* name) {
    if (!context || !element_size_val || !count_val) return NULL;
    
    // Get the array_create function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_create");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[2] = { element_size_val, count_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, 
                        name ? name : "array_create");
}

/**
 * Generate code to resize a Goo array.
 */
LLVMValueRef goo_codegen_array_resize(GooCodegenContext* context, LLVMValueRef array_val, 
                                     LLVMValueRef new_count_val) {
    if (!context || !array_val || !new_count_val) return NULL;
    
    // Get the array_resize function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_resize");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[2] = { array_val, new_count_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, "array_resize");
}

/**
 * Generate code to destroy a Goo array.
 */
LLVMValueRef goo_codegen_array_destroy(GooCodegenContext* context, LLVMValueRef array_val) {
    if (!context || !array_val) return NULL;
    
    // Get the array_destroy function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_destroy");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[1] = { array_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 1, "array_destroy");
}

/**
 * Generate code to set an element in a Goo array.
 */
LLVMValueRef goo_codegen_array_set(GooCodegenContext* context, LLVMValueRef array_val, 
                                 LLVMValueRef index_val, LLVMValueRef value_val) {
    if (!context || !array_val || !index_val || !value_val) return NULL;
    
    // Get the array_set function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_set");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[3] = { array_val, index_val, value_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, "array_set");
}

/**
 * Generate code to get an element from a Goo array.
 */
LLVMValueRef goo_codegen_array_get(GooCodegenContext* context, LLVMValueRef array_val, 
                                 LLVMValueRef index_val, LLVMValueRef value_ptr_val) {
    if (!context || !array_val || !index_val || !value_ptr_val) return NULL;
    
    // Get the array_get function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_get");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[3] = { array_val, index_val, value_ptr_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, "array_get");
}

/**
 * Generate code to get a pointer to an element in a Goo array.
 */
LLVMValueRef goo_codegen_array_get_ptr(GooCodegenContext* context, LLVMValueRef array_val, 
                                     LLVMValueRef index_val, const char* name) {
    if (!context || !array_val || !index_val) return NULL;
    
    // Get the array_get_ptr function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_array_get_ptr");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[2] = { array_val, index_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, 
                        name ? name : "array_get_ptr");
}

/**
 * Create LLVM types for Goo language structures.
 */
void goo_codegen_create_lang_types(GooCodegenContext* context) {
    if (!context) return;
    
    // Create the string type
    LLVMTypeRef char_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(context->context), 0);
    LLVMTypeRef size_t_type = LLVMInt64TypeInContext(context->context);
    
    LLVMTypeRef string_struct_elements[2] = {
        char_ptr_type,    // data
        size_t_type       // length
    };
    
    context->string_type = LLVMStructTypeInContext(context->context, string_struct_elements, 2, false);
    context->string_ptr_type = LLVMPointerType(context->string_type, 0);
    
    // Create the array type
    LLVMTypeRef void_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(context->context), 0);
    
    LLVMTypeRef array_struct_elements[4] = {
        void_ptr_type,    // data
        size_t_type,      // element_size
        size_t_type,      // count
        size_t_type       // capacity
    };
    
    context->array_type = LLVMStructTypeInContext(context->context, array_struct_elements, 4, false);
    context->array_ptr_type = LLVMPointerType(context->array_type, 0);
} 