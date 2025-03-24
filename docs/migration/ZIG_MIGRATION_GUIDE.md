# Migrating Safety-Critical Code from C to Zig

This guide outlines the process for migrating safety-critical C code to Zig, focusing on our Goo Safety System as a case study. The approach leverages Zig's built-in safety features while maintaining full interoperability with existing C code.

## Why Migrate to Zig?

Zig offers several advantages for safety-critical code:

1. **Built-in Safety Features**: Optional types, comptime checks, and explicit error handling
2. **C Interoperability**: Seamless integration with existing C codebases
3. **Compile-time Reflection**: Powerful compile-time capabilities
4. **Overflow Protection**: Built-in protection against integer overflows
5. **Memory Safety**: Better control over memory allocation and ownership
6. **Performance**: Equal to or better than C in most cases
7. **Build System**: Integrated build system that simplifies cross-compilation

## Migration Strategy

Our migration approach follows these key principles:

1. **Incremental Migration**: Replace components one at a time
2. **API Compatibility**: Maintain the same C API for interoperability
3. **Export Symbols**: Use Zig's export feature to expose functions to C
4. **Enhanced Safety**: Leverage Zig's type system for stronger safety guarantees
5. **Test Parity**: Maintain the same test coverage

## Migration Steps

### 1. Identify Safety-Critical Components

Start by identifying the most safety-critical parts of your codebase. Good candidates include:

- Memory management code
- Concurrency primitives
- Type verification systems
- Input validation
- Error handling

For the Goo Safety System, we identified the core type checking and memory safety components as critical.

### 2. Create Zig Counterparts

For each identified component:

1. Create a Zig file that implements the same functionality
2. Use Zig's stronger type system to enhance safety
3. Export C-compatible functions with the same signatures

**Example**: Converting the C type signature function to Zig:

```c
// C implementation
GooTypeSignature goo_type_signature(const char* type_name, size_t type_size) {
    GooTypeSignature sig;
    
    // Generate a hash from the type name for the type ID
    uint64_t hash = 5381;
    const unsigned char* str = (const unsigned char*)type_name;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    hash = ((hash << 5) + hash) + type_size;
    
    sig.type_id = hash;
    sig.type_name = type_name;
    sig.type_size = type_size;
    
    return sig;
}
```

```zig
// Zig implementation
pub fn typeSignature(type_name: [*:0]const u8, type_size: usize) GooTypeSignature {
    return GooTypeSignature.init(type_name, type_size);
}

// Implementation moved to the struct for better organization
pub const GooTypeSignature = extern struct {
    type_id: u64,
    type_name: [*:0]const u8,
    type_size: usize,
    
    pub fn init(name: [*:0]const u8, size: usize) GooTypeSignature {
        // Generate a hash from the type name for the type ID
        var hash: u64 = 5381;
        var i: usize = 0;
        while (name[i] != 0) : (i += 1) {
            hash = ((hash << 5) +% hash) +% name[i];
        }
        
        // Include size in the hash
        hash = ((hash << 5) +% hash) +% @as(u64, @intCast(size));
        
        return GooTypeSignature{
            .type_id = hash,
            .type_name = name,
            .type_size = size,
        };
    }
};

// Export the C-compatible function
export fn goo_type_signature(type_name: [*:0]const u8, type_size: usize) GooTypeSignature {
    return typeSignature(type_name, type_size);
}
```

### 3. Enhance Safety with Zig's Features

Use Zig's built-in features to enhance safety:

- **Optional Types**: Replace NULL pointers with optional types
- **Error Handling**: Use Zig's error union types for robust error handling
- **Overflow Protection**: Use Zig's overflow-checked arithmetic (`+%`, `*%`)
- **Comptime Checks**: Verify properties at compile time when possible
- **Bounds Checking**: Use Zig's slice type for bounds-checked arrays

**Example**: Converting C memory allocation with overflow check to Zig:

```c
// C implementation
void* goo_safe_malloc_with_type(size_t count, size_t size, const char* type_name) {
    /* Check for integer overflow */
    if (count > 0 && size > SIZE_MAX / count) {
        goo_set_error(ENOMEM, "Integer overflow in allocation", __FILE__, __LINE__);
        return NULL;
    }
    
    /* Calculate total size with header */
    size_t total_size = count * size;
    size_t allocation_size = sizeof(GooMemoryHeader) + total_size;
    
    /* Allocate memory with header */
    pthread_mutex_lock(&g_safety_mutex);
    void* mem = malloc(allocation_size);
    pthread_mutex_unlock(&g_safety_mutex);
    
    if (!mem) {
        goo_set_error(ENOMEM, "Failed to allocate memory", __FILE__, __LINE__);
        return NULL;
    }
    
    /* Initialize header */
    GooMemoryHeader* header = (GooMemoryHeader*)mem;
    header->magic = GOO_TYPE_MAGIC;
    header->type_sig = goo_type_signature(type_name, size);
    header->size = total_size;
    
    /* Return pointer after header */
    return (char*)mem + sizeof(GooMemoryHeader);
}
```

```zig
// Zig implementation
pub fn safeMallocWithType(count: usize, size: usize, type_name: [*:0]const u8) ?*anyopaque {
    // Check for integer overflow using Zig's built-in overflow protection
    const total_size = count *% size;
    if (count > 0 and total_size / count != size) {
        setError(c.ENOMEM, "Integer overflow in allocation", @src().file.ptr, @src().line);
        return null;
    }
    
    // Calculate allocation size with header
    const allocation_size = @sizeOf(GooMemoryHeader) + total_size;
    
    // Allocate memory with header using mutex for thread safety
    _ = c.pthread_mutex_lock(&g_safety_mutex);
    const mem = std.c.malloc(allocation_size);
    _ = c.pthread_mutex_unlock(&g_safety_mutex);
    
    if (mem == null) {
        setError(c.ENOMEM, "Failed to allocate memory", @src().file.ptr, @src().line);
        return null;
    }
    
    // Initialize header
    const header = @as(*GooMemoryHeader, @ptrCast(@alignCast(mem.?)));
    header.magic = GOO_TYPE_MAGIC;
    header.type_sig = typeSignature(type_name, size);
    header.size = total_size;
    
    // Return pointer after header
    return @as([*]u8, @ptrCast(mem.?)) + @sizeOf(GooMemoryHeader);
}
```

### 4. Ensuring ABI Compatibility

When migrating to Zig, ensuring ABI (Application Binary Interface) compatibility is critical, especially for structures that cross the language boundary. Here are key considerations:

#### Structs Must Be ABI-Compatible

In Zig, only `extern struct` types can be used in exported function signatures. Regular Zig structs are not guaranteed to have the same memory layout as their C counterparts.

**Common ABI Compatibility Errors:**

```
error: parameter of type 'safety_core.GooTypeSignature' not allowed in function with calling convention 'aarch64_aapcs_darwin'
```

**Solution:** Use `extern struct` for any type that crosses the language boundary:

```zig
// Wrong: Using regular Zig struct
pub const GooTypeSignature = struct {
    type_id: u64,
    type_name: [*:0]const u8,
    type_size: usize,
    // ...
};

// Correct: Using extern struct for C compatibility
pub const GooTypeSignature = extern struct {
    type_id: u64,
    type_name: [*:0]const u8,
    type_size: usize,
    // ...
};
```

#### Memory Layout and Padding

Zig structs and C structs might have different memory layouts due to padding and alignment. Using `extern struct` ensures the layout matches C's expectations.

#### Field Types Must Match C Types

Ensure that field types in Zig structs match their C counterparts:

| C Type | Zig Type |
|--------|----------|
| `int` | `c_int` or `i32` depending on platform |
| `unsigned int` | `c_uint` or `u32` |
| `char*` | `[*:0]u8` or `[*:0]const u8` |
| `void*` | `?*anyopaque` |
| `size_t` | `usize` |

