#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "../ast/ast.h"
#include "optimizer.h"

// Goroutine usage patterns
typedef enum {
    GOROUTINE_PATTERN_UNKNOWN,        // Unknown usage pattern
    GOROUTINE_PATTERN_PARALLELISM,    // Basic parallelism (fire and forget)
    GOROUTINE_PATTERN_WORKER_POOL,    // Worker pool pattern with bounded goroutines
    GOROUTINE_PATTERN_PIPELINE,       // Pipeline pattern with stages
    GOROUTINE_PATTERN_FAN_OUT_FAN_IN, // Fan-out/fan-in pattern
    GOROUTINE_PATTERN_FUTURE,         // Future/promise pattern (awaiting single result)
    GOROUTINE_PATTERN_BACKGROUND      // Background task with no result synchronization
} GoroutinePattern;

// Function usage analysis
typedef struct FunctionAnalysis {
    char* name;                       // Function name
    ASTNode* decl_node;               // Declaration node
    int spawn_count;                  // How many times spawned as goroutine
    int direct_call_count;            // How many times called directly
    bool reads_shared_memory;         // Whether it reads shared memory
    bool writes_shared_memory;        // Whether it writes shared memory
    bool takes_channel_args;          // Whether it takes channel arguments
    bool local_only;                  // Whether it's only called within a single function
    bool can_inline;                  // Whether it can be inlined at goroutine launch sites
    GoroutinePattern pattern;         // Identified usage pattern
    struct FunctionAnalysis* next;    // Next in list
} FunctionAnalysis;

// Goroutine spawn site analysis
typedef struct GoroutineSpawn {
    ASTNode* spawn_node;              // The 'go' expression node
    FunctionAnalysis* target_func;    // The target function being spawned
    bool can_batch;                   // Whether this spawn can be batched
    bool can_defer;                   // Whether this can be deferred to a worker pool
    bool can_be_sequentialized;       // Whether this can be made sequential
    bool requires_result_sync;        // Whether the result must be synchronized
    struct GoroutineSpawn* next;      // Next in list
} GoroutineSpawn;

// Module-level analysis context
typedef struct {
    FunctionAnalysis* functions;      // List of analyzed functions
    GoroutineSpawn* spawns;           // List of goroutine spawn sites
    int worker_pool_count;            // Number of worker pools detected
    bool uses_scheduling_hints;       // Whether scheduling hints are used
    ASTNode* current_function;        // Current function being analyzed
} GoroutineOptimizationContext;

// Create a new goroutine optimization context
static GoroutineOptimizationContext* create_context(void) {
    GoroutineOptimizationContext* ctx = malloc(sizeof(GoroutineOptimizationContext));
    if (!ctx) return NULL;
    
    ctx->functions = NULL;
    ctx->spawns = NULL;
    ctx->worker_pool_count = 0;
    ctx->uses_scheduling_hints = false;
    ctx->current_function = NULL;
    
    return ctx;
}

// Free a goroutine optimization context
static void free_context(GoroutineOptimizationContext* ctx) {
    if (!ctx) return;
    
    // Free the function analysis list
    FunctionAnalysis* func = ctx->functions;
    while (func) {
        FunctionAnalysis* next = func->next;
        free(func->name);
        free(func);
        func = next;
    }
    
    // Free the goroutine spawn list
    GoroutineSpawn* spawn = ctx->spawns;
    while (spawn) {
        GoroutineSpawn* next = spawn->next;
        free(spawn);
        spawn = next;
    }
    
    free(ctx);
}

// Register a function in the context
static FunctionAnalysis* register_function(GoroutineOptimizationContext* ctx, const char* name, ASTNode* decl_node) {
    if (!ctx || !name) return NULL;
    
    // Check if function already registered
    FunctionAnalysis* func = ctx->functions;
    while (func) {
        if (strcmp(func->name, name) == 0) {
            return func;
        }
        func = func->next;
    }
    
    // Create new function analysis
    func = malloc(sizeof(FunctionAnalysis));
    if (!func) return NULL;
    
    func->name = strdup(name);
    func->decl_node = decl_node;
    func->spawn_count = 0;
    func->direct_call_count = 0;
    func->reads_shared_memory = false;
    func->writes_shared_memory = false;
    func->takes_channel_args = false;
    func->local_only = true;
    func->can_inline = false;
    func->pattern = GOROUTINE_PATTERN_UNKNOWN;
    func->next = ctx->functions;
    
    ctx->functions = func;
    return func;
}

