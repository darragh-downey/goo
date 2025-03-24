# Zig Integration in the Goo Programming Language

## Overview

The Goo programming language leverages Zig for several key components of its infrastructure. This document explains how Zig is used within the project, its benefits, and how to work with the Zig-based components.

## Why Zig?

Zig provides several advantages that complement Goo's design philosophy:

1. **Memory Safety**: Zig's approach to memory management helps catch issues at compile time
2. **Performance**: Zig compiles to efficient machine code
3. **Cross-Compilation**: Simplified cross-platform support
4. **Interoperability**: Excellent C interoperability for gradual adoption
5. **Modern Build System**: Zig's build system simplifies complex build configurations

## Zig Components in Goo

### 1. Build System

The entire Goo project uses Zig's build system (`build.zig`) as its primary build tool. This provides:

- Unified build process across components
- Cross-platform compatibility
- Simplified dependency management
- Integration with C code components

### 2. Compiler Modules

Several compiler components are implemented in Zig:

- **Optimizer**: Leverages Zig for intermediate representation (IR) and optimization passes
  - Located in `src/compiler/optimizer/zig/`
  - Includes constant folding, dead code elimination
  - Maintains C bindings for integration with the rest of the compiler

- **Lexer**: Partial Zig implementation for performance-critical lexer components
  - Located in `src/compiler/frontend/lexer/zig/`
  - Complements the flex-based lexer implementation

### 3. Memory Management

Zig provides high-performance memory management components:

- **Core Allocator**: General-purpose memory allocator
  - Located in `src/runtime/memory/goo_allocator_core.zig`
  - Provides efficient memory allocation with safety features

- **Pool Allocator**: Fixed-size block allocator for high-performance object allocation
  - Located in `src/runtime/memory/goo_pool_allocator.zig`
  - Ideal for allocating many objects of the same size

- **Arena Allocator**: Region-based allocator for temporary allocations
  - Located in `src/runtime/memory/goo_arena_allocator.zig`
  - Fast allocation with batch deallocation

- **Region Allocator**: Manages memory in distinct regions
  - Located in `src/runtime/memory/goo_region_allocator.zig`
  - Allows region-based memory management

- **C Bindings**: Exposes Zig allocators to C code
  - Located in `src/runtime/memory/zig/bindings.zig`
  - Header file at `include/goo/runtime/memory/goo_zig_allocator.h`

### 4. SIMD Vectorization

Zig provides a cross-platform SIMD abstraction layer:

- **Vectorization**: High-performance SIMD operations
  - Located in `src/runtime/concurrency/zig/vectorization.zig`
  - Supports x86 (SSE2, AVX, AVX2, AVX-512) and ARM (NEON) instructions
  - Automatic detection of available SIMD capabilities
  - Scalar fallback for unsupported platforms

- **C Bindings**: Exposes Zig SIMD functionality to C code
  - Header file at `include/goo/runtime/concurrency/goo_zig_vectorization.h`
  - Seamless integration with existing C codebase

### 5. Testing Framework

Goo uses Zig's testing features for:

- **Unit Tests**: Testing individual components
  - `tests/unit/test_main.zig` for core runtime functionality
  - `src/compiler/optimizer/zig/test_main.zig` for optimizer-specific tests
  
- **Integration Tests**: Testing component interactions
  - `tests/integration/integration_main.zig` for end-to-end tests

## Zig File Structure

```
/goo
├── build.zig                               # Main build file
├── tests/
│   ├── unit/
│   │   ├── test_main.zig                   # Unit tests
│   │   └── simd_test.zig                   # SIMD-specific tests
│   └── integration/
│       └── integration_main.zig            # Integration tests
├── include/
│   └── goo/
│       ├── runtime/
│       │   ├── memory/
│       │   │   └── goo_zig_allocator.h     # C headers for Zig allocators
│       │   └── concurrency/
│       │       └── goo_zig_vectorization.h # C headers for Zig SIMD
└── src/
    ├── compiler/
    │   ├── optimizer/
    │   │   └── zig/
    │   │       ├── ir.zig                  # IR definition
    │   │       ├── pass_manager.zig        # Optimization pass infrastructure
    │   │       ├── constant_folding.zig    # Constant folding optimizations
    │   │       ├── dead_code_elimination.zig # DCE optimizations
    │   │       └── test_main.zig           # Optimizer tests
    │   └── frontend/
    │       └── lexer/
    │           └── zig/                    # Zig implementation of lexer
    └── runtime/
        ├── memory/
        │   ├── goo_allocator_core.zig      # Core allocator implementation
        │   ├── goo_pool_allocator.zig      # Pool allocator 
        │   ├── goo_arena_allocator.zig     # Arena allocator
        │   ├── goo_region_allocator.zig    # Region allocator
        │   └── zig/
        │       └── bindings.zig            # C bindings for memory allocators
        └── concurrency/
            └── zig/
                └── vectorization.zig       # SIMD vectorization
```

## Working with Zig Code in Goo

### Building Zig Components

The entire project can be built with:

```bash
zig build
```

To build only the Zig components:

```bash
zig build zig-libs
```

To build specific Zig libraries:

```bash
zig build goo_zig_memory     # Build just the memory allocator library
zig build goo_zig_simd       # Build just the SIMD library
```

### Running Zig Tests

To run all tests:

```bash
zig build test
```

To run specific test categories:

```bash
zig build test-optimizer   # Run optimizer tests
zig build integration      # Run integration tests
```

### Adding New Zig Code

When adding new Zig components:

1. Place the code in the appropriate subdirectory (`zig/` folder under the relevant component)
2. Add C bindings if needed for interoperability with other components
3. Add the component to `build.zig` in the appropriate section
4. Add relevant unit tests in the `test` section of the file or a separate test file

### Debugging Zig Code

Zig code can be debugged using:

1. Standard Zig testing tools
2. GDB/LLDB with appropriate Zig debugging symbols
3. VSCode integration with the Zig extension

## Best Practices

When working with Zig in the Goo codebase:

1. **Maintain C Interoperability**: Always provide C bindings for Zig components
2. **Follow Memory Management Conventions**: Use consistent allocation strategies
3. **Document Public APIs**: Document all public Zig functions and types
4. **Write Tests**: Include tests for all Zig functionality
5. **Keep Build System Updated**: Update `build.zig` when adding new Zig files

## Future Expansion

Areas planned for future Zig expansion:

1. **Compiler Backend**: More optimization passes in Zig
2. **JIT Compilation**: Leveraging Zig for runtime code generation
3. **Runtime Components**: More runtime libraries in Zig
4. **FFI System**: Improved foreign function interface

## Resources

- [Zig Language Documentation](https://ziglang.org/documentation/master/)
- [Zig Build System Documentation](https://ziglang.org/documentation/master/std/#A;std:build)
- [Zig C Interoperability Guide](https://ziglang.org/documentation/master/#C)

## Troubleshooting

Common issues when working with Zig components in Goo:

1. **Include Path Issues**: Ensure Zig code can find C headers
   - Solution: Use the appropriate include paths in `build.zig`

2. **C Binding Errors**: Type conversion issues between Zig and C
   - Solution: Use proper C types in Zig code as defined in `@cImport` blocks

3. **Build System Changes**: Updates to Zig may require build system changes
   - Solution: Keep Zig version requirements documented 