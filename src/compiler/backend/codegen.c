#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/BitWriter.h>

// Try to include Scalar.h if available
#if __has_include(<llvm-c/Transforms/Scalar.h>)
#include <llvm-c/Transforms/Scalar.h>
#endif

#if __has_include(<llvm-c/Transforms/IPO.h>)
#include <llvm-c/Transforms/IPO.h>
#endif

#include "codegen.h"
#include "ast_helpers.h"
#include "ast.h"
#include "runtime.h"
#include "symbol_table.h"
#include "type_table.h"

// Module structure definition
struct GooModule {
    LLVMModuleRef llvm_module;
    LLVMExecutionEngineRef engine;
    char* name;
};

// Initialize the code generator
GooCodegenContext* goo_codegen_init(GooAst* ast, GooContext* ctx, const char* module_name) {
    if (!ast || !module_name) return NULL;

    GooCodegenContext* context = (GooCodegenContext*)malloc(sizeof(GooCodegenContext));
    if (!context) return NULL;

    // Initialize LLVM components
    context->context = LLVMContextCreate();
    context->module = LLVMModuleCreateWithNameInContext(module_name, context->context);
    context->builder = LLVMCreateBuilderInContext(context->context);

    // Initialize Goo components
    context->ast = ast;
    context->goo_context = ctx;
    context->debug_mode = false;

    // Initialize language types
    context->string_type = NULL;
    context->string_ptr_type = NULL;
    context->array_type = NULL;
    context->array_ptr_type = NULL;

    // Initialize symbol table
    context->symbol_table = goo_symbol_table_init();
    if (!context->symbol_table) {
        goo_codegen_free(context);
        return NULL;
    }

    // Initialize type table
    context->type_table = goo_type_table_init(context->context);
    if (!context->type_table) {
        goo_codegen_free(context);
        return NULL;
    }

    return context;
}

// Free the code generator context
void goo_codegen_free(GooCodegenContext* context) {
    if (!context) return;

    // Free LLVM components
    if (context->builder) LLVMDisposeBuilder(context->builder);
    if (context->module) LLVMDisposeModule(context->module);
    if (context->context) LLVMContextDispose(context->context);

    // Free Goo components
    if (context->symbol_table) goo_symbol_table_free(context->symbol_table);
    if (context->type_table) goo_type_table_free(context->type_table);

    // Free the context itself
    free(context);
}

// Generate code for the entire AST
bool goo_codegen_generate(GooCodegenContext* context, GooNode* root) {
    if (!context || !root) {
        fprintf(stderr, "Invalid context or root node\n");
        return false;
    }
    
    // Initialize runtime support first
    if (!goo_codegen_init_runtime(context)) {
        fprintf(stderr, "Failed to initialize runtime support\n");
        return false;
    }
    
    // Create a main function to hold the program
    LLVMTypeRef main_type = LLVMFunctionType(
        LLVMInt32TypeInContext(context->context),
        NULL, 0, false);
    
    LLVMValueRef main_func = LLVMAddFunction(context->module, "main", main_type);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main_func, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry);
    
    // Initialize the runtime in the main function
    goo_codegen_init_main_runtime(context, main_func);
    
    // Generate code for the root node
    LLVMValueRef result = goo_codegen_node(context, root);
    if (!result) {
        fprintf(stderr, "Failed to generate code for program\n");
        return false;
    }
    
    // Return 0 from main
    LLVMBuildRet(context->builder, LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, false));
    
    // Verify the module
    return goo_codegen_verify_module(context);
}

// Generate code for a specific node
LLVMValueRef goo_codegen_node(GooCodegenContext* context, GooNode* node) {
    if (!context || !node) return NULL;

    // Dispatch to specific handler based on node type
    switch (node->type) {
        case GOO_NODE_PACKAGE_DECL:
            // Package declarations don't generate code directly
            return LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0); // Just return a dummy value

        case GOO_NODE_IMPORT_DECL:
            // Import declarations are handled at a higher level
            return LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0);

        case GOO_NODE_FUNCTION_DECL:
            return goo_codegen_function(context, (GooFunctionNode*)node);

        case GOO_NODE_VAR_DECL:
            return goo_codegen_var_decl(context, (GooVarDeclNode*)node);

        case GOO_NODE_BLOCK_STMT:
            return goo_codegen_block(context, (GooBlockStmtNode*)node);

        case GOO_NODE_IF_STMT:
            return goo_codegen_if(context, (GooIfStmtNode*)node);

        case GOO_NODE_FOR_STMT:
            return goo_codegen_for(context, (GooForStmtNode*)node);

        case GOO_NODE_RETURN_STMT:
            return goo_codegen_return(context, (GooReturnStmtNode*)node);

        case GOO_NODE_BINARY_EXPR:
            return goo_codegen_binary_expr(context, (GooBinaryExprNode*)node);

        case GOO_NODE_UNARY_EXPR:
            return goo_codegen_unary_expr(context, (GooUnaryExprNode*)node);

        case GOO_NODE_CALL_EXPR:
            return goo_codegen_call_expr(context, (GooCallExprNode*)node);

        case GOO_NODE_IDENTIFIER:
            return goo_codegen_identifier(context, node);

        case GOO_NODE_INT_LITERAL:
        case GOO_NODE_FLOAT_LITERAL:
        case GOO_NODE_STRING_LITERAL:
        case GOO_NODE_BOOL_LITERAL:
            return goo_codegen_literal(context, node);

        case GOO_NODE_RANGE_LITERAL:
            return goo_codegen_range_literal(context, (GooRangeLiteralNode*)node);

        case GOO_NODE_CHANNEL_SEND:
            return goo_codegen_channel_send(context, (GooChannelSendNode*)node);

        case GOO_NODE_CHANNEL_RECV:
            return goo_codegen_channel_recv(context, (GooChannelRecvNode*)node);

        case GOO_NODE_GO_STMT:
            return goo_codegen_go_stmt(context, (GooGoStmtNode*)node);

        case GOO_NODE_GO_PARALLEL:
            return goo_codegen_go_parallel(context, (GooGoParallelNode*)node);

        case GOO_NODE_CAP_TYPE:
            return goo_codegen_cap_type(context, (GooCapTypeNode*)node);

        case GOO_NODE_CAP_CHECK:
            return goo_codegen_cap_check(context, (GooCapCheckNode*)node);

        case GOO_NODE_CAP_GRANT:
            return goo_codegen_cap_grant(context, (GooCapGrantNode*)node);

        case GOO_NODE_CAP_REVOKE:
            return goo_codegen_cap_revoke(context, (GooCapRevokeNode*)node);

        case GOO_NODE_ERROR_UNION:
            return goo_codegen_error_union(context, (GooErrorUnionNode*)node);

        case GOO_NODE_TRY:
            return goo_codegen_try(context, (GooTryNode*)node);

        case GOO_NODE_RECOVER:
            return goo_codegen_recover(context, (GooRecoverNode*)node);

        case GOO_NODE_SUPERVISE:
            return goo_codegen_supervise(context, (GooSuperviseNode*)node);

        case GOO_NODE_SUPERVISE_GO:
            return goo_codegen_supervise_go(context, (GooSuperviseGoNode*)node);

        case GOO_NODE_SUPERVISE_POLICY:
            return goo_codegen_supervise_policy(context, (GooSuperviseNode*)node);

        default:
            fprintf(stderr, "Unsupported node type: %d\n", node->type);
            return NULL;
    }
}

// Verify the generated module
bool goo_codegen_verify_module(GooCodegenContext* context) {
    if (!context || !context->module) return false;

    char* error = NULL;
    LLVMBool result = LLVMVerifyModule(context->module, LLVMPrintMessageAction, &error);
    
    if (result) {
        fprintf(stderr, "Error verifying module: %s\n", error);
        LLVMDisposeMessage(error);
        return false;
    }

    if (error) LLVMDisposeMessage(error);
    return true;
}

// Write the generated module to a file
bool goo_codegen_write_to_file(GooCodegenContext* context, const char* filename) {
    if (!context || !context->module || !filename) return false;

    char* error = NULL;
    if (LLVMPrintModuleToFile(context->module, filename, &error)) {
        fprintf(stderr, "Error writing module to file: %s\n", error);
        LLVMDisposeMessage(error);
        return false;
    }

    return true;
}

// Function for implementing a Goo function node
LLVMValueRef goo_codegen_function(GooCodegenContext* context, GooFunctionNode* node) {
    if (!context || !node) return NULL;

    // Get function name
    char* func_name = node->name->name;
    
    // Check if function already exists
    LLVMValueRef existing_func = LLVMGetNamedFunction(context->module, func_name);
    if (existing_func) {
        fprintf(stderr, "Function %s already defined\n", func_name);
        return existing_func;
    }
    
    // Create parameter types
    unsigned param_count = 0;
    GooNode* param = node->params;
    while (param) {
        param_count++;
        param = param->next;
    }
    
    LLVMTypeRef* param_types = NULL;
    if (param_count > 0) {
        param_types = (LLVMTypeRef*)malloc(param_count * sizeof(LLVMTypeRef));
        if (!param_types) {
            fprintf(stderr, "Failed to allocate memory for parameter types\n");
            return NULL;
        }
    }
    
    // Process parameter types
    param = node->params;
    for (unsigned i = 0; i < param_count && param; i++, param = param->next) {
        GooParamNode* param_node = (GooParamNode*)param;
        param_types[i] = goo_type_to_llvm_type(context, param_node->type);
    }
    
    // Get return type
    LLVMTypeRef return_type;
    if (node->return_type) {
        return_type = goo_type_to_llvm_type(context, node->return_type);
    } else {
        // Default to void if no return type is specified
        return_type = LLVMVoidTypeInContext(context->context);
    }
    
    // Create function type
    LLVMTypeRef func_type = LLVMFunctionType(return_type, param_types, param_count, 0);
    
    // Create function
    LLVMValueRef function = LLVMAddFunction(context->module, func_name, func_type);
    
    // Add function to symbol table
    goo_symbol_table_add(context->symbol_table, func_name, GOO_SYMBOL_FUNCTION, function, (GooNode*)node, func_type);
    
    // Free parameter types
    if (param_types) {
        free(param_types);
    }
    
    // If this is just a function declaration (no body), return now
    if (!node->body) {
        return function;
    }
    
    // Create a new basic block for the function body
    LLVMBasicBlockRef entry = LLVMAppendBasicBlockInContext(context->context, function, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry);
    
    // Create a new scope for the function
    goo_symbol_table_enter_scope(context->symbol_table, true);
    
    // Add parameters to symbol table
    param = node->params;
    for (unsigned i = 0; i < param_count && param; i++, param = param->next) {
        GooParamNode* param_node = (GooParamNode*)param;
        LLVMValueRef param_value = LLVMGetParam(function, i);
        
        // Set parameter name
        LLVMSetValueName(param_value, param_node->name->name);
        
        // Allocate space for the parameter on the stack
        LLVMValueRef param_alloca = LLVMBuildAlloca(context->builder, param_types[i], param_node->name->name);
        LLVMBuildStore(context->builder, param_value, param_alloca);
        
        // Add parameter to symbol table
        goo_symbol_table_add(context->symbol_table, param_node->name->name, GOO_SYMBOL_VARIABLE, param_alloca, 
                          (GooNode*)param_node, param_types[i]);
    }
    
    // Generate code for function body
    LLVMValueRef body_value = goo_codegen_block(context, node->body);
    
    // Add a return instruction if the function doesn't have one
    if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(context->builder))) {
        if (return_type == LLVMVoidTypeInContext(context->context)) {
            LLVMBuildRetVoid(context->builder);
        } else {
            // Default return value (this is a bit of a hack)
            LLVMValueRef default_return;
            if (return_type == LLVMInt32TypeInContext(context->context)) {
                default_return = LLVMConstInt(return_type, 0, 0);
            } else if (return_type == LLVMDoubleTypeInContext(context->context)) {
                default_return = LLVMConstReal(return_type, 0.0);
            } else if (return_type == LLVMInt1TypeInContext(context->context)) {
                default_return = LLVMConstInt(return_type, 0, 0);
            } else {
                // For other types, just return null
                default_return = LLVMConstNull(return_type);
            }
            LLVMBuildRet(context->builder, default_return);
        }
    }
    
    // Exit function scope
    goo_symbol_table_exit_scope(context->symbol_table);
    
    // Verify the function
    char* error = NULL;
    LLVMVerifyFunction(function, LLVMPrintMessageAction, &error);
    if (error) {
        fprintf(stderr, "Error verifying function: %s\n", error);
        LLVMDisposeMessage(error);
    }
    
    return function;
}

// Function for implementing a block statement
LLVMValueRef goo_codegen_block(GooCodegenContext* context, GooBlockStmtNode* node) {
    if (!context || !node) return NULL;
    
    // Create a new scope for the block
    goo_symbol_table_enter_scope(context->symbol_table, false);
    
    // Generate code for each statement in the block
    LLVMValueRef last_value = NULL;
    GooNode* stmt = node->statements;
    
    while (stmt) {
        last_value = goo_codegen_node(context, stmt);
        stmt = stmt->next;
    }
    
    // Exit block scope
    goo_symbol_table_exit_scope(context->symbol_table);
    
    return last_value;
}

// Function for implementing a variable declaration
LLVMValueRef goo_codegen_var_decl(GooCodegenContext* context, GooVarDeclNode* node) {
    if (!context || !node) return NULL;
    
    // Get variable name
    char* var_name = node->name->name;
    
    // Check if variable already exists in current scope
    if (goo_symbol_table_lookup_current_scope(context->symbol_table, var_name)) {
        fprintf(stderr, "Variable %s already defined in current scope\n", var_name);
        return NULL;
    }
    
    // Get variable type
    LLVMTypeRef var_type;
    if (node->type_node) {
        var_type = goo_type_to_llvm_type(context, node->type_node);
    } else if (node->init_expr) {
        // If no type is specified but there's an initializer, infer type from initializer
        // This is a bit of a hack, since we don't have a proper type inference system yet
        fprintf(stderr, "Type inference not yet implemented, defaulting to int\n");
        var_type = LLVMInt32TypeInContext(context->context);
    } else {
        // Default to int if no type or initializer is specified
        var_type = LLVMInt32TypeInContext(context->context);
    }
    
    // Allocate space for the variable on the stack
    LLVMValueRef var_alloca = LLVMBuildAlloca(context->builder, var_type, var_name);
    
    // Add the variable to the symbol table
    goo_symbol_table_add(context->symbol_table, var_name, GOO_SYMBOL_VARIABLE, var_alloca, 
                       (GooNode*)node, var_type);
    
    // If there's an initializer expression, generate code for it and assign to the variable
    if (node->init_expr) {
        LLVMValueRef init_value = goo_codegen_node(context, node->init_expr);
        if (!init_value) {
            fprintf(stderr, "Failed to generate code for initializer expression\n");
            return var_alloca;
        }
        
        // Store the initializer value in the variable
        LLVMBuildStore(context->builder, init_value, var_alloca);
    }
    
    return var_alloca;
}

