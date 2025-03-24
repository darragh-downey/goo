/**
 * comptime_wrapper.c
 * 
 * C wrapper for the Zig implementation of compile-time evaluation features.
 * This file provides the C API for compile-time code generation,
 * evaluation, and reflection.
 */

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../include/memory.h"

// Forward declarations for Zig functions
extern bool comptimeInit(void);
extern void comptimeCleanup(void);
extern void* comptimeContextCreate(void);
extern void comptimeContextDestroy(void* ctx);
extern void* comptimeCreateIntValue(void* ctx, int64_t value);
extern void* comptimeCreateFloatValue(void* ctx, double value);
extern void* comptimeCreateBoolValue(void* ctx, bool value);
extern void* comptimeCreateStringValue(void* ctx, const char* value);
extern void comptimeDestroyValue(void* ctx, void* value);
extern const char* comptimeValueToString(void* ctx, void* value, size_t* out_len);
extern void comptimeFreeString(void* ctx, const char* str, size_t len);
extern void* comptimeEvalBinaryAdd(void* ctx, void* left, void* right);
extern void* comptimeEvalBinarySub(void* ctx, void* left, void* right);
extern void* comptimeEvalBinaryMul(void* ctx, void* left, void* right);
extern void* comptimeEvalBinaryDiv(void* ctx, void* left, void* right);

// Opaque types for C interface
typedef struct GooComptimeContext GooComptimeContext;
typedef struct GooComptimeValue GooComptimeValue;
typedef struct GooComptimeExpression GooComptimeExpression;

// Enum for comptime value types
typedef enum {
    GOO_COMPTIME_INT,
    GOO_COMPTIME_FLOAT,
    GOO_COMPTIME_BOOL,
    GOO_COMPTIME_STRING,
    GOO_COMPTIME_TYPE,
    GOO_COMPTIME_STRUCT,
    GOO_COMPTIME_ARRAY,
    GOO_COMPTIME_NULL
} GooComptimeValueType;

// Enum for binary operations
typedef enum {
    GOO_COMPTIME_OP_ADD,
    GOO_COMPTIME_OP_SUB,
    GOO_COMPTIME_OP_MUL,
    GOO_COMPTIME_OP_DIV
} GooComptimeBinaryOp;

// C wrappers for comptime functionality

/**
 * Initialize the compile-time evaluation subsystem.
 * 
 * @return true if initialization was successful, false otherwise
 */
bool goo_comptime_init(void) {
    return comptimeInit();
}

/**
 * Clean up the compile-time evaluation subsystem.
 */
void goo_comptime_cleanup(void) {
    comptimeCleanup();
}

/**
 * Create a new compile-time evaluation context.
 * 
 * @return A new context, or NULL if creation failed
 */
GooComptimeContext* goo_comptime_context_create(void) {
    return (GooComptimeContext*)comptimeContextCreate();
}

/**
 * Destroy a compile-time evaluation context.
 * 
 * @param ctx The context to destroy
 */
void goo_comptime_context_destroy(GooComptimeContext* ctx) {
    comptimeContextDestroy(ctx);
}

/**
 * Create an integer compile-time value.
 * 
 * @param ctx The compile-time context
 * @param value The integer value
 * @return A new compile-time value, or NULL if creation failed
 */
GooComptimeValue* goo_comptime_create_int(GooComptimeContext* ctx, int64_t value) {
    return (GooComptimeValue*)comptimeCreateIntValue(ctx, value);
}

/**
 * Create a floating-point compile-time value.
 * 
 * @param ctx The compile-time context
 * @param value The floating-point value
 * @return A new compile-time value, or NULL if creation failed
 */
GooComptimeValue* goo_comptime_create_float(GooComptimeContext* ctx, double value) {
    return (GooComptimeValue*)comptimeCreateFloatValue(ctx, value);
}

/**
 * Create a boolean compile-time value.
 * 
 * @param ctx The compile-time context
 * @param value The boolean value
 * @return A new compile-time value, or NULL if creation failed
 */
GooComptimeValue* goo_comptime_create_bool(GooComptimeContext* ctx, bool value) {
    return (GooComptimeValue*)comptimeCreateBoolValue(ctx, value);
}