#### Handling C Macros and Constants

For C macros and constants, you may need to use `@cImport` to access them or redefine them in Zig:

```zig
const c = @cImport({
    @cInclude("pthread.h");
    @cInclude("errno.h");
});

// Using C constants
if (result == c.EINVAL) {
    // Handle invalid argument error
}
```

For some C macros like `PTHREAD_MUTEX_INITIALIZER`, Zig cannot translate them directly:

```
error: unable to translate C expr: unexpected token '{'
pub const PTHREAD_MUTEX_INITIALIZER = @compileError("unable to translate C expr: unexpected token '{'");
```

**Solution:** Initialize the mutex explicitly instead:

```zig
// Instead of using PTHREAD_MUTEX_INITIALIZER
var g_safety_mutex: c.pthread_mutex_t = undefined;
var g_safety_initialized: bool = false;

fn initSafetySystem() bool {
    if (g_safety_initialized) return true;
    
    if (c.pthread_mutex_init(&g_safety_mutex, null) != 0) {
        return false;
    }
    
    g_safety_initialized = true;
    return true;
}
```

### 5. Real-World Challenges and Solutions

During our migration from C to Zig, we encountered several challenges that required specific solutions:

#### Return Type Mismatches

When replacing the C implementation with Zig, we initially missed some return types in the API:

```c
// C header declaration
int goo_safe_free(void* ptr);

// Initial incorrect Zig implementation
export fn goo_safe_free(ptr: ?*anyopaque) void {
    safeFree(ptr);
}
```

This caused test failures because the C code expected and used the return value. The corrected implementation:

```zig
// Corrected Zig implementation
pub fn safeFree(ptr: ?*anyopaque) i32 {
    if (ptr == null) {
        setError(c.EINVAL, "Null pointer in free", @src().file.ptr, @src().line);
        return c.EINVAL;
    }
    
    // Implementation details...
    
    return 0;
}

export fn goo_safe_free(ptr: ?*anyopaque) i32 {
    return safeFree(ptr);
}
```

#### String Handling in Error Reporting

The C code used string pointers for error reporting, but our initial Zig implementation used fixed-size arrays:

```zig
// Initial incorrect struct definition
pub const GooErrorInfo = extern struct {
    error_code: i32,
    error_message: [256]u8,
    file: [256]u8,
    line: i32,
};
```

This didn't match the C header, which used string pointers:

```c
typedef struct {
    int error_code;        /* Error code */
    const char* message;   /* Error message */
    const char* file;      /* Source file where error occurred */
    int line;              /* Line number where error occurred */
} GooErrorInfo;
```

Our solution was to use string pointers in the struct, but with static buffers to ensure memory safety:

```zig
// Static error message buffers to ensure pointers remain valid
var error_message_buffer: [256]u8 = [_]u8{0} ** 256;
var error_file_buffer: [256]u8 = [_]u8{0} ** 256;

// Corrected struct definition matching C
pub const GooErrorInfo = extern struct {
    error_code: i32,
    message: [*:0]const u8,
    file: [*:0]const u8,
    line: i32,
};

// Initialize error info with default values
threadlocal var g_last_error: GooErrorInfo = GooErrorInfo{
    .error_code = 0,
    .message = "No error",
    .file = "unknown",
    .line = 0,
};

// Set error with safe string handling
fn setError(code: i32, message: [*:0]const u8, file: [*:0]const u8, line: i32) void {
    g_last_error.error_code = code;
    
    // Copy message to our static buffer
    var i: usize = 0;
    while (message[i] != 0 and i < error_message_buffer.len - 1) : (i += 1) {
        error_message_buffer[i] = message[i];
    }
    error_message_buffer[i] = 0;
    g_last_error.message = @ptrCast(&error_message_buffer);
    
    // Similar handling for file...
}
```

#### Special Case Handling for Test Code

Our test suite contained a special case for test vectors that was handled uniquely in the C code:

```c
/* For testing purposes, accept any type if it's a TestVector */
if (strncmp(expected_type.type_name, "TestVector", 10) == 0) {
    /* Accept TestVector type without checking memory header */
} else if (!check_type(vector, expected_type)) {
    /* Error handling... */
}
```

We needed to add this special case in our Zig implementation to ensure test compatibility:

```zig
pub fn safetyVectorExecute(vector: ?*anyopaque, expected_type: GooTypeSignature, timeout_ms: u32) bool {
    // Allow "TestVector" type to bypass validation for testing purposes
    const is_test_vector = blk: {
        const test_name = "TestVector";
        var i: usize = 0;
        while (i < 10 and expected_type.type_name[i] != 0) : (i += 1) {
            if (expected_type.type_name[i] != test_name[i]) break :blk false;
        }
        break :blk i == 10 and expected_type.type_name[i] == 0;
    };
    
    // Either it's a test vector or we need to check the type
    if (!is_test_vector and !checkType(vector, expected_type)) {
        setError(c.EINVAL, "Type mismatch in vector execution", @src().file.ptr, @src().line);
        return false;
    }
    
    // Rest of implementation...
}
```

This demonstrates how backward compatibility sometimes requires implementing special cases that might otherwise be avoided in a clean Zig design.

#### Function Signature Documentation

We discovered that a complete understanding of the function signatures is crucial before starting the migration. Small differences in return types or parameter handling can lead to subtle bugs:

**Recommendation:** Create a comprehensive API compatibility checklist by extracting all function declarations from header files before starting migration:

```bash
grep -r "^[a-zA-Z_]* [a-zA-Z_]*(.*);" include/ --include="*.h" | sort > api_functions.txt
```

### 6. Update the Build System

Modify the build system to compile and link the Zig code:

```zig
// Safety system library (Zig implementation)
const safety_lib = b.addStaticLibrary(.{
    .name = "goo-safety",
    .root_source_file = b.path("src/safety/safety_core.zig"),
    .target = target,
    .optimize = optimize,
});
safety_lib.addIncludePath(b.path("include"));
safety_lib.linkLibC();
safety_lib.linkSystemLibrary("pthread");

// Install the Zig safety library
b.installArtifact(safety_lib);
```

### 7. Test and Validate

Maintain the same test coverage to ensure the Zig implementation behaves identically:

1. Use the existing C tests, which should work with the Zig implementation
2. Add Zig-specific tests to verify new safety features
3. Validate both the API compatibility and behavior compatibility

## Benefits Observed

Our migration of the Goo Safety System to Zig has yielded several benefits:

1. **Improved Safety**: Zig's type system caught several potential issues at compile time
2. **Better Performance**: The Zig implementation showed slight performance improvements
3. **Code Clarity**: The Zig code is more concise and intention-revealing
4. **Enhanced Error Handling**: Better error context provided by Zig
5. **Simplified Maintenance**: More maintainable code with fewer error-prone patterns

## Challenges and Solutions

| Challenge | Solution |
|-----------|----------|
| Type compatibility with C | Use Zig's C ABI compatibility and export features |
| Maintaining API stability | Export functions with the exact same signatures |
| Thread safety concerns | Use Zig's threading primitives or C's via cImport |
| Performance overhead | Use Zig's comptime features to optimize at compile time |
| Learning curve | Incrementally migrate, starting with small, isolated components |

## Final Recommendations

1. **Start Small**: Begin with isolated, self-contained components
2. **Maintain Testing**: Ensure existing tests pass with the Zig implementation
3. **Leverage Zig Strengths**: Use Zig's compile-time features, error handling, and optionals
4. **Maintain C Interoperability**: Keep the C API stable for gradual migration
5. **Document**: Keep thorough documentation of design decisions and migration patterns

By following these guidelines, we've successfully migrated our safety-critical core to Zig while maintaining compatibility with the existing C codebase, resulting in safer and more maintainable code. 