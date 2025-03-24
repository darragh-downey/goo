#ifndef GOO_TRACE_H
#define GOO_TRACE_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "goo_inspector.h"

// Forward declarations
typedef struct GooTraceContext GooTraceContext;
typedef struct GooTraceSpan GooTraceSpan;
typedef struct GooTraceEvent GooTraceEvent;

// Trace context status
typedef enum {
    GOO_TRACE_STATUS_OK = 0,
    GOO_TRACE_STATUS_ERROR = 1,
    GOO_TRACE_STATUS_TIMEOUT = 2,
    GOO_TRACE_STATUS_CANCELLED = 3,
    GOO_TRACE_STATUS_UNKNOWN = 4
} GooTraceStatus;

// Event types
typedef enum {
    GOO_TRACE_EVENT_START = 0,
    GOO_TRACE_EVENT_END = 1,
    GOO_TRACE_EVENT_INSTANT = 2,
    GOO_TRACE_EVENT_PROCESS = 3,
    GOO_TRACE_EVENT_THREAD = 4,
    GOO_TRACE_EVENT_COUNTER = 5,
    GOO_TRACE_EVENT_MESSAGE = 6
} GooTraceEventType;

// Message direction for message events
typedef enum {
    GOO_TRACE_MESSAGE_SEND = 0,
    GOO_TRACE_MESSAGE_RECEIVE = 1,
    GOO_TRACE_MESSAGE_PUBLISH = 2,
    GOO_TRACE_MESSAGE_SUBSCRIBE = 3
} GooTraceMessageDirection;

// Configuration options for tracing
typedef struct {
    bool enable_trace;        // Master switch for tracing
    bool console_output;      // Print to console in addition to file
    char* output_file;        // File to output trace data to
    int sampling_rate;        // 1-100, percentage of spans to sample
    int max_events_per_span;  // Maximum events to record per span
    size_t max_attribute_size; // Maximum size of attribute values
    size_t max_message_size;   // Maximum size of message data to record
    bool record_timestamps;    // Record high-precision timestamps
    bool record_stack_traces;  // Record stack traces for events
} GooTraceConfig;

// Trace span attribute (key-value pair)
typedef struct {
    char* key;
    char* value;
} GooTraceAttribute;

// === Trace Context API ===

// Create a new trace context
GooTraceContext* goo_trace_context_create(GooTraceConfig* config);

// Destroy a trace context
void goo_trace_context_destroy(GooTraceContext* context);

// Start a new trace span
GooTraceSpan* goo_trace_span_start(GooTraceContext* context, const char* name);

// Create a child span
GooTraceSpan* goo_trace_span_create_child(GooTraceSpan* parent, const char* name);

// End a trace span
void goo_trace_span_end(GooTraceSpan* span);

// Set status for a span
void goo_trace_span_set_status(GooTraceSpan* span, GooTraceStatus status, const char* description);

// Add attribute to a span
bool goo_trace_span_add_attribute(GooTraceSpan* span, const char* key, const char* value);

// Add event to a span
bool goo_trace_span_add_event(GooTraceSpan* span, GooTraceEventType type, const char* name);

// Add a more detailed event to a span
bool goo_trace_span_add_event_with_attributes(GooTraceSpan* span, GooTraceEventType type, 
                                             const char* name, GooTraceAttribute* attributes, 
                                             int attribute_count);

// Record a message event (send/receive)
bool goo_trace_record_message(GooTraceSpan* span, GooTraceMessageDirection direction, 
                             const char* channel_name, const void* message_data, 
                             size_t message_size, uint64_t message_id);

// Add a counter event
bool goo_trace_record_counter(GooTraceSpan* span, const char* name, 
                             const char** counter_names, double* counter_values, int count);

// Record function entry/exit (convenience wrapper)
void goo_trace_function(GooTraceContext* context, const char* function_name, void* func_ptr);

// === Integration with Inspector ===

// Attach a trace context to an inspector
void goo_trace_attach_inspector(GooTraceContext* context, GooInspector* inspector);

// Export traces to file
bool goo_trace_export(GooTraceContext* context, const char* filename);

// Export as Chrome Trace format
bool goo_trace_export_chrome(GooTraceContext* context, const char* filename);

// Export as Jaeger format
bool goo_trace_export_jaeger(GooTraceContext* context, const char* filename);

// === Utility and Convenience Functions ===

// Create a span with automatic cleanup
#define GOO_TRACE_SPAN_AUTO(context, name) \
    GooTraceSpan* __trace_span_##__LINE__ = goo_trace_span_start(context, name); \
    __attribute__((cleanup(goo_trace_span_cleanup))) GooTraceSpan* __trace_span_auto_##__LINE__ = __trace_span_##__LINE__

// Helper for automatic span cleanup
void goo_trace_span_cleanup(GooTraceSpan** span);

// Get the active trace span for the current thread
GooTraceSpan* goo_trace_get_current_span(GooTraceContext* context);

// Get the trace context from a span
GooTraceContext* goo_trace_span_get_context(GooTraceSpan* span);

// Get high-precision timestamp
uint64_t goo_trace_get_timestamp(void);

#endif // GOO_TRACE_H 