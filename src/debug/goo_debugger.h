#ifndef GOO_DEBUGGER_H
#define GOO_DEBUGGER_H

#include <stdbool.h>
#include <stdint.h>
#include "goo_runtime.h"
#include "goo_inspector.h"
#include "goo_trace.h"

// Forward declarations
typedef struct GooDebugger GooDebugger;
typedef struct GooBreakpoint GooBreakpoint;
typedef struct GooVariableInfo GooVariableInfo;
typedef struct GooStackFrame GooStackFrame;
typedef struct GooThreadDebugInfo GooThreadDebugInfo;

// Debugger message types
typedef enum {
    GOO_DEBUG_MSG_BREAKPOINT_HIT = 0,
    GOO_DEBUG_MSG_STEP_COMPLETE = 1,
    GOO_DEBUG_MSG_EXCEPTION = 2,
    GOO_DEBUG_MSG_THREAD_START = 3,
    GOO_DEBUG_MSG_THREAD_EXIT = 4,
    GOO_DEBUG_MSG_PROCESS_START = 5,
    GOO_DEBUG_MSG_PROCESS_EXIT = 6,
    GOO_DEBUG_MSG_OUTPUT = 7,
    GOO_DEBUG_MSG_MODULE_LOAD = 8,
    GOO_DEBUG_MSG_MODULE_UNLOAD = 9
} GooDebugMessageType;

// Breakpoint types
typedef enum {
    GOO_BREAKPOINT_LINE = 0,       // Break at a specific line
    GOO_BREAKPOINT_FUNCTION = 1,   // Break at function entry
    GOO_BREAKPOINT_EXCEPTION = 2,  // Break on exception
    GOO_BREAKPOINT_DATA = 3,       // Break on data access
    GOO_BREAKPOINT_CONDITIONAL = 4 // Break on condition
} GooBreakpointType;

// Stepping types
typedef enum {
    GOO_STEP_INTO = 0,    // Step into function calls
    GOO_STEP_OVER = 1,    // Step over function calls
    GOO_STEP_OUT = 2,     // Step out of current function
    GOO_STEP_CONTINUE = 3 // Continue execution
} GooStepType;

// Variable types
typedef enum {
    GOO_VAR_INT = 0,
    GOO_VAR_FLOAT = 1,
    GOO_VAR_STRING = 2,
    GOO_VAR_POINTER = 3,
    GOO_VAR_ARRAY = 4,
    GOO_VAR_STRUCT = 5,
    GOO_VAR_FUNCTION = 6,
    GOO_VAR_CHANNEL = 7,
    GOO_VAR_UNKNOWN = 8
} GooVariableType;

// Variable info structure
struct GooVariableInfo {
    char* name;                  // Variable name
    GooVariableType type;        // Variable type
    size_t size;                 // Size in bytes
    void* address;               // Memory address
    char* value_str;             // String representation of value
    bool is_local;               // Whether variable is local to the current frame
    GooVariableInfo** children;  // Child variables (for structs, arrays, etc.)
    int child_count;             // Number of children
};

// Stack frame information
struct GooStackFrame {
    char* function_name;         // Function name
    char* file_name;             // Source file
    int line_number;             // Line number
    void* frame_address;         // Frame address
    GooVariableInfo** locals;    // Local variables
    int local_count;             // Number of local variables
};

// Thread debug information
struct GooThreadDebugInfo {
    uint64_t thread_id;          // Thread ID
    char* thread_name;           // Thread name
    bool is_stopped;             // Whether thread is stopped
    GooStackFrame** call_stack;  // Call stack
    int stack_depth;             // Stack depth
    int current_frame;           // Current frame index
};

// Breakpoint information
struct GooBreakpoint {
    int id;                      // Breakpoint ID
    GooBreakpointType type;      // Breakpoint type
    char* file_name;             // Source file
    int line_number;             // Line number
    char* function_name;         // Function name
    char* condition;             // Condition expression
    bool enabled;                // Whether breakpoint is enabled
    uint64_t hit_count;          // Number of times breakpoint was hit
    int ignore_count;            // Number of times to ignore
};

// Debugger event callback type
typedef void (*GooDebuggerEventCallback)(void* context, GooDebugMessageType type, void* data);

