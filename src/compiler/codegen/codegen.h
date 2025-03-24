#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../ast/ast.h"
#include "../ast/symbol.h"

// Code generation context structure
typedef struct CodegenContext {
    FILE* output;                    // Output C file
    SymbolTable* symtab;             // Symbol table
    char* current_function;          // Name of the function being generated
    int indent_level;                // Current indentation level
    char* module_name;               // Current module name
    bool in_global_scope;            // Whether we're in global scope
    int label_counter;               // Counter for generating unique labels
    int anon_var_counter;            // Counter for anonymous variables
    bool has_runtime_error;          // Whether a runtime error has occurred
    bool has_defer;                  // Whether current function has defer statements
    bool in_defer;                   // Whether we're in a defer block
    char** string_literals;          // Array of string literals
    int string_literal_count;        // Count of string literals
    char** dependencies;             // Array of module dependencies
    int dependency_count;            // Count of module dependencies
    bool optimize;                   // Whether to perform optimizations
} CodegenContext;

// Initialize the code generation context
CodegenContext* codegen_init(FILE* output, SymbolTable* symtab, bool optimize);

// Free the code generation context
void codegen_free(CodegenContext* ctx);

// Set the current module name
void codegen_set_module_name(CodegenContext* ctx, const char* module_name);

// Generate code for an AST node
void codegen_node(CodegenContext* ctx, ASTNode* node);

// Generate code for a block
void codegen_block(CodegenContext* ctx, ASTNode* block);

// Generate code for a statement
void codegen_statement(CodegenContext* ctx, ASTNode* stmt);

// Generate code for an expression
void codegen_expression(CodegenContext* ctx, ASTNode* expr);

// Generate code for a function declaration
void codegen_function_decl(CodegenContext* ctx, ASTNode* func_decl);

// Generate code for a variable declaration
void codegen_var_decl(CodegenContext* ctx, ASTNode* var_decl);

// Generate code for a type declaration
void codegen_type_decl(CodegenContext* ctx, ASTNode* type_decl);

// Generate code for function parameters
void codegen_parameters(CodegenContext* ctx, ASTNode* params);

// Generate code for function arguments
void codegen_arguments(CodegenContext* ctx, ASTNode* args);

// Generate code for a binary expression
void codegen_binary_expr(CodegenContext* ctx, ASTNode* expr);

// Generate code for a unary expression
void codegen_unary_expr(CodegenContext* ctx, ASTNode* expr);

// Generate code for a literal expression
void codegen_literal(CodegenContext* ctx, ASTNode* literal);

// Generate code for a variable reference
void codegen_var_ref(CodegenContext* ctx, ASTNode* var_ref);

// Generate code for an if statement
void codegen_if_stmt(CodegenContext* ctx, ASTNode* if_stmt);

// Generate code for a for loop
void codegen_for_stmt(CodegenContext* ctx, ASTNode* for_stmt);

// Generate code for a while loop
void codegen_while_stmt(CodegenContext* ctx, ASTNode* while_stmt);

// Generate code for a switch statement
void codegen_switch_stmt(CodegenContext* ctx, ASTNode* switch_stmt);

// Generate code for a return statement
void codegen_return_stmt(CodegenContext* ctx, ASTNode* return_stmt);

// Generate code for a break statement
void codegen_break_stmt(CodegenContext* ctx, ASTNode* break_stmt);

// Generate code for a continue statement
void codegen_continue_stmt(CodegenContext* ctx, ASTNode* continue_stmt);

// Generate code for a goto statement
void codegen_goto_stmt(CodegenContext* ctx, ASTNode* goto_stmt);

// Generate code for a label statement
void codegen_label_stmt(CodegenContext* ctx, ASTNode* label_stmt);

// Generate code for a function call
void codegen_call_expr(CodegenContext* ctx, ASTNode* call_expr);

// Generate code for a method call
void codegen_method_call(CodegenContext* ctx, ASTNode* method_call);

// Generate code for a field access
void codegen_field_access(CodegenContext* ctx, ASTNode* field_access);

// Generate code for array indexing
void codegen_array_index(CodegenContext* ctx, ASTNode* array_index);

// Generate code for a slice expression
void codegen_slice_expr(CodegenContext* ctx, ASTNode* slice_expr);