// Find function analysis by name
static FunctionAnalysis* find_function(GoroutineOptimizationContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    FunctionAnalysis* func = ctx->functions;
    while (func) {
        if (strcmp(func->name, name) == 0) {
            return func;
        }
        func = func->next;
    }
    
    return NULL;
}

// Register a goroutine spawn site
static GoroutineSpawn* register_spawn(GoroutineOptimizationContext* ctx, ASTNode* spawn_node, FunctionAnalysis* target_func) {
    if (!ctx || !spawn_node || !target_func) return NULL;
    
    GoroutineSpawn* spawn = malloc(sizeof(GoroutineSpawn));
    if (!spawn) return NULL;
    
    spawn->spawn_node = spawn_node;
    spawn->target_func = target_func;
    spawn->can_batch = false;
    spawn->can_defer = false;
    spawn->can_be_sequentialized = false;
    spawn->requires_result_sync = false;
    spawn->next = ctx->spawns;
    
    ctx->spawns = spawn;
    return spawn;
}

// Forward declarations for recursive functions
static void analyze_expression(GoroutineOptimizationContext* ctx, ASTNode* expr);
static void analyze_statement(GoroutineOptimizationContext* ctx, ASTNode* stmt);
static void analyze_block(GoroutineOptimizationContext* ctx, ASTNode* block);

// Analyze a function call expression
static void analyze_call_expr(GoroutineOptimizationContext* ctx, ASTNode* call) {
    if (!ctx || !call || call->type != AST_CALL_EXPR) {
        return;
    }
    
    // Check if it's calling a known function
    if (call->call_expr.func->type == AST_VAR_REF) {
        const char* func_name = call->call_expr.func->var_ref.name;
        FunctionAnalysis* func = find_function(ctx, func_name);
        
        if (func) {
            func->direct_call_count++;
            
            // If it's called from multiple functions, it's not local only
            if (ctx->current_function && 
                ctx->current_function->type == AST_FUNC_DECL && 
                strcmp(ctx->current_function->func_decl.name, func_name) != 0) {
                func->local_only = false;
            }
        }
    }
    
    // Analyze arguments
    ASTNode* arg = call->call_expr.args;
    while (arg) {
        analyze_expression(ctx, arg);
        arg = arg->next;
    }
}

// Analyze a goroutine spawn
static void analyze_go_expr(GoroutineOptimizationContext* ctx, ASTNode* go_expr) {
    if (!ctx || !go_expr || go_expr->type != AST_GO_EXPR) {
        return;
    }
    
    // Analyze the called function
    ASTNode* call = go_expr->go_expr.call;
    if (call->type == AST_CALL_EXPR && call->call_expr.func->type == AST_VAR_REF) {
        const char* func_name = call->call_expr.func->var_ref.name;
        FunctionAnalysis* func = find_function(ctx, func_name);
        
        if (func) {
            func->spawn_count++;
            
            // Register the spawn site
            GoroutineSpawn* spawn = register_spawn(ctx, go_expr, func);
            
            // Analyze whether this goroutine can be optimized
            if (spawn) {
                // Check if it requires synchronization
                // This is a basic analysis; in practice we'd need more complex analysis
                bool has_channel_arg = false;
                ASTNode* arg = call->call_expr.args;
                while (arg) {
                    if (arg->type == AST_VAR_REF) {
                        // Very simplistic test - in real code we'd check if it's a channel type
                        if (strstr(arg->var_ref.name, "chan") != NULL) {
                            has_channel_arg = true;
                            func->takes_channel_args = true;
                        }
                    }
                    arg = arg->next;
                }
                
                spawn->requires_result_sync = has_channel_arg;
                
                // Simple heuristic: if the function doesn't access shared state
                // and doesn't need result sync, can potentially sequentialize
                spawn->can_be_sequentialized = 
                    !func->reads_shared_memory && 
                    !func->writes_shared_memory && 
                    !spawn->requires_result_sync;
                
                // Can potentially batch if multiple similar goroutines are spawned
                if (func->spawn_count > 1) {
                    spawn->can_batch = true;
                }
                
                // Can potentially defer to worker pool if parallelism isn't critical
                spawn->can_defer = !spawn->requires_result_sync;
            }
        }
    }
    
    // Analyze the call expression
    analyze_expression(ctx, call);
}

