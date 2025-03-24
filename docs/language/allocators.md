# Memory Management in Goo

Goo provides a sophisticated memory management system that combines safety-by-default with Zig-like allocator flexibility. This approach gives developers both safety and control, allowing for performance optimization when needed.

## Safety by Default

Goo is designed with safety as a primary feature. All memory accesses are bounds-checked by default, protecting against buffer overflows, use-after-free, null pointer dereferences, and other memory safety issues.

```goo
// All memory operations are safe by default
var numbers []int = alloc []int[5]

// This is automatically bounds-checked
for i := 0; i < 5; i++ {
    numbers[i] = i * 10
}

// This would trigger a runtime error instead of causing memory corruption
// numbers[10] = 100
```

When safety checks need to be bypassed for performance or low-level operations, the `unsafe` block can be used, making these sections explicit and isolated:

```goo
unsafe {
    // This bypasses safety checks - use with caution!
    var ptr *int = (*int)(&numbers[0])
    ptr[3] = 42  // Directly modifying memory
}
```

## Zig-like Allocator Support

Goo provides a flexible allocator system inspired by Zig. This allows developers to control where and how memory is allocated.

### Built-in Allocator Types

Goo supports several allocator types out of the box:

```goo
// Heap allocator (general purpose)
allocator heapAlloc heap {}

// Arena allocator (allocate many, free all at once)
allocator arenaAlloc arena {
    pageSize: 4096,
    capacity: 10240
}

// Fixed-size allocator (e.g., for stack allocation)
allocator stackAlloc fixed {
    size: 1024
}

// Object pool allocator (for fixed-size objects)
allocator poolAlloc pool {
    objectSize: 64,
    capacity: 100
}

// Bump allocator (fast sequential allocation)
allocator bumpAlloc bump {
    size: 8192
}

// Custom allocator
allocator customAlloc custom {
    // Implement custom allocation logic
}
```

### Allocating Memory with Explicit Allocators

Allocators can be explicitly specified when allocating memory:

```goo
// Allocate an array using a specific allocator
var data []int = alloc []int[1000] [allocator: arenaAlloc]

// Initialize the array
for i := 0; i < 1000; i++ {
    data[i] = i
}

// Free the memory (though Goo can manage this automatically in many cases)
free data [allocator: arenaAlloc]
```

### Scoped Allocations

Goo provides a `scope` block that automatically manages memory using a specified allocator:

```goo
// All allocations in this scope use arenaAlloc
scope (arenaAlloc) {
    // These allocations use arenaAlloc and are freed automatically
    // when the scope ends
    var temp1 []int = alloc []int[100]
    var temp2 string = alloc string[50]
    
    // Use the memory...
    
    // No need to explicitly free - it happens automatically
    // when the scope ends
}
```

### Passing Allocators to Functions

Allocators can be passed to functions, allowing flexible memory management:

```goo
// Function that takes an allocator
func processData(data []int, allocator myAlloc) []int {
    // Use the provided allocator
    var results []int = alloc []int[len(data)] [allocator: myAlloc]
    
    // Process the data
    for i := 0; i < len(data); i++ {
        results[i] = data[i] * 2
    }
    
    return results
}

// Call with different allocators
var result1 = processData(data, heapAlloc)
var result2 = processData(data, arenaAlloc)
```

## Automatic Memory Management

While Goo provides explicit control over memory through allocators, it also offers automatic memory management for convenience:

1. **Scope-based Deallocation**: Memory is automatically freed when it goes out of scope
2. **Reference Counting**: For shared resources
3. **Static Analysis**: Compile-time detection of memory leaks and use-after-free errors

## Performance Considerations

Goo's memory management system is designed to allow for zero-overhead abstractions:

- Safety checks can be disabled in `unsafe` blocks when maximum performance is required
- Allocators can be chosen to match the specific memory usage patterns of your application
- Scope-based allocation allows for efficient bulk deallocation
- Compile-time optimizations eliminate unnecessary checks when safety can be proven

## Best Practices

1. **Use the Default Safety**: Let Goo's safety mechanisms protect your code by default
2. **Choose the Right Allocator**: Match your allocator to your memory usage pattern:
   - `heap` for general-purpose allocation
   - `arena` for temporary allocations that can be freed together
   - `pool` for many objects of the same size
   - `bump` for fast sequential allocation
3. **Scope Your Allocations**: Use `scope` blocks to automatically manage memory
4. **Minimize Unsafe Code**: Keep `unsafe` blocks small and well-documented
5. **Consider Ownership**: Design your data structures with clear ownership semantics 