// Function for implementing an if statement
LLVMValueRef goo_codegen_if(GooCodegenContext* context, GooIfStmtNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the condition
    LLVMValueRef condition = goo_codegen_node(context, node->condition);
    if (!condition) {
        fprintf(stderr, "Failed to generate code for if condition\n");
        return NULL;
    }
    
    // Create basic blocks for the then, else, and merge parts
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
    LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
    
    LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
    LLVMBasicBlockRef else_block = NULL;
    if (node->else_block) {
        else_block = LLVMAppendBasicBlock(function, "else");
    }
    LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");
    
    // Create the conditional branch
    if (node->else_block) {
        LLVMBuildCondBr(context->builder, condition, then_block, else_block);
    } else {
        LLVMBuildCondBr(context->builder, condition, then_block, merge_block);
    }
    
    // Generate code for the then block
    LLVMPositionBuilderAtEnd(context->builder, then_block);
    LLVMValueRef then_value = goo_codegen_node(context, (GooNode*)node->then_block);
    if (!then_value) {
        fprintf(stderr, "Failed to generate code for then block\n");
        return NULL;
    }
    
    // Add a branch to the merge block if there isn't one already
    if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(context->builder))) {
        LLVMBuildBr(context->builder, merge_block);
    }
    
    // Remember the end of the then block to use for a potential phi node
    LLVMBasicBlockRef then_end_block = LLVMGetInsertBlock(context->builder);
    
    // Generate code for the else block if it exists
    LLVMBasicBlockRef else_end_block = NULL;
    LLVMValueRef else_value = NULL;
    if (node->else_block) {
        LLVMPositionBuilderAtEnd(context->builder, else_block);
        else_value = goo_codegen_node(context, (GooNode*)node->else_block);
        if (!else_value) {
            fprintf(stderr, "Failed to generate code for else block\n");
            return NULL;
        }
        
        // Add a branch to the merge block if there isn't one already
        if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(context->builder))) {
            LLVMBuildBr(context->builder, merge_block);
        }
        
        // Remember the end of the else block to use for a potential phi node
        else_end_block = LLVMGetInsertBlock(context->builder);
    }
    
    // Generate code for the merge block
    LLVMPositionBuilderAtEnd(context->builder, merge_block);
    
    // If both the then and else blocks produced values and they are of the same type,
    // create a phi node to combine them
    if (then_value && else_value && 
        LLVMTypeOf(then_value) == LLVMTypeOf(else_value) && 
        LLVMTypeOf(then_value) != LLVMVoidTypeInContext(context->context)) {
        
        LLVMValueRef phi = LLVMBuildPhi(context->builder, LLVMTypeOf(then_value), "ifresult");
        
        LLVMValueRef values[] = { then_value, else_value };
        LLVMBasicBlockRef blocks[] = { then_end_block, else_end_block };
        LLVMAddIncoming(phi, values, blocks, 2);
        
        return phi;
    }
    
    // Otherwise, just return a dummy value (most if statements don't produce values in Goo)
    return LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0);
}

// Function for implementing a for statement
LLVMValueRef goo_codegen_for(GooCodegenContext* context, GooForStmtNode* node) {
    if (!context || !node) return NULL;
    
    // Get the current function and basic block
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
    LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
    
    // Create basic blocks for the loop
    LLVMBasicBlockRef preheader_block = LLVMAppendBasicBlock(function, "preheader");
    LLVMBasicBlockRef loop_block = LLVMAppendBasicBlock(function, "loop");
    LLVMBasicBlockRef after_block = LLVMAppendBasicBlock(function, "after");
    
    // Generate code for the initialization if it exists
    if (node->init) {
        LLVMValueRef init_value = goo_codegen_node(context, node->init);
        if (!init_value) {
            fprintf(stderr, "Failed to generate code for loop initialization\n");
            return NULL;
        }
    }
    
    // Branch to the preheader block
    LLVMBuildBr(context->builder, preheader_block);
    
    // Generate code for the preheader block
    LLVMPositionBuilderAtEnd(context->builder, preheader_block);
    
    // Generate code for the condition if it exists
    LLVMValueRef condition = NULL;
    if (node->condition) {
        condition = goo_codegen_node(context, node->condition);
        if (!condition) {
            fprintf(stderr, "Failed to generate code for loop condition\n");
            return NULL;
        }
    } else {
        // If no condition is specified, create a constant true condition
        condition = LLVMConstInt(LLVMInt1TypeInContext(context->context), 1, 0);
    }
    
    // Create the conditional branch to either the loop block or after block
    LLVMBuildCondBr(context->builder, condition, loop_block, after_block);
    
    // Generate code for the loop block
    LLVMPositionBuilderAtEnd(context->builder, loop_block);
    
    // Generate code for the loop body
    LLVMValueRef body_value = goo_codegen_node(context, (GooNode*)node->body);
    if (!body_value) {
        fprintf(stderr, "Failed to generate code for loop body\n");
        return NULL;
    }
    
    // Generate code for the increment if it exists
    if (node->increment) {
        LLVMValueRef increment_value = goo_codegen_node(context, node->increment);
        if (!increment_value) {
            fprintf(stderr, "Failed to generate code for loop increment\n");
            return NULL;
        }
    }
    
    // Branch back to the preheader block
    LLVMBuildBr(context->builder, preheader_block);
    
    // Generate code for the after block
    LLVMPositionBuilderAtEnd(context->builder, after_block);
    
    // Return a dummy value (for statements don't produce values in Goo)
    return LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0);
}

// Function for implementing a return statement
LLVMValueRef goo_codegen_return(GooCodegenContext* context, GooReturnStmtNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the return value if it exists
    LLVMValueRef return_value = NULL;
    if (node->value) {
        return_value = goo_codegen_node(context, node->value);
        if (!return_value) {
            fprintf(stderr, "Failed to generate code for return value\n");
            return NULL;
        }
    }
    
    // Build the return instruction
    if (return_value) {
        return LLVMBuildRet(context->builder, return_value);
    } else {
        return LLVMBuildRetVoid(context->builder);
    }
}

// Function for implementing a binary expression
LLVMValueRef goo_codegen_binary_expr(GooCodegenContext* context, GooBinaryExprNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the left and right operands
    LLVMValueRef left = goo_codegen_node(context, node->left);
    LLVMValueRef right = goo_codegen_node(context, node->right);
    
    if (!left || !right) {
        fprintf(stderr, "Failed to generate code for binary expression operands\n");
        return NULL;
    }
    
    // Generate code for the binary operation
    switch (node->op) {
        // Arithmetic operations
        case '+':
            return LLVMBuildAdd(context->builder, left, right, "addtmp");
        case '-':
            return LLVMBuildSub(context->builder, left, right, "subtmp");
        case '*':
            return LLVMBuildMul(context->builder, left, right, "multmp");
        case '/':
            // Check for division by zero
            LLVMValueRef zero = LLVMConstInt(LLVMTypeOf(right), 0, 0);
            LLVMValueRef is_zero = LLVMBuildICmp(context->builder, LLVMIntEQ, right, zero, "divzero");
            
            // Create basic blocks for the if-then-else branch
            LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
            LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
            LLVMBasicBlockRef then_block = LLVMAppendBasicBlock(function, "then");
            LLVMBasicBlockRef else_block = LLVMAppendBasicBlock(function, "else");
            LLVMBasicBlockRef merge_block = LLVMAppendBasicBlock(function, "merge");
            
            // Create the conditional branch
            LLVMBuildCondBr(context->builder, is_zero, then_block, else_block);
            
            // Generate code for the then block (division by zero)
            LLVMPositionBuilderAtEnd(context->builder, then_block);
            // For simplicity, we'll just return a 0 value here
            // In a real implementation, we'd want to throw an error or handle it more gracefully
            LLVMValueRef zero_result = LLVMConstInt(LLVMTypeOf(left), 0, 0);
            LLVMBuildBr(context->builder, merge_block);
            
            // Generate code for the else block (normal division)
            LLVMPositionBuilderAtEnd(context->builder, else_block);
            LLVMValueRef div_result = LLVMBuildSDiv(context->builder, left, right, "divtmp");
            LLVMBuildBr(context->builder, merge_block);
            
            // Generate code for the merge block
            LLVMPositionBuilderAtEnd(context->builder, merge_block);
            LLVMValueRef phi = LLVMBuildPhi(context->builder, LLVMTypeOf(left), "divresult");
            LLVMValueRef phi_values[] = { zero_result, div_result };
            LLVMBasicBlockRef phi_blocks[] = { then_block, else_block };
            LLVMAddIncoming(phi, phi_values, phi_blocks, 2);
            
            return phi;
            
        case '%':
            return LLVMBuildSRem(context->builder, left, right, "modtmp");
            
        // Comparison operations
        case '<':
            return LLVMBuildICmp(context->builder, LLVMIntSLT, left, right, "lttmp");
        case '>':
            return LLVMBuildICmp(context->builder, LLVMIntSGT, left, right, "gttmp");
        case GOO_TOKEN_LE:
            return LLVMBuildICmp(context->builder, LLVMIntSLE, left, right, "letmp");
        case GOO_TOKEN_GE:
            return LLVMBuildICmp(context->builder, LLVMIntSGE, left, right, "getmp");
        case GOO_TOKEN_EQ:
            return LLVMBuildICmp(context->builder, LLVMIntEQ, left, right, "eqtmp");
        case GOO_TOKEN_NE:
            return LLVMBuildICmp(context->builder, LLVMIntNE, left, right, "netmp");
            
        // Logical operations
        case GOO_TOKEN_AND:
            return LLVMBuildAnd(context->builder, left, right, "andtmp");
        case GOO_TOKEN_OR:
            return LLVMBuildOr(context->builder, left, right, "ortmp");
            
        // Bitwise operations
        case '&':
            return LLVMBuildAnd(context->builder, left, right, "bitandtmp");
        case '|':
            return LLVMBuildOr(context->builder, left, right, "bitortmp");
        case '^':
            return LLVMBuildXor(context->builder, left, right, "bitxortmp");
        case GOO_TOKEN_LSHIFT:
            return LLVMBuildShl(context->builder, left, right, "lshifttmp");
        case GOO_TOKEN_RSHIFT:
            return LLVMBuildLShr(context->builder, left, right, "rshifttmp");
            
        default:
            fprintf(stderr, "Unsupported binary operator: %d\n", node->op);
            return NULL;
    }
}

// Function for implementing a unary expression
LLVMValueRef goo_codegen_unary_expr(GooCodegenContext* context, GooUnaryExprNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the operand
    LLVMValueRef operand = goo_codegen_node(context, node->expr);
    
    if (!operand) {
        fprintf(stderr, "Failed to generate code for unary expression operand\n");
        return NULL;
    }
    
    // Generate code for the unary operation
    switch (node->op) {
        case '-':
            return LLVMBuildNeg(context->builder, operand, "negtmp");
        case '!':
            return LLVMBuildNot(context->builder, operand, "nottmp");
        case '~':
            return LLVMBuildNot(context->builder, operand, "complmenttmp");
        case '*': // Dereference (for pointers)
            return LLVMBuildLoad(context->builder, operand, "deref");
        case '&': // Address-of
            // This assumes that the operand is an LValue
            // For variables, we can just return the alloca itself
            if (node->expr->type == GOO_NODE_IDENTIFIER) {
                GooIdentifierNode* id_node = (GooIdentifierNode*)node->expr;
                GooSymbol* symbol = goo_symbol_table_lookup(context->symbol_table, id_node->name);
                if (symbol && symbol->kind == GOO_SYMBOL_VARIABLE) {
                    return symbol->llvm_value;
                }
            }
            fprintf(stderr, "Cannot take address of non-lvalue\n");
            return NULL;
            
        default:
            fprintf(stderr, "Unsupported unary operator: %d\n", node->op);
            return NULL;
    }
}

// Function for implementing a function call
LLVMValueRef goo_codegen_call_expr(GooCodegenContext* context, GooCallExprNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the function expression (either an identifier or another expression)
    LLVMValueRef callee = goo_codegen_node(context, node->func);
    
    if (!callee) {
        fprintf(stderr, "Failed to generate code for function callee\n");
        return NULL;
    }
    
    // Check if the callee is a valid function
    if (!LLVMIsAFunction(callee)) {
        fprintf(stderr, "Callee is not a function\n");
        return NULL;
    }
    
    // Count the number of arguments
    unsigned arg_count = 0;
    GooNode* arg = node->args;
    while (arg) {
        arg_count++;
        arg = arg->next;
    }
    
    // Check if the argument count matches the function's parameter count
    unsigned param_count = LLVMCountParams(callee);
    if (arg_count != param_count) {
        fprintf(stderr, "Function call with wrong number of arguments (expected %u, got %u)\n", param_count, arg_count);
        return NULL;
    }
    
    // Generate code for each argument
    LLVMValueRef* args = NULL;
    if (arg_count > 0) {
        args = (LLVMValueRef*)malloc(arg_count * sizeof(LLVMValueRef));
        if (!args) {
            fprintf(stderr, "Failed to allocate memory for arguments\n");
            return NULL;
        }
        
        arg = node->args;
        for (unsigned i = 0; i < arg_count && arg; i++, arg = arg->next) {
            args[i] = goo_codegen_node(context, arg);
            if (!args[i]) {
                fprintf(stderr, "Failed to generate code for argument %u\n", i);
                free(args);
                return NULL;
            }
        }
    }
    
    // Generate the function call
    LLVMValueRef call = LLVMBuildCall(context->builder, callee, args, arg_count, "calltmp");
    
    // Free the arguments array
    if (args) {
        free(args);
    }
    
    return call;
}

// Function for implementing literal values
LLVMValueRef goo_codegen_literal(GooCodegenContext* context, GooNode* node) {
    if (!context || !node) return NULL;
    
    switch (node->type) {
        case GOO_NODE_INT_LITERAL: {
            GooIntLiteralNode* int_node = (GooIntLiteralNode*)node;
            return LLVMConstInt(LLVMInt32TypeInContext(context->context), int_node->value, 0);
        }
        
        case GOO_NODE_FLOAT_LITERAL: {
            GooFloatLiteralNode* float_node = (GooFloatLiteralNode*)node;
            return LLVMConstReal(LLVMDoubleTypeInContext(context->context), float_node->value);
        }
        
        case GOO_NODE_STRING_LITERAL: {
            GooStringLiteralNode* str_node = (GooStringLiteralNode*)node;
            return LLVMBuildGlobalStringPtr(context->builder, str_node->value, "str");
        }
        
        case GOO_NODE_BOOL_LITERAL: {
            GooBoolLiteralNode* bool_node = (GooBoolLiteralNode*)node;
            return LLVMConstInt(LLVMInt1TypeInContext(context->context), bool_node->value ? 1 : 0, 0);
        }
        
        default:
            fprintf(stderr, "Unsupported literal type: %d\n", node->type);
            return NULL;
    }
}

// Function for implementing identifier references
LLVMValueRef goo_codegen_identifier(GooCodegenContext* context, GooNode* node) {
    if (!context || !node || node->type != GOO_NODE_IDENTIFIER) return NULL;
    
    GooIdentifierNode* id_node = (GooIdentifierNode*)node;
    char* name = id_node->name;
    
    // Look up the identifier in the symbol table
    GooSymbol* symbol = goo_symbol_table_lookup(context->symbol_table, name);
    if (!symbol) {
        fprintf(stderr, "Undefined identifier: %s\n", name);
        return NULL;
    }
    
    // Handle the different kinds of symbols
    switch (symbol->kind) {
        case GOO_SYMBOL_VARIABLE: {
            // For variables, load the value from the alloca
            return LLVMBuildLoad(context->builder, symbol->llvm_value, name);
        }
        
        case GOO_SYMBOL_FUNCTION: {
            // For functions, just return the function value
            return symbol->llvm_value;
        }
        
        default:
            fprintf(stderr, "Unsupported symbol kind: %d\n", symbol->kind);
            return NULL;
    }
}

