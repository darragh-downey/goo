#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "../ast/ast.h"
#include "optimizer.h"

// Channel usage patterns
typedef enum {
    CHANNEL_PATTERN_UNKNOWN,       // Unknown usage pattern
    CHANNEL_PATTERN_ONESHOT,       // Channel used for single send/receive only
    CHANNEL_PATTERN_BROADCAST,     // One sender, multiple receivers
    CHANNEL_PATTERN_WORKER_POOL,   // Multiple senders, multiple receivers
    CHANNEL_PATTERN_PIPELINE,      // Chain of channels in a pipeline
    CHANNEL_PATTERN_SYNCHRONIZATION, // Used primarily for synchronization
    CHANNEL_PATTERN_LOCAL_ONLY     // All operations are within the same function
} ChannelPattern;

// Channel analysis information
typedef struct ChannelInfo {
    char* name;                      // Channel variable name
    ASTNode* decl_node;              // Declaration node for the channel
    ChannelPattern pattern;          // Identified usage pattern
    int buffer_size;                 // Buffer size (0 for unbuffered)
    int send_count;                  // Number of send operations
    int recv_count;                  // Number of receive operations
    int close_count;                 // Number of close operations
    bool used_in_select;             // Whether channel is used in select statements
    bool escapes_function;           // Whether channel escapes its creating function
    bool escapes_to_goroutine;       // Whether channel is used in goroutines
    bool optimized_buffer_size;      // Whether buffer size is optimized
    int optimal_buffer_size;         // Computed optimal buffer size
    bool convert_to_local;           // Whether to convert to local-only optimized version
    struct ChannelInfo* next;        // Next in the list
} ChannelInfo;

// Module-level analysis context
typedef struct {
    ChannelInfo* channels;            // List of analyzed channels
    ASTNode* current_function;        // Current function being analyzed
    int pass_number;                  // Current pass number
} ChannelOptimizationContext;

// Create a new channel optimization context
static ChannelOptimizationContext* create_context(void) {
    ChannelOptimizationContext* ctx = malloc(sizeof(ChannelOptimizationContext));
    if (!ctx) return NULL;
    
    ctx->channels = NULL;
    ctx->current_function = NULL;
    ctx->pass_number = 0;
    
    return ctx;
}

// Free a channel optimization context
static void free_context(ChannelOptimizationContext* ctx) {
    if (!ctx) return;
    
    // Free the channel info list
    ChannelInfo* info = ctx->channels;
    while (info) {
        ChannelInfo* next = info->next;
        free(info->name);
        free(info);
        info = next;
    }
    
    free(ctx);
}

// Register a channel in the context
static ChannelInfo* register_channel(ChannelOptimizationContext* ctx, const char* name, ASTNode* decl_node, int buffer_size) {
    if (!ctx || !name) return NULL;
    
    // Check if channel already registered
    ChannelInfo* info = ctx->channels;
    while (info) {
        if (strcmp(info->name, name) == 0) {
            return info;
        }
        info = info->next;
    }
    
    // Create new channel info
    info = malloc(sizeof(ChannelInfo));
    if (!info) return NULL;
    
    info->name = strdup(name);
    info->decl_node = decl_node;
    info->pattern = CHANNEL_PATTERN_UNKNOWN;
    info->buffer_size = buffer_size;
    info->send_count = 0;
    info->recv_count = 0;
    info->close_count = 0;
    info->used_in_select = false;
    info->escapes_function = false;
    info->escapes_to_goroutine = false;
    info->optimized_buffer_size = false;
    info->optimal_buffer_size = buffer_size;
    info->convert_to_local = false;
    info->next = ctx->channels;
    
    ctx->channels = info;
    return info;
}

// Find channel info by name
static ChannelInfo* find_channel(ChannelOptimizationContext* ctx, const char* name) {
    if (!ctx || !name) return NULL;
    
    ChannelInfo* info = ctx->channels;
    while (info) {
        if (strcmp(info->name, name) == 0) {
            return info;
        }
        info = info->next;
    }
    
    return NULL;
}

// Forward declarations for recursive functions
static void analyze_expression(ChannelOptimizationContext* ctx, ASTNode* expr);
static void analyze_statement(ChannelOptimizationContext* ctx, ASTNode* stmt);
static void analyze_block(ChannelOptimizationContext* ctx, ASTNode* block);

