#ifndef GOO_OPTIMIZER_H
#define GOO_OPTIMIZER_H

#include <stdbool.h>
#include "../ast/ast.h"

/**
 * Goo Compiler Optimization Passes
 * 
 * This module provides various optimization passes that transform
 * the AST to improve performance, memory usage, and code size.
 */

// Forward declarations
struct ASTNode;

// ===== Escape Analysis =====

// Check if a name refers to a global variable
bool is_global_variable(const char* name);

// Perform escape analysis to identify heap allocations that can be moved to the stack
void optimize_escape_analysis(ASTNode* root);

// ===== Channel Optimization =====

// Optimize channel operations based on usage patterns
void optimize_channels(ASTNode* root);

// Optimize select statements with known channel behaviors
void optimize_select_statements(ASTNode* root);

// ===== Goroutine Optimization =====

// Perform inlining for simple goroutines that don't need separate thread
void optimize_goroutine_inlining(ASTNode* root);

// Optimize goroutine scheduling based on workload characteristics
void optimize_goroutine_scheduling(ASTNode* root);

// ===== Common Optimizations =====

// Perform constant folding and propagation
void optimize_constants(ASTNode* root);

// Remove dead code (unreachable, unused variables, etc.)
void optimize_dead_code(ASTNode* root);

// Perform function inlining for small, frequently used functions
void optimize_function_inlining(ASTNode* root);

// Perform loop optimizations (unrolling, invariant code motion, etc.)
void optimize_loops(ASTNode* root);

// ===== Main Optimization Functions =====

// Initialize the optimizer
bool optimizer_init(void);

// Clean up the optimizer
void optimizer_cleanup(void);

// Run all enabled optimization passes on the module
void optimize_module(ASTNode* root, int optimization_level);

// Optimization flags
typedef enum {
    OPT_ESCAPE_ANALYSIS     = (1 << 0),
    OPT_CHANNEL_OPTIMIZATION = (1 << 1),
    OPT_GOROUTINE_INLINING  = (1 << 2),
    OPT_CONSTANT_FOLDING    = (1 << 3),
    OPT_DEAD_CODE_REMOVAL   = (1 << 4),
    OPT_FUNCTION_INLINING   = (1 << 5),
    OPT_LOOP_OPTIMIZATION   = (1 << 6),
    OPT_ALL                 = 0xFFFF
} OptimizationFlags;

// Run specific optimization passes on the module
void optimize_module_with_flags(ASTNode* root, OptimizationFlags flags);

#endif // GOO_OPTIMIZER_H 