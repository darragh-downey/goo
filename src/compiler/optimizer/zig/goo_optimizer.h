/**
 * goo_optimizer.h
 * 
 * C interface for the Goo compiler optimizer built in Zig
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_OPTIMIZER_H
#define GOO_OPTIMIZER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque handle to an IR module
 */
typedef struct GooIRModule GooIRModule;

/**
 * @brief Opaque handle to an IR function
 */
typedef struct GooIRFunction GooIRFunction;

/**
 * @brief Opaque handle to an IR basic block
 */
typedef struct GooIRBasicBlock GooIRBasicBlock;

/**
 * @brief Opaque handle to an IR instruction
 */
typedef struct GooIRInstruction GooIRInstruction;

/**
 * @brief Value handle
 */
typedef uint32_t GooIRValue;

/**
 * @brief Instruction opcodes
 */
typedef enum {
    GOO_OP_NOP = 0,
    GOO_OP_CONST,
    GOO_OP_MOVE,
    GOO_OP_LOAD,
    GOO_OP_STORE,
    
    // Arithmetic
    GOO_OP_ADD,
    GOO_OP_SUB,
    GOO_OP_MUL,
    GOO_OP_DIV,
    GOO_OP_MOD,
    GOO_OP_NEG,
    
    // Logical
    GOO_OP_AND,
    GOO_OP_OR,
    GOO_OP_XOR,
    GOO_OP_NOT,
    GOO_OP_SHL,
    GOO_OP_SHR,
    
    // Comparison
    GOO_OP_EQ,
    GOO_OP_NE,
    GOO_OP_LT,
    GOO_OP_LE,
    GOO_OP_GT,
    GOO_OP_GE,
    
    // Control flow
    GOO_OP_JUMP,
    GOO_OP_BRANCH,
    GOO_OP_RETURN,
    GOO_OP_CALL,
    GOO_OP_TAILCALL,
    
    // Memory management
    GOO_OP_ALLOC,
    GOO_OP_FREE,
    
    // Goo-specific
    GOO_OP_GOROUTINE_SPAWN,
    GOO_OP_GOROUTINE_YIELD,
    GOO_OP_GOROUTINE_JOIN,
    GOO_OP_CHANNEL_SEND,
    GOO_OP_CHANNEL_RECEIVE,
    GOO_OP_CHANNEL_CLOSE
} GooIROpcode;

/**
 * @brief Basic block types
 */
typedef enum {
    GOO_BLOCK_ENTRY = 0,
    GOO_BLOCK_NORMAL,
    GOO_BLOCK_LOOP,
    GOO_BLOCK_BRANCH,
    GOO_BLOCK_EXIT
} GooIRBlockType;

/**
 * @brief Optimization levels
 */
typedef enum {
    GOO_OPT_NONE = 0,  // -O0: No optimizations
    GOO_OPT_DEBUG,     // -O1: Basic optimizations that don't interfere with debugging
    GOO_OPT_DEFAULT,   // -O2: Default optimizations for production code
    GOO_OPT_SIZE,      // -Os: Optimize for size
    GOO_OPT_SPEED      // -O3: Optimize for speed, potentially at the cost of binary size
} GooOptimizationLevel;

/**
 * @brief Opaque handle to a pass manager
 */
typedef struct GooPassManager GooPassManager;

/**
 * IR Module Functions
 */

/**
 * @brief Initialize the IR system
 * @return true if successful, false otherwise
 */
bool goo_ir_init(void);

/**
 * @brief Shut down the IR system and release resources
 */
void goo_ir_shutdown(void);

/**
 * @brief Create a new module
 * @param name The name of the module
 * @return Handle to the module or NULL on error
 */
GooIRModule* goo_ir_create_module(const char* name);

/**
 * @brief Destroy a module and free its resources
 * @param module Handle to the module
 */
void goo_ir_destroy_module(GooIRModule* module);

/**
 * @brief Dump a module to a string
 * @param module Handle to the module
 * @param output_buffer Buffer to store the output
 * @param buffer_size Size of the buffer
 * @return Number of bytes written, or 0 on error
 */
size_t goo_ir_dump_module(GooIRModule* module, char* output_buffer, size_t buffer_size);

/**
 * Function manipulation
 */

/**
 * @brief Add a function to a module
 * @param module Handle to the module
 * @param name The function name
 * @return Handle to the function or NULL on error
 */
GooIRFunction* goo_ir_add_function(GooIRModule* module, const char* name);

/**
 * @brief Get a function from a module by name
 * @param module Handle to the module
 * @param name The function name
 * @return Handle to the function or NULL if not found
 */
GooIRFunction* goo_ir_get_function(GooIRModule* module, const char* name);

/**
 * Basic block manipulation
 */

