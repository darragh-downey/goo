/**
 * codegen_memory.c
 * 
 * Implementation of memory management functions for the Goo compiler's code generation.
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <llvm-c/Core.h>
#include "codegen.h"
#include "codegen_memory.h"
#include "../../include/memory.h"

LLVMValueRef goo_codegen_memory_init(GooCodegenContext* context) {
    if (!context) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_memory_init");
    if (!func) return NULL;
    
    LLVMValueRef args[1] = { 0 };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 0, "memory_init");
}

LLVMValueRef goo_codegen_memory_cleanup(GooCodegenContext* context) {
    if (!context) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_memory_cleanup");
    if (!func) return NULL;
    
    LLVMValueRef args[1] = { 0 };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 0, "memory_cleanup");
}

LLVMValueRef goo_codegen_memory_alloc(GooCodegenContext* context, LLVMValueRef size_val, const char* name) {
    if (!context || !size_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_alloc");
    if (!func) return NULL;
    
    LLVMValueRef args[1] = { size_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 1, name ? name : "alloc");
}

LLVMValueRef goo_codegen_memory_free(GooCodegenContext* context, LLVMValueRef ptr_val, LLVMValueRef size_val) {
    if (!context || !ptr_val || !size_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_free");
    if (!func) return NULL;
    
    LLVMValueRef args[2] = { ptr_val, size_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, "free_result");
}

LLVMValueRef goo_codegen_memory_realloc(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                      LLVMValueRef old_size_val, LLVMValueRef new_size_val, 
                                      const char* name) {
    if (!context || !ptr_val || !old_size_val || !new_size_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_realloc");
    if (!func) return NULL;
    
    LLVMValueRef args[3] = { ptr_val, old_size_val, new_size_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, 
                        name ? name : "realloc");
}

LLVMValueRef goo_codegen_memory_alloc_aligned(GooCodegenContext* context, LLVMValueRef size_val, 
                                           LLVMValueRef alignment_val, const char* name) {
    if (!context || !size_val || !alignment_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_alloc_aligned");
    if (!func) return NULL;
    
    LLVMValueRef args[2] = { size_val, alignment_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, 
                        name ? name : "alloc_aligned");
}

LLVMValueRef goo_codegen_memory_free_aligned(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                          LLVMValueRef size_val, LLVMValueRef alignment_val) {
    if (!context || !ptr_val || !size_val || !alignment_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_free_aligned");
    if (!func) return NULL;
    
    LLVMValueRef args[3] = { ptr_val, size_val, alignment_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 3, 
                        "free_aligned_result");
}

LLVMValueRef goo_codegen_memory_realloc_aligned(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                             LLVMValueRef old_size_val, LLVMValueRef new_size_val, 
                                             LLVMValueRef alignment_val, const char* name) {
    if (!context || !ptr_val || !old_size_val || !new_size_val || !alignment_val) return NULL;
    
    LLVMValueRef func = goo_codegen_get_function(context, "goo_realloc_aligned");
    if (!func) return NULL;
    
    LLVMValueRef args[4] = { ptr_val, old_size_val, new_size_val, alignment_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 4, 
                        name ? name : "realloc_aligned");
}

LLVMValueRef goo_codegen_memory_alloc_or_panic(GooCodegenContext* context, LLVMValueRef size_val, 
                                             const char* name) {
    if (!context || !size_val) return NULL;
    
    // Get the function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_alloc_or_panic");
    if (!func) return NULL;
    
    // Call the function
    LLVMValueRef args[1] = { size_val };
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 1, 
                        name ? name : "alloc_or_panic");
}

LLVMValueRef goo_codegen_memory_auto_alloc(GooCodegenContext* context, LLVMValueRef size_val, 
                                         const char* name) {
    if (!context || !size_val) return NULL;
    
    // First allocate memory
    LLVMValueRef mem_ptr = goo_codegen_memory_alloc(context, size_val, name);
    if (!mem_ptr) return NULL;
    
    // Schedule cleanup for this memory when the scope exits
    goo_codegen_memory_auto_cleanup(context, mem_ptr, size_val);
    
    return mem_ptr;
}

LLVMValueRef goo_codegen_memory_auto_cleanup(GooCodegenContext* context, LLVMValueRef ptr_val, 
                                          LLVMValueRef size_val) {
    if (!context || !ptr_val || !size_val) return NULL;
    
    // Create a cleanup entry that will call goo_free when the scope exits
    // This requires the runtime to support scope-based cleanup
    
    // Get the scope cleanup registration function
    LLVMValueRef func = goo_codegen_get_function(context, "goo_scope_register_cleanup");
    if (!func) {
        // If scope cleanup isn't available, we can't register auto-cleanup
        return NULL;
    }
    
    // Create a struct to hold both the pointer and size
    LLVMTypeRef cleanup_data_type = LLVMStructTypeInContext(context->context, (LLVMTypeRef[]){
        LLVMTypeOf(ptr_val),
        LLVMTypeOf(size_val)
    }, 2, 0);
    
    // Allocate memory for the cleanup data
    LLVMValueRef cleanup_data = LLVMBuildAlloca(context->builder, cleanup_data_type, "cleanup_data");
    
    // Store the pointer and size in the struct
    LLVMValueRef ptr_field_ptr = LLVMBuildStructGEP2(context->builder, cleanup_data_type, cleanup_data, 0, "ptr_field");
    LLVMBuildStore(context->builder, ptr_val, ptr_field_ptr);
    
    LLVMValueRef size_field_ptr = LLVMBuildStructGEP2(context->builder, cleanup_data_type, cleanup_data, 1, "size_field");
    LLVMBuildStore(context->builder, size_val, size_field_ptr);
    
    // Get the cleanup callback function
    LLVMValueRef cleanup_cb = goo_codegen_get_function(context, "goo_memory_cleanup_callback");
    if (!cleanup_cb) return NULL;
    
    // Register the cleanup
    LLVMValueRef args[2] = {
        LLVMBuildBitCast(context->builder, cleanup_data, LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), "cleanup_data_ptr"),
        cleanup_cb
    };
    
    return LLVMBuildCall2(context->builder, LLVMGetElementType(LLVMTypeOf(func)), func, args, 2, "cleanup_registration");
} 