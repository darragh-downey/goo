#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../ast/ast.h"
#include "optimizer.h"

// Object allocation state
typedef enum {
    ALLOC_UNKNOWN,       // Initial state, unknown if escapes
    ALLOC_NO_ESCAPE,     // Object does not escape its allocating function
    ALLOC_ARG_ESCAPE,    // Object escapes as an argument to another function
    ALLOC_RETURN_ESCAPE, // Object escapes by being returned
    ALLOC_GLOBAL_ESCAPE, // Object escapes to global scope (or stored in global)
    ALLOC_GOROUTINE_ESCAPE // Object escapes to a goroutine
} AllocEscapeState;

// Structure to track variable escape status
typedef struct VarEscapeInfo {
    char* name;                // Variable name
    AllocEscapeState state;    // Current escape state
    ASTNode* alloc_node;       // Node where allocation happens
    struct VarEscapeInfo* next; // Next in linked list
} VarEscapeInfo;

// Analysis context for a function
typedef struct {
    VarEscapeInfo* vars;       // List of tracked variables
    ASTNode* current_function; // Function being analyzed
    bool has_defer;            // Whether function has defer statements
    bool has_goroutines;       // Whether function spawns goroutines
    bool has_closures;         // Whether function creates closures
} EscapeAnalysisContext;

// Create a new escape analysis context
static EscapeAnalysisContext* create_context(ASTNode* func) {
    EscapeAnalysisContext* ctx = malloc(sizeof(EscapeAnalysisContext));
    if (!ctx) return NULL;
    
    ctx->vars = NULL;
    ctx->current_function = func;
    ctx->has_defer = false;
    ctx->has_goroutines = false;
    ctx->has_closures = false;
    
    return ctx;
}

// Free an escape analysis context
static void free_context(EscapeAnalysisContext* ctx) {
    if (!ctx) return;
    
    // Free tracked variables
    VarEscapeInfo* var = ctx->vars;
    while (var) {
        VarEscapeInfo* next = var->next;
        free(var->name);
        free(var);
        var = next;
    }
    
    free(ctx);
}

// Track a variable in the context
static VarEscapeInfo* track_variable(EscapeAnalysisContext* ctx, const char* name, ASTNode* alloc_node) {
    if (!ctx || !name) return NULL;
    
    // Check if variable is already tracked
    VarEscapeInfo* var = ctx->vars;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var;
        }
        var = var->next;
    }
    
    // Create new tracking info
    var = malloc(sizeof(VarEscapeInfo));
    if (!var) return NULL;
    
    var->name = strdup(name);
    var->state = ALLOC_UNKNOWN;
    var->alloc_node = alloc_node;
    var->next = ctx->vars;
    ctx->vars = var;
    
    return var;
}

// Get a tracked variable by name
static VarEscapeInfo* get_variable(EscapeAnalysisContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    VarEscapeInfo* var = ctx->vars;
    while (var) {
        if (strcmp(var->name, name) == 0) {
            return var;
        }
        var = var->next;
    }
    
    return NULL;
}

// Update a variable's escape state
static void update_escape_state(VarEscapeInfo* var, AllocEscapeState new_state) {
    if (!var) return;
    
    // Only update if the new state is "more escaped" than current
    if (new_state > var->state) {
        var->state = new_state;
    }
}

// Forward declarations for recursive functions
static void analyze_expression(EscapeAnalysisContext* ctx, ASTNode* expr);
static void analyze_statement(EscapeAnalysisContext* ctx, ASTNode* stmt);
static void analyze_block(EscapeAnalysisContext* ctx, ASTNode* block);

// Analyze a variable reference
static void analyze_var_ref(EscapeAnalysisContext* ctx, ASTNode* var_ref) {
    if (!ctx || !var_ref || var_ref->type != AST_VAR_REF) {
        return;
    }
    
    const char* var_name = var_ref->var_ref.name;
    VarEscapeInfo* var = get_variable(ctx, var_name);
    
    // Variable isn't tracked (may not be heap allocated)
    if (!var) return;
    
    // The use of the variable will be analyzed in the parent context
}

// Analyze a binary expression
static void analyze_binary_expr(EscapeAnalysisContext* ctx, ASTNode* binary) {
    if (!ctx || !binary || binary->type != AST_BINARY_EXPR) {
        return;
    }
    
    // Analyze both sides of the expression
    analyze_expression(ctx, binary->binary_expr.left);
    analyze_expression(ctx, binary->binary_expr.right);
}

// Analyze a unary expression
static void analyze_unary_expr(EscapeAnalysisContext* ctx, ASTNode* unary) {
    if (!ctx || !unary || unary->type != AST_UNARY_EXPR) {
        return;
    }
    
    analyze_expression(ctx, unary->unary_expr.expr);
}