// Analyze a channel make expression
static void analyze_make_channel(ChannelOptimizationContext* ctx, ASTNode* make_expr, const char* var_name) {
    if (!ctx || !make_expr || make_expr->type != AST_MAKE_EXPR) {
        return;
    }
    
    // Only analyze if it's a channel
    if (make_expr->make_expr.type_kind != TYPE_CHANNEL) {
        return;
    }
    
    // Get buffer size
    int buffer_size = 0;
    if (make_expr->make_expr.size && make_expr->make_expr.size->type == AST_INT_LIT) {
        buffer_size = make_expr->make_expr.size->int_lit.value;
    }
    
    // Register the channel
    register_channel(ctx, var_name, make_expr, buffer_size);
}

// Analyze a channel send expression
static void analyze_send_expr(ChannelOptimizationContext* ctx, ASTNode* send) {
    if (!ctx || !send || send->type != AST_SEND_EXPR) {
        return;
    }
    
    // Analyze channel expression
    analyze_expression(ctx, send->send_expr.channel);
    
    // Analyze value expression
    analyze_expression(ctx, send->send_expr.value);
    
    // If sending to a channel variable, record the send
    if (send->send_expr.channel->type == AST_VAR_REF) {
        const char* chan_name = send->send_expr.channel->var_ref.name;
        ChannelInfo* info = find_channel(ctx, chan_name);
        
        if (info) {
            info->send_count++;
        }
    }
}

// Analyze a channel receive expression
static void analyze_recv_expr(ChannelOptimizationContext* ctx, ASTNode* recv) {
    if (!ctx || !recv || recv->type != AST_RECV_EXPR) {
        return;
    }
    
    // Analyze channel expression
    analyze_expression(ctx, recv->recv_expr.channel);
    
    // If receiving from a channel variable, record the receive
    if (recv->recv_expr.channel->type == AST_VAR_REF) {
        const char* chan_name = recv->recv_expr.channel->var_ref.name;
        ChannelInfo* info = find_channel(ctx, chan_name);
        
        if (info) {
            info->recv_count++;
        }
    }
}

// Analyze a close expression
static void analyze_close_expr(ChannelOptimizationContext* ctx, ASTNode* close) {
    if (!ctx || !close || close->type != AST_CLOSE_EXPR) {
        return;
    }
    
    // Analyze channel expression
    analyze_expression(ctx, close->close_expr.channel);
    
    // If closing a channel variable, record the close
    if (close->close_expr.channel->type == AST_VAR_REF) {
        const char* chan_name = close->close_expr.channel->var_ref.name;
        ChannelInfo* info = find_channel(ctx, chan_name);
        
        if (info) {
            info->close_count++;
        }
    }
}

// Analyze a select statement
static void analyze_select_stmt(ChannelOptimizationContext* ctx, ASTNode* select) {
    if (!ctx || !select || select->type != AST_SELECT_STMT) {
        return;
    }
    
    // Analyze each case
    ASTNode* case_node = select->select_stmt.cases;
    while (case_node) {
        if (case_node->type == AST_SELECT_CASE) {
            // Analyze comm expression (send or receive)
            ASTNode* comm = case_node->select_case.comm;
            if (comm) {
                analyze_expression(ctx, comm);
                
                // Mark channels used in select
                if (comm->type == AST_SEND_EXPR && comm->send_expr.channel->type == AST_VAR_REF) {
                    ChannelInfo* info = find_channel(ctx, comm->send_expr.channel->var_ref.name);
                    if (info) {
                        info->used_in_select = true;
                    }
                } else if (comm->type == AST_RECV_EXPR && comm->recv_expr.channel->type == AST_VAR_REF) {
                    ChannelInfo* info = find_channel(ctx, comm->recv_expr.channel->var_ref.name);
                    if (info) {
                        info->used_in_select = true;
                    }
                }
            }
            
            // Analyze body
            analyze_block(ctx, case_node->select_case.body);
        }
        case_node = case_node->next;
    }
}

// Analyze a function call
static void analyze_call_expr(ChannelOptimizationContext* ctx, ASTNode* call) {
    if (!ctx || !call || call->type != AST_CALL_EXPR) {
        return;
    }
    
    // Analyze the function expression
    analyze_expression(ctx, call->call_expr.func);
    
    // Analyze arguments - check if channels are passed to functions
    ASTNode* arg = call->call_expr.args;
    while (arg) {
        analyze_expression(ctx, arg);
        
        // If the argument is a channel variable, mark it as potentially escaping
        if (arg->type == AST_VAR_REF) {
            ChannelInfo* info = find_channel(ctx, arg->var_ref.name);
            if (info) {
                info->escapes_function = true;
            }
        }
        
        arg = arg->next;
    }
}