// Function for implementing range literals
LLVMValueRef goo_codegen_range_literal(GooCodegenContext* context, GooRangeLiteralNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the start and end values
    LLVMValueRef start = goo_codegen_node(context, node->start);
    LLVMValueRef end = goo_codegen_node(context, node->end);
    
    if (!start || !end) {
        fprintf(stderr, "Failed to generate code for range bounds\n");
        return NULL;
    }
    
    // Create a struct type for the range
    LLVMTypeRef range_type = LLVMStructTypeInContext(context->context, 
        (LLVMTypeRef[]){LLVMTypeOf(start), LLVMTypeOf(end)}, 2, 0);
    
    // Allocate space for the range struct
    LLVMValueRef range = LLVMBuildAlloca(context->builder, range_type, "range");
    
    // Store the start and end values into the struct
    LLVMValueRef start_ptr = LLVMBuildStructGEP(context->builder, range, 0, "start_ptr");
    LLVMValueRef end_ptr = LLVMBuildStructGEP(context->builder, range, 1, "end_ptr");
    
    LLVMBuildStore(context->builder, start, start_ptr);
    LLVMBuildStore(context->builder, end, end_ptr);
    
    return range;
}

// Enhanced channel send operation with memory management
LLVMValueRef goo_codegen_channel_send(GooCodegenContext* context, GooChannelSendNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the channel and expression
    LLVMValueRef channel = goo_codegen_node(context, node->channel);
    LLVMValueRef expr = goo_codegen_node(context, node->value);
    
    if (!channel || !expr) {
        fprintf(stderr, "Failed to generate code for channel send\n");
        return NULL;
    }
    
    // Get expression type and size
    LLVMTypeRef expr_type = LLVMTypeOf(expr);
    LLVMValueRef expr_size = LLVMSizeOf(expr_type);
    
    // Allocate memory for the data using the Zig allocator
    LLVMTypeRef alloc_func_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
        (LLVMTypeRef[]){LLVMInt64TypeInContext(context->context)},
        1,
        false
    );
    
    LLVMValueRef alloc_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_alloc",
        alloc_func_type
    );
    
    if (!alloc_func) {
        fprintf(stderr, "Failed to find goo_alloc function\n");
        return NULL;
    }
    
    // Allocate memory for the value
    LLVMValueRef buffer = LLVMBuildCall2(
        context->builder,
        alloc_func_type,
        alloc_func,
        (LLVMValueRef[]){expr_size},
        1,
        "send_buffer"
    );
    
    // Cast buffer to expression type pointer
    LLVMValueRef typed_buffer = LLVMBuildBitCast(
        context->builder,
        buffer,
        LLVMPointerType(expr_type, 0),
        "typed_buffer"
    );
    
    // Store the expression value to the buffer
    LLVMBuildStore(context->builder, expr, typed_buffer);
    
    // Generate message flags
    LLVMValueRef flags = LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, false);
    
    // Call channel send function
    LLVMTypeRef send_func_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context),
        (LLVMTypeRef[]){
            LLVMPointerType(goo_type_table_get_type(context->type_table, "GooChannel"), 0),
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
            LLVMInt64TypeInContext(context->context),
            LLVMInt32TypeInContext(context->context)
        },
        4,
        false
    );
    
    LLVMValueRef send_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_channel_send",
        send_func_type
    );
    
    if (!send_func) {
        fprintf(stderr, "Failed to find goo_channel_send function\n");
        return NULL;
    }
    
    LLVMValueRef result = LLVMBuildCall2(
        context->builder,
        send_func_type,
        send_func,
        (LLVMValueRef[]){channel, buffer, expr_size, flags},
        4,
        "send_result"
    );
    
    // Create a deferred free for the buffer after sending
    // This requires runtime support for deferred cleanup, or a try-finally block
    LLVMTypeRef free_func_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context),
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
            LLVMInt64TypeInContext(context->context)
        },
        2,
        false
    );
    
    LLVMValueRef free_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_free",
        free_func_type
    );
    
    if (free_func) {
        // Create a basic block for cleanup
        LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
        LLVMValueRef current_func = LLVMGetBasicBlockParent(current_block);
        LLVMBasicBlockRef cleanup_block = LLVMAppendBasicBlock(current_func, "channel_send_cleanup");
        
        // Insert a branch to the cleanup block
        LLVMBuildBr(context->builder, cleanup_block);
        
        // Position builder at the cleanup block
        LLVMPositionBuilderAtEnd(context->builder, cleanup_block);
        
        // Free the buffer
        LLVMBuildCall2(
            context->builder,
            free_func_type,
            free_func,
            (LLVMValueRef[]){buffer, expr_size},
            2,
            ""
        );
        
        // Return the result
        LLVMBuildRet(context->builder, result);
    }
    
    return result;
}

// Enhanced channel receive operation with memory management
LLVMValueRef goo_codegen_channel_recv(GooCodegenContext* context, GooChannelRecvNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the channel
    LLVMValueRef channel = goo_codegen_node(context, node->channel);
    
    if (!channel) {
        fprintf(stderr, "Failed to generate code for channel receive\n");
        return NULL;
    }
    
    // Get the expected type
    LLVMTypeRef expected_type = node->expected_type ? 
        goo_type_to_llvm_type(context, node->expected_type) : 
        LLVMInt8TypeInContext(context->context);
    
    // Get the size of the expected type
    LLVMValueRef type_size = LLVMSizeOf(expected_type);
    
    // Allocate memory for the received data using the Zig allocator
    LLVMTypeRef alloc_func_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
        (LLVMTypeRef[]){LLVMInt64TypeInContext(context->context)},
        1,
        false
    );
    
    LLVMValueRef alloc_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_alloc",
        alloc_func_type
    );
    
    if (!alloc_func) {
        fprintf(stderr, "Failed to find goo_alloc function\n");
        return NULL;
    }
    
    // Allocate memory for the received value
    LLVMValueRef buffer = LLVMBuildCall2(
        context->builder,
        alloc_func_type,
        alloc_func,
        (LLVMValueRef[]){type_size},
        1,
        "recv_buffer"
    );
    
    // Generate message flags
    LLVMValueRef flags = LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, false);
    
    // Allocate memory for the received size
    LLVMValueRef size_ptr = LLVMBuildAlloca(
        context->builder,
        LLVMInt64TypeInContext(context->context),
        "received_size"
    );
    
    // Call channel receive function
    LLVMTypeRef recv_func_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context),
        (LLVMTypeRef[]){
            LLVMPointerType(goo_type_table_get_type(context->type_table, "GooChannel"), 0),
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
            LLVMInt64TypeInContext(context->context),
            LLVMPointerType(LLVMInt64TypeInContext(context->context), 0),
            LLVMInt32TypeInContext(context->context)
        },
        5,
        false
    );
    
    LLVMValueRef recv_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_channel_receive",
        recv_func_type
    );
    
    if (!recv_func) {
        fprintf(stderr, "Failed to find goo_channel_receive function\n");
        return NULL;
    }
    
    // Call receive function
    LLVMValueRef recv_result = LLVMBuildCall2(
        context->builder,
        recv_func_type,
        recv_func,
        (LLVMValueRef[]){channel, buffer, type_size, size_ptr, flags},
        5,
        "recv_result"
    );
    
    // Create basic blocks for handling success and failure
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
    LLVMValueRef current_func = LLVMGetBasicBlockParent(current_block);
    
    LLVMBasicBlockRef success_block = LLVMAppendBasicBlock(current_func, "recv_success");
    LLVMBasicBlockRef failure_block = LLVMAppendBasicBlock(current_func, "recv_failure");
    LLVMBasicBlockRef cleanup_block = LLVMAppendBasicBlock(current_func, "recv_cleanup");
    
    // Create conditional branch based on receive result
    LLVMBuildCondBr(context->builder, recv_result, success_block, failure_block);
    
    // In success block, cast buffer to expected type and load value
    LLVMPositionBuilderAtEnd(context->builder, success_block);
    LLVMValueRef typed_buffer = LLVMBuildBitCast(
        context->builder,
        buffer,
        LLVMPointerType(expected_type, 0),
        "typed_recv_buffer"
    );
    
    LLVMValueRef loaded_value = LLVMBuildLoad2(
        context->builder,
        expected_type,
        typed_buffer,
        "received_value"
    );
    
    // Store loaded value in a temporary location to return it after cleanup
    LLVMValueRef result_store = LLVMBuildAlloca(context->builder, expected_type, "result_store");
    LLVMBuildStore(context->builder, loaded_value, result_store);
    
    // Branch to cleanup
    LLVMBuildBr(context->builder, cleanup_block);
    
    // In failure block, create a default value
    LLVMPositionBuilderAtEnd(context->builder, failure_block);
    
    // Create appropriate default value based on type
    LLVMValueRef default_value;
    switch (LLVMGetTypeKind(expected_type)) {
        case LLVMIntegerTypeKind:
            default_value = LLVMConstInt(expected_type, 0, false);
            break;
        case LLVMFloatTypeKind:
        case LLVMDoubleTypeKind:
            default_value = LLVMConstReal(expected_type, 0.0);
            break;
        case LLVMPointerTypeKind:
            default_value = LLVMConstNull(expected_type);
            break;
        default:
            // For other types, use null or zero as appropriate
            default_value = LLVMConstNull(expected_type);
    }
    
    // Store default value
    LLVMValueRef default_store = LLVMBuildAlloca(context->builder, expected_type, "default_store");
    LLVMBuildStore(context->builder, default_value, default_store);
    
    // Branch to cleanup
    LLVMBuildBr(context->builder, cleanup_block);
    
    // In cleanup block, free the buffer
    LLVMPositionBuilderAtEnd(context->builder, cleanup_block);
    
    // Create a PHI node to select between result_store and default_store
    LLVMValueRef phi = LLVMBuildPhi(context->builder, LLVMPointerType(expected_type, 0), "result_phi");
    
    // Add incoming values to PHI
    LLVMAddIncoming(phi, &result_store, &success_block, 1);
    LLVMAddIncoming(phi, &default_store, &failure_block, 1);
    
    // Free the buffer using goo_free
    LLVMTypeRef free_func_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context),
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
            LLVMInt64TypeInContext(context->context)
        },
        2,
        false
    );
    
    LLVMValueRef free_func = goo_symbol_table_get_function(
        context->symbol_table,
        "goo_free",
        free_func_type
    );
    
    if (free_func) {
        LLVMBuildCall2(
            context->builder,
            free_func_type,
            free_func,
            (LLVMValueRef[]){buffer, type_size},
            2,
            ""
        );
    }
    
    // Load the final result from PHI
    return LLVMBuildLoad2(context->builder, expected_type, phi, "final_result");
}