// Analyze a function call
static void analyze_call_expr(EscapeAnalysisContext* ctx, ASTNode* call) {
    if (!ctx || !call || call->type != AST_CALL_EXPR) {
        return;
    }
    
    // Analyze the function expression
    analyze_expression(ctx, call->call_expr.func);
    
    // Analyze arguments - they may escape through the call
    ASTNode* arg = call->call_expr.args;
    while (arg) {
        analyze_expression(ctx, arg);
        
        // If the argument is a variable reference, mark it as escaping to an argument
        if (arg->type == AST_VAR_REF) {
            VarEscapeInfo* var = get_variable(ctx, arg->var_ref.name);
            if (var) {
                update_escape_state(var, ALLOC_ARG_ESCAPE);
            }
        }
        
        arg = arg->next;
    }
}

// Analyze a goroutine spawn
static void analyze_go_expr(EscapeAnalysisContext* ctx, ASTNode* go_expr) {
    if (!ctx || !go_expr || go_expr->type != AST_GO_EXPR) {
        return;
    }
    
    ctx->has_goroutines = true;
    
    // Any variable used in a goroutine must be considered as escaping
    analyze_expression(ctx, go_expr->go_expr.call);
    
    // If the goroutine call has arguments, mark them as goroutine escapes
    if (go_expr->go_expr.call->type == AST_CALL_EXPR) {
        ASTNode* args = go_expr->go_expr.call->call_expr.args;
        while (args) {
            if (args->type == AST_VAR_REF) {
                VarEscapeInfo* var = get_variable(ctx, args->var_ref.name);
                if (var) {
                    update_escape_state(var, ALLOC_GOROUTINE_ESCAPE);
                }
            }
            args = args->next;
        }
    }
}

// Analyze an allocation expression
static void analyze_alloc_expr(EscapeAnalysisContext* ctx, ASTNode* alloc, const char* var_name) {
    if (!ctx || !alloc) {
        return;
    }
    
    if (var_name) {
        // Register this as a tracked allocation
        track_variable(ctx, var_name, alloc);
    }
    
    // Analyze any subexpressions within the allocation
    if (alloc->type == AST_NEW_EXPR) {
        // Nothing to analyze for the type
    } else if (alloc->type == AST_MAKE_EXPR) {
        // Analyze size expressions
        if (alloc->make_expr.size) {
            analyze_expression(ctx, alloc->make_expr.size);
        }
        if (alloc->make_expr.capacity) {
            analyze_expression(ctx, alloc->make_expr.capacity);
        }
    }
}

// Analyze an assignment statement
static void analyze_assignment(EscapeAnalysisContext* ctx, ASTNode* assign) {
    if (!ctx || !assign || assign->type != AST_ASSIGN_STMT) {
        return;
    }
    
    // Analyze right side first
    analyze_expression(ctx, assign->assign_stmt.right);
    
    // Check for allocation on the right side
    bool is_alloc = false;
    if (assign->assign_stmt.right->type == AST_NEW_EXPR ||
        assign->assign_stmt.right->type == AST_MAKE_EXPR) {
        is_alloc = true;
    }
    
    // Handle the left side
    if (assign->assign_stmt.left->type == AST_VAR_REF) {
        const char* var_name = assign->assign_stmt.left->var_ref.name;
        
        if (is_alloc) {
            // This is an allocation being assigned to a variable
            analyze_alloc_expr(ctx, assign->assign_stmt.right, var_name);
        } else {
            // This could be assigning one tracked variable to another
            VarEscapeInfo* var = get_variable(ctx, var_name);
            
            // If we're assigning from another variable
            if (assign->assign_stmt.right->type == AST_VAR_REF) {
                const char* right_name = assign->assign_stmt.right->var_ref.name;
                VarEscapeInfo* right_var = get_variable(ctx, right_name);
                
                if (right_var && var) {
                    // Propagate escape state
                    update_escape_state(var, right_var->state);
                }
            }
        }
    }
    
    // Handle struct field assignments that could cause escape
    if (assign->assign_stmt.left->type == AST_FIELD_ACCESS) {
        ASTNode* object = assign->assign_stmt.left->field_access.object;
        
        if (object->type == AST_VAR_REF) {
            // Accessing a field of a variable
            VarEscapeInfo* var = get_variable(ctx, object->var_ref.name);
            if (var) {
                // Check if we're assigning to a global structure
                if (is_global_variable(object->var_ref.name)) {
                    update_escape_state(var, ALLOC_GLOBAL_ESCAPE);
                }
            }
        }
    }
}

