#ifndef GOO_H
#define GOO_H

#include <stdint.h>
#include <stdbool.h>
#include "goo_core.h"

// Allocator function signatures
typedef void* (*GooAllocFn)(void* ctx, size_t size, size_t alignment);
typedef void* (*GooReallocFn)(void* ctx, void* ptr, size_t old_size, size_t new_size, size_t alignment);
typedef void (*GooFreeFn)(void* ctx, void* ptr, size_t size);

// Allocator structure
struct GooAllocator {
    GooAllocatorType type;
    void* context;
    GooAllocFn alloc;
    GooReallocFn realloc;
    GooFreeFn free;
};

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

// Allocator functions
GooAllocator* goo_allocator_create(GooAllocatorType type, void* context);
GooAllocator* goo_allocator_create_custom(GooAllocFn alloc_fn, GooReallocFn realloc_fn, GooFreeFn free_fn, void* context);
void* goo_alloc(GooAllocator* allocator, size_t size);
void* goo_realloc(GooAllocator* allocator, void* ptr, size_t old_size, size_t new_size);
void goo_free(GooAllocator* allocator, void* ptr, size_t size);
void goo_allocator_destroy(GooAllocator* allocator);

// Set the default allocator for a context
void goo_set_default_allocator(GooContext* ctx, GooAllocator* allocator);

// Cleanup
void goo_cleanup(GooContext* ctx);

#endif // GOO_H 