// Function for implementing goroutines
LLVMValueRef goo_codegen_go_stmt(GooCodegenContext* context, GooGoStmtNode* node) {
    if (!context || !node) return NULL;

    // Create a wrapper function for the goroutine
    // This will capture the required variables and provide a standard signature for the runtime
    
    // First, determine if we need to wrap a function call or an expression
    bool is_call = node->expr->type == GOO_NODE_CALL_EXPR;
    GooCallExprNode* call_node = is_call ? (GooCallExprNode*)node->expr : NULL;
    
    // Generate a unique name for the wrapper function
    char wrapper_name[128];
    static int go_counter = 0;
    snprintf(wrapper_name, sizeof(wrapper_name), "__goo_goroutine_%d", go_counter++);
    
    // Create the wrapper function type (void (*)(void*))
    LLVMTypeRef void_type = LLVMVoidTypeInContext(context->context);
    LLVMTypeRef void_ptr_type = LLVMPointerType(LLVMInt8TypeInContext(context->context), 0);
    LLVMTypeRef param_types[] = { void_ptr_type };
    LLVMTypeRef func_type = LLVMFunctionType(void_type, param_types, 1, false);
    
    // Create the wrapper function
    LLVMValueRef wrapper_func = LLVMAddFunction(context->module, wrapper_name, func_type);
    
    // Add capability metadata to the wrapper function
    // This will ensure that the goroutine has the same capabilities as the parent function
    LLVMBasicBlockRef current_block = LLVMGetInsertBlock(context->builder);
    LLVMValueRef current_function = LLVMGetBasicBlockParent(current_block);
    
    // Apply the same capability attributes as the parent function
    if (LLVMGetValueName(current_function)) {
        // Copy attributes from parent function to wrapper
        // In a real implementation, we'd have a proper capability system that
        // would propagate capabilities through metadata
        LLVMCopyFunctionAttributes(wrapper_func, current_function);
    }
    
    // Create entry block for wrapper function
    LLVMBasicBlockRef entry_block = LLVMAppendBasicBlock(wrapper_func, "entry");
    LLVMPositionBuilderAtEnd(context->builder, entry_block);
    
    // Get the function argument (closure data)
    LLVMValueRef closure_ptr = LLVMGetParam(wrapper_func, 0);
    
    // Add cleanup block for proper resource management
    LLVMBasicBlockRef cleanup_block = LLVMAppendBasicBlock(wrapper_func, "cleanup");
    
    // If it's a function call, we need to extract arguments from the closure
    if (is_call) {
        // Count the number of arguments
        int arg_count = 0;
        GooNode* arg = call_node->args;
        while (arg) {
            arg_count++;
            arg = arg->next;
        }
        
        // Create a struct type for the closure
        LLVMTypeRef* field_types = (LLVMTypeRef*)malloc((arg_count) * sizeof(LLVMTypeRef));
        if (!field_types) {
            fprintf(stderr, "Failed to allocate memory for closure field types\n");
        return NULL;
    }
    
        // Initialize with argument types
        arg = call_node->args;
        for (int i = 0; i < arg_count; i++) {
            // Generate code for the argument to determine its type
            LLVMValueRef arg_value = goo_codegen_node(context, arg);
            field_types[i] = LLVMTypeOf(arg_value);
            arg = arg->next;
        }
        
        // Create the closure struct type
        LLVMTypeRef closure_type = LLVMStructTypeInContext(context->context, field_types, arg_count, false);
        free(field_types);
        
        // Cast closure pointer to our struct type
        LLVMValueRef closure_struct_ptr = LLVMBuildBitCast(
            context->builder,
            closure_ptr,
            LLVMPointerType(closure_type, 0),
            "closure_struct"
        );
        
        // Extract arguments from the closure
        LLVMValueRef* args = (LLVMValueRef*)malloc(arg_count * sizeof(LLVMValueRef));
        if (!args) {
            fprintf(stderr, "Failed to allocate memory for extracted arguments\n");
        return NULL;
    }
    
        for (int i = 0; i < arg_count; i++) {
            // Get a pointer to the field
            LLVMValueRef field_ptr = LLVMBuildStructGEP(
                context->builder,
                closure_struct_ptr,
                i,
                "arg_ptr"
            );
            
            // Load the value from the field
            args[i] = LLVMBuildLoad(
                context->builder,
                field_ptr,
                "arg"
            );
        }
        
        // Generate the actual function call
        LLVMValueRef func = goo_codegen_node(context, call_node->func);
        
        // Check if func is valid - add safety check
        LLVMValueRef is_valid_func = LLVMBuildICmp(
            context->builder,
            LLVMIntNE,
            func,
            LLVMConstNull(LLVMTypeOf(func)),
            "is_valid_func"
        );
        
        // Create blocks for the function validity check
        LLVMBasicBlockRef valid_func_block = LLVMAppendBasicBlock(wrapper_func, "valid_func");
        LLVMBasicBlockRef invalid_func_block = LLVMAppendBasicBlock(wrapper_func, "invalid_func");
        
        // Branch based on function validity
        LLVMBuildCondBr(context->builder, is_valid_func, valid_func_block, invalid_func_block);
        
        // Build the valid func block
        LLVMPositionBuilderAtEnd(context->builder, valid_func_block);
        
        // Call the function
        LLVMValueRef result = LLVMBuildCall(
            context->builder,
            func,
            args,
            arg_count,
            ""
        );
        
        // Branch to cleanup
        LLVMBuildBr(context->builder, cleanup_block);
        
        // Build the invalid func block with error handling
        LLVMPositionBuilderAtEnd(context->builder, invalid_func_block);
        
        // Call runtime panic function for invalid function pointer
        LLVMValueRef runtime_panic_func = LLVMGetNamedFunction(context->module, "goo_runtime_panic");
        if (!runtime_panic_func) {
            // Declare the panic function if not already declared
            LLVMTypeRef panic_type = LLVMFunctionType(
                void_type,
                &void_ptr_type,
                1,
                false
            );
            runtime_panic_func = LLVMAddFunction(context->module, "goo_runtime_panic", panic_type);
        }
        
        // Create error message for invalid function
        LLVMValueRef error_msg = LLVMBuildGlobalStringPtr(
            context->builder,
            "Null function pointer in goroutine",
            "error_msg"
        );
        
        // Call panic
        LLVMValueRef panic_args[] = { error_msg };
        LLVMBuildCall(
            context->builder,
            runtime_panic_func,
            panic_args,
            1,
            ""
        );
        
        // Branch to cleanup after panic
        LLVMBuildBr(context->builder, cleanup_block);
        
        free(args);
    } else {
        // For non-call expressions, just evaluate the expression
        goo_codegen_node(context, node->expr);
        
        // Branch to cleanup
        LLVMBuildBr(context->builder, cleanup_block);
    }
    
    // Build the cleanup block - for resource management
    LLVMPositionBuilderAtEnd(context->builder, cleanup_block);
    
    // Add any cleanup code here (if we allocated resources that need to be freed)
    
    // Add a return void at the end of the wrapper function
    LLVMBuildRetVoid(context->builder);
    
    // Restore the original insertion point
    LLVMPositionBuilderAtEnd(context->builder, current_block);
    
    // Allocate space for the closure data (if needed)
    LLVMValueRef closure_data = NULL;
    
    if (is_call) {
        // Create a structure to hold all argument values
        int arg_count = 0;
        GooNode* arg = call_node->args;
        while (arg) {
            arg_count++;
            arg = arg->next;
        }
        
        // Create an array of LLVM types for the closure fields
        LLVMTypeRef* field_types = (LLVMTypeRef*)malloc(arg_count * sizeof(LLVMTypeRef));
        if (!field_types) {
            fprintf(stderr, "Failed to allocate memory for closure field types\n");
        return NULL;
    }
    
        // Create an array of LLVM values for each argument
        LLVMValueRef* arg_values = (LLVMValueRef*)malloc(arg_count * sizeof(LLVMValueRef));
        if (!arg_values) {
            fprintf(stderr, "Failed to allocate memory for argument values\n");
            free(field_types);
        return NULL;
    }
    
        // Generate code for each argument and collect types
        arg = call_node->args;
        for (int i = 0; i < arg_count; i++) {
            arg_values[i] = goo_codegen_node(context, arg);
            field_types[i] = LLVMTypeOf(arg_values[i]);
            arg = arg->next;
        }
        
        // Create the closure struct type
        LLVMTypeRef closure_type = LLVMStructTypeInContext(
            context->context,
            field_types,
            arg_count,
            false
        );
        
        // Allocate memory for the closure struct on the heap
        // Call our runtime allocation function for better memory management
        LLVMValueRef alloc_func = LLVMGetNamedFunction(context->module, "goo_runtime_alloc");
        if (!alloc_func) {
            // Declare the allocator if not already declared
            LLVMTypeRef alloc_type = LLVMFunctionType(
                void_ptr_type,
                &LLVMInt64TypeInContext(context->context),
                1,
                false
            );
            alloc_func = LLVMAddFunction(context->module, "goo_runtime_alloc", alloc_type);
        }
        
        // Calculate the size of the closure struct
        LLVMTypeRef size_t_type = LLVMInt64TypeInContext(context->context);
        LLVMValueRef closure_size = LLVMConstInt(
            size_t_type,
            LLVMABISizeOfType(LLVMGetModuleDataLayout(context->module), closure_type),
            false
        );
        
        // Call the allocator to allocate memory for the closure
        LLVMValueRef alloc_args[] = { closure_size };
        closure_data = LLVMBuildCall(
            context->builder,
            alloc_func,
            alloc_args,
            1,
            "closure_data"
        );
        
        // Cast the allocated memory to the closure type
        LLVMValueRef closure_struct_ptr = LLVMBuildBitCast(
            context->builder,
            closure_data,
            LLVMPointerType(closure_type, 0),
            "closure_struct"
        );
        
        // Store each argument value in the closure struct
        for (int i = 0; i < arg_count; i++) {
            // Get a pointer to the field
            LLVMValueRef field_ptr = LLVMBuildStructGEP(
                context->builder,
                closure_struct_ptr,
                i,
                "arg_ptr"
            );
            
            // Store the argument value in the field
            LLVMBuildStore(
                context->builder,
                arg_values[i],
                field_ptr
            );
        }
        
        free(field_types);
        free(arg_values);
    } else {
        // For non-call expressions, we don't need a closure
        closure_data = LLVMConstNull(void_ptr_type);
    }
    
    // Get the goroutine spawn function with capability support
    LLVMValueRef spawn_func = LLVMGetNamedFunction(context->module, "goo_goroutine_spawn_with_caps");
    if (!spawn_func) {
        // Declare the spawn function if not already declared
        LLVMTypeRef taskfunc_type = LLVMFunctionType(void_type, &void_ptr_type, 1, false);
        LLVMTypeRef taskfunc_ptr_type = LLVMPointerType(taskfunc_type, 0);
        
        LLVMTypeRef spawn_params[3];
        spawn_params[0] = taskfunc_ptr_type;  // function pointer
        spawn_params[1] = void_ptr_type;      // argument
        spawn_params[2] = void_ptr_type;      // capability set
        
        LLVMTypeRef spawn_type = LLVMFunctionType(
            LLVMInt32TypeInContext(context->context),  // bool return type (success/failure)
            spawn_params,
            3,
            false
        );
        spawn_func = LLVMAddFunction(context->module, "goo_goroutine_spawn_with_caps", spawn_type);
    }
    
    // Get the current capability set
    LLVMValueRef get_caps_func = LLVMGetNamedFunction(context->module, "goo_runtime_get_current_caps");
    if (!get_caps_func) {
        // Declare the get_caps function if not already declared
        LLVMTypeRef get_caps_type = LLVMFunctionType(
            void_ptr_type,
            NULL,
            0,
            false
        );
        get_caps_func = LLVMAddFunction(context->module, "goo_runtime_get_current_caps", get_caps_type);
    }
    
    // Get current capability set to pass to the goroutine
    LLVMValueRef current_caps = LLVMBuildCall(
        context->builder,
        get_caps_func,
        NULL,
        0,
        "current_caps"
    );
    
    // Call the goroutine spawn function with capabilities
    LLVMValueRef spawn_args[3];
    spawn_args[0] = wrapper_func;    // the wrapper function
    spawn_args[1] = closure_data;    // closure data pointer
    spawn_args[2] = current_caps;    // capability set
    
    return LLVMBuildCall(
        context->builder,
        spawn_func,
        spawn_args,
        3,
        "spawn_result"
    );
}

// Function for implementing parallel execution blocks
        // Default to one-for-one
        policy_type = LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0);
    }
    
    // Get the max restarts
    LLVMValueRef max_restarts;
    if (node->max_restarts > 0) {
        max_restarts = LLVMConstInt(LLVMInt32TypeInContext(context->context), node->max_restarts, 0);
    } else {
        // Default to 10 restarts
        max_restarts = LLVMConstInt(LLVMInt32TypeInContext(context->context), 10, 0);
    }
    
    // Get the restart time window
    LLVMValueRef time_window;
    if (node->time_window > 0) {
        time_window = LLVMConstInt(LLVMInt32TypeInContext(context->context), node->time_window, 0);
    } else {
        // Default to 5 seconds
        time_window = LLVMConstInt(LLVMInt32TypeInContext(context->context), 5, 0);
    }
    
    // Set the policy
    LLVMValueRef args[] = { node->supervisor, policy_type, max_restarts, time_window };
    return LLVMBuildCall(context->builder, policy_func, args, 4, "policy_result");
}

// Function to run LLVM optimization passes on the module
bool goo_codegen_optimize(GooCodegenContext* context) {
    if (!context || !context->module) return false;
    
    // Create a pass manager
    LLVMPassManagerRef pass_manager = LLVMCreatePassManager();
    if (!pass_manager) {
        fprintf(stderr, "Failed to create pass manager\n");
        return false;
    }
    
    // Add optimization passes based on the optimization level
    int opt_level = context->goo_context->opt_level;
    bool optimize = context->goo_context->optimize;
    
    if (optimize) {
        // Add function analysis passes
        LLVMPassManagerBuilderRef builder = LLVMPassManagerBuilderCreate();
        LLVMPassManagerBuilderSetOptLevel(builder, opt_level);
        
        // Set size level (0 = none, 1 = optimize for size, 2 = optimize for size aggressively)
        LLVMPassManagerBuilderSetSizeLevel(builder, 0);
        
        // Populate the pass manager with the passes specified by the optimization level
        LLVMPassManagerBuilderPopulateModulePassManager(builder, pass_manager);
        
        // Add specific passes based on optimization level
        if (opt_level >= 1) {
            // Basic optimizations
            LLVMAddPromoteMemoryToRegisterPass(pass_manager);
            LLVMAddInstructionCombiningPass(pass_manager);
            LLVMAddReassociatePass(pass_manager);
            LLVMAddGVNPass(pass_manager);
            LLVMAddCFGSimplificationPass(pass_manager);
        }
        
        if (opt_level >= 2) {
            // Intermediate optimizations
            LLVMAddTailCallEliminationPass(pass_manager);
            LLVMAddConstantPropagationPass(pass_manager);
            LLVMAddSCCPPass(pass_manager);
            LLVMAddDeadStoreEliminationPass(pass_manager);
            LLVMAddAggressiveDCEPass(pass_manager);
        }
        
        if (opt_level >= 3) {
            // Advanced optimizations
            LLVMAddFunctionInliningPass(pass_manager);
            LLVMAddJumpThreadingPass(pass_manager);
            LLVMAddLoopUnrollPass(pass_manager);
            LLVMAddLoopVectorizePass(pass_manager);
            LLVMAddSLPVectorizePass(pass_manager);
        }
        
        LLVMPassManagerBuilderDispose(builder);
    }
    
    // Run the pass manager on the module
    bool result = LLVMRunPassManager(pass_manager, context->module);
    
    // Clean up
    LLVMDisposePassManager(pass_manager);
    
    return result;
}

// Function to update code generation based on the context settings
bool goo_codegen_apply_context_settings(GooCodegenContext* context) {
    if (!context || !context->goo_context) return false;
    
    // Set target triple if specified
    if (context->goo_context->target_triple) {
        LLVMSetTarget(context->module, context->goo_context->target_triple);
    }
    
    // Set target CPU if specified
    if (context->goo_context->cpu) {
        // This requires more complex setup with target machine
        // For now, we'll just log that this would be set
        fprintf(stderr, "Setting target CPU to %s (functionality limited)\n", context->goo_context->cpu);
    }
    
    // Set target features if specified
    if (context->goo_context->features) {
        // This requires more complex setup with target machine
        // For now, we'll just log that this would be set
        fprintf(stderr, "Setting target features to %s (functionality limited)\n", context->goo_context->features);
    }
    
    // Set debug mode if specified
    context->debug_mode = context->goo_context->debug;
    
    return true;
}

// Enhanced function to generate optimized code
bool goo_codegen_generate_optimized(GooCodegenContext* context) {
    if (!context || !context->ast) return false;
    
    // Apply context settings to the code generator
    if (!goo_codegen_apply_context_settings(context)) {
        fprintf(stderr, "Failed to apply context settings\n");
        return false;
    }
    
    // Generate code for each node in the AST
    if (!goo_codegen_generate(context)) {
        fprintf(stderr, "Failed to generate code\n");
        return false;
    }
    
    // Run optimization passes if optimization is enabled
    if (context->goo_context->optimize) {
        if (!goo_codegen_optimize(context)) {
            fprintf(stderr, "Warning: Failed to run optimization passes\n");
            // Continue anyway as this is not fatal
        }
    }
    
    // Verify the module
    return goo_codegen_verify_module(context);
}

// Function to emit LLVM IR if requested
bool goo_codegen_emit_llvm(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // If emit_llvm is set, write LLVM IR to a file
    if (context->goo_context->emit_llvm) {
        // Construct the LLVM IR filename
        char* ir_filename = NULL;
        if (context->goo_context->output_file) {
            // Use the output file name with .ll extension
            size_t len = strlen(context->goo_context->output_file);
            ir_filename = (char*)malloc(len + 4); // +4 for .ll and null terminator
            if (!ir_filename) {
                fprintf(stderr, "Failed to allocate memory for IR filename\n");
                return false;
            }
            strcpy(ir_filename, context->goo_context->output_file);
            strcat(ir_filename, ".ll");
        } else if (context->goo_context->input_file) {
            // Use the input file name with .ll extension
            size_t len = strlen(context->goo_context->input_file);
            ir_filename = (char*)malloc(len + 4); // +4 for .ll and null terminator
            if (!ir_filename) {
                fprintf(stderr, "Failed to allocate memory for IR filename\n");
                return false;
            }
            strcpy(ir_filename, context->goo_context->input_file);
            char* dot = strrchr(ir_filename, '.');
            if (dot) *dot = '\0';
            strcat(ir_filename, ".ll");
        } else {
            // Use a default name
            ir_filename = strdup("output.ll");
            if (!ir_filename) {
                fprintf(stderr, "Failed to allocate memory for IR filename\n");
                return false;
            }
        }
        
        // Write the module to the IR file
        bool result = goo_codegen_write_to_file(context, ir_filename);
        
        // Clean up
        free(ir_filename);
        
        return result;
    }
    
    return true;
}

