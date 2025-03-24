# Goo Programming Language Style Guide

This document outlines the coding style and conventions for the Goo programming language project.

## C Code Style Guidelines

### Naming Conventions

1. **Files**:
   - Use lowercase with underscores: `goo_package.c`, `ast_builder.h`
   - Implementation files: `.c` extension
   - Header files: `.h` extension
   - Zig implementation files: `.zig` extension

2. **Functions**:
   - Use snake_case for all functions
   - Prefix with component name: `goo_package_create()`, `ast_node_free()`
   - Use descriptive verb-noun combinations
   - Public API: always include component prefix
   - Private functions in `.c` files: optionally prefixed with `_` or `static`

3. **Types**:
   - Structs, unions, enums: PascalCase
   - Typedef to remove `struct` keyword: `typedef struct GooPackage GooPackage;`
   - Enum values: SCREAMING_SNAKE_CASE with component prefix
   - Example: `enum GooTokenType { GOO_TOKEN_IDENTIFIER, GOO_TOKEN_NUMBER }`

4. **Variables**:
   - Local variables: snake_case
   - Global variables: g_snake_case
   - Constants: SCREAMING_SNAKE_CASE
   - Member variables: snake_case

5. **Macros**:
   - Always SCREAMING_SNAKE_CASE
   - Prefer inline functions when possible

### Layout and Formatting

1. **Indentation**: 
   - 4 spaces (no tabs)
   - Use spaces consistently

2. **Line Length**:
   - Max 100 characters
   - Exception: long string literals or messages

3. **Braces**:
   - For functions: new line for opening brace
   ```c
   void function_name(void)
   {
       // Code
   }
   ```
   - For control structures: same line for opening brace
   ```c
   if (condition) {
       // Code
   } else {
       // Code
   }
   ```

4. **Function Declarations**:
   - Return type and function name on same line when possible
   - For long declarations, parameters on separate lines with aligned indentation
   ```c
   void goo_package_manager_create(
       const char* name,
       const char* version,
       bool verbose,
       GooPackageOptions* options
   ) {
       // Implementation
   }
   ```

5. **Comments**:
   - // for single-line comments
   - /* */ for multi-line comments
   - Commented code should be removed, not commented out

### Headers

1. **Include Guards**:
   - Use `#ifndef/#define/#endif` with consistent naming:
   ```c
   #ifndef GOO_PACKAGE_H
   #define GOO_PACKAGE_H
   
   // Header content
   
   #endif /* GOO_PACKAGE_H */
   ```
   - Alternatively, `#pragma once` can be used in addition to classic guards

2. **Include Order**:
   - Standard library headers
   - System headers
   - Goo public headers (alphabetical)
   - Goo internal headers (alphabetical)
   - Local headers
   ```c
   #include <stdio.h>
   #include <stdlib.h>
   
   #include <pthread.h>
   
   #include "goo.h"
   #include "goo_runtime.h"
   
   #include "internal/ast_helpers.h"
   ```

3. **Header Documentation**:
   - Each header should start with a comment block describing its purpose
   - Public API functions should have documentation comments

### Memory Management

1. **Allocation**:
   - Use consistent patterns for resource allocation/deallocation
   - Functions that allocate memory should clearly document ownership transfer
   - Pair each `malloc` with corresponding `free` in cleanup functions

2. **Error Handling**:
   - Check return values from memory allocation functions
   - Use consistent pattern for error propagation
   - Clean up resources on error paths

### Documentation

1. **Function Documentation**:
   - Document all public API functions with a consistent style:
   ```c
   /**
    * Create a new package with the given name and version.
    *
    * @param name The package name
    * @param version The package version (semver format)
    * @return A newly allocated package or NULL on error
    */
   GooPackage* goo_package_create(const char* name, const char* version);
   ```

2. **Struct Documentation**:
   - Document struct members with comments
   ```c
   typedef struct {
       char* name;         /* Package name */
       char* version;      /* Package version in semver format */
       char* description;  /* Optional package description */
       bool is_published;  /* Whether the package has been published */
   } GooPackage;
   ```

## Goo Language Code Style

As Goo evolves, these guidelines will reflect the recommended style for Goo language programs.

1. **Indentation**: 4 spaces

2. **Naming**:
   - Variables and functions: camelCase
   - Types: PascalCase
   - Constants: SCREAMING_SNAKE_CASE

3. **Braces**: Keep opening braces on the same line
   ```goo
   func foo() {
       if (condition) {
           // Code
       }
   }
   ```

4. **Comments**:
   - Line comments: `// Comment`
   - Doc comments: `/// Documentation comment`
   - Block comments: `/* ... */`

## Code Organization

1. **File Organization**:
   - One primary type/concept per file
   - Match filename to primary type: `goo_package.h` for `GooPackage`
   - Group related functions together within a file

2. **Public vs Private**:
   - Clearly separate public API from private implementation details
   - Use internal headers or static functions for implementation details

## Commit and Pull Request Guidelines

1. **Commit Messages**:
   - Use the imperative mood: "Add feature" not "Added feature"
   - Start with component name: "compiler: Add type checking for arrays"
   - Keep first line under 72 characters, followed by blank line and details

2. **Pull Requests**:
   - Reference issue numbers in PR descriptions
   - Keep PRs focused on single concerns
   - Include relevant tests

## Enforcement

- Consider using clang-format with a configuration file matching these guidelines
- Set up CI checks to enforce style guidelines automatically

This style guide is a living document and may be updated as the project evolves. 