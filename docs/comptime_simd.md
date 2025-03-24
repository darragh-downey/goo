# Compile-time SIMD in Goo

## Overview

Goo's compile-time SIMD feature provides a powerful, safe, and portable way to leverage SIMD (Single Instruction, Multiple Data) instructions for high-performance parallel computation. By using `comptime` blocks, Goo can perform static analysis of SIMD operations, ensuring safety while eliminating runtime overhead.

## Key Benefits

1. **Zero Runtime Overhead**: All SIMD type checking, alignment verification, and feature detection is performed at compile-time.
2. **Safety Guarantees**: The compiler can statically verify memory access patterns, preventing out-of-bounds access.
3. **Portability**: Write once, run anywhere with automatic fallback to appropriate SIMD instructions for the target hardware.
4. **Code Clarity**: SIMD operations look like normal Goo code, no need for intrinsics or inline assembly.

## Syntax and Usage

### Basic SIMD Configuration

```go
comptime simd {
    // Define SIMD types
    vector Float4 float32 aligned(16) {
        safe;
        arch = auto;
    };
    
    // Define operations
    add_f4 = add vector = Float4;
    mul_f4 = mul vector = Float4 fused;
    
    // Global configuration options
    arch = "avx2";
    auto_detect = true;
    allow_fallback = true;
}
```

### Using SIMD Types and Operations

```go
func process_array(data []float32) {
    for i := 0; i < len(data); i += 4 {
        // Load data into a SIMD vector
        v := Float4.load(data[i:])
        
        // Perform operations
        v = add_f4(v, v)
        v = mul_f4(v, v)
        
        // Store result back
        v.store(data[i:])
    }
}
```

### Using SIMD in Unsafe Blocks

```go
unsafe func low_level_processing(ptr *float32, count int) {
    comptime {
        // Check that this is safe
        if count % 4 != 0 {
            @compile_error("Count must be a multiple of 4");
        }
    }
    
    for i := 0; i < count; i += 4 {
        v := Float4.load_aligned(ptr + i)
        // Process data
        v.store_aligned(ptr + i)
    }
}
```

## Compile-time Safety Guarantees

The Goo compiler performs the following safety checks at compile-time:

1. **Alignment Verification**: Ensures that aligned loads and stores meet the required alignment.
2. **Bounds Checking**: Verifies that vector operations don't access memory beyond array bounds.
3. **Hardware Compatibility**: Checks that the target hardware supports the requested SIMD instructions.
4. **Type Compatibility**: Ensures that operands have compatible vector types.

## Available Vector Operations

| Operation | Description | Available Types |
|-----------|-------------|----------------|
| `add`     | Vector addition | All numeric types |
| `sub`     | Vector subtraction | All numeric types |
| `mul`     | Vector multiplication | All numeric types |
| `div`     | Vector division | All numeric types |
| `fma`     | Fused multiply-add | Float types (with `fused` option) |
| `sqrt`    | Square root | Float types |
| `min`     | Minimum values | All numeric types |
| `max`     | Maximum values | All numeric types |
| `and`     | Bitwise AND | Integer types |
| `or`      | Bitwise OR | Integer types |
| `xor`     | Bitwise XOR | Integer types |
| `not`     | Bitwise NOT | Integer types |

## Advanced Features

### Masked Operations

```go
comptime simd {
    // Define a masked operation
    mask_add_f4 = add vector = Float4 mask;
}

func conditional_add(a []float32, b []float32, mask []bool) {
    for i := 0; i < len(a); i += 4 {
        va := Float4.load(a[i:])
        vb := Float4.load(b[i:])
        vmask := Bool4.load(mask[i:])
        
        // Only add elements where mask is true
        vc := mask_add_f4(va, vb, vmask)
        
        vc.store(a[i:])
    }
}
```

### Fused Operations

```go
comptime simd {
    // Define a fused multiply-add operation
    fma_f4 = fma vector = Float4 fused;
}

func calculate(a []float32, b []float32, c []float32) {
    for i := 0; i < len(a); i += 4 {
        va := Float4.load(a[i:])
        vb := Float4.load(b[i:])
        vc := Float4.load(c[i:])
        
        // Compute a = (a * b) + c in a single instruction
        va = fma_f4(va, vb, vc)
        
        va.store(a[i:])
    }
}
```

## Performance Considerations

1. **Alignment**: Aligned memory access is much faster. Use `aligned(N)` types when possible.
2. **Stride**: Process data in contiguous blocks for optimal cache usage.
3. **Vector Width**: Choose appropriate vector width for your target hardware.
4. **Fallbacks**: Consider whether scalar fallbacks are acceptable for compatibility (use `allow_fallback`).

## Comparison with Unsafe SIMD

| Feature | Comptime SIMD | Unsafe SIMD Intrinsics |
|---------|--------------|------------------------|
| Safety | Static verification | No safety guarantees |
| Portability | Automatic for target | Platform-specific |
| Overhead | Zero at runtime | Zero, but manual management |
| Readability | High-level abstractions | Low-level intrinsics |
| Error Detection | Compile-time | Runtime crashes |
| Hardware Support | Feature-detection | Manual detection required | 