// Function to initialize the target machine for code generation
LLVMTargetMachineRef goo_codegen_get_target_machine(GooCodegenContext* context) {
    if (!context || !context->goo_context) return NULL;
    
    // Initialize all targets
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();
    
    // Get the target triple
    const char* target_triple = context->goo_context->target_triple;
    if (!target_triple) {
        // Get the default target triple if none is specified
        target_triple = LLVMGetDefaultTargetTriple();
    }
    
    // Find the target
    char* error = NULL;
    LLVMTargetRef target;
    if (LLVMGetTargetFromTriple(target_triple, &target, &error)) {
        fprintf(stderr, "Could not get target from triple: %s\n", error);
        LLVMDisposeMessage(error);
        return NULL;
    }
    
    // Get the target CPU and features
    const char* cpu = context->goo_context->cpu ? context->goo_context->cpu : "generic";
    const char* features = context->goo_context->features ? context->goo_context->features : "";
    
    // Create the target machine
    LLVMCodeGenOptLevel opt_level;
    switch (context->goo_context->opt_level) {
        case 0: opt_level = LLVMCodeGenLevelNone; break;
        case 1: opt_level = LLVMCodeGenLevelLess; break;
        case 2: opt_level = LLVMCodeGenLevelDefault; break;
        case 3: opt_level = LLVMCodeGenLevelAggressive; break;
        default: opt_level = LLVMCodeGenLevelDefault;
    }
    
    LLVMRelocMode reloc = LLVMRelocDefault;
    LLVMCodeModel code_model = LLVMCodeModelDefault;
    
    LLVMTargetMachineRef target_machine = LLVMCreateTargetMachine(
        target, target_triple, cpu, features, opt_level, reloc, code_model);
    
    if (!target_machine) {
        fprintf(stderr, "Could not create target machine\n");
        return NULL;
    }
    
    return target_machine;
}

