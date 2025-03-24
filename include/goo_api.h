#ifndef GOO_API_H
#define GOO_API_H

/**
 * Main API Header for the Goo Language
 * This brings together all components of the Goo language in a coherent API
 */

// Include core types
#include "goo/core/types.h"

// Include runtime components
#include "goo/runtime/memory.h"
#include "goo/runtime/vectorization.h"

// External C API
#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// Compiler API
// ==============================

// Initialize Goo compiler context
GooContext* goo_init(void);

// Parse a Goo source file
GooAst* goo_parse_file(GooContext* ctx, const char* filename);

// Compile a Goo module
GooModule* goo_compile(GooContext* ctx, GooAst* ast, GooCompilationMode mode);

// Execute a compiled module
int goo_execute(GooContext* ctx, GooModule* module, int argc, char** argv);

// Set context mode (safe, unsafe, kernel, user)
void goo_set_context_mode(GooContext* ctx, GooContextMode mode);

// Create a capability object
GooCapability* goo_create_capability(GooCapabilityType type, const char* name);

// Register a capability with a module
bool goo_register_capability(GooModule* module, GooCapability* capability);

// ==============================
// Allocator API - Wrapper functions to memory.h
// ==============================

// Create a custom allocator
GooAllocator* goo_allocator_create(GooAllocatorType type, void* context);

// Create a custom allocator with specific functions
GooAllocator* goo_allocator_create_custom(
    GooAllocFn alloc_fn, 
    GooReallocFn realloc_fn, 
    GooFreeFn free_fn, 
    void* context
);

// Allocate memory with specific allocator
void* goo_alloc(GooAllocator* allocator, size_t size);

// Reallocate memory with specific allocator
void* goo_realloc(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size);

// Free memory with specific allocator
void goo_free(GooAllocator* allocator, void* ptr, size_t size);

// Destroy an allocator
void goo_allocator_destroy(GooAllocator* allocator);

// Set the default allocator for a context
void goo_set_default_allocator(GooContext* ctx, GooAllocator* allocator);

// ==============================
// Channel API
// ==============================

// Channel options
typedef struct {
    size_t capacity;        // Buffer capacity (0 for unbuffered)
    GooChannelPattern type; // Channel pattern
    bool distributed;       // Whether channel is distributed
    const char* name;       // Optional name for the channel
} GooChannelOptions;

// Create a channel
GooChannel* goo_channel_create(size_t element_size, size_t capacity, GooChannelPattern type);

// Create a channel with options
GooChannel* goo_channel_create_with_options(const GooChannelOptions* options);

// Send data through a channel
bool goo_channel_send(GooChannel* channel, void* data);

// Receive data from a channel
bool goo_channel_receive(GooChannel* channel, void* data);

// Try to send data (non-blocking)
bool goo_channel_try_send(GooChannel* channel, void* data, int timeout_ms);

// Try to receive data (non-blocking)
bool goo_channel_try_receive(GooChannel* channel, void* data, int timeout_ms);

// Close a channel
void goo_channel_close(GooChannel* channel);

// Destroy a channel
void goo_channel_destroy(GooChannel* channel);

// ==============================
// Parallel API
// ==============================

// Execute a function in parallel
bool goo_parallel_exec(void (*func)(void* context, size_t index), void* context, size_t count);

// Execute a distributed function
bool goo_distributed_exec(void (*func)(void* context, size_t node_id), void* context, size_t node_count);

// Cleanup resources
void goo_cleanup(GooContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // GOO_API_H 