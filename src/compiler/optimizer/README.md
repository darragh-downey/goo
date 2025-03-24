# Goo Optimizer

This directory contains the Goo compiler's optimizer, implemented in Zig. The optimizer is designed to be used both from Zig code directly and from C code via bindings.

## Components

The optimizer is composed of several key components:

1. **IR (Intermediate Representation)** - The core data structures used to represent code for optimization.
2. **Pass Manager** - Controls the execution of optimization passes.
3. **Optimization Passes**:
   - **Constant Folding** - Evaluates constant expressions at compile time.
   - **Dead Code Elimination** - Removes unreachable code and unused variables.
4. **C API** - Bindings to use the optimizer from C.

## Building

To build the optimizer, run:

```
make
```

This will compile the Zig library and generate the necessary C bindings.

## Testing

To run the unit tests, use:

```
make test
```

For testing just the C API, use:

```
make c-test
```

Or you can run the test directly:

```
cd test
./build.sh
```

## Using from C

To use the optimizer from C, include the header file:

```c
#include "optimizer/goo_optimizer.h"
```

Then initialize the system, create a pass manager, and run optimizations:

```c
// Initialize
if (!goo_ir_init()) {
    // Handle error
}

// Create a module
GooIRModule* module = goo_ir_create_module("my_module");

// Add functions, blocks, and instructions
// ...

// Create a pass manager with default optimization level
GooPassManager* pass_manager = goo_pass_manager_create(GOO_OPT_DEFAULT, true);

// Add optimization passes
goo_pass_manager_add_constant_folding(pass_manager);
goo_pass_manager_add_dead_code_elimination(pass_manager);

// Run optimizations
bool changed = goo_pass_manager_run(pass_manager, module);

// Clean up
goo_pass_manager_destroy(pass_manager);
goo_ir_destroy_module(module);
goo_ir_shutdown();
```

## Available Passes

### Constant Folding

Evaluates constant expressions at compile time. For example, `x = 2 + 3` would be optimized to `x = 5`.

### Dead Code Elimination (DCE)

The Dead Code Elimination pass performs two primary optimizations:

1. **Unreachable Block Elimination** - Removes basic blocks that can never be executed because they have no predecessors.

2. **Dead Instruction Elimination** - Removes instructions that compute values that are never used.

The DCE pass helps reduce code size and improve performance by removing unnecessary computations.

## License

This code is part of the Goo compiler and is licensed under the same terms as the Goo project. 