// Analyze a return statement
static void analyze_return_stmt(EscapeAnalysisContext* ctx, ASTNode* ret) {
    if (!ctx || !ret || ret->type != AST_RETURN_STMT) {
        return;
    }
    
    // Analyze the return expression
    if (ret->return_stmt.expr) {
        analyze_expression(ctx, ret->return_stmt.expr);
        
        // If returning a variable, mark it as return-escaping
        if (ret->return_stmt.expr->type == AST_VAR_REF) {
            VarEscapeInfo* var = get_variable(ctx, ret->return_stmt.expr->var_ref.name);
            if (var) {
                update_escape_state(var, ALLOC_RETURN_ESCAPE);
            }
        }
    }
}

// Analyze an if statement
static void analyze_if_stmt(EscapeAnalysisContext* ctx, ASTNode* if_stmt) {
    if (!ctx || !if_stmt || if_stmt->type != AST_IF_STMT) {
        return;
    }
    
    // Analyze condition
    analyze_expression(ctx, if_stmt->if_stmt.condition);
    
    // Analyze true branch
    analyze_statement(ctx, if_stmt->if_stmt.then_stmt);
    
    // Analyze else branch if present
    if (if_stmt->if_stmt.else_stmt) {
        analyze_statement(ctx, if_stmt->if_stmt.else_stmt);
    }
}

// Analyze a for loop
static void analyze_for_stmt(EscapeAnalysisContext* ctx, ASTNode* for_stmt) {
    if (!ctx || !for_stmt || for_stmt->type != AST_FOR_STMT) {
        return;
    }
    
    // Analyze initializer if present
    if (for_stmt->for_stmt.init) {
        analyze_statement(ctx, for_stmt->for_stmt.init);
    }
    
    // Analyze condition if present
    if (for_stmt->for_stmt.condition) {
        analyze_expression(ctx, for_stmt->for_stmt.condition);
    }
    
    // Analyze post statement if present
    if (for_stmt->for_stmt.post) {
        analyze_statement(ctx, for_stmt->for_stmt.post);
    }
    
    // Analyze loop body
    analyze_statement(ctx, for_stmt->for_stmt.body);
}

// Analyze a switch statement
static void analyze_switch_stmt(EscapeAnalysisContext* ctx, ASTNode* switch_stmt) {
    if (!ctx || !switch_stmt || switch_stmt->type != AST_SWITCH_STMT) {
        return;
    }
    
    // Analyze switch expression
    if (switch_stmt->switch_stmt.expr) {
        analyze_expression(ctx, switch_stmt->switch_stmt.expr);
    }
    
    // Analyze cases
    ASTNode* current_case = switch_stmt->switch_stmt.cases;
    while (current_case) {
        if (current_case->type == AST_CASE_CLAUSE) {
            // Analyze case expression
            if (current_case->case_clause.expr) {
                analyze_expression(ctx, current_case->case_clause.expr);
            }
            
            // Analyze case body
            analyze_block(ctx, current_case->case_clause.body);
        }
        current_case = current_case->next;
    }
}

// Analyze a defer statement
static void analyze_defer_stmt(EscapeAnalysisContext* ctx, ASTNode* defer_stmt) {
    if (!ctx || !defer_stmt || defer_stmt->type != AST_DEFER_STMT) {
        return;
    }
    
    ctx->has_defer = true;
    
    // Any variable used in a defer must be considered as escaping
    analyze_expression(ctx, defer_stmt->defer_stmt.call);
    
    // If the defer call has arguments, mark them as escaping
    if (defer_stmt->defer_stmt.call->type == AST_CALL_EXPR) {
        ASTNode* args = defer_stmt->defer_stmt.call->call_expr.args;
        while (args) {
            if (args->type == AST_VAR_REF) {
                VarEscapeInfo* var = get_variable(ctx, args->var_ref.name);
                if (var) {
                    update_escape_state(var, ALLOC_ARG_ESCAPE);
                }
            }
            args = args->next;
        }
    }
}