// Generate code for an array literal
void codegen_array_literal(CodegenContext* ctx, ASTNode* array_literal);

// Generate code for a struct literal
void codegen_struct_literal(CodegenContext* ctx, ASTNode* struct_literal);

// Generate code for a map literal
void codegen_map_literal(CodegenContext* ctx, ASTNode* map_literal);

// Generate code for a channel operation
void codegen_channel_operation(CodegenContext* ctx, ASTNode* channel_op);

// Generate code for goroutine creation
void codegen_goroutine(CodegenContext* ctx, ASTNode* goroutine);

// Generate code for a defer statement
void codegen_defer_stmt(CodegenContext* ctx, ASTNode* defer_stmt);

// Generate code for a panic statement
void codegen_panic_stmt(CodegenContext* ctx, ASTNode* panic_stmt);

// Generate code for a recover expression
void codegen_recover_expr(CodegenContext* ctx, ASTNode* recover_expr);

// Generate code for a select statement
void codegen_select_stmt(CodegenContext* ctx, ASTNode* select_stmt);

// Emit raw code to the output file
void codegen_emit_raw(CodegenContext* ctx, const char* code);

// Emit code with indentation
void codegen_emit(CodegenContext* ctx, const char* format, ...);

// Increase indentation level
void codegen_indent(CodegenContext* ctx);

// Decrease indentation level
void codegen_dedent(CodegenContext* ctx);

// Generate a unique label
char* codegen_new_label(CodegenContext* ctx, const char* prefix);

// Generate a unique variable name
char* codegen_new_var(CodegenContext* ctx, const char* prefix);

// Add a string literal to the context
int codegen_add_string_literal(CodegenContext* ctx, const char* value);

// Get a string literal by index
const char* codegen_get_string_literal(CodegenContext* ctx, int index);

// Add a module dependency
void codegen_add_dependency(CodegenContext* ctx, const char* module_name);

// Generate prologue code (includes, etc.)
void codegen_prologue(CodegenContext* ctx);

// Generate epilogue code
void codegen_epilogue(CodegenContext* ctx);

// ===== Memory Management Functions =====

// Generate code for arena allocator creation
void codegen_arena_create(CodegenContext* ctx, const char* var_name, size_t page_size);

// Generate code for pool allocator creation
void codegen_pool_create(CodegenContext* ctx, const char* var_name, size_t obj_size, size_t capacity);

// Generate code for allocation from an arena
void codegen_arena_alloc(CodegenContext* ctx, const char* arena_var, const char* result_var, 
                        const char* type_name, size_t size, size_t alignment);

// Generate code for allocation from a pool
void codegen_pool_alloc(CodegenContext* ctx, const char* pool_var, const char* result_var, 
                       const char* type_name);

// Generate code for pool deallocate
void codegen_pool_free(CodegenContext* ctx, const char* pool_var, const char* ptr_expr);

// Generate code for arena reset
void codegen_arena_reset(CodegenContext* ctx, const char* arena_var);

// Generate code for pool reset
void codegen_pool_reset(CodegenContext* ctx, const char* pool_var);

// Generate code for runtime allocation
void codegen_runtime_alloc(CodegenContext* ctx, const char* result_var, 
                          const char* type_name, size_t size);

// Generate code for runtime deallocation
void codegen_runtime_free(CodegenContext* ctx, const char* ptr_expr);

// ===== Capability System Functions =====

// Generates code for capability checking
void codegen_capability_check(CodegenContext* ctx, int capability_type);

// Generates code for granting a capability
void codegen_capability_grant(CodegenContext* ctx, int capability_type, const char* data_expr);

// Generates code for revoking a capability
void codegen_capability_revoke(CodegenContext* ctx, int capability_type);

// Processes a capability attribute in the AST
void codegen_process_capability_attr(CodegenContext* ctx, ASTNode* attr, ASTNode* func_decl);

// Generate code for spawning a goroutine with capabilities
void codegen_goroutine_spawn_with_caps(CodegenContext* ctx, 
                                      const char* func_ptr, 
                                      const char* arg_ptr, 
                                      bool inherit_caps);

// Map a capability name to its runtime ID
int codegen_get_capability_id(const char* name);

#endif // CODEGEN_H 