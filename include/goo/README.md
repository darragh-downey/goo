# Goo Public API Headers

This directory contains the public API headers for the Goo programming language.

## Overview

These headers define the stable interfaces that developers can use to:

- Write Goo programs
- Interact with the Goo runtime
- Extend the language with plugins and libraries
- Interface with the compiler

## Directory Structure

- `core/` - Core language interfaces
  - Type definitions
  - AST interfaces
  - Compiler interfaces
- `runtime/` - Runtime interfaces
  - Memory management
  - Concurrency primitives
  - I/O operations
  - Error handling

## Usage Guidelines

### Including Headers

In Goo source files:

```goo
import goo.runtime.memory
```

In C source files:

```c
#include "goo/core/types.h"
#include "goo/runtime/memory.h"
```

### API Stability

Headers in this directory adhere to semantic versioning:

- Major version changes may break API compatibility
- Minor version changes add functionality without breaking existing APIs
- Patch version changes fix bugs without API changes

## Header Development Guidelines

When working on public API headers:

1. Document all functions, types, and constants thoroughly
2. Use opaque types for implementation hiding when appropriate
3. Maintain backward compatibility within the same major version
4. Follow the [project coding style](../../CODING_STYLE.md) for header organization
5. Mark experimental APIs with appropriate macros
