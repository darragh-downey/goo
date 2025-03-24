# Build Instructions for Goo Language Project

This document outlines how to build and test the Goo language project, with a particular focus on the safety features implemented.

## Prerequisites

- Zig 0.14.0 or newer
- C compiler that supports C23 features
- POSIX-compliant environment for thread support
- Bison and Flex (for parser and lexer generation)

## Build Commands

### Build All Components

```sh
# Full build of all components
zig build
```

### Building Specific Components

```sh
# Build the compiler only
zig build goo

# Build the SIMD benchmark
zig build simd

# Build the PGM example
zig build pgm

# Build the safe SIMD demo
zig build safe-simd
```

### Running Tests

```sh
# Run all tests
zig build test

# Run specific test suites
zig build safety-test   # Run safety-specific tests
```

## Safety Features Implementation

The safety features in this project focus on two main areas:

1. **Type Safety**: Implemented in `include/goo_type_safety.h` and `src/safety/goo_type_safety.c`
2. **Concurrency Safety**: Implemented in `include/goo_concurrency.h` and `src/safety/goo_concurrency.c`
3. **Combined Safety System**: Implemented in `include/goo_safety.h` and `src/safety/goo_safety.c`

### Type Safety

Type safety features include:
- Runtime type tracking and validation
- Buffer bounds checking
- Overflow detection
- Function parameter validation

### Concurrency Safety

Concurrency safety features include:
- Thread safety annotations
- Explicit memory ordering
- Lock-free data structures
- Read-write locks with timeouts
- Thread-local error handling

### Demo Applications

The `safe_simd_demo` application demonstrates both type safety and concurrency safety in action, applying them to vectorized SIMD operations.

## Troubleshooting

### Common Build Issues

1. **Include path issues**
   - Ensure that include paths are properly set in build scripts
   - Check for correct header inclusion in source files

2. **C23 Compatibility**
   - If your compiler does not fully support C23, you may need to disable certain features

3. **Parallel Infrastructure Errors**
   - The parallel infrastructure requires proper thread support
   - Check that pthread is available and linked correctly

## Next Steps

After building, you can run the tests to verify that the safety features are working correctly:

```sh
# Run the safe SIMD demo
./zig-out/bin/safe_simd_demo
```

To explore the implementation details, start by examining:
1. `include/goo_safety.h` - The main safety interface
2. `examples/safe_simd_demo.c` - Example usage of safety features
3. `test/safety/goo_safety_test.c` - Comprehensive tests for safety features 