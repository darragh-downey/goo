# Goo Programming Language Coding Style Guide

This document outlines the coding style and conventions for the Goo programming language project. Following these guidelines ensures consistency across the codebase and makes collaboration easier.

## Table of Contents

1. [Directory Structure](#directory-structure)
2. [File Organization](#file-organization)
3. [Naming Conventions](#naming-conventions)
4. [Formatting Guidelines](#formatting-guidelines)
5. [Documentation](#documentation)
6. [Header Files](#header-files)
7. [Error Handling](#error-handling)
8. [Memory Management](#memory-management)
9. [Testing](#testing)

## Directory Structure

Follow the canonical directory structure:

```
goo/
├── src/
│   ├── compiler/            # Compiler implementation
│   │   ├── frontend/        # Frontend components
│   │   │   ├── lexer/       # Lexical analysis
│   │   │   └── parser/      # Syntax analysis
│   │   ├── ast/             # Abstract syntax tree
│   │   ├── analysis/        # Semantic analysis
│   │   ├── type/            # Type system
│   │   ├── backend/         # Code generation
│   │   └── optimizer/       # Optimizations
│   ├── runtime/             # Runtime support
│   │   ├── memory/          # Memory management
│   │   ├── concurrency/     # Concurrency primitives
│   │   ├── messaging/       # Channel implementation
│   │   └── safety/          # Safety features
│   ├── common/              # Common utilities
│   └── tools/               # Development tools
├── include/                 # Public headers
│   └── goo/                 # Namespaced headers
├── tests/                   # Test suite
│   ├── unit/                # Unit tests
│   ├── integration/         # Integration tests
│   └── benchmark/           # Performance tests
└── docs/                    # Documentation
```

Each component should be self-contained with minimal dependencies on other components.

## File Organization

### General Structure

Files should be structured in the following order:

1. License/copyright header
2. File description comment
3. Include directives (grouped and ordered)
4. Macro definitions
5. Type declarations
6. Function prototypes
7. Global variables
8. Static functions
9. Public functions
10. Main function (if applicable)

### Include Order

1. Standard library headers
2. External library headers
3. Project public headers (from `include/`)
4. Project internal headers (from `src/`)

Example:
```c
/* Standard libraries */
#include <stdio.h>
#include <stdlib.h>

/* External libraries */
#include <zlib.h>

/* Goo public headers */
#include "goo/compiler.h"
#include "goo/runtime/memory.h"

/* Internal headers */
#include "compiler/frontend/lexer/lexer_internal.h"
```

## Naming Conventions

### Files

- C source files: lowercase with underscores (e.g., `memory_allocator.c`)
- Zig source files: camelCase (e.g., `memoryAllocator.zig`)
- Header files: match implementation file name (e.g., `memory_allocator.h`)
- Test files: append `_test` (e.g., `memory_allocator_test.c`)

### Functions

- Public API functions: `goo_component_action()` (e.g., `goo_memory_allocate()`)
- Internal functions: `component_action()` (e.g., `memory_allocate()`)
- Static functions: `static_action()` (e.g., `static_allocate()`)
- Zig functions: `camelCase()`

### Types

- Structures: `GooComponentName` (e.g., `GooMemoryAllocator`)
- Enumerations: `GooComponentCategory` (e.g., `GooMemoryType`)
- Typedefs: Match the type they define (e.g., `typedef struct GooLexer GooLexer;`)
- Opaque handles: `GooComponentHandle` (e.g., `GooLexerHandle`)

### Variables

- Global variables: `g_descriptive_name` (e.g., `g_memory_pool`)
- Static variables: `s_descriptive_name` (e.g., `s_token_count`)
- Local variables: lowercase with underscores (e.g., `current_token`)
- Parameters: lowercase with underscores (e.g., `buffer_size`)
- Constants: ALL_CAPS_WITH_UNDERSCORES (e.g., `MAX_TOKEN_LENGTH`)

### Macros and Defines

- Macros: ALL_CAPS_WITH_UNDERSCORES (e.g., `GOO_MEMORY_ALIGN(x)`)
- Include guards: `GOO_COMPONENT_FILENAME_H` (e.g., `GOO_LEXER_TOKEN_H`)

## Formatting Guidelines

### Indentation and Spacing

- Use 4 spaces for indentation (not tabs)
- Place opening braces on the same line for functions and control structures
- Place closing braces on their own line
- One statement per line
- One variable declaration per line
- Add a space after keywords (`if`, `for`, `while`, etc.)
- Add a space around operators (`+`, `-`, `*`, `/`, `=`, etc.)
- No space between function name and opening parenthesis
- Add a space after commas in argument lists

Example:
```c
void goo_lexer_tokenize(GooLexer *lexer, const char *source, size_t length) {
    int token_count = 0;
    bool in_comment = false;
    
    for (size_t i = 0; i < length; i++) {
        if (source[i] == '/' && source[i + 1] == '*') {
            in_comment = true;
            i++;
            continue;
        }
        
        if (in_comment) {
            if (source[i] == '*' && source[i + 1] == '/') {
                in_comment = false;
                i++;
            }
            continue;
        }
        
        // Process tokens
        token_count++;
    }
}
```

### Line Length

- Aim for a maximum line length of 100 characters
- Split long lines logically, with the continuation indented
- For function declarations that exceed line length, align parameters:

```c
GooResult goo_memory_allocate_aligned(
    GooMemoryAllocator *allocator,
    size_t size,
    size_t alignment,
    void **out_ptr
) {
    // Implementation
}
```

## Documentation

### File Headers

Each file should begin with a descriptive header:

```c
/**
 * @file memory_allocator.c
 * @brief Implementation of the Goo memory allocator
 * @details Provides memory allocation, deallocation, and tracking
 *          with customizable allocation strategies.
 */
```

### Function Documentation

Document all public API functions using the following format:

```c
/**
 * @brief Allocates memory using the provided allocator
 * 
 * @param allocator The memory allocator to use
 * @param size The number of bytes to allocate
 * @param out_ptr Pointer to store the allocated memory address
 * 
 * @return GooResult GOO_OK on success, error code otherwise
 * 
 * @note The allocated memory must be freed with goo_memory_free()
 * 
 * @example
 * void *memory = NULL;
 * GooResult result = goo_memory_allocate(allocator, 1024, &memory);
 * if (result == GOO_OK) {
 *     // Use memory
 *     goo_memory_free(allocator, memory);
 * }
 */
GooResult goo_memory_allocate(GooMemoryAllocator *allocator, size_t size, void **out_ptr);
```

### Comments

- Use `//` for single-line comments
- Use `/* */` for multi-line comments
- Include a space after comment markers
- Write comments in complete sentences with proper punctuation
- Comment complex logic, but avoid obvious descriptions
- Use TODO, FIXME, NOTE prefixes for special comments:

```c
// TODO: Implement memory pooling for small allocations
// FIXME: This has a memory leak when an error occurs
// NOTE: This algorithm has O(n²) complexity
```

## Header Files

### Include Guards

Use include guards in all header files:

```c
#ifndef GOO_MEMORY_ALLOCATOR_H
#define GOO_MEMORY_ALLOCATOR_H

// Header content goes here

#ifdef __cplusplus
extern "C" {
#endif

// C API declarations

#ifdef __cplusplus
}
#endif

#endif // GOO_MEMORY_ALLOCATOR_H
```

### Header Organization

1. Public headers (`include/goo/*.h`):
   - Minimal dependencies
   - Stable API
   - Comprehensive documentation
   - Clear versioning

2. Internal headers (`src/*/include/*.h`):
   - Component-specific types and functions
   - Properly documented, but marked as internal
   - Clearly separate from public API

## Error Handling

### Error Codes

- Use enum for error codes:
```c
typedef enum {
    GOO_OK = 0,
    GOO_ERROR_INVALID_ARGUMENT,
    GOO_ERROR_OUT_OF_MEMORY,
    GOO_ERROR_IO,
    // etc.
} GooResult;
```

### Error Reporting

- Functions should return error codes for failure
- Use output parameters for returning values
- Provide detailed error messages when appropriate
- Check all error cases:

```c
GooResult goo_lexer_create(const char *source, GooLexer **out_lexer) {
    if (source == NULL || out_lexer == NULL) {
        return GOO_ERROR_INVALID_ARGUMENT;
    }
    
    GooLexer *lexer = goo_memory_allocate(sizeof(GooLexer));
    if (lexer == NULL) {
        return GOO_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize lexer
    
    *out_lexer = lexer;
    return GOO_OK;
}
```

## Memory Management

### Allocation

- Be explicit about memory ownership
- Consistently use allocation/deallocation pairs
- Document memory management expectations
- Handle out-of-memory conditions

### Resource Cleanup

- Always clean up resources in reverse order of acquisition
- Use cleanup labels for error paths:

```c
GooResult function_with_resources() {
    Resource1 *r1 = acquire_resource1();
    if (r1 == NULL) {
        return GOO_ERROR_RESOURCE_ACQUISITION;
    }
    
    Resource2 *r2 = acquire_resource2();
    if (r2 == NULL) {
        release_resource1(r1);
        return GOO_ERROR_RESOURCE_ACQUISITION;
    }
    
    // Use resources
    
    // Clean up in reverse order
    release_resource2(r2);
    release_resource1(r1);
    return GOO_OK;
    
error:
    if (r2 != NULL) release_resource2(r2);
    if (r1 != NULL) release_resource1(r1);
    return error_code;
}
```

## Testing

### Test Organization

- Place unit tests in `tests/unit/`
- One test file per implementation file
- Name test files to match implementation: `memory_allocator_test.c`

### Test Structure

- Group tests by functionality
- Clear test function names: `test_memory_allocate_basic()`
- Setup and teardown for each test group
- Descriptive assertions

Example:
```c
void test_lexer_tokenize_empty_string() {
    // Setup
    GooLexer *lexer = NULL;
    goo_lexer_create(&lexer);
    
    // Test
    GooResult result = goo_lexer_tokenize(lexer, "", 0);
    
    // Verify
    ASSERT_EQUAL(GOO_OK, result);
    ASSERT_EQUAL(0, lexer->token_count);
    
    // Teardown
    goo_lexer_destroy(lexer);
}
``` 