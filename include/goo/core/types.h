#ifndef GOO_CORE_TYPES_H
#define GOO_CORE_TYPES_H

/**
 * Core type definitions for the Goo programming language
 * This is the single source of truth for all shared types and enums
 * to prevent redefinition issues.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations of core structures
typedef struct GooAst GooAst;
typedef struct GooContext GooContext;
typedef struct GooModule GooModule;
typedef struct GooCapability GooCapability;
typedef struct GooAllocator GooAllocator;
typedef struct GooCodegenContext GooCodegenContext;
typedef struct GooChannel GooChannel;

// ==============================
// Compilation and Runtime Modes
// ==============================

// Supported compilation modes
typedef enum {
    GOO_MODE_COMPILE,   // Compile only
    GOO_MODE_RUN,       // Compile and run
    GOO_MODE_TEST,      // Run tests
    GOO_MODE_BUILD,     // Use comptime build block
    GOO_MODE_KERNEL,    // Compile for kernel mode
    GOO_MODE_JIT,       // JIT compile and execute
    GOO_MODE_INTERPRET  // Interpret without compiling
} GooCompilationMode;

// Channel types and patterns
typedef enum {
    GOO_CHANNEL_NORMAL = 0,  // Bidirectional point-to-point
    GOO_CHANNEL_BUFFERED,    // Buffered channel
    GOO_CHANNEL_BROADCAST,   // One-to-many channel
    GOO_CHANNEL_MULTICAST,   // Select receivers
    GOO_CHANNEL_PRIORITY     // Priority-based channel
} GooChannelPattern;

// Channel operation patterns
typedef enum {
    GOO_CHAN_DEFAULT,  // Bidirectional channel
    GOO_CHAN_PUB,      // Publisher
    GOO_CHAN_SUB,      // Subscriber
    GOO_CHAN_PUSH,     // Push
    GOO_CHAN_PULL,     // Pull
    GOO_CHAN_REQ,      // Request
    GOO_CHAN_REP,      // Reply
    GOO_CHAN_DEALER,   // Dealer
    GOO_CHAN_ROUTER,   // Router
    GOO_CHAN_PAIR      // Exclusive pair
} GooChannelOperationPattern;

// Safety and context modes
typedef enum {
    GOO_CONTEXT_DEFAULT, // Regular mode (safe by default)
    GOO_CONTEXT_SAFE,    // Explicit safe mode, with safety checks enabled
    GOO_CONTEXT_UNSAFE,  // Unsafe mode, with safety checks disabled
    GOO_CONTEXT_KERNEL,  // Kernel mode
    GOO_CONTEXT_USER     // User mode
} GooContextMode;

// Supervision policies for fault tolerance
typedef enum {
    GOO_SUPERVISE_ONE_FOR_ONE,   // Restart only the failed process
    GOO_SUPERVISE_ONE_FOR_ALL,   // Restart all processes if one fails
    GOO_SUPERVISE_REST_FOR_ONE   // Restart processes that depend on the failed one
} GooSupervisionPolicy;

// ==============================
// Memory Management
// ==============================

// Allocator types and operations
typedef enum {
    GOO_ALLOC_HEAP,     // General heap allocation
    GOO_ALLOC_ARENA,    // Arena allocation (free all at once)
    GOO_ALLOC_FIXED,    // Fixed-size allocation (e.g., stack)
    GOO_ALLOC_POOL,     // Object pool allocation
    GOO_ALLOC_BUMP,     // Bump allocation (fast sequential allocation)
    GOO_ALLOC_CUSTOM    // Custom allocator
} GooAllocatorType;

// Allocation options
typedef enum {
    GOO_ALLOC_DEFAULT = 0,
    GOO_ALLOC_ZERO = 1,       // Zero memory after allocation
    GOO_ALLOC_NOFAIL = 2,     // Must not fail (panic on failure)
    GOO_ALLOC_GROWABLE = 4,   // Can be resized efficiently
    GOO_ALLOC_PERSIST = 8,    // Persists beyond scope (for scope allocators)
    GOO_ALLOC_TEMP = 16       // Very short-lived allocation
} GooAllocOptions;

// Allocation failure strategies
typedef enum {
    GOO_ALLOC_STRATEGY_NULL,   // Return NULL on failure
    GOO_ALLOC_STRATEGY_PANIC,  // Panic on failure
    GOO_ALLOC_STRATEGY_RETRY,  // Retry after running OOM handler
    GOO_ALLOC_STRATEGY_GC      // Trigger garbage collection and retry
} GooAllocStrategy;

// ==============================
// Security and Capabilities
// ==============================

// Capability types for microkernel support
typedef enum {
    GOO_CAP_NONE      = 0,      // No capabilities (default)
    GOO_CAP_FILE_IO   = 1,      // File system access
    GOO_CAP_NETWORK   = 2,      // Network access
    GOO_CAP_PROCESS   = 3,      // Process control/creation
    GOO_CAP_MEMORY    = 4,      // Advanced memory operations
    GOO_CAP_TIME      = 5,      // Time manipulation
    GOO_CAP_SIGNAL    = 6,      // Signal handling
    GOO_CAP_DEVICE    = 7,      // Device access
    GOO_CAP_UNSAFE    = 8,      // Unsafe operations
    GOO_CAP_SANDBOX   = 9,      // Sandbox capabilities
    GOO_CAP_ALL       = 0xFFFF, // All capabilities (privileged)
    
    // Reserved for user-defined capabilities (10000-19999)
    GOO_CAP_USER_MIN  = 10000,
    GOO_CAP_USER_MAX  = 19999
} GooCapabilityType;

// ==============================
// SIMD and Vectorization
// ==============================

// Basic SIMD types
typedef enum {
    GOO_SIMD_AUTO,     // Automatically detect best available
    GOO_SIMD_SCALAR,   // Fallback scalar implementation (no SIMD)
    GOO_SIMD_SSE2,     // Intel SSE2
    GOO_SIMD_SSE4,     // Intel SSE4
    GOO_SIMD_AVX,      // Intel AVX
    GOO_SIMD_AVX2,     // Intel AVX2
    GOO_SIMD_AVX512,   // Intel AVX-512
    GOO_SIMD_NEON      // ARM NEON
} GooSIMDType;

// Vector data types
typedef enum {
    GOO_VEC_INT8,      // 8-bit signed integer
    GOO_VEC_UINT8,     // 8-bit unsigned integer
    GOO_VEC_INT16,     // 16-bit signed integer
    GOO_VEC_UINT16,    // 16-bit unsigned integer
    GOO_VEC_INT32,     // 32-bit signed integer
    GOO_VEC_UINT32,    // 32-bit unsigned integer
    GOO_VEC_INT64,     // 64-bit signed integer
    GOO_VEC_UINT64,    // 64-bit unsigned integer
    GOO_VEC_FLOAT,     // 32-bit floating point
    GOO_VEC_DOUBLE     // 64-bit floating point
} GooVectorDataType;

// Vector operations
typedef enum {
    GOO_VECTOR_ADD,      // Element-wise addition
    GOO_VECTOR_SUB,      // Element-wise subtraction
    GOO_VECTOR_MUL,      // Element-wise multiplication
    GOO_VECTOR_DIV,      // Element-wise division
    GOO_VECTOR_FMA,      // Fused multiply-add
    GOO_VECTOR_ABS,      // Absolute value
    GOO_VECTOR_SQRT,     // Square root
    GOO_VECTOR_CUSTOM    // Custom operation function
} GooVectorOp;

// Extended vector operations (new API)
typedef enum {
    // Arithmetic operations
    GOO_VECTOR_OP_ADD = 0,
    GOO_VECTOR_OP_SUB = 1,
    GOO_VECTOR_OP_MUL = 2,
    GOO_VECTOR_OP_DIV = 3,
    
    // Bitwise operations
    GOO_VECTOR_OP_AND = 10,
    GOO_VECTOR_OP_OR = 11,
    GOO_VECTOR_OP_XOR = 12,
    GOO_VECTOR_OP_NOT = 13,
    
    // Comparison operations
    GOO_VECTOR_OP_EQ = 20,
    GOO_VECTOR_OP_NE = 21,
    GOO_VECTOR_OP_LT = 22,
    GOO_VECTOR_OP_LE = 23,
    GOO_VECTOR_OP_GT = 24,
    GOO_VECTOR_OP_GE = 25,
    
    // Mathematical functions
    GOO_VECTOR_OP_SQRT = 30,
    GOO_VECTOR_OP_ABS = 31,
    GOO_VECTOR_OP_MIN = 32,
    GOO_VECTOR_OP_MAX = 33,
    
    // Movement operations
    GOO_VECTOR_OP_LOAD = 40,
    GOO_VECTOR_OP_STORE = 41,
    GOO_VECTOR_OP_GATHER = 42,
    GOO_VECTOR_OP_SCATTER = 43,
    
    // Utility operations
    GOO_VECTOR_OP_BLEND = 50,
    GOO_VECTOR_OP_SHUFFLE = 51,
    GOO_VECTOR_OP_SET1 = 52
} GooVectorOpExtended;

// Version information
#define GOO_VERSION_MAJOR 0
#define GOO_VERSION_MINOR 1
#define GOO_VERSION_PATCH 0

#ifdef __cplusplus
}
#endif

#endif // GOO_CORE_TYPES_H 