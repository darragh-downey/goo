# Goo Code Organization Implementation Plan

## Overview

This document outlines the specific implementation steps to reorganize the Goo programming language project based on the existing CODE_ORGANIZATION_PLAN.md and REFACTORING.md documents. The goal is to improve maintainability, readability, and developer experience.

## Implementation Phases

### Phase 1: Directory Structure Setup (Immediate)

1. **Create and validate new directory structure**
   ```bash
   ./restructure.sh
   ```
   
2. **Fix any issues with the restructure script**
   - Ensure all directories are properly created
   - Verify file movements maintain proper relationships

3. **Consolidate test directories**
   - Move tests from `src/tests`, `test/`, and `tests/` into a single `tests/` directory
   - Organize by test type (unit, integration, functional)

### Phase 2: Code Migration (1-2 days)

1. **Move compiler components to canonical locations**
   - Lexer: `src/compiler/frontend/lexer/`
   - Parser: `src/compiler/frontend/parser/`
   - AST: `src/compiler/ast/`
   - Type system: `src/compiler/type/`
   - Code generation: `src/compiler/backend/`

2. **Reorganize runtime components**
   - Memory management: `src/runtime/memory/`
   - Concurrency: `src/runtime/concurrency/`
   - Safety features: `src/runtime/safety/`
   - Messaging: `src/runtime/messaging/`

3. **Fix header organization**
   - Public API headers → `include/goo/`
   - Internal headers → component-specific include directories
   - Standardize include guards

### Phase 3: Build System Unification (1 day)

1. **Choose primary build system**
   - Complete transition to Zig build system OR
   - Fully modernize Makefile-based system

2. **Update build scripts**
   - Update paths in Makefile/build.zig
   - Add proper dependency tracking
   - Create targets for all components

3. **Fix CI integration**
   - Update GitHub workflows
   - Ensure all tests are run

### Phase 4: Documentation and Standardization (1-2 days)

1. **Update documentation**
   - Update READMEs for all main components
   - Create comprehensive API documentation
   - Document architecture and design decisions

2. **Standardize naming conventions**
   - Files: lowercase with underscores for C, camelCase for Zig
   - Functions: `goo_component_action()` for public API
   - Types: `GooComponentName` format
   - Constants: `GOO_COMPONENT_CONSTANT`

3. **Implement code style checks**
   - Add linting rules
   - Document in STYLE.md

## Detailed Tasks

### Directory Structure Setup

```bash
# Create the canonical structure
mkdir -p src/compiler/frontend/lexer
mkdir -p src/compiler/frontend/parser
mkdir -p src/compiler/ast
mkdir -p src/compiler/type
mkdir -p src/compiler/backend
mkdir -p src/common
mkdir -p src/runtime/memory
mkdir -p src/runtime/concurrency
mkdir -p src/runtime/messaging
mkdir -p src/runtime/safety
mkdir -p include/goo
mkdir -p tests/unit
mkdir -p tests/integration
mkdir -p tests/system
mkdir -p tests/benchmark
```

### Specific File Migrations

1. Move lexer files:
```bash
mv src/lexer/* src/compiler/frontend/lexer/
```

2. Move parser files:
```bash
mv src/parser/* src/compiler/frontend/parser/
```

3. Move AST files:
```bash
mv src/ast/* src/compiler/ast/
```

4. Move type system files:
```bash
mv src/type/* src/compiler/type/
```

5. Move codegen files:
```bash
mv src/codegen/* src/compiler/backend/
```

### Header Organization

1. Move all public headers:
```bash
# Find headers and move them to the appropriate location
find src -name "*.h" -type f -exec grep -l "public api" {} \; | xargs -I {} cp {} include/goo/
```

2. Update include directives:
```bash
# Create a script to update include paths
cat > update_includes.sh << 'EOF'
#!/bin/bash
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "lexer/|#include "compiler/frontend/lexer/|g'
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "parser/|#include "compiler/frontend/parser/|g'
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "ast/|#include "compiler/ast/|g'
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "type/|#include "compiler/type/|g'
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|#include "codegen/|#include "compiler/backend/|g'
EOF
chmod +x update_includes.sh
```

## Success Criteria

1. All code is in the canonical directory structure as outlined in CODE_ORGANIZATION_PLAN.md
2. Build system successfully compiles the entire project
3. All tests pass in the new structure
4. Documentation is updated to reflect the new organization
5. CI integration is working properly

## Post-Implementation Review

After implementation, conduct a review to ensure:

1. All code migration is complete
2. No regressions in functionality
3. Build system works efficiently
4. Documentation is accurate
5. The development experience is improved 