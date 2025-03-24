# Goo Language Code Organization Plan

## Current Structure Analysis

The Goo programming language project has a good foundation but could benefit from improved organization to enhance maintainability, scalability, and developer experience.

### Current Directory Structure
```
/goo
├── benchmark/          # Benchmarking code
├── bin/                # Compiled binaries
├── build/              # Build artifacts
├── docs/               # Documentation
├── examples/           # Example code
├── include/            # Public headers
├── src/                # Source code
│   ├── ast/            # Abstract syntax tree
│   ├── codegen/        # Code generation
│   ├── compiler/       # Compiler core
│   ├── debug/          # Debugging utilities
│   ├── generated/      # Generated code
│   ├── include/        # Internal headers
│   ├── lexer/          # Lexer implementations
│   ├── memory/         # Memory management
│   ├── messaging/      # Channel/messaging
│   ├── package/        # Package management
│   ├── parallel/       # Parallelism support
│   ├── parser/         # Parser implementation
│   ├── runtime/        # Runtime support
│   ├── safety/         # Safety features
│   ├── testing/        # Testing utilities
│   ├── tests/          # Internal tests
│   ├── tools/          # Development tools
│   └── type/           # Type system
├── templates/          # Templates
├── test/               # Test suites
├── tests/              # Additional tests
└── zig/                # Zig-specific code
```

### Identified Issues

1. **Inconsistent Naming**: Different naming conventions across components
2. **Duplicate Test Directories**: Multiple test-related directories with unclear boundaries
3. **Header Organization**: Scattered header files without clear public/private separation
4. **Build System Complexity**: Multiple build files with overlapping functionality
5. **Component Coupling**: Tight coupling between components
6. **Documentation Fragmentation**: Documentation spread across various files
7. **Inconsistent Interface Design**: Varying API styles across components

## Proposed Structure

### Directory Structure Reorganization

```
/goo
├── .github/            # GitHub specific files (CI workflows, templates)
├── build/              # Build system
│   ├── cmake/          # CMake modules
│   ├── zig/            # Zig build configuration
│   └── scripts/        # Build scripts
├── docs/               # Documentation
│   ├── api/            # API documentation
│   ├── design/         # Design documents
│   ├── examples/       # Documented examples
│   └── internals/      # Implementation details
├── include/            # Public headers (stable API)
│   └── goo/            # Namespaced headers
│       ├── core/       # Core language features
│       ├── runtime/    # Runtime features
│       └── ...
├── src/                # Implementation
│   ├── common/         # Common utilities
│   ├── compiler/       # Compiler implementation
│   │   ├── frontend/   # Lexer and parser
│   │   ├── ast/        # Abstract syntax tree
│   │   ├── analysis/   # Semantic analysis
│   │   ├── type/       # Type system
│   │   └── backend/    # Code generation
│   ├── runtime/        # Runtime library
│   │   ├── memory/     # Memory management
│   │   ├── concurrency/# Concurrency primitives
│   │   ├── messaging/  # Channel implementation
│   │   └── io/         # I/O operations
│   └── tools/          # Compiler tools
│       ├── formatter/  # Code formatter
│       ├── linter/     # Code linter
│       └── analyzer/   # Static analyzer
├── tests/              # Unified test directory
│   ├── unit/           # Unit tests
│   ├── integration/    # Integration tests
│   ├── system/         # System tests
│   └── benchmark/      # Performance tests
├── examples/           # Example programs
│   ├── beginner/       # Basic examples
│   ├── intermediate/   # More complex examples
│   └── advanced/       # Advanced usage examples
└── third_party/        # Third-party dependencies (if any)
```

## Implementation Plan

### Phase 1: Foundation and Structure

1. **Create Directory Structure**
   - Set up the new directory tree without moving files
   - Establish gitignore patterns for build artifacts

2. **Standardize Build System**
   - Consolidate build scripts
   - Create consistent build targets
   - Ensure cross-platform compatibility

3. **Define Header Organization**
   - Establish clear header file guidelines
   - Separate public and internal interfaces
   - Create template headers for consistency

### Phase 2: Component Migration

4. **Migrate Compiler Frontend**
   - Move lexer code to `src/compiler/frontend/lexer`
   - Move parser code to `src/compiler/frontend/parser`
   - Ensure interfaces remain compatible
   - Update include paths

