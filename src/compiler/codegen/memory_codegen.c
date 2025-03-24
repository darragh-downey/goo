#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "../../include/goo_memory.h"

// Generate code for arena allocator creation
void codegen_arena_create(CodegenContext* ctx, const char* var_name, size_t page_size) {
    if (!ctx || !var_name) return;
    
    char* code = malloc(256);
    sprintf(code, "GooCustomAllocator* %s = goo_runtime_create_arena(%zu);\n", var_name, page_size);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for pool allocator creation
void codegen_pool_create(CodegenContext* ctx, const char* var_name, size_t obj_size, size_t capacity) {
    if (!ctx || !var_name) return;
    
    char* code = malloc(256);
    sprintf(code, "GooCustomAllocator* %s = goo_runtime_create_pool(%zu, %zu);\n", 
            var_name, obj_size, capacity);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for allocation from an arena
void codegen_arena_alloc(CodegenContext* ctx, const char* arena_var, const char* result_var, 
                        const char* type_name, size_t size, size_t alignment) {
    if (!ctx || !arena_var || !result_var || !type_name) return;
    
    char* code = malloc(512);
    sprintf(code, 
            "%s* %s = (%s*)goo_custom_alloc(%s, %zu, %zu);\n"
            "if (!%s) {\n"
            "    goo_runtime_panic(\"Out of memory\");\n"
            "}\n",
            type_name, result_var, type_name, arena_var, size, alignment, result_var);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for allocation from a pool
void codegen_pool_alloc(CodegenContext* ctx, const char* pool_var, const char* result_var, 
                       const char* type_name) {
    if (!ctx || !pool_var || !result_var || !type_name) return;
    
    char* code = malloc(512);
    sprintf(code, 
            "%s* %s = (%s*)goo_custom_alloc(%s, 0, 0);\n"
            "if (!%s) {\n"
            "    goo_runtime_panic(\"Out of memory\");\n"
            "}\n",
            type_name, result_var, type_name, pool_var, result_var);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for pool deallocate
void codegen_pool_free(CodegenContext* ctx, const char* pool_var, const char* ptr_expr) {
    if (!ctx || !pool_var || !ptr_expr) return;
    
    char* code = malloc(256);
    sprintf(code, "goo_custom_free(%s, %s);\n", pool_var, ptr_expr);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for arena reset
void codegen_arena_reset(CodegenContext* ctx, const char* arena_var) {
    if (!ctx || !arena_var) return;
    
    char* code = malloc(256);
    sprintf(code, "goo_custom_reset(%s);\n", arena_var);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for pool reset
void codegen_pool_reset(CodegenContext* ctx, const char* pool_var) {
    if (!ctx || !pool_var) return;
    
    char* code = malloc(256);
    sprintf(code, "goo_custom_reset(%s);\n", pool_var);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for runtime allocation
void codegen_runtime_alloc(CodegenContext* ctx, const char* result_var, 
                          const char* type_name, size_t size) {
    if (!ctx || !result_var || !type_name) return;
    
    char* code = malloc(512);
    sprintf(code, 
            "%s* %s = (%s*)goo_runtime_alloc(%zu);\n"
            "if (!%s) {\n"
            "    goo_runtime_panic(\"Out of memory\");\n"
            "}\n",
            type_name, result_var, type_name, size, result_var);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Generate code for runtime deallocation
void codegen_runtime_free(CodegenContext* ctx, const char* ptr_expr) {
    if (!ctx || !ptr_expr) return;
    
    char* code = malloc(256);
    sprintf(code, "goo_runtime_free(%s);\n", ptr_expr);
    
    codegen_emit_raw(ctx, code);
    free(code);
}

// Process memory allocation keywords in the AST
void codegen_process_memory_keyword(CodegenContext* ctx, ASTNode* expr) {
    if (!ctx || !expr || expr->type != AST_CALL_EXPR) return;
    
    // Get the function name
    const char* func_name = NULL;
    if (expr->call_expr.function->type == AST_VAR_REF) {
        func_name = expr->call_expr.function->var_ref.name;
    }
    
    if (!func_name) return;
    
    // Check for memory keywords
    if (strcmp(func_name, "new") == 0) {
        // new(Type) -> runtime allocation
        if (expr->call_expr.args && expr->call_expr.args->type == AST_TYPE_EXPR) {
            // Get the type name
            const char* type_name = "void"; // Default
            size_t type_size = 8;           // Default size
            
            // TODO: Determine type name and size from the AST
            
            // Generate code
            char* result_var = codegen_new_var(ctx, "obj");
            codegen_runtime_alloc(ctx, result_var, type_name, type_size);
            
            // TODO: Return the result variable
        }
    } else if (strcmp(func_name, "make") == 0) {
        // make(Type, ...) -> runtime allocation with initialization
        if (expr->call_expr.args && expr->call_expr.args->type == AST_TYPE_EXPR) {
            // TODO: Handle different types (slice, map, channel)
        }
    }
}

// Process a memory pool or arena declaration
void codegen_process_memory_declaration(CodegenContext* ctx, ASTNode* decl) {
    if (!ctx || !decl || decl->type != AST_VAR_DECL) return;
    
    // Check if this is a memory type declaration
    ASTNode* type_node = decl->var_decl.type_expr;
    if (!type_node || type_node->type != AST_TYPE_EXPR) return;
    
    const char* type_name = type_node->type_expr.name;
    if (!type_name) return;
    
    // Generate code based on memory type
    if (strcmp(type_name, "ArenaAllocator") == 0) {
        // Arena allocator declaration
        size_t page_size = 4096; // Default
        
        // Check for initialization expression
        if (decl->var_decl.init_expr) {
            // TODO: Extract page size from initialization expression
        }
        
        codegen_arena_create(ctx, decl->var_decl.name, page_size);
    } else if (strcmp(type_name, "PoolAllocator") == 0) {
        // Pool allocator declaration
        size_t obj_size = 8;     // Default
        size_t capacity = 16;    // Default
        
        // Check for initialization expression
        if (decl->var_decl.init_expr) {
            // TODO: Extract object size and capacity from initialization expression
        }
        
        codegen_pool_create(ctx, decl->var_decl.name, obj_size, capacity);
    }
} 