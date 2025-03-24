/**
 * optimizer_test.c
 * 
 * Test program for the Goo optimizer C API
 */

#include "../zig/goo_optimizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple test function to create and optimize a module
static bool test_create_and_optimize_module(void) {
    printf("Testing module creation and optimization...\n");
    
    // Initialize the IR system
    if (!goo_ir_init()) {
        fprintf(stderr, "Failed to initialize IR system\n");
        return false;
    }
    
    // Create a module
    GooIRModule* module = goo_ir_create_module("test_module");
    if (!module) {
        fprintf(stderr, "Failed to create module\n");
        goo_ir_shutdown();
        return false;
    }
    
    // Add a function to the module
    GooIRFunction* func = goo_ir_add_function(module, "test_function");
    if (!func) {
        fprintf(stderr, "Failed to create function\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create parameters
    GooIRValue param1 = goo_ir_create_param(func, "a");
    GooIRValue param2 = goo_ir_create_param(func, "b");
    if (param1 == 0 || param2 == 0) {
        fprintf(stderr, "Failed to create parameters\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a local variable for the result
    GooIRValue result = goo_ir_create_local(func, "result");
    if (result == 0) {
        fprintf(stderr, "Failed to create local variable\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a basic block
    GooIRBasicBlock* entry_block = goo_ir_add_basic_block(func, GOO_BLOCK_ENTRY, "entry");
    if (!entry_block) {
        fprintf(stderr, "Failed to create entry block\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create an exit block
    GooIRBasicBlock* exit_block = goo_ir_add_basic_block(func, GOO_BLOCK_EXIT, "exit");
    if (!exit_block) {
        fprintf(stderr, "Failed to create exit block\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Link the blocks
    if (!goo_ir_link_blocks(entry_block, exit_block)) {
        fprintf(stderr, "Failed to link blocks\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a binary operation: result = a + b
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_ADD, param1, param2, result)) {
        fprintf(stderr, "Failed to create add instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a return instruction
    if (!goo_ir_create_return(exit_block, result)) {
        fprintf(stderr, "Failed to create return instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Dump the module before optimization
    char buffer[4096] = {0};
    size_t written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump module\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module before optimization:\n%s\n", buffer);
    
    // Create a pass manager
    GooPassManager* pass_manager = goo_pass_manager_create(GOO_OPT_DEFAULT, true);
    if (!pass_manager) {
        fprintf(stderr, "Failed to create pass manager\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Add optimization passes
    if (!goo_pass_manager_add_constant_folding(pass_manager, 3)) {
        fprintf(stderr, "Failed to add constant folding pass\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Run optimizations
    printf("Running optimizations...\n");
    bool changed = goo_pass_manager_run(pass_manager, module);
    printf("Optimizations %s the module.\n", changed ? "modified" : "did not modify");
    
    // Dump the module after optimization
    memset(buffer, 0, sizeof(buffer));
    written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump optimized module\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module after optimization:\n%s\n", buffer);
    
    // Clean up
    goo_pass_manager_destroy(pass_manager);
    goo_ir_destroy_module(module);
    goo_ir_shutdown();
    
    return true;
}

// Test creating a more complex IR with constant folding opportunities
static bool test_constant_folding(void) {
    printf("Testing constant folding optimization...\n");
    
    // Initialize the IR system
    if (!goo_ir_init()) {
        fprintf(stderr, "Failed to initialize IR system\n");
        return false;
    }
    
    // Create a module
    GooIRModule* module = goo_ir_create_module("folding_test");
    if (!module) {
        fprintf(stderr, "Failed to create module\n");
        goo_ir_shutdown();
        return false;
    }
    
    // Add a function to the module
    GooIRFunction* func = goo_ir_add_function(module, "fold_constants");
    if (!func) {
        fprintf(stderr, "Failed to create function\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create local variables
    GooIRValue const1 = goo_ir_create_local(func, "const1");
    GooIRValue const2 = goo_ir_create_local(func, "const2");
    GooIRValue sum = goo_ir_create_local(func, "sum");
    GooIRValue product = goo_ir_create_local(func, "product");
    GooIRValue result = goo_ir_create_local(func, "result");
    
    if (const1 == 0 || const2 == 0 || sum == 0 || product == 0 || result == 0) {
        fprintf(stderr, "Failed to create local variables\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create basic blocks
    GooIRBasicBlock* entry_block = goo_ir_add_basic_block(func, GOO_BLOCK_ENTRY, "entry");
    GooIRBasicBlock* exit_block = goo_ir_add_basic_block(func, GOO_BLOCK_EXIT, "exit");
    
    if (!entry_block || !exit_block) {
        fprintf(stderr, "Failed to create blocks\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Link the blocks
    if (!goo_ir_link_blocks(entry_block, exit_block)) {
        fprintf(stderr, "Failed to link blocks\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create constant instructions: const1 = 10, const2 = 20
    if (!goo_ir_create_const(entry_block, 10, const1) ||
        !goo_ir_create_const(entry_block, 20, const2)) {
        fprintf(stderr, "Failed to create constant instructions\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create sum = const1 + const2 (10 + 20 = 30)
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_ADD, const1, const2, sum)) {
        fprintf(stderr, "Failed to create add instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create product = const1 * const2 (10 * 20 = 200)
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_MUL, const1, const2, product)) {
        fprintf(stderr, "Failed to create multiply instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create result = sum + product (30 + 200 = 230)
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_ADD, sum, product, result)) {
        fprintf(stderr, "Failed to create final add instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a return instruction
    if (!goo_ir_create_return(exit_block, result)) {
        fprintf(stderr, "Failed to create return instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Dump the module before optimization
    char buffer[4096] = {0};
    size_t written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump module\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module before optimization:\n%s\n", buffer);
    
    // Create a pass manager with constant folding pass
    GooPassManager* pass_manager = goo_pass_manager_create(GOO_OPT_DEFAULT, true);
    if (!pass_manager) {
        fprintf(stderr, "Failed to create pass manager\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Add constant folding pass
    if (!goo_pass_manager_add_constant_folding(pass_manager, 5)) {
        fprintf(stderr, "Failed to add constant folding pass\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Run optimizations
    printf("Running constant folding optimization...\n");
    bool changed = goo_pass_manager_run(pass_manager, module);
    printf("Constant folding %s the module.\n", changed ? "modified" : "did not modify");
    
    // Dump the module after optimization
    memset(buffer, 0, sizeof(buffer));
    written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump optimized module\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module after optimization:\n%s\n", buffer);
    
    // Ideally, optimization should have folded all constants to result = 230
    
    // Clean up
    goo_pass_manager_destroy(pass_manager);
    goo_ir_destroy_module(module);
    goo_ir_shutdown();
    
    return true;
}

// Test the binding self-tests
static bool test_bindings(void) {
    printf("Running IR binding tests...\n");
    if (!goo_ir_test_bindings()) {
        fprintf(stderr, "IR binding tests failed\n");
        return false;
    }
    printf("IR binding tests passed\n");
    return true;
}

// Test creating a function with dead code and unreachable blocks
static bool test_dead_code_elimination(void) {
    printf("Testing dead code elimination...\n");
    
    // Initialize the IR system
    if (!goo_ir_init()) {
        fprintf(stderr, "Failed to initialize IR system\n");
        return false;
    }
    
    // Create a module
    GooIRModule* module = goo_ir_create_module("dce_test");
    if (!module) {
        fprintf(stderr, "Failed to create module\n");
        goo_ir_shutdown();
        return false;
    }
    
    // Add a function to the module
    GooIRFunction* func = goo_ir_add_function(module, "eliminate_dead_code");
    if (!func) {
        fprintf(stderr, "Failed to create function\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create local variables
    GooIRValue a = goo_ir_create_local(func, "a");
    GooIRValue b = goo_ir_create_local(func, "b");
    GooIRValue c = goo_ir_create_local(func, "c");
    GooIRValue unused = goo_ir_create_local(func, "unused");
    
    if (a == 0 || b == 0 || c == 0 || unused == 0) {
        fprintf(stderr, "Failed to create local variables\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create basic blocks - including one that will be unreachable
    GooIRBasicBlock* entry_block = goo_ir_add_basic_block(func, GOO_BLOCK_ENTRY, "entry");
    GooIRBasicBlock* unreachable_block = goo_ir_add_basic_block(func, GOO_BLOCK_NORMAL, "unreachable");
    GooIRBasicBlock* exit_block = goo_ir_add_basic_block(func, GOO_BLOCK_EXIT, "exit");
    
    if (!entry_block || !unreachable_block || !exit_block) {
        fprintf(stderr, "Failed to create blocks\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Only link entry to exit, leaving unreachable_block isolated
    if (!goo_ir_link_blocks(entry_block, exit_block)) {
        fprintf(stderr, "Failed to link blocks\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create instructions in entry block
    // a = 10
    if (!goo_ir_create_const(entry_block, 10, a)) {
        fprintf(stderr, "Failed to create a constant\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // b = 20
    if (!goo_ir_create_const(entry_block, 20, b)) {
        fprintf(stderr, "Failed to create b constant\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // c = a + b (used in return)
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_ADD, a, b, c)) {
        fprintf(stderr, "Failed to create add instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // unused = a * b (dead code - not used)
    if (!goo_ir_create_binary_op(entry_block, GOO_OP_MUL, a, b, unused)) {
        fprintf(stderr, "Failed to create multiply instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create an instruction in the unreachable block
    GooIRInstruction* unreachable_instr = goo_ir_create_instruction(unreachable_block, GOO_OP_CONST);
    if (!unreachable_instr) {
        fprintf(stderr, "Failed to create instruction in unreachable block\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    if (!goo_ir_add_instruction(unreachable_block, unreachable_instr)) {
        fprintf(stderr, "Failed to add instruction to unreachable block\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Create a return instruction
    if (!goo_ir_create_return(exit_block, c)) {
        fprintf(stderr, "Failed to create return instruction\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Dump the module before optimization
    char buffer[4096] = {0};
    size_t written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump module\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module before optimization:\n%s\n", buffer);
    
    // Create a pass manager
    GooPassManager* pass_manager = goo_pass_manager_create(GOO_OPT_DEFAULT, true);
    if (!pass_manager) {
        fprintf(stderr, "Failed to create pass manager\n");
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Add dead code elimination pass
    if (!goo_pass_manager_add_dead_code_elimination(pass_manager)) {
        fprintf(stderr, "Failed to add dead code elimination pass\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    // Run optimizations
    printf("Running dead code elimination...\n");
    bool changed = goo_pass_manager_run(pass_manager, module);
    printf("DCE %s the module.\n", changed ? "modified" : "did not modify");
    
    // Dump the module after optimization
    memset(buffer, 0, sizeof(buffer));
    written = goo_ir_dump_module(module, buffer, sizeof(buffer));
    if (written == 0) {
        fprintf(stderr, "Failed to dump optimized module\n");
        goo_pass_manager_destroy(pass_manager);
        goo_ir_destroy_module(module);
        goo_ir_shutdown();
        return false;
    }
    
    printf("Module after optimization:\n%s\n", buffer);
    
    // Clean up
    goo_pass_manager_destroy(pass_manager);
    goo_ir_destroy_module(module);
    goo_ir_shutdown();
    
    return true;
}

int main(void) {
    printf("=== Goo Optimizer C API Test ===\n\n");
    
    bool success = true;
    
    // Test the bindings
    success = test_bindings() && success;
    printf("\n");
    
    // Test creating and optimizing a simple module
    success = test_create_and_optimize_module() && success;
    printf("\n");
    
    // Test constant folding
    success = test_constant_folding() && success;
    printf("\n");
    
    // Test dead code elimination
    success = test_dead_code_elimination() && success;
    printf("\n");
    
    if (success) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("Some tests failed!\n");
        return 1;
    }
} 