// Analyze a goroutine spawn
static void analyze_go_expr(ChannelOptimizationContext* ctx, ASTNode* go_expr) {
    if (!ctx || !go_expr || go_expr->type != AST_GO_EXPR) {
        return;
    }
    
    // Analyze the call
    ASTNode* call = go_expr->go_expr.call;
    if (call->type == AST_CALL_EXPR) {
        // Check arguments for channel references
        ASTNode* arg = call->call_expr.args;
        while (arg) {
            if (arg->type == AST_VAR_REF) {
                ChannelInfo* info = find_channel(ctx, arg->var_ref.name);
                if (info) {
                    info->escapes_to_goroutine = true;
                }
            }
            
            // Recursively analyze the argument expression
            analyze_expression(ctx, arg);
            
            arg = arg->next;
        }
    }
    
    // Analyze the full call expression
    analyze_expression(ctx, call);
}

// Analyze an expression
static void analyze_expression(ChannelOptimizationContext* ctx, ASTNode* expr) {
    if (!ctx || !expr) {
        return;
    }
    
    switch (expr->type) {
        case AST_SEND_EXPR:
            analyze_send_expr(ctx, expr);
            break;
            
        case AST_RECV_EXPR:
            analyze_recv_expr(ctx, expr);
            break;
            
        case AST_CLOSE_EXPR:
            analyze_close_expr(ctx, expr);
            break;
            
        case AST_CALL_EXPR:
            analyze_call_expr(ctx, expr);
            break;
            
        case AST_GO_EXPR:
            analyze_go_expr(ctx, expr);
            break;
            
        case AST_BINARY_EXPR:
            analyze_expression(ctx, expr->binary_expr.left);
            analyze_expression(ctx, expr->binary_expr.right);
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
            
        case AST_SLICE_EXPR:
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
            // Other expression types don't affect channel analysis
            break;
    }
}

// Analyze a variable declaration
static void analyze_var_decl(ChannelOptimizationContext* ctx, ASTNode* var_decl) {
    if (!ctx || !var_decl || var_decl->type != AST_VAR_DECL) {
        return;
    }
    
    // Check the variable type and initializer
    if (var_decl->var_decl.type_ref->type_ref.kind == TYPE_CHANNEL && var_decl->var_decl.init) {
        // If it's a channel created with make
        if (var_decl->var_decl.init->type == AST_MAKE_EXPR) {
            analyze_make_channel(ctx, var_decl->var_decl.init, var_decl->var_decl.name);
        } else {
            // Analyze the initializer expression
            analyze_expression(ctx, var_decl->var_decl.init);
        }
    }
}

// Analyze a statement
static void analyze_statement(ChannelOptimizationContext* ctx, ASTNode* stmt) {
    if (!ctx || !stmt) {
        return;
    }
    
    switch (stmt->type) {
        case AST_BLOCK:
            analyze_block(ctx, stmt);
            break;
            
        case AST_VAR_DECL:
            analyze_var_decl(ctx, stmt);
            break;
            
        case AST_ASSIGN_STMT:
            // Check for channel initializations in assignments
            if (stmt->assign_stmt.right->type == AST_MAKE_EXPR && 
                stmt->assign_stmt.left->type == AST_VAR_REF) {
                analyze_make_channel(ctx, stmt->assign_stmt.right, stmt->assign_stmt.left->var_ref.name);
            }
            // Analyze the expressions
            analyze_expression(ctx, stmt->assign_stmt.left);
            analyze_expression(ctx, stmt->assign_stmt.right);
            break;
            
        case AST_SELECT_STMT:
            analyze_select_stmt(ctx, stmt);
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
            
        default:
            // Other statement types don't affect channel analysis
            break;
    }
}

