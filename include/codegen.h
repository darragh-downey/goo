#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>
#include <stdint.h>
#include <llvm-c/Core.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/ExecutionEngine.h>
#include <llvm-c/Target.h>
#include <llvm-c/BitWriter.h>

// Try to include Scalar.h if available
#if __has_include(<llvm-c/Transforms/Scalar.h>)
#include <llvm-c/Transforms/Scalar.h>
#endif

#if __has_include(<llvm-c/Transforms/IPO.h>)
#include <llvm-c/Transforms/IPO.h>
#endif

#include <llvm-c/DebugInfo.h>
#include "ast.h"
#include "goo_core.h"
#include "goo.h"
#include "goo_runtime.h"

// Forward declaration for LLVM context (to avoid LLVM headers here)
typedef struct LLVMOpaqueContext* LLVMContextRef;
typedef struct LLVMOpaqueModule* LLVMModuleRef;
typedef struct LLVMOpaqueBuilder* LLVMBuilderRef;
typedef struct LLVMOpaqueDIBuilder* LLVMDIBuilderRef;

// Channel type definition
typedef enum {
    GOO_CHANNEL_NORMAL,      // Standard channel
    GOO_CHANNEL_BUFFERED,    // Buffered channel
    GOO_CHANNEL_BROADCAST,   // One-to-many channel
    GOO_CHANNEL_MULTICAST,   // Select receivers
    GOO_CHANNEL_PRIORITY     // Priority-based channel
} GooChannelType;

// Interpreter state structure
typedef struct {
    LLVMExecutionEngineRef engine;
    LLVMValueRef current_function;
    void** args;
    unsigned arg_count;
    GooCodegenContext* context;
} GooInterpreterState;

// Code generator context
typedef struct {
    GooNode* root;                    // Root node to generate code from
    LLVMModuleRef module;             // LLVM module
    LLVMBuilderRef builder;           // LLVM instruction builder
    LLVMContextRef context;           // LLVM context
    GooCompilationMode mode;          // Compilation mode
    struct SymbolTable* symbol_table; // Symbol table for variable lookups
    struct TypeTable* type_table;     // Type table for type lookups
    
    // Language types
    LLVMTypeRef string_type;          // Goo string type
    LLVMTypeRef string_ptr_type;      // Pointer to Goo string type
    LLVMTypeRef array_type;           // Goo array type
    LLVMTypeRef array_ptr_type;       // Pointer to Goo array type
    
    // Runtime options
    bool enable_distributed;          // Enable distributed channels
    int thread_pool_size;             // Thread pool size for goroutines
    GooSupervisionPolicy supervision_policy; // Default supervision policy
    char* runtime_lib_path;           // Path to the runtime library
    
    // Debug information
    bool debug_mode;                  // Whether to generate debug info
    LLVMDIBuilderRef di_builder;      // Debug info builder
    LLVMMetadataRef compile_unit;     // Current compilation unit
    
    // JIT and interpreter state
    LLVMExecutionEngineRef jit_engine; // JIT execution engine
    GooInterpreterState* interpreter;  // Interpreter state
    
    // Optimization level
    bool optimize;                     // Enable optimization
} GooCodegenContext;

// Initialize the code generator
GooCodegenContext* goo_codegen_init(GooCompilationMode mode, bool optimize);

// Free the code generator context
void goo_codegen_free(GooCodegenContext* context);

// Initialize the runtime support
bool goo_codegen_init_runtime(GooCodegenContext* context);

// Initialize the runtime in the main function
void goo_codegen_init_main_runtime(GooCodegenContext* context, LLVMValueRef main_func);

// Generate code for the root node
bool goo_codegen_generate(GooCodegenContext* context, GooNode* root);

// Generate code for a specific node
LLVMValueRef goo_codegen_node(GooCodegenContext* context, GooNode* node);

// Verify the generated module
bool goo_codegen_verify_module(GooCodegenContext* context);

// Emit the generated module to a file
bool goo_codegen_emit(GooCodegenContext* context, const char* filename);

// JIT compilation and execution
bool goo_codegen_init_jit(GooCodegenContext* context);
LLVMValueRef goo_codegen_jit_get_function(GooCodegenContext* context, const char* name);
bool goo_codegen_jit_run(GooCodegenContext* context, LLVMValueRef func, void** args, int arg_count);
bool goo_codegen_run_jit(GooCodegenContext* context);

// Interpreter functions
bool goo_interpreter_init(GooCodegenContext* context);
void goo_interpreter_free(GooInterpreterState* state);
bool goo_interpreter_set_function(GooInterpreterState* state, const char* name);
bool goo_interpreter_add_arg(GooInterpreterState* state, void* arg);
bool goo_interpreter_run(GooInterpreterState* state);
bool goo_codegen_interpret(GooCodegenContext* context);