/**
 * @brief Add a basic block to a function
 * @param function Handle to the function
 * @param block_type The type of block
 * @param name The block name (can be NULL)
 * @return Handle to the basic block or NULL on error
 */
GooIRBasicBlock* goo_ir_add_basic_block(GooIRFunction* function, GooIRBlockType block_type, const char* name);

/**
 * @brief Link two blocks (predecessor -> successor)
 * @param pred Handle to the predecessor block
 * @param succ Handle to the successor block
 * @return true if successful, false otherwise
 */
bool goo_ir_link_blocks(GooIRBasicBlock* pred, GooIRBasicBlock* succ);

/**
 * Value manipulation
 */

/**
 * @brief Create a local variable in a function
 * @param function Handle to the function
 * @param name The variable name (can be NULL)
 * @return Value ID for the variable, or 0 on error
 */
GooIRValue goo_ir_create_local(GooIRFunction* function, const char* name);

/**
 * @brief Create a function parameter
 * @param function Handle to the function
 * @param name The parameter name (can be NULL)
 * @return Value ID for the parameter, or 0 on error
 */
GooIRValue goo_ir_create_param(GooIRFunction* function, const char* name);

/**
 * Instruction manipulation
 */

/**
 * @brief Create a new instruction
 * @param block Handle to the block
 * @param opcode The instruction opcode
 * @return Handle to the instruction or NULL on error
 */
GooIRInstruction* goo_ir_create_instruction(GooIRBasicBlock* block, GooIROpcode opcode);

/**
 * @brief Add an operand to an instruction
 * @param instr Handle to the instruction
 * @param value The value ID to add as operand
 * @return true if successful, false otherwise
 */
bool goo_ir_add_operand(GooIRInstruction* instr, GooIRValue value);

/**
 * @brief Set the result of an instruction
 * @param instr Handle to the instruction
 * @param value The value ID to set as result
 * @return true if successful, false otherwise
 */
bool goo_ir_set_result(GooIRInstruction* instr, GooIRValue value);

/**
 * @brief Add an instruction to a basic block
 * @param block Handle to the block
 * @param instr Handle to the instruction
 * @return true if successful, false otherwise
 */
bool goo_ir_add_instruction(GooIRBasicBlock* block, GooIRInstruction* instr);

/**
 * Helper functions for common instructions
 */

/**
 * @brief Create a constant instruction
 * @param block Handle to the block
 * @param value The constant value
 * @param result The value ID to store the result
 * @return true if successful, false otherwise
 */
bool goo_ir_create_const(GooIRBasicBlock* block, int64_t value, GooIRValue result);

/**
 * @brief Create a binary operation instruction
 * @param block Handle to the block
 * @param opcode The operation opcode
 * @param left Left operand value ID
 * @param right Right operand value ID
 * @param result The value ID to store the result
 * @return true if successful, false otherwise
 */
bool goo_ir_create_binary_op(GooIRBasicBlock* block, GooIROpcode opcode, GooIRValue left, GooIRValue right, GooIRValue result);

/**
 * @brief Create a return instruction
 * @param block Handle to the block
 * @param value The value ID to return (can be 0 for void return)
 * @return true if successful, false otherwise
 */
bool goo_ir_create_return(GooIRBasicBlock* block, GooIRValue value);

/**
 * Test functions
 */

/**
 * @brief Run tests for the IR bindings
 * @return true if tests pass, false otherwise
 */
bool goo_ir_test_bindings(void);

/**
 * Pass manager functions
 */

/**
 * @brief Create a new pass manager
 * @param opt_level The optimization level
 * @param verbose Enable verbose output
 * @return Handle to the pass manager or NULL on error
 */
GooPassManager* goo_pass_manager_create(GooOptimizationLevel opt_level, bool verbose);

/**
 * @brief Destroy a pass manager and free its resources
 * @param pass_manager Handle to the pass manager
 */
void goo_pass_manager_destroy(GooPassManager* pass_manager);

/**
 * @brief Run all passes on a module
 * @param pass_manager Handle to the pass manager
 * @param module Handle to the module
 * @return true if any changes were made, false otherwise or on error
 */
bool goo_pass_manager_run(GooPassManager* pass_manager, GooIRModule* module);

/**
 * Standard optimization passes
 */

/**
 * @brief Add the constant folding pass to the pass manager
 * @param pass_manager Handle to the pass manager
 * @param max_iterations Maximum number of folding iterations
 * @return true if successful, false otherwise
 */
bool goo_pass_manager_add_constant_folding(GooPassManager* pass_manager, uint32_t max_iterations);

/**
 * @brief Add the dead code elimination pass to the pass manager
 * @param pass_manager Handle to the pass manager
 * @return true if successful, false otherwise
 */
bool goo_pass_manager_add_dead_code_elimination(GooPassManager* pass_manager);

#ifdef __cplusplus
}
#endif

#endif /* GOO_OPTIMIZER_H */ 