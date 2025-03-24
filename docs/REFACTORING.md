# Goo Project Refactoring Plan

This document outlines the necessary refactoring steps to improve the organization, maintainability, and build process of the Goo language project.

## 1. Directory Structure Standardization

### Current Issues:
- Test files are scattered across multiple locations
- Duplicate directories (e.g., `src/codegen` and `src/compiler/codegen`)
- Inconsistent file placement patterns
- Mixed Zig and C implementation files

### Proposed Structure:
```
goo/
├── include/               # Public header files
│   ├── goo/               # Main language headers
│   ├── runtime/           # Runtime headers
│   └── tools/             # Headers for tools
├── src/                   # Source code
│   ├── compiler/          # Compiler components
│   │   ├── ast/           # Abstract Syntax Tree
│   │   ├── lexer/         # Lexical analyzer
│   │   ├── parser/        # Parser
│   │   ├── codegen/       # Code generation
│   │   └── optimizer/     # Optimizations
│   ├── runtime/           # Runtime implementation
│   │   ├── memory/        # Memory management
│   │   ├── parallel/      # Parallelism support
│   │   ├── safety/        # Safety features
│   │   └── messaging/     # Messaging system
│   ├── package/           # Package management
│   ├── debug/             # Debugging infrastructure
│   └── tools/             # Command-line tools
├── test/                  # All tests
│   ├── unit/              # Unit tests
│   ├── integration/       # Integration tests
│   └── samples/           # Sample programs for testing
├── build/                 # Build output (generated)
│   ├── bin/               # Executables
│   ├── lib/               # Libraries
│   └── obj/               # Object files
├── docs/                  # Documentation
│   ├── api/               # API documentation
│   ├── internals/         # Internal documentation
│   └── guides/            # User guides
└── examples/              # Example programs
```

## 2. Build System Consolidation

### Current Issues:
- Both Make and Zig build systems present
- Makefile doesn't properly compile all components
- Minimal integration between component Makefiles

### Recommendations:
1. **Choose a primary build system**:
   - Since there's movement toward Zig, commit to it or fully revert to Make
   - For C-centric development, enhance the Makefile
   - For eventual Zig integration, improve the build.zig

2. **Restructure the Makefile**:
   - Create a modular Makefile system with proper dependency tracking
   - Define clear targets for all major components
   - Support incremental builds properly
   - Add proper test integration

3. **Compile a Complete Binary**:
   - Update the compiler target to include all necessary objects
   - Link against all required components

## 3. Header Organization

### Current Issues:
- Headers exist in both `include/` and component directories
- No clear pattern for what should be public vs. private headers

### Recommendations:
1. **Public API Headers**:
   - Move all public API headers to `include/goo/`
   - Keep a consistent naming scheme (`goo_*.h`)
   - Document the API with clear comments

2. **Internal Headers**:
   - Keep internal headers within their component directories
   - Use a different naming pattern for internal-only headers

3. **Include Guards**:
   - Standardize include guards: `GOO_<COMPONENT>_<FILE>_H`
   - Consider using `#pragma once` as a supplement

## 4. Test Structure Improvements

### Current Issues:
- Tests scattered across multiple directories
- No consistent test framework usage

### Recommendations:
1. **Centralize Tests**:
   - Move all tests to the `test/` directory
   - Organize by type: unit, integration, and functional tests

2. **Test Framework**:
   - Standardize on a single test framework
   - Add a test discovery and running mechanism

3. **CI Integration**:
   - Ensure all tests can be run via continuous integration
   - Add test coverage tracking

## 5. Code Style and Documentation

### Recommendations:
1. **Standardize Code Style**:
   - Document the coding style in a STYLE.md file
   - Use consistent naming conventions across the codebase
   - Add linting to enforce style

2. **Documentation**:
   - Add documentation comments to all public functions
   - Ensure README files exist for all major components
   - Maintain up-to-date examples

## 6. Dependency Management

### Current Issues:
- External dependencies not clearly tracked
- No vendoring or dependency versioning

### Recommendations:
1. **Track Dependencies**:
   - Document all external dependencies
   - Consider vendoring critical dependencies
   - Add dependency version checks to the build

## Implementation Plan

1. **Phase 1: Directory Restructuring**
   - Reorganize files according to the proposed structure
   - Create missing directories
   - Update include paths in code

2. **Phase 2: Build System Improvement**
   - Enhance the primary build system
   - Ensure all components compile properly
   - Add test integration

3. **Phase 3: Header Cleanup**
   - Reorganize headers according to recommendations
   - Update include paths throughout the codebase
   - Add proper documentation

4. **Phase 4: Testing Improvements**
   - Centralize and organize tests
   - Implement test discovery and running
   - Add coverage tracking

5. **Phase 5: Documentation and Style**
   - Document the code style
   - Add missing documentation
   - Improve examples 