5. **Reorganize AST and Type System**
   - Move AST implementation to `src/compiler/ast`
   - Restructure type system in `src/compiler/type`
   - Update dependencies between components

6. **Consolidate Runtime Components**
   - Move memory management to `src/runtime/memory`
   - Restructure concurrency features in `src/runtime/concurrency`
   - Update messaging system in `src/runtime/messaging`

### Phase 3: Testing and Documentation

7. **Unify Test Framework**
   - Consolidate test directories
   - Establish consistent testing patterns
   - Ensure tests run in the new structure

8. **Improve Documentation**
   - Update README files for each component
   - Create comprehensive API documentation
   - Document architecture and design decisions

### Phase 4: Tooling and Integration

9. **Enhance Developer Tools**
   - Reorganize compiler tools
   - Improve build integration
   - Add code quality checks

10. **Continuous Integration**
    - Update CI workflows
    - Add automated testing for each component
    - Implement code coverage reporting

## Coding Standards

### Naming Conventions

1. **Files and Directories**
   - Use lowercase with underscores for C files (`memory_allocator.c`)
   - Use camelCase for Zig files (`memoryAllocator.zig`)
   - Header files match their implementation files

2. **Functions**
   - Public API: `goo_component_action()` (e.g., `goo_memory_allocate()`)
   - Internal functions: `component_action()` (e.g., `memory_allocate()`)
   - Static functions: `static_action()` (e.g., `static_allocate()`)

3. **Types**
   - Structs/Classes: `GooComponentName` (e.g., `GooMemoryAllocator`)
   - Enums: `GooComponentCategory` (e.g., `GooAllocatorType`)
   - Typedefs follow the same pattern

4. **Constants and Macros**
   - Constants: `GOO_COMPONENT_CONSTANT` (e.g., `GOO_MEMORY_DEFAULT_SIZE`)
   - Macros: `GOO_COMPONENT_ACTION` (e.g., `GOO_MEMORY_ALIGN`)

### Header Organization

1. **Public Headers** (`include/goo/...`)
   - Minimal dependencies
   - Comprehensive documentation
   - Stable API contracts
   - Clear versioning

2. **Internal Headers** (`src/.../include/...`)
   - Implementation details
   - Component-specific types
   - Limited visibility

3. **Header Structure**
   ```c
   /**
    * @file component_name.h
    * @brief Brief description
    * @details Detailed description
    */
   #ifndef GOO_COMPONENT_NAME_H
   #define GOO_COMPONENT_NAME_H

   #include <standard_headers>
   #include "goo/other_components.h"

   #ifdef __cplusplus
   extern "C" {
   #endif

   // Types, constants, function declarations

   #ifdef __cplusplus
   }
   #endif

   #endif // GOO_COMPONENT_NAME_H
   ```

## Interface Design Principles

1. **Explicit Initialization/Cleanup**
   - Functions follow `goo_component_init()` / `goo_component_free()` pattern
   - Resources are explicitly managed
   - Ownership is clearly documented

2. **Error Handling**
   - Consistent error reporting
   - Functions return error codes or use out parameters
   - Detailed error messages available

3. **Memory Management**
   - Explicit allocator parameters
   - Consistent ownership transfer
   - Clear documentation of allocation responsibilities

4. **Thread Safety**
   - Explicitly documented thread safety guarantees
   - Consistent locking patterns
   - Thread-local resources clearly marked

## Documentation Standards

1. **API Documentation**
   - Every public function documented
   - Parameters and return values explained
   - Examples provided for complex functions

2. **Architecture Documentation**
   - Component interactions described
   - Design decisions explained
   - Diagrams for complex subsystems

3. **Code Examples**
   - Each major feature has examples
   - Examples are tested as part of CI
   - Progressive complexity in examples

## Migration Strategy

To minimize disruption, the migration will be gradual:

1. **Start with One Component**: Begin with lexer/parser
2. **Maintain Compatibility**: Keep interfaces working during transition
3. **Update Tests First**: Ensure tests pass before and after changes
4. **Incremental PRs**: Small, focused pull requests
5. **Regular Integration**: Merge changes frequently to main branch

This plan provides a comprehensive roadmap for improving the organization of the Goo language codebase while maintaining functionality and developer productivity. 