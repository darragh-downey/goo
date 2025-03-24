/**
 * @file comptime.h
 * @brief Compile-time evaluation for the Goo programming language.
 *
 * This header provides functions for compile-time code evaluation, 
 * allowing constant expressions to be evaluated during compilation.
 */

#ifndef GOO_COMPTIME_H
#define GOO_COMPTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque type representing a compile-time context.
 */
typedef struct GooComptimeContext GooComptimeContext;

/**
 * @brief Opaque type representing a compile-time value.
 */
typedef struct GooComptimeValue GooComptimeValue;

/**
 * @brief Opaque type representing a compile-time expression.
 */
typedef struct GooComptimeExpression GooComptimeExpression;

/**
 * @brief Enumeration of compile-time value types.
 */
typedef enum GooComptimeValueType {
    GOO_COMPTIME_VALUE_INT,
    GOO_COMPTIME_VALUE_FLOAT,
    GOO_COMPTIME_VALUE_BOOL,
    GOO_COMPTIME_VALUE_STRING,
    GOO_COMPTIME_VALUE_TYPE,
    GOO_COMPTIME_VALUE_STRUCT,
    GOO_COMPTIME_VALUE_ARRAY,
    GOO_COMPTIME_VALUE_NULL
} GooComptimeValueType;

/**
 * @brief Enumeration of binary operations.
 */
typedef enum GooComptimeBinaryOp {
    GOO_COMPTIME_OP_ADD,
    GOO_COMPTIME_OP_SUB,
    GOO_COMPTIME_OP_MUL,
    GOO_COMPTIME_OP_DIV,
    GOO_COMPTIME_OP_MOD,
    GOO_COMPTIME_OP_EQ,
    GOO_COMPTIME_OP_NEQ,
    GOO_COMPTIME_OP_LT,
    GOO_COMPTIME_OP_LE,
    GOO_COMPTIME_OP_GT,
    GOO_COMPTIME_OP_GE,
    GOO_COMPTIME_OP_AND,
    GOO_COMPTIME_OP_OR,
    GOO_COMPTIME_OP_BIT_AND,
    GOO_COMPTIME_OP_BIT_OR,
    GOO_COMPTIME_OP_BIT_XOR,
    GOO_COMPTIME_OP_SHL,
    GOO_COMPTIME_OP_SHR
} GooComptimeBinaryOp;

/**
 * @brief Initialize the compile-time evaluation subsystem.
 *
 * @return true if initialization succeeds, false otherwise.
 */
bool goo_comptime_init(void);

/**
 * @brief Clean up the compile-time evaluation subsystem.
 */
void goo_comptime_cleanup(void);

/**
 * @brief Create a new compile-time context.
 *
 * @return A pointer to the new context, or NULL if creation fails.
 */
GooComptimeContext* goo_comptime_context_create(void);

/**
 * @brief Destroy a compile-time context.
 *
 * @param context The context to destroy.
 */
void goo_comptime_context_destroy(GooComptimeContext* context);

/**
 * @brief Create a compile-time integer value.
 *
 * @param context The context to use.
 * @param value The integer value.
 * @return A pointer to the new value, or NULL if creation fails.
 */
GooComptimeValue* goo_comptime_create_int(GooComptimeContext* context, int64_t value);

/**
 * @brief Create a compile-time float value.
 *
 * @param context The context to use.
 * @param value The float value.
 * @return A pointer to the new value, or NULL if creation fails.
 */
GooComptimeValue* goo_comptime_create_float(GooComptimeContext* context, double value);

/**
 * @brief Create a compile-time boolean value.
 *
 * @param context The context to use.
 * @param value The boolean value.
 * @return A pointer to the new value, or NULL if creation fails.
 */
GooComptimeValue* goo_comptime_create_bool(GooComptimeContext* context, bool value);

/**
 * @brief Create a compile-time string value.
 *
 * @param context The context to use.
 * @param value The string value.
 * @return A pointer to the new value, or NULL if creation fails.
 */
GooComptimeValue* goo_comptime_create_string(GooComptimeContext* context, const char* value);

/**
 * @brief Destroy a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to destroy.
 */
void goo_comptime_destroy_value(GooComptimeContext* context, GooComptimeValue* value);

/**
 * @brief Get the type of a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to get the type of.
 * @return The type of the value.
 */
GooComptimeValueType goo_comptime_get_value_type(GooComptimeContext* context, GooComptimeValue* value);

/**
 * @brief Get the integer value of a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_value Pointer to store the integer value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_comptime_get_int(GooComptimeContext* context, GooComptimeValue* value, int64_t* out_value);

/**
 * @brief Get the float value of a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_value Pointer to store the float value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_comptime_get_float(GooComptimeContext* context, GooComptimeValue* value, double* out_value);

/**
 * @brief Get the boolean value of a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_value Pointer to store the boolean value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_comptime_get_bool(GooComptimeContext* context, GooComptimeValue* value, bool* out_value);

/**
 * @brief Get the string value of a compile-time value.
 *
 * @param context The context to use.
 * @param value The value to get.
 * @param out_value Pointer to store the string value.
 * @return true if getting succeeds, false otherwise.
 */
bool goo_comptime_get_string(GooComptimeContext* context, GooComptimeValue* value, const char** out_value);

/**
 * @brief Convert a compile-time value to a string representation.
 *
 * @param context The context to use.
 * @param value The value to convert.
 * @return A string representation of the value, or NULL if conversion fails.
 *         The caller is responsible for freeing the string.
 */
char* goo_comptime_value_to_string(GooComptimeContext* context, GooComptimeValue* value);

/**
 * @brief Evaluate a binary operation at compile time.
 *
 * @param context The context to use.
 * @param left The left operand.
 * @param op The operation to perform.
 * @param right The right operand.
 * @return A pointer to the result value, or NULL if evaluation fails.
 */
GooComptimeValue* goo_comptime_eval_binary_op(GooComptimeContext* context, GooComptimeValue* left, GooComptimeBinaryOp op, GooComptimeValue* right);

/**
 * @brief Convenience function to evaluate addition at compile time.
 *
 * @param context The context to use.
 * @param left The left operand.
 * @param right The right operand.
 * @return A pointer to the result value, or NULL if evaluation fails.
 */
GooComptimeValue* goo_comptime_eval_add(GooComptimeContext* context, GooComptimeValue* left, GooComptimeValue* right);

/**
 * @brief Convenience function to evaluate subtraction at compile time.
 *
 * @param context The context to use.
 * @param left The left operand.
 * @param right The right operand.
 * @return A pointer to the result value, or NULL if evaluation fails.
 */
GooComptimeValue* goo_comptime_eval_sub(GooComptimeContext* context, GooComptimeValue* left, GooComptimeValue* right);

/**
 * @brief Convenience function to evaluate multiplication at compile time.
 *
 * @param context The context to use.
 * @param left The left operand.
 * @param right The right operand.
 * @return A pointer to the result value, or NULL if evaluation fails.
 */
GooComptimeValue* goo_comptime_eval_mul(GooComptimeContext* context, GooComptimeValue* left, GooComptimeValue* right);

/**
 * @brief Convenience function to evaluate division at compile time.
 *
 * @param context The context to use.
 * @param left The left operand.
 * @param right The right operand.
 * @return A pointer to the result value, or NULL if evaluation fails.
 */
GooComptimeValue* goo_comptime_eval_div(GooComptimeContext* context, GooComptimeValue* left, GooComptimeValue* right);

#ifdef __cplusplus
}
#endif

#endif /* GOO_COMPTIME_H */ 