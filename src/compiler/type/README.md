# Goo Type System

This directory contains the implementation of the advanced type system for the Goo programming language.

## Components

### Core Type System

- `goo_type_system.h`: Main header file defining the type system structures and interfaces.
- `goo_type_system.c`: Implementation of core type system functionality.

### Trait System

- `goo_type_traits.c`: Implementation of the trait and constraint system.

### Type Checking

- `goo_type_checker.c`: Implementation of the type checking functionality.

### Utilities

- `goo_type_utils.c`: Utility functions for working with types.

### Tests

- `type_system_test.c`: Test program for the type system functionality.

## Features

The Goo type system includes several advanced features:

1. **Type Hierarchy**:
   - Basic types (void, unit, bool, int, float, char, string)
   - Composite types (arrays, slices, tuples, structs, enums, unions)
   - Function types (functions, closures)
   - Reference types (immutable refs, mutable refs, owned types)
   - Concurrency types (channels, goroutines)
   - Generic types (type variables, type parameters)
   - Special types (trait objects, error, never, unknown, any)

2. **Type Safety**:
   - Static type checking
   - Capability checking
   - Thread safety analysis
   - Memory region tracking

3. **Type Inference**:
   - Type variable resolution
   - Constraint-based unification
   - Subtyping relationships

4. **Trait System**:
   - Trait definitions with required methods
   - Trait implementations for concrete types
   - Trait objects (dynamic dispatch)
   - Trait constraints on type variables

5. **Memory Safety**:
   - Lifetime annotations
   - Memory region analysis
   - Borrow checking

## Relationship to Other Components

The type system integrates with other compiler components:

- **Parser/AST**: Consumes AST nodes for type checking
- **Diagnostics**: Reports type errors through the diagnostics system
- **Code Generation**: Provides type information for the code generator
- **Optimizer**: Provides semantic information for optimizations

## Usage

The type system can be used both for static type checking during compilation and for runtime type information.

### Basic Types

```c
// Create basic types
GooType* int32_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
GooType* float64_type = goo_type_system_create_float_type(ctx, GOO_FLOAT_64);
GooType* bool_type = goo_type_system_create_bool_type(ctx);
```

### Composite Types

```c
// Create an array type
GooType* array_type = goo_type_system_create_array_type(ctx, element_type, 10);

// Create a struct type
const char* field_names[2] = {"x", "y"};
GooType* field_types[2] = {int_type, int_type};
GooType* point_type = goo_type_system_create_struct_type(ctx, "Point", field_names, field_types, 2);
```

### Type Checking

```c
// Type check an expression
GooType* expr_type = goo_type_system_check_expr(ctx, expr_node);

// Type check a module
bool success = goo_type_system_check_module(ctx, module_node);
```

### Trait System

```c
// Create a trait
GooTrait* trait = goo_type_system_create_trait(ctx, "Hashable", method_names, method_types, 2);

// Implement a trait
GooTypeImpl* impl = goo_type_system_create_impl(ctx, type, trait, NULL, 0);

// Check if a type implements a trait
bool implements = goo_type_system_type_implements_trait(ctx, type, trait, NULL);
```

## Build and Test

The type system can be built and tested using the Zig build system:

```bash
# Build the type system library
zig build

# Run type system tests
zig build run-type-test
```
