#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "../../include/goo_error.h"

// Generate code for a try block
void codegen_try_block(CodegenContext* ctx, ASTNode* try_block) {
    if (!ctx || !try_block || try_block->type != AST_TRY_BLOCK) {
        return;
    }
    
    // Create a unique label for the try/recover
    char* try_label = codegen_new_label(ctx, "try");
    char* end_label = codegen_new_label(ctx, "try_end");
    
    // Generate the recovery setup code
    codegen_emit(ctx, "// Begin try block\n");
    codegen_emit(ctx, "if (goo_recover_setup()) {\n");
    codegen_indent(ctx);
    
    // Remember that we're in a try block
    bool old_has_defer = ctx->has_defer;
    ctx->has_defer = true;
    
    // Generate the try block
    if (try_block->try_block.body) {
        codegen_block(ctx, try_block->try_block.body);
    }
    
    // Generate the try block exit
    codegen_emit(ctx, "goo_recover_finish();\n");
    codegen_emit(ctx, "goto %s;\n", end_label);
    codegen_dedent(ctx);
    
    // Generate the recover block
    codegen_emit(ctx, "} else {\n");
    codegen_indent(ctx);
    
    // Generate recover variable declaration if needed
    if (try_block->try_block.recover_var) {
        const char* var_name = try_block->try_block.recover_var;
        codegen_emit(ctx, "void* %s = goo_get_panic_value();\n", var_name);
    }
    
    // Generate recover block
    if (try_block->try_block.recover_block) {
        codegen_block(ctx, try_block->try_block.recover_block);
    }
    
    // Clear the panic state at the end of the recover block
    codegen_emit(ctx, "goo_clear_panic();\n");
    codegen_emit(ctx, "goo_recover_finish();\n");
    codegen_dedent(ctx);
    codegen_emit(ctx, "}\n");
    
    // End label
    codegen_emit(ctx, "%s:;\n", end_label);
    codegen_emit(ctx, "// End try block\n");
    
    // Restore defer state
    ctx->has_defer = old_has_defer;
    
    // Free the labels
    free(try_label);
    free(end_label);
}

// Generate code for a panic statement
void codegen_panic_stmt(CodegenContext* ctx, ASTNode* panic_stmt) {
    if (!ctx || !panic_stmt || panic_stmt->type != AST_PANIC_STMT) {
        return;
    }
    
    // Generate the panic value expression
    ASTNode* value_expr = panic_stmt->panic_stmt.value;
    if (value_expr) {
        // For now, we'll generate the value expression and pass it to goo_panic
        // In a full implementation, we would need to handle different types
        
        // Generate a temporary variable for the panic value
        char* value_var = codegen_new_var(ctx, "panic_value");
        
        // Generate code to evaluate the expression
        codegen_emit(ctx, "void* %s = (void*)(", value_var);
        codegen_expression(ctx, value_expr);
        codegen_emit(ctx, ");\n");
        
        // Generate the message if present
        if (panic_stmt->panic_stmt.message) {
            // String literal message
            codegen_emit(ctx, "goo_panic(%s, \"%s\");\n", 
                        value_var, panic_stmt->panic_stmt.message);
        } else {
            // No message
            codegen_emit(ctx, "goo_panic(%s, NULL);\n", value_var);
        }
        
        free(value_var);
    } else {
        // No value, just a message
        if (panic_stmt->panic_stmt.message) {
            codegen_emit(ctx, "goo_panic(NULL, \"%s\");\n", 
                        panic_stmt->panic_stmt.message);
        } else {
            codegen_emit(ctx, "goo_panic(NULL, \"panic\");\n");
        }
    }
}

// Generate code for a recover expression
void codegen_recover_expr(CodegenContext* ctx, ASTNode* recover_expr) {
    if (!ctx || !recover_expr || recover_expr->type != AST_RECOVER_EXPR) {
        return;
    }
    
    // Generate code to check if in a panic state and get the panic value
    codegen_emit(ctx, "(goo_is_panic() ? goo_get_panic_value() : NULL)");
    
    // Note: In a full implementation, we would handle type casting based on
    // the expected recovery type, but for now we just return the raw value
}

// Process error handling attributes
void codegen_process_error_attr(CodegenContext* ctx, ASTNode* attr, ASTNode* node) {
    if (!ctx || !attr || !node || attr->type != AST_ATTRIBUTE) {
        return;
    }
    
    // Check if this is an error handling attribute
    if (strcmp(attr->attr.name, "propagates") == 0) {
        // This function can propagate errors
        // In the future, we might generate different code for error propagation
    } else if (strcmp(attr->attr.name, "nopanic") == 0) {
        // This function promises not to panic
        // In the future, we might generate assertions or checks
    }
}

// Generate code for error propagation
void codegen_propagate_error(CodegenContext* ctx, ASTNode* expr) {
    if (!ctx || !expr) {
        return;
    }
    
    // Generate code to check for errors and propagate them
    char* result_var = codegen_new_var(ctx, "result");
    
    // Generate the expression result
    codegen_emit(ctx, "{\n");
    codegen_indent(ctx);
    
    codegen_emit(ctx, "void* %s = ", result_var);
    codegen_expression(ctx, expr);
    codegen_emit(ctx, ";\n");
    
    // Check if the result indicates an error and propagate it
    codegen_emit(ctx, "if (%s != NULL && goo_is_error(%s)) {\n", result_var, result_var);
    codegen_indent(ctx);
    codegen_emit(ctx, "return %s; // Propagate error\n", result_var);
    codegen_dedent(ctx);
    codegen_emit(ctx, "}\n");
    
    free(result_var);
    
    codegen_dedent(ctx);
    codegen_emit(ctx, "}\n");
} 