// Function to generate object file from LLVM module
bool goo_codegen_generate_object_file(GooCodegenContext* context, const char* output_file) {
    if (!context || !context->module || !output_file) return false;
    
    // Get the target machine
    LLVMTargetMachineRef target_machine = goo_codegen_get_target_machine(context);
    if (!target_machine) {
        fprintf(stderr, "Failed to get target machine\n");
        return false;
    }
    
    // Set the target triple and data layout in the module
    char* target_triple = LLVMGetTargetMachineTriple(target_machine);
    LLVMSetTarget(context->module, target_triple);
    LLVMDisposeMessage(target_triple);
    
    char* data_layout = LLVMGetTargetMachineDataLayout(target_machine);
    LLVMSetDataLayout(context->module, data_layout);
    LLVMDisposeMessage(data_layout);
    
    // Generate the object file
    char* error = NULL;
    if (LLVMTargetMachineEmitToFile(target_machine, context->module, 
                                 (char*)output_file, LLVMObjectFile, &error)) {
        fprintf(stderr, "Could not emit object file: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeTargetMachine(target_machine);
        return false;
    }
    
    // Clean up
    LLVMDisposeTargetMachine(target_machine);
    
    return true;
}

// Function to generate assembly file from LLVM module
bool goo_codegen_generate_assembly_file(GooCodegenContext* context, const char* output_file) {
    if (!context || !context->module || !output_file) return false;
    
    // Get the target machine
    LLVMTargetMachineRef target_machine = goo_codegen_get_target_machine(context);
    if (!target_machine) {
        fprintf(stderr, "Failed to get target machine\n");
        return false;
    }
    
    // Set the target triple and data layout in the module
    char* target_triple = LLVMGetTargetMachineTriple(target_machine);
    LLVMSetTarget(context->module, target_triple);
    LLVMDisposeMessage(target_triple);
    
    char* data_layout = LLVMGetTargetMachineDataLayout(target_machine);
    LLVMSetDataLayout(context->module, data_layout);
    LLVMDisposeMessage(data_layout);
    
    // Generate the assembly file
    char* error = NULL;
    if (LLVMTargetMachineEmitToFile(target_machine, context->module, 
                                 (char*)output_file, LLVMAssemblyFile, &error)) {
        fprintf(stderr, "Could not emit assembly file: %s\n", error);
        LLVMDisposeMessage(error);
        LLVMDisposeTargetMachine(target_machine);
        return false;
    }
    
    // Clean up
    LLVMDisposeTargetMachine(target_machine);
    
    return true;
}

// Initialize JIT execution engine for a module
bool goo_codegen_init_jit(GooCodegenContext* context) {
    if (!context || !context->module) return false;
    
    // Create the JIT execution engine
    char* error = NULL;
    
    // Link in the runtime library
    if (LLVMLinkInMCJIT()) {
        fprintf(stderr, "Failed to link in MCJIT\n");
        return false;
    }
    
    // Initialize the native target
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    
    // Create the execution engine
    LLVMExecutionEngineRef engine;
    if (LLVMCreateExecutionEngineForModule(&engine, context->module, &error)) {
        fprintf(stderr, "Failed to create execution engine: %s\n", error);
        LLVMDisposeMessage(error);
        return false;
    }
    
    // Store the execution engine in the context
    context->engine = engine;
    
    return true;
}

// Lookup a function in the JIT engine by name
LLVMValueRef goo_codegen_jit_get_function(GooCodegenContext* context, const char* name) {
    if (!context || !context->engine || !name) return NULL;
    
    // Get the function from the module
    LLVMValueRef function = LLVMGetNamedFunction(context->module, name);
    if (!function) {
        fprintf(stderr, "Function %s not found in module\n", name);
        return NULL;
    }
    
    return function;
}

// Execute a function using JIT
int goo_codegen_jit_run(GooCodegenContext* context, const char* function_name, void** args) {
    if (!context || !context->engine || !function_name) return -1;
    
    // Get the function from the module
    LLVMValueRef function = goo_codegen_jit_get_function(context, function_name);
    if (!function) {
        return -1;
    }
    
    // Get the function address
    uint64_t function_addr = LLVMGetFunctionAddress(context->engine, function_name);
    if (!function_addr) {
        fprintf(stderr, "Failed to get function address for %s\n", function_name);
        return -1;
    }
    
    // Cast the address to a function pointer
    int (*func_ptr)(void**) = (int (*)(void**))function_addr;
    
    // Call the function
    return func_ptr(args);
}

// Run JIT compilation and execution
bool goo_codegen_run_jit(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // Make sure we're in JIT mode
    if (context->goo_context->mode != GOO_MODE_JIT) {
        fprintf(stderr, "Not in JIT mode\n");
        return false;
    }
    
    // Initialize the JIT engine
    if (!goo_codegen_init_jit(context)) {
        fprintf(stderr, "Failed to initialize JIT engine\n");
        return false;
    }
    
    // Look for the main function
    LLVMValueRef main_func = goo_codegen_jit_get_function(context, "main");
    if (!main_func) {
        fprintf(stderr, "No main function found in module\n");
        return false;
    }
    
    // Run the main function
    void* args[] = { NULL };  // No arguments for now
    int result = goo_codegen_jit_run(context, "main", args);
    
    // Print the result
    printf("JIT execution completed with result: %d\n", result);
    
    return true;
}

// Update the code emission function to handle JIT mode
bool goo_codegen_emit(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // Determine the output type based on the compilation mode
    switch (context->goo_context->mode) {
        case GOO_MODE_COMPILE: {
            // Emit object file
            const char* output_file = context->goo_context->output_file;
            if (!output_file) {
                fprintf(stderr, "No output file specified for compilation\n");
                return false;
            }
            
            return goo_codegen_generate_object_file(context, output_file);
        }
        
        case GOO_MODE_JIT: {
            // JIT mode doesn't write to a file, but executes the code
            return goo_codegen_run_jit(context);
        }
        
        case GOO_MODE_INTERPRET: {
            // Interpret the code
            return goo_codegen_interpret(context);
        }
        
        default: {
            fprintf(stderr, "Unknown compilation mode: %d\n", context->goo_context->mode);
            return false;
        }
    }
}

// Structure to hold the interpreter state
typedef struct GooInterpreterState {
    GooCodegenContext* codegen_ctx;
    LLVMExecutionEngineRef engine;
    LLVMValueRef current_function;
    LLVMGenericValueRef* args;
    int arg_count;
} GooInterpreterState;

// Initialize the interpreter
GooInterpreterState* goo_interpreter_init(GooCodegenContext* context) {
    if (!context || !context->module) return NULL;
    
    // Create the interpreter state
    GooInterpreterState* state = (GooInterpreterState*)malloc(sizeof(GooInterpreterState));
    if (!state) {
        fprintf(stderr, "Failed to allocate memory for interpreter state\n");
        return NULL;
    }
    
    // Initialize the state
    state->codegen_ctx = context;
    state->engine = NULL;
    state->current_function = NULL;
    state->args = NULL;
    state->arg_count = 0;
    
    // Initialize the interpreter engine
    if (LLVMLinkInInterpreter()) {
        fprintf(stderr, "Failed to link in interpreter\n");
        free(state);
        return NULL;
    }
    
    // Initialize the native target
    LLVMInitializeNativeTarget();
    LLVMInitializeNativeAsmPrinter();
    
    // Create the execution engine for interpretation
    char* error = NULL;
    if (LLVMCreateInterpreterForModule(&state->engine, context->module, &error)) {
        fprintf(stderr, "Failed to create interpreter: %s\n", error);
        LLVMDisposeMessage(error);
        free(state);
        return NULL;
    }
    
    return state;
}

// Clean up the interpreter state
void goo_interpreter_free(GooInterpreterState* state) {
    if (!state) return;
    
    // Free the arguments
    if (state->args) {
        for (int i = 0; i < state->arg_count; i++) {
            if (state->args[i]) {
                LLVMDisposeGenericValue(state->args[i]);
            }
        }
        free(state->args);
    }
    
    // Dispose the engine
    if (state->engine) {
        LLVMDisposeExecutionEngine(state->engine);
    }
    
    // Free the state
    free(state);
}

// Set the current function to interpret
bool goo_interpreter_set_function(GooInterpreterState* state, const char* function_name) {
    if (!state || !state->codegen_ctx || !function_name) return false;
    
    // Get the function from the module
    LLVMValueRef function = LLVMGetNamedFunction(state->codegen_ctx->module, function_name);
    if (!function) {
        fprintf(stderr, "Function %s not found in module\n", function_name);
        return false;
    }
    
    // Set the current function
    state->current_function = function;
    
    return true;
}

// Add an argument for the function
bool goo_interpreter_add_arg(GooInterpreterState* state, LLVMGenericValueRef arg) {
    if (!state) return false;
    
    // Resize the argument array
    state->arg_count++;
    state->args = (LLVMGenericValueRef*)realloc(state->args, state->arg_count * sizeof(LLVMGenericValueRef));
    if (!state->args) {
        fprintf(stderr, "Failed to allocate memory for argument\n");
        state->arg_count--;
        return false;
    }
    
    // Add the argument
    state->args[state->arg_count - 1] = arg;
    
    return true;
}

// Run the interpreter on the current function
LLVMGenericValueRef goo_interpreter_run(GooInterpreterState* state) {
    if (!state || !state->engine || !state->current_function) return NULL;
    
    // Run the function
    return LLVMRunFunction(state->engine, state->current_function, state->arg_count, state->args);
}

// Interpret Goo code
bool goo_codegen_interpret(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // Make sure we're in interpret mode
    if (context->goo_context->mode != GOO_MODE_INTERPRET) {
        fprintf(stderr, "Not in interpret mode\n");
        return false;
    }
    
    // Initialize the interpreter
    GooInterpreterState* state = goo_interpreter_init(context);
    if (!state) {
        fprintf(stderr, "Failed to initialize interpreter\n");
        return false;
    }
    
    // Look for the main function
    if (!goo_interpreter_set_function(state, "main")) {
        fprintf(stderr, "No main function found or couldn't set it\n");
        goo_interpreter_free(state);
        return false;
    }
    
    // Run the main function
    LLVMGenericValueRef result = goo_interpreter_run(state);
    if (!result) {
        fprintf(stderr, "Failed to run main function\n");
        goo_interpreter_free(state);
        return false;
    }
    
    // Print the result
    int int_result = (int)LLVMGenericValueToInt(result, false);
    printf("Interpreter execution completed with result: %d\n", int_result);
    
    // Clean up
    LLVMDisposeGenericValue(result);
    goo_interpreter_free(state);
    
    return true;
}

// Update goo_codegen_emit function for the interpreter mode
bool goo_codegen_emit(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // Determine the output type based on the compilation mode
    switch (context->goo_context->mode) {
        case GOO_MODE_COMPILE: {
            // Emit object file
            const char* output_file = context->goo_context->output_file;
            if (!output_file) {
                fprintf(stderr, "No output file specified for compilation\n");
                return false;
            }
            
            return goo_codegen_generate_object_file(context, output_file);
        }
        
        case GOO_MODE_JIT: {
            // JIT mode doesn't write to a file, but executes the code
            return goo_codegen_run_jit(context);
        }
        
        case GOO_MODE_INTERPRET: {
            // Interpret the code
            return goo_codegen_interpret(context);
        }
        
        default: {
            fprintf(stderr, "Unknown compilation mode: %d\n", context->goo_context->mode);
            return false;
        }
    }
}

// Add debug information to the module
bool goo_codegen_add_debug_info(GooCodegenContext* context) {
    if (!context || !context->module || !context->goo_context) return false;
    
    // Only add debug info if debug mode is enabled
    if (!context->goo_context->debug) return true;
    
    // Create the debug info builder
    LLVMDIBuilderRef di_builder = LLVMCreateDIBuilder(context->module);
    if (!di_builder) {
        fprintf(stderr, "Failed to create debug info builder\n");
        return false;
    }
    
    // Store the debug info builder in the context
    context->di_builder = di_builder;
    
    // Create the compilation unit
    const char* filename = context->goo_context->input_file ? context->goo_context->input_file : "unknown.goo";
    const char* directory = ".";  // Current directory by default
    
    // Create the file descriptor
    LLVMMetadataRef file = LLVMDIBuilderCreateFile(di_builder, filename, strlen(filename), 
                                                 directory, strlen(directory));
    
    // Create the compilation unit
    LLVMMetadataRef cu = LLVMDIBuilderCreateCompileUnit(
        di_builder, LLVMDWARFSourceLanguageC99,  // Use C99 as the language for now
        file, "Goo Compiler", strlen("Goo Compiler"),
        0,  // Is optimized
        "",  // Flags
        0,   // Flags length
        0,   // Runtime version
        "",  // Split name
        0,   // Split name length
        LLVMDWARFEmissionFull,
        0,   // DWARF version
        0,   // Debug info for profiling
        0,   // Debug info for sampleprof
        "",  // Sys root
        0,   // Sys root length
        ""   // SDK
    );
    
    // Store the compilation unit in the context
    context->di_compile_unit = cu;
    
    return true;
}

// Create debug information for a function
LLVMMetadataRef goo_codegen_create_function_debug_info(GooCodegenContext* context, 
                                                    LLVMValueRef function, 
                                                    const char* name,
                                                    unsigned line_number) {
    if (!context || !context->di_builder || !context->di_compile_unit || !function || !name) return NULL;
    
    // Only add debug info if debug mode is enabled
    if (!context->goo_context->debug) return NULL;
    
    // Get the function type
    LLVMTypeRef function_type = LLVMGetElementType(LLVMTypeOf(function));
    
    // Create the function type debug info
    unsigned param_count = LLVMCountParamTypes(function_type);
    LLVMMetadataRef* param_types = (LLVMMetadataRef*)malloc(param_count * sizeof(LLVMMetadataRef));
    if (!param_types) {
        fprintf(stderr, "Failed to allocate memory for parameter types\n");
        return NULL;
    }
    
    // Create the return type debug info
    LLVMTypeRef return_type = LLVMGetReturnType(function_type);
    LLVMMetadataRef di_return_type = LLVMDIBuilderCreateBasicType(
        context->di_builder, "int", 3, 32, LLVMDWARFTypeEncoding_Signed);
    
    // For now, we assume all parameters are integers
    for (unsigned i = 0; i < param_count; i++) {
        param_types[i] = LLVMDIBuilderCreateBasicType(
            context->di_builder, "int", 3, 32, LLVMDWARFTypeEncoding_Signed);
    }
    
    // Create the subroutine type
    LLVMMetadataRef function_type_di = LLVMDIBuilderCreateSubroutineType(
        context->di_builder, context->di_compile_unit, param_types, param_count, 0);
    
    // Free the parameter types array
    free(param_types);
    
    // Create the function debug info
    LLVMMetadataRef function_di = LLVMDIBuilderCreateFunction(
        context->di_builder, context->di_compile_unit, name, strlen(name),
        name, strlen(name), context->di_compile_unit,
        line_number, function_type_di, false, true,
        line_number, LLVMDIFlagPrototyped, false);
    
    // Set the function debug info
    LLVMSetSubprogram(function, function_di);
    
    return function_di;
}

// Create debug information for a local variable
LLVMMetadataRef goo_codegen_create_local_var_debug_info(GooCodegenContext* context,
                                                     LLVMValueRef variable,
                                                     const char* name,
                                                     unsigned line_number,
                                                     LLVMMetadataRef scope) {
    if (!context || !context->di_builder || !variable || !name || !scope) return NULL;
    
    // Only add debug info if debug mode is enabled
    if (!context->goo_context->debug) return NULL;
    
    // Create the file descriptor
    const char* filename = context->goo_context->input_file ? context->goo_context->input_file : "unknown.goo";
    const char* directory = ".";  // Current directory by default
    LLVMMetadataRef file = LLVMDIBuilderCreateFile(context->di_builder, filename, strlen(filename),
                                                 directory, strlen(directory));
    
    // Create the type debug info (assume int for now)
    LLVMMetadataRef type_di = LLVMDIBuilderCreateBasicType(
        context->di_builder, "int", 3, 32, LLVMDWARFTypeEncoding_Signed);
    
    // Create the local variable debug info
    LLVMMetadataRef var_di = LLVMDIBuilderCreateAutoVariable(
        context->di_builder, scope, name, strlen(name),
        file, line_number, type_di, false, 0, 0);
    
    // Get the block where the variable is declared
    LLVMBasicBlockRef block = LLVMGetInstructionParent(variable);
    
    // Create a debug location
    LLVMMetadataRef loc = LLVMDIBuilderCreateDebugLocation(
        context->context, line_number, 0, scope, NULL);
    
    // Insert a declaration
    LLVMDIBuilderInsertDeclareAtEnd(
        context->di_builder, variable, var_di, loc, block);
    
    return var_di;
}

// Enhanced goo_codegen_init function with debug support
GooCodegenContext* goo_codegen_init(GooAst* ast, GooContext* goo_ctx, const char* module_name) {
    if (!ast || !goo_ctx) return NULL;
    
    // Allocate memory for the codegen context
    GooCodegenContext* context = (GooCodegenContext*)malloc(sizeof(GooCodegenContext));
    if (!context) {
        fprintf(stderr, "Failed to allocate memory for codegen context\n");
        return NULL;
    }
    
    // Initialize the LLVM context
    context->context = LLVMContextCreate();
    if (!context->context) {
        fprintf(stderr, "Failed to create LLVM context\n");
        free(context);
        return NULL;
    }
    
    // Set the module name
    const char* actual_module_name = module_name ? module_name : "goo_module";
    
    // Create the module
    context->module = LLVMModuleCreateWithNameInContext(actual_module_name, context->context);
    if (!context->module) {
        fprintf(stderr, "Failed to create LLVM module\n");
        LLVMContextDispose(context->context);
        free(context);
        return NULL;
    }
    
    // Initialize the IR builder
    context->builder = LLVMCreateBuilderInContext(context->context);
    if (!context->builder) {
        fprintf(stderr, "Failed to create LLVM IR builder\n");
        LLVMDisposeModule(context->module);
        LLVMContextDispose(context->context);
        free(context);
        return NULL;
    }
    
    // Initialize the symbol table
    context->symbol_table = goo_symbol_table_init();
    if (!context->symbol_table) {
        fprintf(stderr, "Failed to initialize symbol table\n");
        LLVMDisposeBuilder(context->builder);
        LLVMDisposeModule(context->module);
        LLVMContextDispose(context->context);
        free(context);
        return NULL;
    }
    
    // Initialize the AST and Goo context
    context->ast = ast;
    context->goo_context = goo_ctx;
    
    // Set debug mode
    context->debug_mode = goo_ctx->debug;
    
    // Initialize the engine and other fields
    context->engine = NULL;
    context->di_builder = NULL;
    context->di_compile_unit = NULL;
    context->next_goroutine_id = 0;
    context->next_supervision_id = 0;
    
    // Declare runtime functions
    declare_runtime_functions(context);
    
    // Add debug information if in debug mode
    if (context->debug_mode) {
        if (!goo_codegen_add_debug_info(context)) {
            fprintf(stderr, "Warning: Failed to add debug information\n");
            // Continue anyway as this is not fatal
        }
    }
    
    return context;
}

// Enhanced goo_codegen_free function with debug support
void goo_codegen_free(GooCodegenContext* context) {
    if (!context) return;
    
    // Finalize the debug info
    if (context->di_builder) {
        LLVMDIBuilderFinalize(context->di_builder);
        LLVMDisposeDIBuilder(context->di_builder);
    }
    
    // Free the symbol table
    if (context->symbol_table) {
        goo_symbol_table_free(context->symbol_table);
    }
    
    // Free the engine
    if (context->engine) {
        LLVMDisposeExecutionEngine(context->engine);
    }
    
    // Free the builder
    if (context->builder) {
        LLVMDisposeBuilder(context->builder);
    }
    
    // Free the module
    if (context->module) {
        LLVMDisposeModule(context->module);
    }
    
    // Free the context
    if (context->context) {
        LLVMContextDispose(context->context);
    }
    
    // Free the context structure itself
    free(context);
}

// Initialize the code generator's runtime support
bool goo_codegen_init_runtime(GooCodegenContext* context) {
    if (!context || !context->module) return false;
    
    // Add memory management functions
    bool memory_init_ok = true;
    
    // Memory initialization function
    LLVMTypeRef memory_init_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        NULL, 0, false); // No parameters
    
    LLVMValueRef memory_init_func = LLVMAddFunction(
        context->module,
        "goo_memory_init",
        memory_init_type);
    
    memory_init_ok &= (memory_init_func != NULL);
    
    // Memory cleanup function
    LLVMTypeRef memory_cleanup_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        NULL, 0, false); // No parameters
    
    LLVMValueRef memory_cleanup_func = LLVMAddFunction(
        context->module,
        "goo_memory_cleanup",
        memory_cleanup_type);
    
    memory_init_ok &= (memory_cleanup_func != NULL);
    
    // Basic memory allocation function
    LLVMTypeRef alloc_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* return type
        (LLVMTypeRef[]){LLVMInt64TypeInContext(context->context)}, // size_t parameter
        1, false);
    
    LLVMValueRef alloc_func = LLVMAddFunction(
        context->module,
        "goo_alloc",
        alloc_type);
    
    memory_init_ok &= (alloc_func != NULL);
    
    // Memory reallocation function
    LLVMTypeRef realloc_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* return type
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* ptr
            LLVMInt64TypeInContext(context->context), // size_t old_size
            LLVMInt64TypeInContext(context->context) // size_t new_size
        }, 
        3, false);
    
    LLVMValueRef realloc_func = LLVMAddFunction(
        context->module,
        "goo_realloc",
        realloc_type);
    
    memory_init_ok &= (realloc_func != NULL);
    
    // Memory free function
    LLVMTypeRef free_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* ptr
            LLVMInt64TypeInContext(context->context) // size_t size
        }, 
        2, false);
    
    LLVMValueRef free_func = LLVMAddFunction(
        context->module,
        "goo_free",
        free_type);
    
    memory_init_ok &= (free_func != NULL);
    
    // Aligned memory functions
    LLVMTypeRef alloc_aligned_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* return type
        (LLVMTypeRef[]){
            LLVMInt64TypeInContext(context->context), // size_t size
            LLVMInt64TypeInContext(context->context) // size_t alignment
        }, 
        2, false);
    
    LLVMValueRef alloc_aligned_func = LLVMAddFunction(
        context->module,
        "goo_alloc_aligned",
        alloc_aligned_type);
    
    memory_init_ok &= (alloc_aligned_func != NULL);
    
    // Realloc aligned function
    LLVMTypeRef realloc_aligned_type = LLVMFunctionType(
        LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* return type
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* ptr
            LLVMInt64TypeInContext(context->context), // size_t old_size
            LLVMInt64TypeInContext(context->context), // size_t new_size
            LLVMInt64TypeInContext(context->context) // size_t alignment
        }, 
        4, false);
    
    LLVMValueRef realloc_aligned_func = LLVMAddFunction(
        context->module,
        "goo_realloc_aligned",
        realloc_aligned_type);
    
    memory_init_ok &= (realloc_aligned_func != NULL);
    
    // Free aligned function
    LLVMTypeRef free_aligned_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* ptr
            LLVMInt64TypeInContext(context->context), // size_t size
            LLVMInt64TypeInContext(context->context) // size_t alignment
        }, 
        3, false);
    
    LLVMValueRef free_aligned_func = LLVMAddFunction(
        context->module,
        "goo_free_aligned",
        free_aligned_type);
    
    memory_init_ok &= (free_aligned_func != NULL);
    
    // Add channel functions
    bool channel_init_ok = true;
    
    // Define GooChannelOptions structure type
    LLVMTypeRef channel_options_fields[] = {
        LLVMInt64TypeInContext(context->context),    // buffer_size
        LLVMInt1TypeInContext(context->context),     // is_blocking
        LLVMInt32TypeInContext(context->context),    // pattern
        LLVMInt32TypeInContext(context->context)     // timeout_ms
    };
    
    LLVMTypeRef channel_options_type = LLVMStructTypeInContext(
        context->context,
        channel_options_fields,
        4,
        false
    );
    
    goo_type_table_add_type(context->type_table, "GooChannelOptions", channel_options_type);
    
    // Define GooChannel structure as opaque for now
    LLVMTypeRef channel_type = LLVMStructCreateNamed(context->context, "GooChannel");
    goo_type_table_add_type(context->type_table, "GooChannel", channel_type);
    
    // Channel creation function
    LLVMTypeRef channel_create_type = LLVMFunctionType(
        LLVMPointerType(channel_type, 0), // GooChannel* return type
        (LLVMTypeRef[]){
            LLVMPointerType(channel_options_type, 0) // const GooChannelOptions* options
        }, 
        1, false);
    
    LLVMValueRef channel_create_func = LLVMAddFunction(
        context->module,
        "goo_channel_create",
        channel_create_type);
    
    channel_init_ok &= (channel_create_func != NULL);
    
    // Distributed channel creation
    LLVMTypeRef dist_channel_create_type = LLVMFunctionType(
        LLVMPointerType(channel_type, 0), // GooChannel* return type
        (LLVMTypeRef[]){
            LLVMPointerType(channel_options_type, 0), // const GooChannelOptions* options
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // const char* endpoint
            LLVMInt32TypeInContext(context->context) // int element_size
        }, 
        3, false);
    
    LLVMValueRef dist_channel_create_func = LLVMAddFunction(
        context->module,
        "goo_create_distributed_channel",
        dist_channel_create_type);
    
    channel_init_ok &= (dist_channel_create_func != NULL);
    
    // Channel send function
    LLVMTypeRef channel_send_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMPointerType(channel_type, 0), // GooChannel* channel
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // const void* data
            LLVMInt64TypeInContext(context->context), // size_t size
            LLVMInt32TypeInContext(context->context) // int flags
        }, 
        4, false);
    
    LLVMValueRef channel_send_func = LLVMAddFunction(
        context->module,
        "goo_channel_send",
        channel_send_type);
    
    channel_init_ok &= (channel_send_func != NULL);
    
    // Channel receive function
    LLVMTypeRef channel_recv_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMPointerType(channel_type, 0), // GooChannel* channel
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* data
            LLVMInt64TypeInContext(context->context), // size_t size
            LLVMPointerType(LLVMInt64TypeInContext(context->context), 0), // size_t* size_received
            LLVMInt32TypeInContext(context->context) // int flags
        }, 
        5, false);
    
    LLVMValueRef channel_recv_func = LLVMAddFunction(
        context->module,
        "goo_channel_receive",
        channel_recv_type);
    
    channel_init_ok &= (channel_recv_func != NULL);
    
    // Thread pool and goroutine functions
    LLVMAddFunction(context->module, "goo_thread_pool_init",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context), 
            (LLVMTypeRef[]) { LLVMInt32TypeInContext(context->context) }, // thread_count
            1, 0));
    
    LLVMAddFunction(context->module, "goo_goroutine_spawn",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context),
            (LLVMTypeRef[]) {
                LLVMPointerType(LLVMFunctionType(LLVMVoidTypeInContext(context->context),
                    (LLVMTypeRef[]) { LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) },
                    1, 0), 0),
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0)  // supervisor
            },
            3, 0));
    
    // Supervision functions
    LLVMAddFunction(context->module, "goo_supervise_init",
        LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
            NULL, 0, 0));
    
    LLVMAddFunction(context->module, "goo_supervise_free",
        LLVMFunctionType(LLVMVoidTypeInContext(context->context),
            (LLVMTypeRef[]) { LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) },
            1, 0));
    
    LLVMAddFunction(context->module, "goo_supervise_register",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context),
            (LLVMTypeRef[]) {
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),  // supervisor
                LLVMPointerType(LLVMFunctionType(LLVMVoidTypeInContext(context->context),
                    (LLVMTypeRef[]) { LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) },
                    1, 0), 0),  // function
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0)   // arg
            },
            3, 0));
    
    LLVMAddFunction(context->module, "goo_supervise_set_policy",
        LLVMFunctionType(LLVMVoidTypeInContext(context->context),
            (LLVMTypeRef[]) {
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),  // supervisor
                LLVMInt32TypeInContext(context->context),                    // policy
                LLVMInt32TypeInContext(context->context),                    // max_restarts
                LLVMInt32TypeInContext(context->context)                     // time_window
            },
            4, 0));
    
    LLVMAddFunction(context->module, "goo_supervise_start",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context),
            (LLVMTypeRef[]) { LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) },
            1, 0));
    
    // Error handling functions
    LLVMAddFunction(context->module, "goo_panic",
        LLVMFunctionType(LLVMVoidTypeInContext(context->context),
            (LLVMTypeRef[]) { 
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),  // value
                LLVMPointerType(LLVMInt8TypeInContext(context->context), 0)   // message
            },
            2, 0));
    
    LLVMAddFunction(context->module, "goo_is_panic",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context),
            NULL, 0, 0));
    
    LLVMAddFunction(context->module, "goo_clear_panic",
        LLVMFunctionType(LLVMVoidTypeInContext(context->context),
            NULL, 0, 0));
    
    // Runtime initialization and cleanup
    LLVMAddFunction(context->module, "goo_runtime_init",
        LLVMFunctionType(LLVMInt1TypeInContext(context->context),
            (LLVMTypeRef[]) { LLVMInt32TypeInContext(context->context) },  // log_level
            1, 0));
    
    LLVMAddFunction(context->module, "goo_runtime_cleanup",
        LLVMFunctionType(LLVMVoidTypeInContext(context->context),
            NULL, 0, 0));
            
    // Parallel execution functions
    bool parallel_init_ok = true;
    
    // Define GooThreadPool, GooTask, GooParallelFor, and GooParallelReduce as opaque types
    LLVMTypeRef thread_pool_type = LLVMStructCreateNamed(context->context, "GooThreadPool");
    goo_type_table_add_type(context->type_table, "GooThreadPool", thread_pool_type);
    
    LLVMTypeRef task_type = LLVMStructCreateNamed(context->context, "GooTask");
    goo_type_table_add_type(context->type_table, "GooTask", task_type);
    
    LLVMTypeRef parallel_for_type = LLVMStructCreateNamed(context->context, "GooParallelFor");
    goo_type_table_add_type(context->type_table, "GooParallelFor", parallel_for_type);
    
    LLVMTypeRef parallel_reduce_type = LLVMStructCreateNamed(context->context, "GooParallelReduce");
    goo_type_table_add_type(context->type_table, "GooParallelReduce", parallel_reduce_type);
    
    // Parallel initialization function
    LLVMTypeRef parallel_init_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        NULL, 0, false); // No parameters
    
    LLVMValueRef parallel_init_func = LLVMAddFunction(
        context->module,
        "goo_parallel_init",
        parallel_init_type);
    
    parallel_init_ok &= (parallel_init_func != NULL);
    
    // Parallel cleanup function
    LLVMTypeRef parallel_cleanup_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        NULL, 0, false); // No parameters
    
    LLVMValueRef parallel_cleanup_func = LLVMAddFunction(
        context->module,
        "goo_parallel_cleanup",
        parallel_cleanup_type);
    
    parallel_init_ok &= (parallel_cleanup_func != NULL);
    
    // Thread pool creation function
    LLVMTypeRef thread_pool_create_type = LLVMFunctionType(
        LLVMPointerType(thread_pool_type, 0), // GooThreadPool* return type
        (LLVMTypeRef[]){LLVMInt64TypeInContext(context->context)}, // size_t num_threads
        1, false);
    
    LLVMValueRef thread_pool_create_func = LLVMAddFunction(
        context->module,
        "goo_thread_pool_create",
        thread_pool_create_type);
    
    parallel_init_ok &= (thread_pool_create_func != NULL);
    
    // Thread pool destruction function
    LLVMTypeRef thread_pool_destroy_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(thread_pool_type, 0)}, // GooThreadPool* pool
        1, false);
    
    LLVMValueRef thread_pool_destroy_func = LLVMAddFunction(
        context->module,
        "goo_thread_pool_destroy",
        thread_pool_destroy_type);
    
    parallel_init_ok &= (thread_pool_destroy_func != NULL);
    
    // Task creation function
    LLVMTypeRef task_create_type = LLVMFunctionType(
        LLVMPointerType(task_type, 0), // GooTask* return type
        (LLVMTypeRef[]){
            LLVMPointerType(LLVMFunctionType(LLVMVoidTypeInContext(context->context),
                (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context->context), 0)},
                1, false), 0), // void (*execute_fn)(void* data)
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
        },
        2, false);
    
    LLVMValueRef task_create_func = LLVMAddFunction(
        context->module,
        "goo_task_create",
        task_create_type);
    
    parallel_init_ok &= (task_create_func != NULL);
    
    // Task destruction function
    LLVMTypeRef task_destroy_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(task_type, 0)}, // GooTask* task
        1, false);
    
    LLVMValueRef task_destroy_func = LLVMAddFunction(
        context->module,
        "goo_task_destroy",
        task_destroy_type);
    
    parallel_init_ok &= (task_destroy_func != NULL);
    
    // Task execution function
    LLVMTypeRef task_execute_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(task_type, 0)}, // GooTask* task
        1, false);
    
    LLVMValueRef task_execute_func = LLVMAddFunction(
        context->module,
        "goo_task_execute",
        task_execute_type);
    
    parallel_init_ok &= (task_execute_func != NULL);
    
    // Thread pool submit function
    LLVMTypeRef thread_pool_submit_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMPointerType(thread_pool_type, 0), // GooThreadPool* pool
            LLVMPointerType(task_type, 0) // GooTask* task
        },
        2, false);
    
    LLVMValueRef thread_pool_submit_func = LLVMAddFunction(
        context->module,
        "goo_thread_pool_submit",
        thread_pool_submit_type);
    
    parallel_init_ok &= (thread_pool_submit_func != NULL);
    
    // Thread pool wait all function
    LLVMTypeRef thread_pool_wait_all_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(thread_pool_type, 0)}, // GooThreadPool* pool
        1, false);
    
    LLVMValueRef thread_pool_wait_all_func = LLVMAddFunction(
        context->module,
        "goo_thread_pool_wait_all",
        thread_pool_wait_all_type);
    
    parallel_init_ok &= (thread_pool_wait_all_func != NULL);
    
    // ParallelFor creation function
    LLVMTypeRef parallel_for_create_type = LLVMFunctionType(
        LLVMPointerType(parallel_for_type, 0), // GooParallelFor* return type
        (LLVMTypeRef[]){
            LLVMPointerType(thread_pool_type, 0), // GooThreadPool* pool
            LLVMInt64TypeInContext(context->context), // size_t start
            LLVMInt64TypeInContext(context->context), // size_t end
            LLVMInt64TypeInContext(context->context), // size_t step
            LLVMPointerType(LLVMFunctionType(LLVMVoidTypeInContext(context->context),
                (LLVMTypeRef[]){
                    LLVMInt64TypeInContext(context->context), // size_t idx
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
                },
                2, false), 0), // void (*fn_ptr)(size_t idx, void* data)
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
        },
        6, false);
    
    LLVMValueRef parallel_for_create_func = LLVMAddFunction(
        context->module,
        "goo_parallel_for_create",
        parallel_for_create_type);
    
    parallel_init_ok &= (parallel_for_create_func != NULL);
    
    // ParallelFor destruction function
    LLVMTypeRef parallel_for_destroy_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(parallel_for_type, 0)}, // GooParallelFor* parallel_for
        1, false);
    
    LLVMValueRef parallel_for_destroy_func = LLVMAddFunction(
        context->module,
        "goo_parallel_for_destroy",
        parallel_for_destroy_type);
    
    parallel_init_ok &= (parallel_for_destroy_func != NULL);
    
    // ParallelFor execution function
    LLVMTypeRef parallel_for_execute_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){LLVMPointerType(parallel_for_type, 0)}, // GooParallelFor* parallel_for
        1, false);
    
    LLVMValueRef parallel_for_execute_func = LLVMAddFunction(
        context->module,
        "goo_parallel_for_execute",
        parallel_for_execute_type);
    
    parallel_init_ok &= (parallel_for_execute_func != NULL);
    
    // ParallelReduce creation function
    LLVMTypeRef parallel_reduce_create_type = LLVMFunctionType(
        LLVMPointerType(parallel_reduce_type, 0), // GooParallelReduce* return type
        (LLVMTypeRef[]){
            LLVMPointerType(thread_pool_type, 0), // GooThreadPool* pool
            LLVMInt64TypeInContext(context->context), // size_t start
            LLVMInt64TypeInContext(context->context), // size_t end
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* identity_value
            LLVMPointerType(LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                (LLVMTypeRef[]){
                    LLVMInt64TypeInContext(context->context), // size_t idx
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
                },
                2, false), 0), // void* (*mapper_fn)(size_t idx, void* data)
            LLVMPointerType(LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                (LLVMTypeRef[]){
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* a
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* b
                },
                2, false), 0), // void* (*reducer_fn)(void* a, void* b)
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
        },
        7, false);
    
    LLVMValueRef parallel_reduce_create_func = LLVMAddFunction(
        context->module,
        "goo_parallel_reduce_create",
        parallel_reduce_create_type);
    
    parallel_init_ok &= (parallel_reduce_create_func != NULL);
    
    // ParallelReduce destruction function
    LLVMTypeRef parallel_reduce_destroy_type = LLVMFunctionType(
        LLVMVoidTypeInContext(context->context), // void return type
        (LLVMTypeRef[]){LLVMPointerType(parallel_reduce_type, 0)}, // GooParallelReduce* parallel_reduce
        1, false);
    
    LLVMValueRef parallel_reduce_destroy_func = LLVMAddFunction(
        context->module,
        "goo_parallel_reduce_destroy",
        parallel_reduce_destroy_type);
    
    parallel_init_ok &= (parallel_reduce_destroy_func != NULL);
    
    // ParallelReduce execution function
    LLVMTypeRef parallel_reduce_execute_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMPointerType(parallel_reduce_type, 0), // GooParallelReduce* parallel_reduce
            LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), 0) // void** result
        },
        2, false);
    
    LLVMValueRef parallel_reduce_execute_func = LLVMAddFunction(
        context->module,
        "goo_parallel_reduce_execute",
        parallel_reduce_execute_type);
    
    parallel_init_ok &= (parallel_reduce_execute_func != NULL);
    
    // Convenience functions
    LLVMTypeRef parallel_for_func_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMInt64TypeInContext(context->context), // size_t start
            LLVMInt64TypeInContext(context->context), // size_t end
            LLVMInt64TypeInContext(context->context), // size_t step
            LLVMPointerType(LLVMFunctionType(LLVMVoidTypeInContext(context->context),
                (LLVMTypeRef[]){
                    LLVMInt64TypeInContext(context->context), // size_t idx
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
                },
                2, false), 0), // void (*fn_ptr)(size_t idx, void* data)
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* data
            LLVMInt64TypeInContext(context->context) // size_t num_threads
        },
        6, false);
    
    LLVMValueRef parallel_for_func = LLVMAddFunction(
        context->module,
        "goo_parallel_for",
        parallel_for_func_type);
    
    parallel_init_ok &= (parallel_for_func != NULL);
    
    LLVMTypeRef parallel_reduce_func_type = LLVMFunctionType(
        LLVMInt1TypeInContext(context->context), // bool return type
        (LLVMTypeRef[]){
            LLVMInt64TypeInContext(context->context), // size_t start
            LLVMInt64TypeInContext(context->context), // size_t end
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* identity_value
            LLVMPointerType(LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                (LLVMTypeRef[]){
                    LLVMInt64TypeInContext(context->context), // size_t idx
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* data
                },
                2, false), 0), // void* (*mapper_fn)(size_t idx, void* data)
            LLVMPointerType(LLVMFunctionType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                (LLVMTypeRef[]){
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* a
                    LLVMPointerType(LLVMInt8TypeInContext(context->context), 0) // void* b
                },
                2, false), 0), // void* (*reducer_fn)(void* a, void* b)
            LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), // void* data
            LLVMPointerType(LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), 0), // void** result
            LLVMInt64TypeInContext(context->context) // size_t num_threads
        },
        8, false);
    
    LLVMValueRef parallel_reduce_func = LLVMAddFunction(
        context->module,
        "goo_parallel_reduce",
        parallel_reduce_func_type);
    
    parallel_init_ok &= (parallel_reduce_func != NULL);

    return memory_init_ok && channel_init_ok && parallel_init_ok;
}

