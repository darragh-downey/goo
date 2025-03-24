# Goo Compiler Architecture

This document outlines the architecture for completing the Goo compiler, leveraging Zig for core components where appropriate.

## Overview

The Goo compiler is structured as a pipeline with several distinct phases, integrating C and Zig components. The architecture aims to leverage Zig's memory safety and performance while building on the existing C codebase.

```
Source Code -> Lexer -> Parser -> AST -> Type Checker -> IR Generation -> LLVM Backend -> Binary Output
```

## Component Architecture

### 1. Frontend

#### Lexer (Zig)
- Upgrade the existing Zig lexer implementation (`src/compiler/frontend/lexer/zig/lexer.zig`)
- Provide a C API for integration with the rest of the compiler
- Support all Goo-specific tokens and syntax features

#### Parser (Zig + Bison)
- Convert the existing Bison parser (`src/compiler/frontend/parser/parser.y`) to use the Zig lexer
- Implement a recursive descent parser in Zig for better error handling
- Generate AST nodes using the existing C AST structure

### 2. Middle-End

#### AST Processing (C with Zig helpers)
- Use the existing C AST definition (`include/ast.h`)
- Implement AST traversal and transformation in Zig
- Provide visitor patterns for various compiler phases

#### Type Checker (Zig)
- Implement a full type checker in Zig
- Support Goo's advanced type system features (capabilities, allocators)
- Integrate with the C type representation

#### Semantic Analysis (Zig)
- Perform scope and symbol resolution
- Validate Goo-specific constructs (channels, concurrency)
- Error reporting with source locations

### 3. Backend

#### IR Generation (Zig)
- Implement a Zig-based IR that maps cleanly to LLVM
- Handle Goo-specific features with custom IR constructs
- Optimize IR before LLVM conversion

#### LLVM Integration (C++)
- Leverage LLVM 18 for code generation
- Provide Zig bindings to LLVM API
- Implement custom LLVM passes for Goo features

#### Runtime Integration (Zig)
- Link with Goo runtime components
- Generate initialization code for runtime features
- Support embedding runtime into static binaries

## Compiler Driver

The main compiler driver will be implemented in Zig, providing:
- Command-line interface
- Build system integration
- Plugin architecture for extensions
- Cross-compilation support

## Memory Management

- Zig allocators for compiler internals
- Explicit arena allocators for parse/compile phases
- Minimal allocations during compilation

## Implementation Plan

1. **Phase 1: Frontend Completion**
   - Complete Zig lexer implementation
   - Port parser to use Zig lexer
   - Implement AST generation

2. **Phase 2: Type System**
   - Implement type checker in Zig
   - Add semantic analysis
   - Support full Goo type system

3. **Phase 3: IR Generation**
   - Implement Goo IR in Zig
   - Add optimizations
   - Connect to LLVM backend

4. **Phase 4: Runtime Integration**
   - Link compiler with runtime
   - Support all Goo features (messaging, memory, etc.)
   - Add standard library

5. **Phase 5: Tooling**
   - Implement compiler driver
   - Add build system
   - Create development tools

## Zig-C Interaction

The compiler will use a carefully designed boundary between Zig and C components:

1. **Zig to C**: 
   - Expose Zig components via C ABI
   - Use explicit memory management at boundaries
   - Define clear ownership semantics

2. **C to Zig**:
   - Wrap C API calls in Zig safety wrappers
   - Validate C data before use in Zig
   - Handle errors across language boundaries 