// Analyze a shared variable access
static void analyze_var_ref(GoroutineOptimizationContext* ctx, ASTNode* var_ref, bool is_write) {
    if (!ctx || !var_ref || var_ref->type != AST_VAR_REF) {
        return;
    }
    
    // In a real implementation, we'd have a mechanism to track shared variables
    // For this example, we'll use a simple naming convention for illustration
    
    // Just a placeholder example - in practice you'd need data flow analysis
    if (strstr(var_ref->var_ref.name, "shared_") != NULL ||
        strstr(var_ref->var_ref.name, "global_") != NULL) {
        
        // Check all functions that have been spawned as goroutines
        FunctionAnalysis* func = ctx->functions;
        while (func) {
            if (func->spawn_count > 0) {
                if (is_write) {
                    func->writes_shared_memory = true;
                } else {
                    func->reads_shared_memory = true;
                }
            }
            func = func->next;
        }
    }
}

// Analyze a binary expression
static void analyze_binary_expr(GoroutineOptimizationContext* ctx, ASTNode* expr) {
    if (!ctx || !expr || expr->type != AST_BINARY_EXPR) {
        return;
    }
    
    // Check for assignments to shared variables
    if (expr->binary_expr.op == OP_ASSIGN && expr->binary_expr.left->type == AST_VAR_REF) {
        analyze_var_ref(ctx, expr->binary_expr.left, true);
    }
    
    // Recursively analyze both sides
    analyze_expression(ctx, expr->binary_expr.left);
    analyze_expression(ctx, expr->binary_expr.right);
}

// Analyze an expression
static void analyze_expression(GoroutineOptimizationContext* ctx, ASTNode* expr) {
    if (!ctx || !expr) {
        return;
    }
    
    switch (expr->type) {
        case AST_GO_EXPR:
            analyze_go_expr(ctx, expr);
            break;
            
        case AST_CALL_EXPR:
            analyze_call_expr(ctx, expr);
            break;
            
        case AST_BINARY_EXPR:
            analyze_binary_expr(ctx, expr);
            break;
            
        case AST_VAR_REF:
            analyze_var_ref(ctx, expr, false); // reading
            break;
            
        case AST_UNARY_EXPR:
            analyze_expression(ctx, expr->unary_expr.expr);
            break;
            
        case AST_FIELD_ACCESS:
            analyze_expression(ctx, expr->field_access.object);
            break;
            
        case AST_INDEX_EXPR:
            analyze_expression(ctx, expr->index_expr.array);
            analyze_expression(ctx, expr->index_expr.index);
            break;
            
        default:
            // Other expression types don't affect goroutine analysis
            break;
    }
}

// Analyze an assignment statement
static void analyze_assign_stmt(GoroutineOptimizationContext* ctx, ASTNode* stmt) {
    if (!ctx || !stmt || stmt->type != AST_ASSIGN_STMT) {
        return;
    }
    
    // Check for assignments to shared variables
    if (stmt->assign_stmt.left->type == AST_VAR_REF) {
        analyze_var_ref(ctx, stmt->assign_stmt.left, true);
    }
    
    // Analyze both sides
    analyze_expression(ctx, stmt->assign_stmt.left);
    analyze_expression(ctx, stmt->assign_stmt.right);
}

// Analyze a statement
static void analyze_statement(GoroutineOptimizationContext* ctx, ASTNode* stmt) {
    if (!ctx || !stmt) {
        return;
    }
    
    switch (stmt->type) {
        case AST_BLOCK:
            analyze_block(ctx, stmt);
            break;
            
        case AST_ASSIGN_STMT:
            analyze_assign_stmt(ctx, stmt);
            break;
            
        case AST_EXPR_STMT:
            analyze_expression(ctx, stmt->expr_stmt.expr);
            break;
            
        case AST_IF_STMT:
            analyze_expression(ctx, stmt->if_stmt.condition);
            analyze_statement(ctx, stmt->if_stmt.then_stmt);
            if (stmt->if_stmt.else_stmt) {
                analyze_statement(ctx, stmt->if_stmt.else_stmt);
            }
            break;
            
        case AST_FOR_STMT:
            if (stmt->for_stmt.init) {
                analyze_statement(ctx, stmt->for_stmt.init);
            }
            if (stmt->for_stmt.condition) {
                analyze_expression(ctx, stmt->for_stmt.condition);
            }
            if (stmt->for_stmt.post) {
                analyze_statement(ctx, stmt->for_stmt.post);
            }
            analyze_statement(ctx, stmt->for_stmt.body);
            break;
            
        case AST_SWITCH_STMT:
            if (stmt->switch_stmt.expr) {
                analyze_expression(ctx, stmt->switch_stmt.expr);
            }
            
            // Analyze cases
            ASTNode* case_node = stmt->switch_stmt.cases;
            while (case_node) {
                if (case_node->type == AST_CASE_CLAUSE) {
                    if (case_node->case_clause.expr) {
                        analyze_expression(ctx, case_node->case_clause.expr);
                    }
                    analyze_block(ctx, case_node->case_clause.body);
                }
                case_node = case_node->next;
            }
            break;
            
        case AST_SELECT_STMT:
            // Analyze select cases
            ASTNode* select_case = stmt->select_stmt.cases;
            while (select_case) {
                if (select_case->type == AST_SELECT_CASE) {
                    if (select_case->select_case.comm) {
                        analyze_expression(ctx, select_case->select_case.comm);
                    }
                    analyze_block(ctx, select_case->select_case.body);
                }
                select_case = select_case->next;
            }
            break;
            
        default:
            // Other statement types don't affect goroutine analysis
            break;
    }
}