// Enhanced channel send operation with support for distributed channels
    return true;
}

// Enhanced channel send operation with support for distributed channels
LLVMValueRef goo_codegen_enhanced_channel_send(GooCodegenContext* context, GooChannelSendNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the channel
    LLVMValueRef channel = goo_codegen_node(context, node->channel);
    if (!channel) {
        fprintf(stderr, "Failed to generate code for channel\n");
        return NULL;
    }
    
    // Generate code for the value
    LLVMValueRef value = goo_codegen_node(context, node->value);
    if (!value) {
        fprintf(stderr, "Failed to generate code for value\n");
        return NULL;
    }
    
    // Get the function to send data to a channel
    LLVMValueRef func;
    if (node->is_distributed) {
        func = LLVMGetNamedFunction(context->module, "goo_distributed_channel_send");
    } else {
        func = LLVMGetNamedFunction(context->module, "goo_channel_send");
    }
    
    if (!func) {
        fprintf(stderr, "Failed to get channel send function\n");
        return NULL;
    }
    
    // Create a pointer to the value
    LLVMValueRef value_ptr = LLVMBuildAlloca(context->builder, LLVMTypeOf(value), "value_ptr");
    LLVMBuildStore(context->builder, value, value_ptr);
    
    // Cast the pointer to void*
    LLVMValueRef value_ptr_cast = LLVMBuildBitCast(context->builder, value_ptr, 
                                              LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                                              "value_ptr_cast");
    
    // Build the call to send the value
    LLVMValueRef args[2];
    args[0] = channel; // Channel pointer
    args[1] = value_ptr_cast; // Value as void*
    
    return LLVMBuildCall(
        context->builder,
        func,
        args,
        2,
        "send_result"
    );
}