// Debugger configuration
typedef struct {
    bool enable_remote;          // Enable remote debugging
    char* host;                  // Remote host (for server)
    int port;                    // Remote port (for server)
    bool wait_for_connection;    // Wait for client connection before starting
    char* symbols_path;          // Path to debug symbols
    bool debug_runtime;          // Debug runtime itself
    bool catch_exceptions;       // Break on exceptions
    bool break_on_start;         // Break on program start
    int log_level;               // 0=off, 1=error, 2=warn, 3=info, 4=debug
    GooInspector* inspector;     // Inspector to use (if any)
    GooTraceContext* trace_ctx;  // Trace context (if any)
} GooDebuggerConfig;

// === Debugger API ===

// Create a new debugger
GooDebugger* goo_debugger_create(GooDebuggerConfig* config);

// Destroy a debugger
void goo_debugger_destroy(GooDebugger* debugger);

// Set global debugger instance
void goo_debugger_set_global(GooDebugger* debugger);

// Get global debugger instance
GooDebugger* goo_debugger_get_global(void);

// Connect to a remote debugging client
bool goo_debugger_connect(GooDebugger* debugger, const char* host, int port);

// Start debugging server
bool goo_debugger_start_server(GooDebugger* debugger);

// Stop debugging server
void goo_debugger_stop_server(GooDebugger* debugger);

// Register event callback
void goo_debugger_set_event_callback(GooDebugger* debugger, GooDebuggerEventCallback callback, void* context);

// === Breakpoint API ===

// Add a breakpoint at a specific line
int goo_debugger_add_breakpoint_line(GooDebugger* debugger, const char* file, int line);

// Add a breakpoint at a function
int goo_debugger_add_breakpoint_function(GooDebugger* debugger, const char* function);

// Add a conditional breakpoint
int goo_debugger_add_breakpoint_conditional(GooDebugger* debugger, const char* file, int line, const char* condition);

// Enable or disable a breakpoint
bool goo_debugger_enable_breakpoint(GooDebugger* debugger, int id, bool enable);

// Remove a breakpoint
bool goo_debugger_remove_breakpoint(GooDebugger* debugger, int id);

// Get all breakpoints
GooBreakpoint** goo_debugger_get_breakpoints(GooDebugger* debugger, int* count);

// === Execution Control API ===

// Start debugging
bool goo_debugger_start(GooDebugger* debugger);

// Stop debugging
void goo_debugger_stop(GooDebugger* debugger);

// Pause execution
bool goo_debugger_pause(GooDebugger* debugger);

// Continue execution
bool goo_debugger_continue(GooDebugger* debugger);

// Step execution
bool goo_debugger_step(GooDebugger* debugger, GooStepType step_type);

// === Inspection API ===

// Get all threads
GooThreadDebugInfo** goo_debugger_get_threads(GooDebugger* debugger, int* count);

// Get current thread
GooThreadDebugInfo* goo_debugger_get_current_thread(GooDebugger* debugger);

// Set current thread
bool goo_debugger_set_current_thread(GooDebugger* debugger, uint64_t thread_id);

// Get call stack for thread
GooStackFrame** goo_debugger_get_stack_trace(GooDebugger* debugger, uint64_t thread_id, int* depth);

// Get local variables for frame
GooVariableInfo** goo_debugger_get_locals(GooDebugger* debugger, uint64_t thread_id, int frame_idx, int* count);

// Evaluate expression in context
char* goo_debugger_evaluate(GooDebugger* debugger, uint64_t thread_id, int frame_idx, const char* expression);

// Get variable information
GooVariableInfo* goo_debugger_get_variable(GooDebugger* debugger, uint64_t thread_id, int frame_idx, const char* name);

// Get memory contents
uint8_t* goo_debugger_read_memory(GooDebugger* debugger, void* address, size_t size);

// Write memory
bool goo_debugger_write_memory(GooDebugger* debugger, void* address, uint8_t* data, size_t size);

// === Integration with Inspector and Tracer ===

// Attach an inspector
void goo_debugger_attach_inspector(GooDebugger* debugger, GooInspector* inspector);

// Attach a trace context
void goo_debugger_attach_trace(GooDebugger* debugger, GooTraceContext* context);

// === Memory Management ===

// Free debug information structures
void goo_thread_debug_info_free(GooThreadDebugInfo* info);
void goo_stack_frame_free(GooStackFrame* frame);
void goo_variable_info_free(GooVariableInfo* var);
void goo_breakpoint_free(GooBreakpoint* breakpoint);

#endif // GOO_DEBUGGER_H 