// Analyze a block of statements
static void analyze_block(GoroutineOptimizationContext* ctx, ASTNode* block) {
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

// Analyze function parameters for channels
static void analyze_function_params(GoroutineOptimizationContext* ctx, ASTNode* func) {
    if (!ctx || !func || 
        (func->type != AST_FUNC_DECL && func->type != AST_FUNC_LIT)) {
        return;
    }
    
    // Get parameter list
    ASTNode* params = NULL;
    if (func->type == AST_FUNC_DECL) {
        params = func->func_decl.params;
    } else if (func->type == AST_FUNC_LIT) {
        params = func->func_lit.params;
    }
    
    // Check each parameter
    ASTNode* param = params;
    while (param) {
        if (param->type == AST_PARAM_DECL) {
            // Check if parameter is a channel type
            if (param->param_decl.type_ref->type_ref.kind == TYPE_CHANNEL) {
                // Find the function in our analysis
                const char* func_name = NULL;
                if (func->type == AST_FUNC_DECL) {
                    func_name = func->func_decl.name;
                }
                
                if (func_name) {
                    FunctionAnalysis* analysis = find_function(ctx, func_name);
                    if (analysis) {
                        analysis->takes_channel_args = true;
                    }
                }
            }
        }
        param = param->next;
    }
}

// Analyze a function definition
static void analyze_function(GoroutineOptimizationContext* ctx, ASTNode* func) {
    if (!ctx || !func || 
        (func->type != AST_FUNC_DECL && func->type != AST_FUNC_LIT)) {
        return;
    }
    
    // Register the function if it's a named function
    if (func->type == AST_FUNC_DECL) {
        register_function(ctx, func->func_decl.name, func);
    }
    
    // Save previous current function
    ASTNode* prev_function = ctx->current_function;
    ctx->current_function = func;
    
    // Check parameters for channels
    analyze_function_params(ctx, func);
    
    // Get function body
    ASTNode* body = NULL;
    if (func->type == AST_FUNC_DECL) {
        body = func->func_decl.body;
    } else if (func->type == AST_FUNC_LIT) {
        body = func->func_lit.body;
    }
    
    // Analyze function body if present
    if (body) {
        analyze_block(ctx, body);
    }
    
    // Restore previous current function
    ctx->current_function = prev_function;
}

// Identify function patterns
static void identify_function_patterns(GoroutineOptimizationContext* ctx) {
    if (!ctx) return;
    
    FunctionAnalysis* func = ctx->functions;
    while (func) {
        // Check if this function is used in goroutines
        if (func->spawn_count > 0) {
            // Identify function pattern based on analysis
            if (func->takes_channel_args && !func->reads_shared_memory && !func->writes_shared_memory) {
                // Likely a worker or pipeline stage
                if (func->spawn_count > 1) {
                    func->pattern = GOROUTINE_PATTERN_WORKER_POOL;
                } else {
                    func->pattern = GOROUTINE_PATTERN_PIPELINE;
                }
            } else if (!func->takes_channel_args && !func->reads_shared_memory && !func->writes_shared_memory) {
                // Pure function, possibly parallelizable
                func->pattern = GOROUTINE_PATTERN_PARALLELISM;
                func->can_inline = true;
            } else if (func->reads_shared_memory && !func->writes_shared_memory) {
                // Read-only access to shared memory
                func->pattern = GOROUTINE_PATTERN_FAN_OUT_FAN_IN;
            } else if (!func->takes_channel_args && (func->reads_shared_memory || func->writes_shared_memory)) {
                // Background task with no communication
                func->pattern = GOROUTINE_PATTERN_BACKGROUND;
            }
        }
        
        // See if a function can be inlined at goroutine spawn sites
        // Simple heuristic: short functions that don't call other functions
        if (func->decl_node && func->decl_node->type == AST_FUNC_DECL && 
            func->decl_node->func_decl.body) {
            
            // Count statements in the function body
            int stmt_count = 0;
            ASTNode* stmt = func->decl_node->func_decl.body->block.stmts;
            bool has_calls = false;
            
            while (stmt && stmt_count < 5) { // arbitrary limit for "small" functions
                stmt_count++;
                
                // Check if this statement contains function calls
                // This is a simplified check - a real implementation would be more thorough
                if (stmt->type == AST_EXPR_STMT && 
                    stmt->expr_stmt.expr->type == AST_CALL_EXPR) {
                    has_calls = true;
                }
                
                stmt = stmt->next;
            }
            
            if (stmt_count < 5 && !has_calls && 
                !func->reads_shared_memory && !func->writes_shared_memory) {
                func->can_inline = true;
            }
        }
        
        func = func->next;
    }
}

// Apply goroutine optimizations
static void apply_goroutine_optimizations(GoroutineOptimizationContext* ctx) {
    if (!ctx) return;
    
    // First, identify patterns for functions
    identify_function_patterns(ctx);
    
    // Then optimize spawn sites based on identified patterns
    GoroutineSpawn* spawn = ctx->spawns;
    while (spawn) {
        FunctionAnalysis* func = spawn->target_func;
        
        // Apply optimizations based on pattern
        if (func->pattern == GOROUTINE_PATTERN_PARALLELISM && func->can_inline) {
            printf("Goroutine Optimization: Can inline function '%s' at spawn site\n", 
                   func->name);
                   
            // In a real implementation, we'd transform the AST here
            // For this example, we just mark the node for later transformation
            
            // This would create a wrapper around the function body to inline it
            // instead of going through the goroutine creation path
            spawn->spawn_node->go_expr.inline_target = true;
        }
        
        // For worker pool pattern, suggest batch processing
        if (func->pattern == GOROUTINE_PATTERN_WORKER_POOL && spawn->can_batch) {
            printf("Goroutine Optimization: Can batch spawn of function '%s' in worker pool\n", 
                   func->name);
                   
            // In a real implementation, we'd transform multiple goroutine spawns
            // into a single worker pool creation + batch job submission
            spawn->spawn_node->go_expr.use_worker_pool = true;
            ctx->worker_pool_count++;
        }
        
        // For pipeline pattern, optimize the chain
        if (func->pattern == GOROUTINE_PATTERN_PIPELINE) {
            printf("Goroutine Optimization: Function '%s' part of pipeline pattern\n", 
                   func->name);
                   
            // Pipeline optimizations might include:
            // - Balancing stages
            // - Adjusting buffer sizes
            // - Combining stages
            spawn->spawn_node->go_expr.pipeline_stage = true;
        }
        
        // For background tasks, possibly defer startup
        if (func->pattern == GOROUTINE_PATTERN_BACKGROUND && !spawn->requires_result_sync) {
            printf("Goroutine Optimization: Function '%s' can be deferred as background task\n", 
                   func->name);
                   
            // For background tasks that don't need immediate startup,
            // we can defer their launch to reduce initial load
            spawn->spawn_node->go_expr.defer_startup = true;
        }
        
        spawn = spawn->next;
    }
}

// Entry point for goroutine optimization
void optimize_goroutines(ASTNode* root) {
    if (!root) return;
    
    // Create optimization context
    GoroutineOptimizationContext* ctx = create_context();
    if (!ctx) return;
    
    // First, analyze all functions
    ASTNode* node = root;
    while (node) {
        if (node->type == AST_FUNC_DECL) {
            analyze_function(ctx, node);
        }
        node = node->next;
    }
    
    // Apply optimizations based on analysis
    apply_goroutine_optimizations(ctx);
    
    // If worker pools were created, add runtime initialization code
    if (ctx->worker_pool_count > 0) {
        printf("Goroutine Optimization: Adding runtime worker pool initialization for %d pools\n", 
               ctx->worker_pool_count);
               
        // In a real implementation, we'd add worker pool initialization
        // code to the program's init function or main function
    }
    
    // Clean up
    free_context(ctx);
} 