// Enhanced channel receive operation
LLVMValueRef goo_codegen_enhanced_channel_recv(GooCodegenContext* context, GooChannelRecvNode* node) {
    if (!context || !node) return NULL;
    
    // Generate code for the channel
    LLVMValueRef channel = goo_codegen_node(context, node->channel);
    if (!channel) {
        fprintf(stderr, "Failed to generate code for channel\n");
        return NULL;
    }
    
    // Create a temporary variable to store the result
    LLVMTypeRef type = goo_type_to_llvm_type(context, node->result_type);
    if (!type) {
        fprintf(stderr, "Failed to get type for channel receive\n");
        return NULL;
    }
    
    // Allocate space for the result
    LLVMValueRef result_ptr = LLVMBuildAlloca(context->builder, type, "result_ptr");
    
    // Cast the pointer to void*
    LLVMValueRef result_ptr_cast = LLVMBuildBitCast(context->builder, result_ptr, 
                                               LLVMPointerType(LLVMInt8TypeInContext(context->context), 0),
                                               "result_ptr_cast");
    
    // Get the function to receive data from a channel
    LLVMValueRef func = LLVMGetNamedFunction(context->module, "goo_channel_recv");
    if (!func) {
        fprintf(stderr, "Failed to get channel receive function\n");
        return NULL;
    }
    
    // Build the call to receive the value
    LLVMValueRef args[2];
    args[0] = channel; // Channel pointer
    args[1] = result_ptr_cast; // Result pointer as void*
    
    // Call the function and get the success/failure result
    LLVMValueRef success = LLVMBuildCall(
        context->builder,
        func,
        args,
        2,
        "recv_success"
    );
    
    // Load the received value from the result pointer
    LLVMValueRef result = LLVMBuildLoad(context->builder, result_ptr, "recv_value");
    
    // Create a basic block for handling the success case
    LLVMBasicBlockRef success_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(context->builder)),
        "recv_success_block"
    );
    
    // Create a basic block for handling the failure case
    LLVMBasicBlockRef failure_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(context->builder)),
        "recv_failure_block"
    );
    
    // Create a basic block for the continuation
    LLVMBasicBlockRef continue_block = LLVMAppendBasicBlock(
        LLVMGetBasicBlockParent(LLVMGetInsertBlock(context->builder)),
        "recv_continue_block"
    );
    
    // Create a conditional branch based on the success value
    LLVMBuildCondBr(context->builder, success, success_block, failure_block);
    
    // In the success block, we'll just load the result and continue
    LLVMPositionBuilderAtEnd(context->builder, success_block);
    LLVMBuildBr(context->builder, continue_block);
    
    // In the failure block, we'll set a default value (0 for int) and continue
    LLVMPositionBuilderAtEnd(context->builder, failure_block);
    LLVMBuildStore(context->builder, LLVMConstInt(type, 0, false), result_ptr);
    LLVMBuildBr(context->builder, continue_block);
    
    // Continue with the loaded value
    LLVMPositionBuilderAtEnd(context->builder, continue_block);
    LLVMValueRef final_result = LLVMBuildLoad(context->builder, result_ptr, "final_recv_value");
    
    return final_result;
}

// Create a channel with an optional endpoint
LLVMValueRef goo_codegen_channel_create_with_endpoint(GooCodegenContext* context, 
                                                  LLVMTypeRef element_type, 
                                                  int capacity, 
                                                  GooChannelType channel_type,
                                                  const char* endpoint_url) {
    if (!context || !element_type) return NULL;
    
    // Get element size from the type
    LLVMValueRef elem_size = LLVMSizeOf(element_type);
    
    // Create capacity as constant
    LLVMValueRef cap_val = LLVMConstInt(LLVMInt32TypeInContext(context->context), capacity, false);
    
    // Create channel type as constant
    LLVMValueRef type_val = LLVMConstInt(LLVMInt32TypeInContext(context->context), channel_type, false);
    
    // Create channel options
    LLVMTypeRef options_type = goo_type_table_get_type(context->type_table, "GooChannelOptions");
    LLVMValueRef options = LLVMBuildAlloca(context->builder, options_type, "channel_options");
    
    // Set buffer_size field
    LLVMValueRef buffer_size_ptr = LLVMBuildStructGEP2(context->builder, options_type, options, 0, "buffer_size_ptr");
    LLVMBuildStore(context->builder, cap_val, buffer_size_ptr);
    
    // Set is_blocking field (true by default)
    LLVMValueRef is_blocking_ptr = LLVMBuildStructGEP2(context->builder, options_type, options, 1, "is_blocking_ptr");
    LLVMBuildStore(context->builder, LLVMConstInt(LLVMInt1TypeInContext(context->context), 1, false), is_blocking_ptr);
    
    // Set pattern field
    LLVMValueRef pattern_ptr = LLVMBuildStructGEP2(context->builder, options_type, options, 2, "pattern_ptr");
    LLVMBuildStore(context->builder, type_val, pattern_ptr);
    
    // Set timeout_ms field (default to -1 for infinite)
    LLVMValueRef timeout_ptr = LLVMBuildStructGEP2(context->builder, options_type, options, 3, "timeout_ptr");
    LLVMBuildStore(context->builder, LLVMConstInt(LLVMInt32TypeInContext(context->context), -1, true), timeout_ptr);
    
    // Create and distribute channel based on the endpoint
    LLVMValueRef channel;
    if (endpoint_url) {
        // Convert endpoint to string constant
        LLVMValueRef endpoint_str = goo_codegen_create_string_constant(context, endpoint_url);
        
        // Call the distributed channel creation function
        LLVMTypeRef func_type = LLVMFunctionType(
            LLVMPointerType(goo_type_table_get_type(context->type_table, "GooChannel"), 0),
            (LLVMTypeRef[]){LLVMPointerType(options_type, 0), LLVMPointerType(LLVMInt8TypeInContext(context->context), 0), LLVMInt32TypeInContext(context->context)},
            3,
            false
        );
        
        LLVMValueRef create_func = goo_symbol_table_get_function(
            context->symbol_table, 
            "goo_create_distributed_channel",
            func_type
        );
        
        if (!create_func) {
            fprintf(stderr, "Failed to find goo_create_distributed_channel function\n");
            return NULL;
        }
        
        // Call function with options, endpoint, and element size
        channel = LLVMBuildCall2(
            context->builder,
            func_type,
            create_func,
            (LLVMValueRef[]){options, endpoint_str, elem_size},
            3,
            "distributed_channel"
        );
    } else {
        // Call the normal channel creation function
        LLVMTypeRef func_type = LLVMFunctionType(
            LLVMPointerType(goo_type_table_get_type(context->type_table, "GooChannel"), 0),
            (LLVMTypeRef[]){LLVMPointerType(options_type, 0)},
            1,
            false
        );
        
        LLVMValueRef create_func = goo_symbol_table_get_function(
            context->symbol_table, 
            "goo_channel_create",
            func_type
        );
        
        if (!create_func) {
            fprintf(stderr, "Failed to find goo_channel_create function\n");
            return NULL;
        }
        
        channel = LLVMBuildCall2(
            context->builder,
            func_type,
            create_func,
            (LLVMValueRef[]){options},
            1,
            "channel"
        );
    }
    
    return channel;
}

// Initialize the runtime in the main function
void goo_codegen_init_main_runtime(GooCodegenContext* context, LLVMValueRef main_func) {
    if (!context || !main_func) return;
    
    // Get the entry block
    LLVMBasicBlockRef entry = LLVMGetEntryBasicBlock(main_func);
    
    // Position at the beginning of the entry block
    LLVMBuilderRef builder = context->builder;
    LLVMValueRef first_instr = LLVMGetFirstInstruction(entry);
    
    if (first_instr) {
        LLVMPositionBuilderBefore(builder, first_instr);
    } else {
        LLVMPositionBuilderAtEnd(builder, entry);
    }
    
    // Get the runtime init function
    LLVMValueRef init_func = LLVMGetNamedFunction(context->module, "goo_runtime_init");
    if (!init_func) {
        fprintf(stderr, "Failed to get runtime init function\n");
        return;
    }
    
    // Call the runtime init function
    LLVMValueRef init_result = LLVMBuildCall(builder, init_func, NULL, 0, "runtime_init");
    
    // Check if initialization succeeded
    LLVMBasicBlockRef runtime_ok = LLVMAppendBasicBlock(main_func, "runtime_ok");
    LLVMBasicBlockRef runtime_error = LLVMAppendBasicBlock(main_func, "runtime_error");
    
    LLVMBuildCondBr(builder, init_result, runtime_ok, runtime_error);
    
    // Handle initialization error
    LLVMPositionBuilderAtEnd(builder, runtime_error);
    
    // Print error message and exit
    LLVMValueRef error_str = LLVMBuildGlobalStringPtr(builder, 
                                               "Failed to initialize runtime\n", 
                                               "runtime_error_msg");
    
    // Get the printf function
    LLVMValueRef printf_func = LLVMGetNamedFunction(context->module, "printf");
    if (!printf_func) {
        printf_func = LLVMAddFunction(context->module, "printf",
                                  LLVMFunctionType(LLVMInt32TypeInContext(context->context),
                                               (LLVMTypeRef[]){LLVMPointerType(LLVMInt8TypeInContext(context->context), 0)},
                                               1, 1));
    }
    
    // Print the error message
    LLVMValueRef printf_args[] = { error_str };
    LLVMBuildCall(builder, printf_func, printf_args, 1, "");
    
    // Return with error code
    LLVMBuildRet(builder, LLVMConstInt(LLVMInt32TypeInContext(context->context), 1, 0));
    
    // Continue with normal execution
    LLVMPositionBuilderAtEnd(builder, runtime_ok);
    
    // Add cleanup at the end of the main function
    LLVMBasicBlockRef last_block = LLVMGetLastBasicBlock(main_func);
    LLVMPositionBuilderAtEnd(builder, last_block);
    
    // Find the terminator instruction
    LLVMValueRef terminator = LLVMGetBasicBlockTerminator(last_block);
    
    if (terminator) {
        LLVMPositionBuilderBefore(builder, terminator);
    }
    
    // Get the runtime cleanup function
    LLVMValueRef cleanup_func = LLVMGetNamedFunction(context->module, "goo_runtime_cleanup");
    if (!cleanup_func) {
        fprintf(stderr, "Failed to get runtime cleanup function\n");
        return;
    }
    
    // Call the cleanup function
    LLVMBuildCall(builder, cleanup_func, NULL, 0, "");
}

// Function for implementing compile-time SIMD blocks
LLVMValueRef goo_codegen_comptime_simd(GooCodegenContext* context, GooNode* node) {
    if (!context || !node) return NULL;
    
    // Get the SIMD block node
    GooComptimeSIMDNode* simd_node = (GooComptimeSIMDNode*)node;
    
    // Create or get the global SIMD context
    GooComptimeSIMD* simd_ctx = NULL;
    
    // Check if the SIMD context already exists in the GooContext
    if (!context->runtime_context->simd_ctx) {
        // Initialize a new SIMD context
        simd_ctx = malloc(sizeof(GooComptimeSIMD));
        if (!simd_ctx) {
            fprintf(stderr, "Failed to allocate memory for SIMD context\n");
            return NULL;
        }
        
        // Set default values
        simd_ctx->types = NULL;
        simd_ctx->type_count = 0;
        simd_ctx->target_arch = GOO_SIMD_AUTO;
        simd_ctx->allow_fallback = true;
        simd_ctx->runtime_detection = true;
        
        // Store the SIMD context in the runtime context
        context->runtime_context->simd_ctx = simd_ctx;
    } else {
        simd_ctx = context->runtime_context->simd_ctx;
    }
    
    // Process each declaration in the SIMD block
    GooNode* decl = simd_node->block;
    while (decl) {
        switch (decl->type) {
            case GOO_NODE_SIMD_TYPE_DECL: {
                GooSIMDTypeNode* type_node = (GooSIMDTypeNode*)decl;
                
                // Add the SIMD type to the context
                GooComptimeSIMDType* simd_type = malloc(sizeof(GooComptimeSIMDType));
                if (!simd_type) {
                    fprintf(stderr, "Failed to allocate memory for SIMD type\n");
                    break;
                }
                
                // Set the type properties
                simd_type->data_type = type_node->data_type;
                simd_type->vector_width = type_node->vector_width;
                simd_type->simd_impl = type_node->simd_type;
                simd_type->is_aligned = (type_node->alignment > 0);
                simd_type->alignment = type_node->alignment;
                simd_type->is_safe = type_node->is_safe;
                
                // Add the type to the SIMD context's type array
                GooComptimeSIMDType** new_types = realloc(simd_ctx->types, 
                                                   (simd_ctx->type_count + 1) * sizeof(GooComptimeSIMDType*));
                if (!new_types) {
                    fprintf(stderr, "Failed to reallocate memory for SIMD types array\n");
                    free(simd_type);
                    break;
                }
                
                simd_ctx->types = new_types;
                simd_ctx->types[simd_ctx->type_count++] = simd_type;
                
                // Add the type name to the symbol table
                // This allows the type to be used by name in the code
                LLVMTypeRef llvm_type = NULL;
                
                // Create an LLVM vector type based on the element type and width
                LLVMTypeRef elem_type = NULL;
                switch (type_node->data_type) {
                    case GOO_VEC_INT8:
                    case GOO_VEC_UINT8:
                        elem_type = LLVMInt8TypeInContext(context->context);
                        break;
                    case GOO_VEC_INT16:
                    case GOO_VEC_UINT16:
                        elem_type = LLVMInt16TypeInContext(context->context);
                        break;
                    case GOO_VEC_INT32:
                    case GOO_VEC_UINT32:
                        elem_type = LLVMInt32TypeInContext(context->context);
                        break;
                    case GOO_VEC_INT64:
                    case GOO_VEC_UINT64:
                        elem_type = LLVMInt64TypeInContext(context->context);
                        break;
                    case GOO_VEC_FLOAT:
                        elem_type = LLVMFloatTypeInContext(context->context);
                        break;
                    case GOO_VEC_DOUBLE:
                        elem_type = LLVMDoubleTypeInContext(context->context);
                        break;
                    default:
                        fprintf(stderr, "Unsupported vector element type\n");
                        break;
                }
                
                if (elem_type) {
                    // Create the vector type
                    llvm_type = LLVMVectorType(elem_type, type_node->vector_width);
                    
                    // Add it to the symbol table
                    goo_symbol_table_add(context->symbol_table, type_node->name, GOO_SYMBOL_TYPE, 
                                     NULL, (GooNode*)type_node, llvm_type);
                }
                break;
            }
            
            case GOO_NODE_SIMD_OP_DECL: {
                GooSIMDOpNode* op_node = (GooSIMDOpNode*)decl;
                
                // Process the operation declaration
                // In a real implementation, we would analyze the operation and generate
                // appropriate LLVM IR for it. For simplicity, we're just recording it.
                
                // Add the operation to the symbol table
                GooComptimeSIMDOperation* simd_op = malloc(sizeof(GooComptimeSIMDOperation));
                if (!simd_op) {
                    fprintf(stderr, "Failed to allocate memory for SIMD operation\n");
                    break;
                }
                
                // Set operation properties
                simd_op->op = op_node->op;
                simd_op->has_mask = op_node->is_masked;
                simd_op->is_fused = op_node->is_fused;
                
                // Find the vector type for this operation
                if (op_node->vec_type && op_node->vec_type->type == GOO_NODE_IDENTIFIER) {
                    char* type_name = ((struct GooIdentifierNode*)op_node->vec_type)->name;
                    
                    // Look up the type in the symbol table
                    GooSymbol* sym = goo_symbol_table_lookup(context->symbol_table, type_name);
                    if (sym && sym->kind == GOO_SYMBOL_TYPE) {
                        // Find the corresponding comptime SIMD type
                        for (size_t i = 0; i < simd_ctx->type_count; i++) {
                            GooSIMDTypeNode* type_node = (GooSIMDTypeNode*)sym->node;
                            if (strcmp(type_node->name, type_name) == 0) {
                                simd_op->vec_type = simd_ctx->types[i];
                                simd_op->is_safe = simd_ctx->types[i]->is_safe;
                                break;
                            }
                        }
                    }
                }
                
                // Add operation to symbol table
                goo_symbol_table_add(context->symbol_table, op_node->name, GOO_SYMBOL_FUNCTION, 
                                   NULL, (GooNode*)op_node, NULL);
                break;
            }
            
            default:
                // Handle config options
                break;
        }
        
        decl = decl->next;
    }
    
    // Return a dummy value (SIMD blocks don't produce values at runtime)
    return LLVMConstInt(LLVMInt32TypeInContext(context->context), 0, 0);
}

// ... existing code ... 