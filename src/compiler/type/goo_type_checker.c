/**
 * goo_type_checker.c
 * 
 * Type checking implementation for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "goo_type_system.h"
#include "goo_type_utils.h"
#include "type_error_codes.h"
#include "ast_simple.h"
#include "type_error_adapter.h"

// Helper function to report a type error
static void report_type_error(GooTypeContext* ctx, GooAstNode* node, const char* error_code, const char* message) {
    if (!ctx || !node) {
        return;
    }
    
    // Use our adapter function to report the error
    goo_type_report_error(ctx, (void*)node, error_code, message);
}

// Helper function to report a type mismatch error
static void report_type_mismatch(GooTypeContext* ctx, GooAstNode* node, 
                                GooType* expected, GooType* found) {
    if (!ctx || !node) {
        return;
    }
    
    // Use our adapter function to report the type mismatch
    goo_type_report_mismatch(ctx, (void*)node, expected, found);
}

// Helper function to add a note to a diagnostic
static void add_diagnostic_note(GooTypeContext* ctx, GooAstNode* node, const char* message) {
    if (!ctx || !node) {
        return;
    }
    
    // Use our adapter function to add a note
    goo_type_add_note(ctx, (void*)node, message);
}

// Helper function to add a suggestion to a diagnostic
static void add_diagnostic_suggestion(GooTypeContext* ctx, GooAstNode* node, 
                                    const char* message, const char* replacement) {
    if (!ctx || !node) {
        return;
    }
    
    // Use our adapter function to add a suggestion
    goo_type_add_suggestion(ctx, (void*)node, message, replacement);
}

// Helper function to check if we should abort due to errors
static bool should_abort_due_to_errors(GooTypeContext* ctx) {
    if (!ctx) {
        return false;
    }
    
    // Use our adapter function to check if we should abort
    return goo_type_should_abort(ctx);
}

// Helper function to enter a new scope
static void goo_type_system_enter_scope(GooTypeContext* ctx) {
    if (!ctx) return;
    ctx->current_scope_level++;
}

// Helper function to exit a scope
static void goo_type_system_exit_scope(GooTypeContext* ctx) {
    if (!ctx) return;
    if (ctx->current_scope_level > 0) {
        ctx->current_scope_level--;
    }
}

// Type check a module
bool goo_type_system_check_module(GooTypeContext* ctx, GooAstNode* module) {
    if (!ctx || !module) return false;
    
    // Enter the module scope
    goo_type_system_enter_scope(ctx);
    
    bool success = true;
    
    // Process declarations in the module
    if (module->type == GOO_NODE_MODULE) {
        GooModuleDeclNode* module_node = (GooModuleDeclNode*)module;
        GooAstNode* decl = module_node->declarations;
        
        while (decl) {
            // Type check each declaration
            switch (decl->type) {
                case GOO_NODE_FUNCTION_DECL:
                    if (!goo_type_system_check_function_decl(ctx, decl)) {
                        success = false;
                    }
                    break;
                    
                case GOO_NODE_VAR_DECL:
                    if (!goo_type_system_check_var_decl(ctx, decl)) {
                        success = false;
                    }
                    break;
                    
                case GOO_NODE_STRUCT:
                    // Handle struct declaration (TBD)
                    break;
                    
                case GOO_NODE_ENUM:
                    // Handle enum declaration (TBD)
                    break;
                    
                case GOO_NODE_TRAIT:
                    // Handle trait declaration (TBD)
                    break;
                    
                default:
                    // Skip other node types for now
                    break;
            }
            
            decl = decl->next;
        }
    }
    
    // Exit the module scope
    goo_type_system_exit_scope(ctx);
    
    return success;
}

// Type check a function declaration
GooType* goo_type_system_check_function_decl(GooTypeContext* ctx, GooAstNode* function_decl) {
    if (!ctx || !function_decl || function_decl->type != GOO_NODE_FUNCTION_DECL) {
        return NULL;
    }
    
    GooFunctionNode* func = (GooFunctionNode*)function_decl;
    
    // Enter function scope
    goo_type_system_enter_scope(ctx);
    
    // Type check return type
    GooType* return_type = NULL;
    if (func->return_type) {
        return_type = goo_type_system_check_expr(ctx, func->return_type);
    } else {
        // Default to unit type if no return type specified
        return_type = goo_type_system_create_bool_type(ctx); // Use appropriate default
    }
    
    // Type check parameters
    GooAstNode* param = func->params;
    GooType** param_types = NULL;
    int param_count = 0;
    
    // Count parameters and allocate array
    GooAstNode* temp_param = param;
    while (temp_param) {
        param_count++;
        temp_param = temp_param->next;
    }
    
    if (param_count > 0) {
        param_types = (GooType**)malloc(sizeof(GooType*) * param_count);
        
        // Type check each parameter
        int i = 0;
        while (param) {
            if (param->type == GOO_NODE_PARAM) {
                GooParamNode* param_node = (GooParamNode*)param;
                
                // Check parameter type
                GooType* param_type = NULL;
                if (param_node->type) {
                    param_type = goo_type_system_check_expr(ctx, param_node->type);
                } else {
                    // Error: Parameter must have a type
                    report_type_error(ctx, param, GOO_ERR_PARAMETER_TYPE, "Parameter must have a type");
                    param_type = NULL; // Use unknown type
                }
                
                param_types[i++] = param_type;
            }
            
            param = param->next;
        }
    }
    
    // Create the function type
    GooType* func_type = goo_type_system_create_function_type(
        ctx, return_type, param_types, param_count, func->is_unsafe, func->is_kernel);
    
    // Type check function body if present
    if (func->body) {
        goo_type_system_check_stmt(ctx, func->body);
    }
    
    // Exit function scope
    goo_type_system_exit_scope(ctx);
    
    // Free parameter types array (not the types themselves)
    free(param_types);
    
    return func_type;
}

// Type check a variable declaration
GooType* goo_type_system_check_var_decl(GooTypeContext* ctx, GooAstNode* var_decl) {
    if (!ctx || !var_decl || var_decl->type != GOO_NODE_VAR_DECL) {
        return NULL;
    }
    
    GooVarDeclNode* var = (GooVarDeclNode*)var_decl;
    GooType* var_type = NULL;
    GooType* init_type = NULL;
    
    // Check the declared type if present
    if (var->type) {
        var_type = goo_type_system_check_expr(ctx, var->type);
    }
    
    // Check the initializer expression if present
    if (var->init_expr) {
        init_type = goo_type_system_check_expr(ctx, var->init_expr);
    }
    
    // If no explicit type but has initializer, infer type from initializer
    if (!var_type && init_type) {
        var_type = init_type;
    } else if (!var_type) {
        // Error: Cannot infer type
        report_type_error(ctx, var_decl, GOO_ERR_TYPE_INFERENCE, "Cannot infer type for variable declaration");
        return NULL;
    }
    
    // If both type and initializer are present, check type compatibility
    if (var_type && init_type) {
        if (!goo_type_system_unify(ctx, var_type, init_type)) {
            // Error: Type mismatch
            report_type_mismatch(ctx, var_decl, var_type, init_type);
        }
    }
    
    return var_type;
}

// Type check a binary expression
GooType* goo_type_system_check_binary_expr(GooTypeContext* ctx, GooAstNode* binary_expr) {
    if (!ctx || !binary_expr || binary_expr->type != GOO_NODE_BINARY_EXPR) {
        return NULL;
    }
    
    GooBinaryExprNode* expr = (GooBinaryExprNode*)binary_expr;
    
    // Type check operands
    GooType* left_type = goo_type_system_check_expr(ctx, expr->left);
    GooType* right_type = goo_type_system_check_expr(ctx, expr->right);
    
    if (!left_type || !right_type) {
        return NULL;
    }
    
    // Handle different operators
    switch (expr->operator) {
        // Arithmetic operators
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
            // Check if both operands are numeric
            if ((left_type->kind == GOO_TYPE_INT || left_type->kind == GOO_TYPE_FLOAT) &&
                (right_type->kind == GOO_TYPE_INT || right_type->kind == GOO_TYPE_FLOAT)) {
                
                // If either operand is float, result is float
                if (left_type->kind == GOO_TYPE_FLOAT || right_type->kind == GOO_TYPE_FLOAT) {
                    return goo_type_system_create_float_type(ctx, GOO_FLOAT_64); // Use appropriate precision
                } else {
                    // Result is int with appropriate width
                    GooIntWidth width = left_type->int_info.width > right_type->int_info.width ? 
                                        left_type->int_info.width : right_type->int_info.width;
                    bool is_signed = left_type->int_info.is_signed || right_type->int_info.is_signed;
                    return goo_type_system_create_int_type(ctx, width, is_signed);
                }
            } else {
                // Error: Invalid operands for arithmetic
                report_type_error(ctx, binary_expr, GOO_ERR_INVALID_OPERANDS, "Invalid operands for arithmetic operation");
                return NULL;
            }
            
        // Comparison operators
        case '<':
        case '>':
        case GOO_OP_LE: // Assuming LE is defined as a constant
        case GOO_OP_GE: // Assuming GE is defined as a constant
            // Check if both operands are comparable
            if ((left_type->kind == GOO_TYPE_INT || left_type->kind == GOO_TYPE_FLOAT) &&
                (right_type->kind == GOO_TYPE_INT || right_type->kind == GOO_TYPE_FLOAT)) {
                return goo_type_system_create_bool_type(ctx);
            } else {
                // Error: Invalid operands for comparison
                report_type_error(ctx, binary_expr, GOO_ERR_INVALID_OPERANDS, "Invalid operands for comparison operation");
                return NULL;
            }
            
        // Equality operators
        case GOO_OP_EQ: // Assuming EQ is defined as a constant
        case GOO_OP_NE: // Assuming NE is defined as a constant
            // Almost any types can be compared for equality
            if (goo_type_system_unify(ctx, left_type, right_type)) {
                return goo_type_system_create_bool_type(ctx);
            } else {
                // Error: Cannot compare these types for equality
                report_type_error(ctx, binary_expr, GOO_ERR_TYPE_COMPARISON, "Cannot compare these types for equality");
                return NULL;
            }
            
        // Logical operators
        case GOO_OP_AND: // Assuming AND is defined as a constant
        case GOO_OP_OR:  // Assuming OR is defined as a constant
            // Check if both operands are boolean
            if (left_type->kind == GOO_TYPE_BOOL && right_type->kind == GOO_TYPE_BOOL) {
                return goo_type_system_create_bool_type(ctx);
            } else {
                // Error: Invalid operands for logical operation
                report_type_error(ctx, binary_expr, GOO_ERR_INVALID_OPERANDS, "Invalid operands for logical operation");
                return NULL;
            }
            
        default:
            // Unsupported operator
            report_type_error(ctx, binary_expr, GOO_ERR_UNSUPPORTED_OPERATOR, "Unsupported binary operator");
            return NULL;
    }
}

// Type check a unary expression
GooType* goo_type_system_check_unary_expr(GooTypeContext* ctx, GooAstNode* unary_expr) {
    if (!ctx || !unary_expr || unary_expr->type != GOO_NODE_UNARY_EXPR) {
        return NULL;
    }
    
    GooUnaryExprNode* expr = (GooUnaryExprNode*)unary_expr;
    
    // Type check operand
    GooType* operand_type = goo_type_system_check_expr(ctx, expr->expr);
    
    if (!operand_type) {
        return NULL;
    }
    
    // Handle different operators
    switch (expr->operator) {
        // Negation
        case '-':
            // Check if operand is numeric
            if (operand_type->kind == GOO_TYPE_INT || operand_type->kind == GOO_TYPE_FLOAT) {
                return operand_type; // Return same type
            } else {
                // Error: Invalid operand for negation
                report_type_error(ctx, unary_expr, GOO_ERR_INVALID_OPERAND, "Invalid operand for negation");
                return NULL;
            }
            
        // Logical NOT
        case '!':
            // Check if operand is boolean
            if (operand_type->kind == GOO_TYPE_BOOL) {
                return goo_type_system_create_bool_type(ctx);
            } else {
                // Error: Invalid operand for logical NOT
                report_type_error(ctx, unary_expr, GOO_ERR_INVALID_OPERAND, "Invalid operand for logical NOT");
                return NULL;
            }
            
        // Reference
        case '&':
            // Create a reference type
            return goo_type_system_create_ref_type(ctx, operand_type, NULL, false);
            
        // Mutable reference
        case GOO_OP_MUT_REF: // Assuming MUT_REF is defined as a constant
            // Create a mutable reference type
            return goo_type_system_create_ref_type(ctx, operand_type, NULL, true);
            
        // Dereference
        case '*':
            // Check if operand is a reference
            if (operand_type->kind == GOO_TYPE_REF || operand_type->kind == GOO_TYPE_MUT_REF) {
                return operand_type->ref_info.referenced_type;
            } else {
                // Error: Cannot dereference non-reference type
                report_type_error(ctx, unary_expr, GOO_ERR_NON_REFERENCE, "Cannot dereference non-reference type");
                return NULL;
            }
            
        default:
            // Unsupported operator
            report_type_error(ctx, unary_expr, GOO_ERR_UNSUPPORTED_OPERATOR, "Unsupported unary operator");
            return NULL;
    }
}

// Type check a function call expression
GooType* goo_type_system_check_call_expr(GooTypeContext* ctx, GooAstNode* call_expr) {
    if (!ctx || !call_expr || call_expr->type != GOO_NODE_CALL_EXPR) {
        return NULL;
    }
    
    GooCallExprNode* expr = (GooCallExprNode*)call_expr;
    
    // Type check the function expression
    GooType* func_type = goo_type_system_check_expr(ctx, expr->func);
    
    if (!func_type) {
        return NULL;
    }
    
    // Check if the expression is a function
    if (func_type->kind != GOO_TYPE_FUNCTION) {
        report_type_error(ctx, call_expr, GOO_ERR_NON_FUNCTION, "Cannot call a non-function type");
        return NULL;
    }
    
    // Count arguments
    int arg_count = 0;
    GooAstNode* arg = expr->args;
    while (arg) {
        arg_count++;
        arg = arg->next;
    }
    
    // Check argument count
    if (arg_count != func_type->function_info.param_count) {
        report_type_error(ctx, call_expr, GOO_ERR_ARGUMENT_COUNT, "Wrong number of arguments");
        return NULL;
    }
    
    // Type check each argument against parameter types
    arg = expr->args;
    for (int i = 0; i < arg_count && arg; i++) {
        GooType* arg_type = goo_type_system_check_expr(ctx, arg);
        
        if (!arg_type) {
            return NULL;
        }
        
        // Check argument type against parameter type
        if (!goo_type_system_unify(ctx, arg_type, func_type->function_info.param_types[i])) {
            report_type_mismatch(ctx, arg, func_type->function_info.param_types[i], arg_type);
        }
        
        arg = arg->next;
    }
    
    // Return the function's return type
    return func_type->function_info.return_type;
}

// Type check an if statement
GooType* goo_type_system_check_if_stmt(GooTypeContext* ctx, GooAstNode* if_stmt) {
    if (!ctx || !if_stmt || if_stmt->type != GOO_NODE_IF_STMT) {
        return NULL;
    }
    
    GooIfStmtNode* stmt = (GooIfStmtNode*)if_stmt;
    
    // Type check condition
    GooType* cond_type = goo_type_system_check_expr(ctx, stmt->condition);
    
    if (!cond_type) {
        return NULL;
    }
    
    // Check if condition is boolean
    if (cond_type->kind != GOO_TYPE_BOOL) {
        report_type_error(ctx, stmt->condition, GOO_ERR_TYPE_CONDITION, "If condition must be a boolean expression");
    }
    
    // Enter then block scope
    goo_type_system_enter_scope(ctx);
    
    // Type check then block
    if (stmt->then_block) {
        goo_type_system_check_stmt(ctx, stmt->then_block);
    }
    
    // Exit then block scope
    goo_type_system_exit_scope(ctx);
    
    // Type check else block if present
    if (stmt->else_block) {
        if (stmt->else_block->type == GOO_NODE_IF_STMT) {
            // This is an else-if, type check it directly
            goo_type_system_check_if_stmt(ctx, stmt->else_block);
        } else {
            // Enter else block scope
            goo_type_system_enter_scope(ctx);
            
            // Type check else block
            goo_type_system_check_stmt(ctx, stmt->else_block);
            
            // Exit else block scope
            goo_type_system_exit_scope(ctx);
        }
    }
    
    // If statements have unit type
    return NULL;
}

// Type check a for statement
GooType* goo_type_system_check_for_stmt(GooTypeContext* ctx, GooAstNode* for_stmt) {
    if (!ctx || !for_stmt || for_stmt->type != GOO_NODE_FOR_STMT) {
        return NULL;
    }
    
    GooForStmtNode* stmt = (GooForStmtNode*)for_stmt;
    
    // Enter for loop scope
    goo_type_system_enter_scope(ctx);
    
    // Type check initialization if present
    if (stmt->init_expr) {
        goo_type_system_check_expr(ctx, stmt->init_expr);
    }
    
    // Type check condition
    if (stmt->condition) {
        GooType* cond_type = goo_type_system_check_expr(ctx, stmt->condition);
        
        if (cond_type && cond_type->kind != GOO_TYPE_BOOL && !stmt->is_range) {
            report_type_error(ctx, stmt->condition, GOO_ERR_TYPE_CONDITION, "For loop condition must be a boolean expression");
        }
    }
    
    // Type check update expression if present
    if (stmt->update_expr) {
        goo_type_system_check_expr(ctx, stmt->update_expr);
    }
    
    // Type check loop body
    if (stmt->body) {
        goo_type_system_check_stmt(ctx, stmt->body);
    }
    
    // Exit for loop scope
    goo_type_system_exit_scope(ctx);
    
    // For statements have unit type
    return NULL;
}

// Type check a return statement
GooType* goo_type_system_check_return_stmt(GooTypeContext* ctx, GooAstNode* return_stmt) {
    if (!ctx || !return_stmt || return_stmt->type != GOO_NODE_RETURN_STMT) {
        return NULL;
    }
    
    GooReturnStmtNode* stmt = (GooReturnStmtNode*)return_stmt;
    
    // Type check return expression if present
    if (stmt->expr) {
        GooType* expr_type = goo_type_system_check_expr(ctx, stmt->expr);
        return expr_type;
    }
    
    // Empty return has unit type
    return NULL;
}

// Type check a channel send operation
GooType* goo_type_system_check_channel_send(GooTypeContext* ctx, GooAstNode* send_expr) {
    if (!ctx || !send_expr || send_expr->type != GOO_NODE_CHANNEL_SEND) {
        return NULL;
    }
    
    GooChannelSendNode* expr = (GooChannelSendNode*)send_expr;
    
    // Type check channel expression
    GooType* channel_type = goo_type_system_check_expr(ctx, expr->channel);
    
    if (!channel_type) {
        return NULL;
    }
    
    // Check if expression is a channel
    if (channel_type->kind != GOO_TYPE_CHANNEL) {
        report_type_error(ctx, expr->channel, GOO_ERR_TYPE_CHANNEL, "Expected a channel type");
        return NULL;
    }
    
    // Type check value expression
    GooType* value_type = goo_type_system_check_expr(ctx, expr->value);
    
    if (!value_type) {
        return NULL;
    }
    
    // Check if value type is compatible with channel element type
    if (!goo_type_system_unify(ctx, value_type, channel_type->channel_info.element_type)) {
        report_type_mismatch(ctx, expr->value, channel_type->channel_info.element_type, value_type);
    }
    
    // Channel send operations have unit type
    return NULL;
}

// Type check a channel receive operation
GooType* goo_type_system_check_channel_recv(GooTypeContext* ctx, GooAstNode* recv_expr) {
    if (!ctx || !recv_expr || recv_expr->type != GOO_NODE_CHANNEL_RECV) {
        return NULL;
    }
    
    GooChannelRecvNode* expr = (GooChannelRecvNode*)recv_expr;
    
    // Type check channel expression
    GooType* channel_type = goo_type_system_check_expr(ctx, expr->channel);
    
    if (!channel_type) {
        return NULL;
    }
    
    // Check if expression is a channel
    if (channel_type->kind != GOO_TYPE_CHANNEL) {
        report_type_error(ctx, expr->channel, GOO_ERR_TYPE_CHANNEL, "Expected a channel type");
        return NULL;
    }
    
    // Channel receive operations return the channel's element type
    return channel_type->channel_info.element_type;
}

// Entry point for type checking an expression
GooType* goo_type_system_check_expr(GooTypeContext* ctx, GooAstNode* expr) {
    if (!ctx || !expr) return NULL;
    
    switch (expr->type) {
        case GOO_NODE_BINARY_EXPR:
            return goo_type_system_check_binary_expr(ctx, expr);
            
        case GOO_NODE_UNARY_EXPR:
            return goo_type_system_check_unary_expr(ctx, expr);
            
        case GOO_NODE_CALL_EXPR:
            return goo_type_system_check_call_expr(ctx, expr);
            
        case GOO_NODE_CHANNEL_SEND:
            return goo_type_system_check_channel_send(ctx, expr);
            
        case GOO_NODE_CHANNEL_RECV:
            return goo_type_system_check_channel_recv(ctx, expr);
            
        case GOO_NODE_VAR_DECL:
            return goo_type_system_check_var_decl(ctx, expr);
            
        case GOO_NODE_INT_LITERAL:
            // Integer literals have int type
            return goo_type_system_create_int_type(ctx, GOO_INT_64, true);
            
        case GOO_NODE_FLOAT_LITERAL:
            // Float literals have float type
            return goo_type_system_create_float_type(ctx, GOO_FLOAT_64);
            
        case GOO_NODE_BOOL_LITERAL:
            // Boolean literals have bool type
            return goo_type_system_create_bool_type(ctx);
            
        case GOO_NODE_STRING_LITERAL:
            // String literals have string type
            return goo_type_system_create_string_type(ctx);
            
        case GOO_NODE_IDENTIFIER:
            // For identifiers, we need to look up the type in a symbol table
            // This is just a stub - actual implementation would depend on symbol table
            report_type_error(ctx, expr, GOO_ERR_IDENTIFIER_RESOLUTION, "Identifier resolution not implemented yet");
            return NULL;
            
        default:
            // Unsupported expression type
            report_type_error(ctx, expr, GOO_ERR_UNSUPPORTED_EXPRESSION, "Unsupported expression type");
            return NULL;
    }
}

// Entry point for type checking a statement
GooType* goo_type_system_check_stmt(GooTypeContext* ctx, GooAstNode* stmt) {
    if (!ctx || !stmt) return NULL;
    
    switch (stmt->type) {
        case GOO_NODE_BLOCK_STMT:
            // Enter block scope
            goo_type_system_enter_scope(ctx);
            
            // Type check each statement in the block
            if (stmt->type == GOO_NODE_BLOCK_STMT) {
                GooBlockStmtNode* block = (GooBlockStmtNode*)stmt;
                GooAstNode* current = block->statements;
                
                while (current) {
                    goo_type_system_check_stmt(ctx, current);
                    current = current->next;
                }
            }
            
            // Exit block scope
            goo_type_system_exit_scope(ctx);
            return NULL;
            
        case GOO_NODE_IF_STMT:
            return goo_type_system_check_if_stmt(ctx, stmt);
            
        case GOO_NODE_FOR_STMT:
            return goo_type_system_check_for_stmt(ctx, stmt);
            
        case GOO_NODE_RETURN_STMT:
            return goo_type_system_check_return_stmt(ctx, stmt);
            
        case GOO_NODE_EXPR_STMT:
            // Type check the expression
            if (stmt->type == GOO_NODE_EXPR_STMT) {
                GooAstNode* expr = ((GooExprStmtNode*)stmt)->expr;
                return goo_type_system_check_expr(ctx, expr);
            }
            return NULL;
            
        case GOO_NODE_VAR_DECL:
            return goo_type_system_check_var_decl(ctx, stmt);
            
        default:
            // Unsupported statement type
            report_type_error(ctx, stmt, GOO_ERR_UNSUPPORTED_STATEMENT, "Unsupported statement type");
            return NULL;
    }
} 