// Debug information functions
bool goo_codegen_add_debug_info(GooCodegenContext* context, const char* filename);
LLVMMetadataRef goo_codegen_create_function_debug_info(GooCodegenContext* context, 
                                                    LLVMValueRef func, 
                                                    const char* name, 
                                                    const char* linkage_name,
                                                    LLVMMetadataRef file,
                                                    unsigned line_no);
LLVMMetadataRef goo_codegen_create_local_var_debug_info(GooCodegenContext* context,
                                                     LLVMValueRef local_var,
                                                     const char* name,
                                                     LLVMMetadataRef scope,
                                                     LLVMMetadataRef file,
                                                     unsigned line_no);

// Enhanced channel operations
LLVMValueRef goo_codegen_enhanced_channel_send(GooCodegenContext* context, GooChannelSendNode* node);
LLVMValueRef goo_codegen_enhanced_channel_recv(GooCodegenContext* context, GooChannelRecvNode* node);
LLVMValueRef goo_codegen_channel_create_with_endpoint(GooCodegenContext* context, 
                                                 LLVMTypeRef element_type, 
                                                 int capacity, 
                                                 GooChannelType channel_type,
                                                 const char* endpoint_url);

// Helper functions for specific node types
LLVMValueRef goo_codegen_function(GooCodegenContext* context, GooFunctionNode* node);
LLVMValueRef goo_codegen_var_decl(GooCodegenContext* context, GooVarDeclNode* node);
LLVMValueRef goo_codegen_block(GooCodegenContext* context, GooBlockStmtNode* node);
LLVMValueRef goo_codegen_if(GooCodegenContext* context, GooIfStmtNode* node);
LLVMValueRef goo_codegen_for(GooCodegenContext* context, GooForStmtNode* node);
LLVMValueRef goo_codegen_return(GooCodegenContext* context, GooReturnStmtNode* node);
LLVMValueRef goo_codegen_binary_expr(GooCodegenContext* context, GooBinaryExprNode* node);
LLVMValueRef goo_codegen_unary_expr(GooCodegenContext* context, GooUnaryExprNode* node);
LLVMValueRef goo_codegen_call_expr(GooCodegenContext* context, GooCallExprNode* node);
LLVMValueRef goo_codegen_literal(GooCodegenContext* context, GooNode* node);
LLVMValueRef goo_codegen_identifier(GooCodegenContext* context, GooNode* node);
LLVMValueRef goo_codegen_range_literal(GooCodegenContext* context, GooRangeLiteralNode* node);
LLVMValueRef goo_codegen_channel_send(GooCodegenContext* context, GooChannelSendNode* node);
LLVMValueRef goo_codegen_channel_recv(GooCodegenContext* context, GooChannelRecvNode* node);
LLVMValueRef goo_codegen_go_stmt(GooCodegenContext* context, GooGoStmtNode* node);
LLVMValueRef goo_codegen_go_parallel(GooCodegenContext* context, GooGoParallelNode* node);
LLVMValueRef goo_codegen_comptime_var(GooCodegenContext* context, GooComptimeVarNode* node);
LLVMValueRef goo_codegen_comptime_build(GooCodegenContext* context, GooComptimeBuildNode* node);
LLVMValueRef goo_codegen_module(GooCodegenContext* context, GooModuleNode* node);
LLVMValueRef goo_codegen_import(GooCodegenContext* context, GooImportNode* node);
LLVMValueRef goo_codegen_cap_type(GooCodegenContext* context, GooCapTypeNode* node);
LLVMValueRef goo_codegen_cap_check(GooCodegenContext* context, GooCapCheckNode* node);
LLVMValueRef goo_codegen_cap_grant(GooCodegenContext* context, GooCapGrantNode* node);
LLVMValueRef goo_codegen_cap_revoke(GooCodegenContext* context, GooCapRevokeNode* node);
LLVMValueRef goo_codegen_error_union(GooCodegenContext* context, GooErrorUnionNode* node);
LLVMValueRef goo_codegen_try_expr(GooCodegenContext* context, GooTryExprNode* node);
LLVMValueRef goo_codegen_recover_block(GooCodegenContext* context, GooRecoverBlockNode* node);
LLVMValueRef goo_codegen_supervise(GooCodegenContext* context, GooSuperviseNode* node);

// Type conversion helpers
LLVMTypeRef goo_type_to_llvm_type(GooCodegenContext* context, GooNode* type_node);

#endif // CODEGEN_H 