// Analyze a block of statements
static void analyze_block(ChannelOptimizationContext* ctx, ASTNode* block) {
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

// Analyze a function definition
static void analyze_function(ChannelOptimizationContext* ctx, ASTNode* func) {
    if (!ctx || !func || (func->type != AST_FUNC_DECL && func->type != AST_FUNC_LIT)) {
        return;
    }
    
    // Set current function
    ctx->current_function = func;
    
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
    
    // Reset current function
    ctx->current_function = NULL;
}

// Apply optimizations to channels
static void apply_channel_optimizations(ChannelOptimizationContext* ctx) {
    if (!ctx) return;
    
    ChannelInfo* info = ctx->channels;
    while (info) {
        // Determine the channel pattern
        if (info->send_count == 1 && info->recv_count == 1 && !info->used_in_select) {
            info->pattern = CHANNEL_PATTERN_ONESHOT;
        } else if (info->send_count == 1 && info->recv_count > 1) {
            info->pattern = CHANNEL_PATTERN_BROADCAST;
        } else if (info->send_count > 1 && info->recv_count > 1) {
            info->pattern = CHANNEL_PATTERN_WORKER_POOL;
        } else if (!info->escapes_function && !info->escapes_to_goroutine) {
            info->pattern = CHANNEL_PATTERN_LOCAL_ONLY;
        } else if (info->used_in_select && (info->send_count > 0 || info->recv_count > 0)) {
            info->pattern = CHANNEL_PATTERN_SYNCHRONIZATION;
        }
        
        // Optimize based on the pattern
        switch (info->pattern) {
            case CHANNEL_PATTERN_ONESHOT:
                // For one-shot channels, buffer size of 1 is optimal
                if (info->buffer_size != 1) {
                    info->optimized_buffer_size = true;
                    info->optimal_buffer_size = 1;
                }
                break;
                
            case CHANNEL_PATTERN_BROADCAST:
                // For broadcast channels, buffer size should match receiver count
                if (info->buffer_size < info->recv_count) {
                    info->optimized_buffer_size = true;
                    info->optimal_buffer_size = info->recv_count;
                }
                break;
                
            case CHANNEL_PATTERN_WORKER_POOL:
                // For worker pools, buffer size should be a multiple of worker count
                // Simple heuristic: max(send_count, recv_count)
                if (info->buffer_size < info->send_count || info->buffer_size < info->recv_count) {
                    info->optimized_buffer_size = true;
                    info->optimal_buffer_size = (info->send_count > info->recv_count) ? 
                                               info->send_count : info->recv_count;
                }
                break;
                
            case CHANNEL_PATTERN_LOCAL_ONLY:
                // Local-only channels can be optimized to use direct function calls
                info->convert_to_local = true;
                break;
                
            case CHANNEL_PATTERN_SYNCHRONIZATION:
                // Synchronization channels are often unbuffered by design
                if (info->buffer_size != 0) {
                    info->optimized_buffer_size = true;
                    info->optimal_buffer_size = 0;
                }
                break;
                
            default:
                // Unknown pattern, no optimization
                break;
        }
        
        // Apply the optimizations to the AST
        if (info->optimized_buffer_size && info->decl_node && 
            info->decl_node->type == AST_MAKE_EXPR &&
            info->decl_node->make_expr.type_kind == TYPE_CHANNEL) {
            
            // Create a new integer literal for the size
            ASTNode* size_node = malloc(sizeof(ASTNode));
            if (size_node) {
                size_node->type = AST_INT_LIT;
                size_node->int_lit.value = info->optimal_buffer_size;
                
                // Replace the old size expression
                if (info->decl_node->make_expr.size) {
                    // TODO: Properly free the old node to avoid memory leaks
                }
                info->decl_node->make_expr.size = size_node;
                
                printf("Channel Optimization: Optimized buffer size for channel '%s' "
                      "from %d to %d (pattern: %d)\n", 
                      info->name, info->buffer_size, info->optimal_buffer_size, info->pattern);
            }
        }
        
        // For local-only channels, add optimization hint
        if (info->convert_to_local && info->decl_node && 
            info->decl_node->type == AST_MAKE_EXPR &&
            info->decl_node->make_expr.type_kind == TYPE_CHANNEL) {
            
            info->decl_node->make_expr.local_only = true;
            
            printf("Channel Optimization: Marked channel '%s' as local-only for "
                  "direct function call optimization\n", info->name);
        }
        
        info = info->next;
    }
}

// Optimize select statements with known channel behaviors
void optimize_select_statements(ASTNode* root) {
    // Implementation placeholder
    // This would identify select statements where channel behavior is known
    // and optimize them (e.g., removing cases that can never execute)
}

// Entry point for channel optimization
void optimize_channels(ASTNode* root) {
    if (!root) return;
    
    // Create optimization context
    ChannelOptimizationContext* ctx = create_context();
    if (!ctx) return;
    
    // First pass: Analyze channel usage patterns
    ctx->pass_number = 1;
    ASTNode* node = root;
    while (node) {
        if (node->type == AST_FUNC_DECL) {
            analyze_function(ctx, node);
        }
        node = node->next;
    }
    
    // Apply optimizations based on analysis
    apply_channel_optimizations(ctx);
    
    // Clean up
    free_context(ctx);
    
    // Optimize select statements separately
    optimize_select_statements(root);
} 