// Analyze an expression
static void analyze_expression(EscapeAnalysisContext* ctx, ASTNode* expr) {
    if (!ctx || !expr) {
        return;
    }
    
    switch (expr->type) {
        case AST_VAR_REF:
            analyze_var_ref(ctx, expr);
            break;
            
        case AST_BINARY_EXPR:
            analyze_binary_expr(ctx, expr);
            break;
            
        case AST_UNARY_EXPR:
            analyze_unary_expr(ctx, expr);
            break;
            
        case AST_CALL_EXPR:
            analyze_call_expr(ctx, expr);
            break;
            
        case AST_GO_EXPR:
            analyze_go_expr(ctx, expr);
            break;
            
        case AST_FIELD_ACCESS:
            // Analyze the object being accessed
            analyze_expression(ctx, expr->field_access.object);
            break;
            
        case AST_INDEX_EXPR:
            // Analyze the array/map and index
            analyze_expression(ctx, expr->index_expr.array);
            analyze_expression(ctx, expr->index_expr.index);
            break;
            
        case AST_SLICE_EXPR:
            // Analyze the array and indices
            analyze_expression(ctx, expr->slice_expr.array);
            if (expr->slice_expr.low) {
                analyze_expression(ctx, expr->slice_expr.low);
            }
            if (expr->slice_expr.high) {
                analyze_expression(ctx, expr->slice_expr.high);
            }
            if (expr->slice_expr.max) {
                analyze_expression(ctx, expr->slice_expr.max);
            }
            break;
            
        default:
            // Other expression types don't affect escape analysis
            break;
    }
}

// Analyze a statement
static void analyze_statement(EscapeAnalysisContext* ctx, ASTNode* stmt) {
    if (!ctx || !stmt) {
        return;
    }
    
    switch (stmt->type) {
        case AST_BLOCK:
            analyze_block(ctx, stmt);
            break;
            
        case AST_ASSIGN_STMT:
            analyze_assignment(ctx, stmt);
            break;
            
        case AST_RETURN_STMT:
            analyze_return_stmt(ctx, stmt);
            break;
            
        case AST_IF_STMT:
            analyze_if_stmt(ctx, stmt);
            break;
            
        case AST_FOR_STMT:
            analyze_for_stmt(ctx, stmt);
            break;
            
        case AST_SWITCH_STMT:
            analyze_switch_stmt(ctx, stmt);
            break;
            
        case AST_DEFER_STMT:
            analyze_defer_stmt(ctx, stmt);
            break;
            
        case AST_EXPR_STMT:
            analyze_expression(ctx, stmt->expr_stmt.expr);
            break;
            
        default:
            // Other statement types don't affect escape analysis
            break;
    }
}

// Analyze a block of statements
static void analyze_block(EscapeAnalysisContext* ctx, ASTNode* block) {
    if (!ctx || !block || block->type != AST_BLOCK) {
        return;
    }
    
    // Analyze each statement in the block
    ASTNode* stmt = block->block.stmts;
    while (stmt) {
        analyze_statement(ctx, stmt);
        stmt = stmt->next;
    }
}

// Analyze a function and determine allocation escapes
static void analyze_function(ASTNode* func) {
    if (!func || (func->type != AST_FUNC_DECL && func->type != AST_FUNC_LIT)) {
        return;
    }
    
    // Create analysis context
    EscapeAnalysisContext* ctx = create_context(func);
    if (!ctx) return;
    
    // Get function body
    ASTNode* body = NULL;
    if (func->type == AST_FUNC_DECL) {
        body = func->func_decl.body;
    } else if (func->type == AST_FUNC_LIT) {
        body = func->func_lit.body;
        // Function literals can close over variables
        ctx->has_closures = true;
    }
    
    // Analyze function body if present
    if (body) {
        analyze_block(ctx, body);
    }
    
    // Process results and apply optimizations
    VarEscapeInfo* var = ctx->vars;
    while (var) {
        // If the allocation doesn't escape, mark it for stack allocation
        if (var->state == ALLOC_UNKNOWN || var->state == ALLOC_NO_ESCAPE) {
            if (var->alloc_node) {
                // Add a stack allocation hint to the AST node
                if (var->alloc_node->type == AST_NEW_EXPR) {
                    var->alloc_node->new_expr.stack_allocate = true;
                } else if (var->alloc_node->type == AST_MAKE_EXPR) {
                    var->alloc_node->make_expr.stack_allocate = true;
                }
                
                // Add optimization note
                printf("Escape Analysis: Variable '%s' can be stack-allocated\n", var->name);
            }
        }
        var = var->next;
    }
    
    // Clean up
    free_context(ctx);
}

// Check if a name refers to a global variable
bool is_global_variable(const char* name) {
    // This is a simplification - in a real compiler we'd check symbol tables
    // For now, assume identifiers starting with uppercase are globals
    return name && name[0] >= 'A' && name[0] <= 'Z';
}

// Entry point for escape analysis optimization
void optimize_escape_analysis(ASTNode* root) {
    if (!root) return;
    
    // Process all functions in the module
    ASTNode* node = root;
    while (node) {
        if (node->type == AST_FUNC_DECL) {
            analyze_function(node);
        }
        node = node->next;
    }
} 