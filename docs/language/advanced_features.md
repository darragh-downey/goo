# Advanced Features in Goo

## Safety Features

### Safe and Unsafe Contexts

Goo provides mechanisms to control safety at different levels of the program:

```goo
// Safe context - bounds checking and safety features enabled
safe {
    // Code here gets extra safety checks
    array[i] = 10; // Will check array bounds
}

// Unsafe context - for performance or low-level operations
unsafe {
    // Code here bypasses safety features
    *ptr = 10; // Direct memory access
}
```

Variables can also be declared with safety guarantees:

```goo
safe var counter int = 0; // Safe variable with bounds checking
```

## Fault Tolerance

### Try and Recover

Goo provides built-in error handling with try/recover blocks:

```goo
// Try with specific error type
try {
    result := riskyOperation();
} recover (err) {
    // Handle the error
    println("Error occurred:", err);
}
```

### Supervision

Goo's supervision feature allows for automatic error recovery:

```goo
// Supervise a function - it will be restarted if it fails
supervise runServer();

// More complex supervision can be done with blocks
supervise {
    startDatabase();
    connectToService();
    runWebServer();
}
```

## Concurrency and Parallelism

### Goroutines

Similar to Go, Goo supports lightweight threads called goroutines:

```goo
// Start a function in a new goroutine
go processData(largeArray);
```

### Parallel Processing with Range Literals

Goo introduces range literals for easy parallel processing:

```goo
// Process items 0 through 999 in parallel
go parallel [range: 0..999, shared: data, private: results] {
    // 'range' is implicitly available in the block
    results[range] = processItem(data[range]);
}

// Multi-dimensional ranges
go parallel [range: 0..99, 0..99, shared: matrix] {
    i, j := range; // range is a tuple (i, j)
    matrix[i][j] = computeValue(i, j);
}
```

### Channels

Goo offers sophisticated channel patterns for distributed communication:

```goo
// Simple bidirectional channel
channel msgs chan<string> = new;

// Publisher-subscriber pattern
channel headlines pub chan<string> = new;
channel newsReader sub chan<string> = new;

// Distributed channel with endpoint
channel remoteData chan<string>[endpoint="tcp://server:1234"] = new;

// Send and receive
msgs <- "Hello";  // Send
message := <-msgs; // Receive
```

## Microkernel Architecture Support

### Kernel and User Functions

Goo supports microkernel development with specialized functions:

```goo
// Kernel-space function
kernel func allocateMemory(size int) int64 {
    // Privileged operations
    unsafe {
        // Direct memory allocation
        return allocPhysicalMemory(size);
    }
}

// User-space function
user func main() {
    // Unprivileged operations
    address := syscall(SYS_ALLOCATE, 1024);
}
```

### Modules and Capabilities

Goo provides modules with capability-based security:

```goo
// Define a module with capabilities
module FileSystem {
    // Declare capabilities required
    cap<io> channel FileOpen chan<string> = new;
    
    // Functions within this module have access to IO
    func openFile(path string) File {
        // ...
    }
}

// Using a capability-restricted channel
cap<memory> channel memoryOps chan<Request> = new;
```

### Reflection

Goo supports runtime introspection with the reflect keyword:

```goo
// Get type information at runtime
typeInfo := reflect typeof(myVariable);
println("Type:", typeInfo.name);

// Create instances dynamically
newInstance := reflect new(MyStruct);
```

## Range Literals

Goo introduces range literals for expressing intervals:

```goo
// Integer range
for i in 1..10 {
    println(i); // Prints 1 through 10
}

// Character range
for c in 'a'..'z' {
    print(c);
}

// Using ranges in array slices
elements := array[2..5]; // Elements at indices 2, 3, 4, 5
``` 