/**
 * Create a string compile-time value.
 * 
 * @param ctx The compile-time context
 * @param value The string value
 * @return A new compile-time value, or NULL if creation failed
 */
GooComptimeValue* goo_comptime_create_string(GooComptimeContext* ctx, const char* value) {
    return (GooComptimeValue*)comptimeCreateStringValue(ctx, value);
}

/**
 * Destroy a compile-time value.
 * 
 * @param ctx The compile-time context
 * @param value The value to destroy
 */
void goo_comptime_destroy_value(GooComptimeContext* ctx, GooComptimeValue* value) {
    comptimeDestroyValue(ctx, value);
}

/**
 * Convert a compile-time value to a string representation.
 * The returned string must be freed with goo_comptime_free_string.
 * 
 * @param ctx The compile-time context
 * @param value The value to convert
 * @param out_len Pointer to store the length of the resulting string
 * @return The string representation, or NULL if conversion failed
 */
const char* goo_comptime_value_to_string(GooComptimeContext* ctx, GooComptimeValue* value, size_t* out_len) {
    return comptimeValueToString(ctx, value, out_len);
}

/**
 * Free a string returned by goo_comptime_value_to_string.
 * 
 * @param ctx The compile-time context
 * @param str The string to free
 * @param len The length of the string
 */
void goo_comptime_free_string(GooComptimeContext* ctx, const char* str, size_t len) {
    comptimeFreeString(ctx, str, len);
}

/**
 * Evaluate a binary addition operation at compile time.
 * 
 * @param ctx The compile-time context
 * @param left The left operand
 * @param right The right operand
 * @return The result of the operation, or NULL if evaluation failed
 */
GooComptimeValue* goo_comptime_eval_add(GooComptimeContext* ctx, GooComptimeValue* left, GooComptimeValue* right) {
    return (GooComptimeValue*)comptimeEvalBinaryAdd(ctx, left, right);
}

/**
 * Evaluate a binary subtraction operation at compile time.
 * 
 * @param ctx The compile-time context
 * @param left The left operand
 * @param right The right operand
 * @return The result of the operation, or NULL if evaluation failed
 */
GooComptimeValue* goo_comptime_eval_sub(GooComptimeContext* ctx, GooComptimeValue* left, GooComptimeValue* right) {
    return (GooComptimeValue*)comptimeEvalBinarySub(ctx, left, right);
}

/**
 * Evaluate a binary multiplication operation at compile time.
 * 
 * @param ctx The compile-time context
 * @param left The left operand
 * @param right The right operand
 * @return The result of the operation, or NULL if evaluation failed
 */
GooComptimeValue* goo_comptime_eval_mul(GooComptimeContext* ctx, GooComptimeValue* left, GooComptimeValue* right) {
    return (GooComptimeValue*)comptimeEvalBinaryMul(ctx, left, right);
}

/**
 * Evaluate a binary division operation at compile time.
 * 
 * @param ctx The compile-time context
 * @param left The left operand
 * @param right The right operand
 * @return The result of the operation, or NULL if evaluation failed
 */
GooComptimeValue* goo_comptime_eval_div(GooComptimeContext* ctx, GooComptimeValue* left, GooComptimeValue* right) {
    return (GooComptimeValue*)comptimeEvalBinaryDiv(ctx, left, right);
}

/**
 * Evaluate a binary operation at compile time.
 * 
 * @param ctx The compile-time context
 * @param op The operation to perform
 * @param left The left operand
 * @param right The right operand
 * @return The result of the operation, or NULL if evaluation failed
 */
GooComptimeValue* goo_comptime_eval_binary_op(GooComptimeContext* ctx, GooComptimeBinaryOp op, 
                                             GooComptimeValue* left, GooComptimeValue* right) {
    switch (op) {
        case GOO_COMPTIME_OP_ADD:
            return goo_comptime_eval_add(ctx, left, right);
        case GOO_COMPTIME_OP_SUB:
            return goo_comptime_eval_sub(ctx, left, right);
        case GOO_COMPTIME_OP_MUL:
            return goo_comptime_eval_mul(ctx, left, right);
        case GOO_COMPTIME_OP_DIV:
            return goo_comptime_eval_div(ctx, left, right);
        default:
            // Unsupported operation
            return NULL;
    }
} 