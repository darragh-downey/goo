/**
 * goo_type_system.h
 * 
 * Enhanced type system for the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#ifndef GOO_TYPE_SYSTEM_H
#define GOO_TYPE_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "type_table_simple.h"
#include "stub_headers/diagnostics_include.h"

// Forward declarations
typedef struct GooType GooType;
typedef struct GooAstNode GooAstNode;
typedef struct GooTypeConstraint GooTypeConstraint;
typedef struct GooTypeVar GooTypeVar;
typedef struct GooTrait GooTrait;
typedef struct GooTypeImpl GooTypeImpl;
typedef struct GooTypeContext GooTypeContext;
typedef struct GooLifetime GooLifetime;
typedef struct GooRegion GooRegion;

/**
 * Type variable used for type inference
 */
typedef struct GooTypeVar {
    uint32_t id;                  // Unique identifier
    GooType* resolved_type;       // The resolved type, or NULL if unresolved
    GooTypeConstraint* constraints; // Constraints on this type variable
} GooTypeVar;

/**
 * Trait definition
 */
typedef struct GooTrait {
    char* name;                   // Trait name
    char** method_names;          // Names of required methods
    GooType** method_types;       // Types of required methods
    int method_count;             // Number of methods
    GooTypeVar** type_params;     // Type parameters for this trait
    int type_param_count;         // Number of type parameters
    struct GooTrait** super_traits; // Super traits
    int super_trait_count;        // Number of super traits
} GooTrait;

/**
 * Type implementation for a trait
 */
typedef struct GooTypeImpl {
    GooType* type;                // Type that implements
    GooTrait* trait;              // Trait being implemented
    GooType** type_args;          // Type arguments (for generic traits)
    int type_arg_count;           // Number of type arguments
    char** method_impls;          // Names of implementing methods
    int method_count;             // Number of methods
} GooTypeImpl;

/**
 * Lifetime annotation for memory safety
 */
typedef struct GooLifetime {
    char* name;                   // Lifetime name
    int scope_level;              // Lexical scope level
    bool is_static;               // Whether this is a static lifetime
    bool is_anonymous;            // Whether this is an anonymous lifetime
} GooLifetime;

/**
 * Memory region for memory safety analysis
 */
typedef struct GooRegion {
    char* name;                   // Region name
    GooLifetime* lifetime;        // Lifetime of this region
    bool is_mutable;              // Whether this region allows mutable access
    int borrow_count;             // Number of active borrows
    int mut_borrow_count;         // Number of active mutable borrows
} GooRegion;

/**
 * Enhanced type constraint
 */
typedef struct GooTypeConstraint {
    enum {
        GOO_CONSTRAINT_SUBTYPE,   // Type must be a subtype of another
        GOO_CONSTRAINT_TRAIT,     // Type must implement a trait
        GOO_CONSTRAINT_EQUALITY,  // Type must be equal to another
        GOO_CONSTRAINT_REGION,    // Type must be in a specific memory region
        GOO_CONSTRAINT_LIFETIME   // Type must have at least a specific lifetime
    } kind;
    
    union {
        GooType* subtype_of;      // For subtype constraint
        GooTrait* trait;          // For trait constraint
        GooType* equal_to;        // For equality constraint
        GooRegion* region;        // For region constraint
        GooLifetime* lifetime;    // For lifetime constraint
    };
    
    struct GooTypeConstraint* next;  // Linked list of constraints
} GooTypeConstraint;

/**
 * Enhanced type kind
 */
typedef enum {
    // Basic types
    GOO_TYPE_VOID,             // void type
    GOO_TYPE_UNIT,             // Unit type (empty tuple, like () in Rust)
    GOO_TYPE_BOOL,             // Boolean type
    GOO_TYPE_INT,              // Integer type
    GOO_TYPE_UINT,             // Unsigned integer type
    GOO_TYPE_FLOAT,            // Floating point type
    GOO_TYPE_CHAR,             // Character type
    GOO_TYPE_STRING,           // String type
    
    // Composite types
    GOO_TYPE_ARRAY,            // Array type
    GOO_TYPE_SLICE,            // Slice type
    GOO_TYPE_TUPLE,            // Tuple type
    GOO_TYPE_STRUCT,           // Structure type
    GOO_TYPE_ENUM,             // Enumeration type
    GOO_TYPE_UNION,            // Union type
    
    // Function types
    GOO_TYPE_FUNCTION,         // Function type
    GOO_TYPE_CLOSURE,          // Closure type
    
    // Reference types
    GOO_TYPE_REF,              // Immutable reference
    GOO_TYPE_MUT_REF,          // Mutable reference
    GOO_TYPE_OWNED,            // Owned type (like Box<T>)
    
    // Concurrency types
    GOO_TYPE_CHANNEL,          // Channel type
    GOO_TYPE_GO_ROUTINE,       // Goroutine type
    
    // Generic types
    GOO_TYPE_VAR,              // Type variable
    GOO_TYPE_PARAM,            // Type parameter
    
    // Special types
    GOO_TYPE_TRAIT_OBJECT,     // Trait object type
    GOO_TYPE_ERROR,            // Error type
    GOO_TYPE_NEVER,            // Never type (bottom type)
    GOO_TYPE_UNKNOWN,          // Unknown type (for errors)
    GOO_TYPE_ANY,              // Any type (top type)
    
    // Type constructors
    GOO_TYPE_TYPE_CONSTRUCTOR  // Type constructor for generic types
} GooTypeKind;

/**
 * Integer bit width
 */
typedef enum {
    GOO_INT_8,
    GOO_INT_16,
    GOO_INT_32,
    GOO_INT_64,
    GOO_INT_128,
    GOO_INT_SIZE  // For size_t/usize/isize
} GooIntWidth;

/**
 * Float precision
 */
typedef enum {
    GOO_FLOAT_32,
    GOO_FLOAT_64
} GooFloatPrecision;

/**
 * Enhanced type representation
 */
struct GooType {
    GooTypeKind kind;            // Type kind
    
    union {
        // Integer type info
        struct {
            GooIntWidth width;
            bool is_signed;
        } int_info;
        
        // Float type info
        struct {
            GooFloatPrecision precision;
        } float_info;
        
        // Array type info
        struct {
            GooType* element_type;
            size_t size;  // 0 for flexible size
        } array_info;
        
        // Slice type info
        struct {
            GooType* element_type;
        } slice_info;
        
        // Tuple type info
        struct {
            GooType** element_types;
            int element_count;
        } tuple_info;
        
        // Struct type info
        struct {
            char* name;
            char** field_names;
            GooType** field_types;
            int field_count;
            GooTypeImpl** impls;
            int impl_count;
        } struct_info;
        
        // Enum type info
        struct {
            char* name;
            char** variant_names;
            GooType** variant_types;  // NULL for simple variants
            int variant_count;
        } enum_info;
        
        // Union type info
        struct {
            char* name;
            GooType** member_types;
            int member_count;
        } union_info;
        
        // Function type info
        struct {
            GooType* return_type;
            GooType** param_types;
            int param_count;
            GooLifetime** param_lifetimes;  // Can be NULL
            bool is_unsafe;
            bool is_kernel;
        } function_info;
        
        // Closure type info
        struct {
            GooType* function_type;
            GooType** capture_types;
            int capture_count;
        } closure_info;
        
        // Reference type info
        struct {
            GooType* referenced_type;
            GooLifetime* lifetime;  // Can be NULL
            GooRegion* region;     // Can be NULL
        } ref_info;
        
        // Channel type info
        struct {
            GooType* element_type;
            int buffer_size;  // 0 for unbuffered
            bool is_distributed;
        } channel_info;
        
        // Type variable info
        GooTypeVar* type_var;
        
        // Type parameter info
        struct {
            char* name;
            GooTypeConstraint* constraints;
        } type_param_info;
        
        // Trait object info
        struct {
            GooTrait* trait;
            GooLifetime* lifetime;  // For trait objects with lifetime
        } trait_object_info;
        
        // Type constructor info
        struct {
            char* name;
            GooTypeVar** type_params;
            int type_param_count;
            GooType* template_type;  // The type with parameters replaced by vars
        } constructor_info;
    };
    
    // Metadata for all types
    bool is_capability;      // Whether this type has capability semantics
    bool is_copyable;        // Whether this type can be trivially copied
    bool is_sized;           // Whether this type has a known size at compile time
    bool is_thread_safe;     // Whether this type is safe to share between threads
};

/**
 * Type context for type checking and inference
 */
struct GooTypeContext {
    GooTypeTable* type_table;         // Type table for storing types
    GooTypeVar** type_vars;           // Type variables in the current context
    int type_var_count;               // Number of type variables
    int next_type_var_id;             // Next ID for type variables
    GooLifetime** lifetimes;          // Lifetimes in the current context
    int lifetime_count;               // Number of lifetimes
    int current_scope_level;          // Current lexical scope level
    GooTypeVar* self_type;            // 'self' type for method resolution
    GooDiagnosticContext* diagnostics; // Diagnostics context
    bool in_unsafe_block;              // Whether we're in an unsafe block
};

// Type system initialization and cleanup
GooTypeContext* goo_type_system_create(GooDiagnosticContext* diagnostics);
void goo_type_system_destroy(GooTypeContext* ctx);

// Type creation
GooType* goo_type_system_create_int_type(GooTypeContext* ctx, GooIntWidth width, bool is_signed);
GooType* goo_type_system_create_float_type(GooTypeContext* ctx, GooFloatPrecision precision);
GooType* goo_type_system_create_bool_type(GooTypeContext* ctx);
GooType* goo_type_system_create_char_type(GooTypeContext* ctx);
GooType* goo_type_system_create_string_type(GooTypeContext* ctx);
GooType* goo_type_system_create_array_type(GooTypeContext* ctx, GooType* element_type, size_t size);
GooType* goo_type_system_create_slice_type(GooTypeContext* ctx, GooType* element_type);
GooType* goo_type_system_create_tuple_type(GooTypeContext* ctx, GooType** element_types, int element_count);
GooType* goo_type_system_create_struct_type(GooTypeContext* ctx, const char* name, const char** field_names, GooType** field_types, int field_count);
GooType* goo_type_system_create_enum_type(GooTypeContext* ctx, const char* name, const char** variant_names, GooType** variant_types, int variant_count);
GooType* goo_type_system_create_function_type(GooTypeContext* ctx, GooType* return_type, GooType** param_types, int param_count, bool is_unsafe, bool is_kernel);
GooType* goo_type_system_create_ref_type(GooTypeContext* ctx, GooType* referenced_type, GooLifetime* lifetime, bool is_mutable);
GooType* goo_type_system_create_channel_type(GooTypeContext* ctx, GooType* element_type, int buffer_size, bool is_distributed);

// Type variables and constraints
GooTypeVar* goo_type_system_create_type_var(GooTypeContext* ctx);
void goo_type_system_add_trait_constraint(GooTypeContext* ctx, GooTypeVar* var, GooTrait* trait);
void goo_type_system_add_subtype_constraint(GooTypeContext* ctx, GooTypeVar* var, GooType* super_type);
void goo_type_system_add_equality_constraint(GooTypeContext* ctx, GooTypeVar* var, GooType* type);
bool goo_type_system_unify(GooTypeContext* ctx, GooType* type1, GooType* type2);

// Trait system
GooTrait* goo_type_system_create_trait(GooTypeContext* ctx, const char* name, const char** method_names, GooType** method_types, int method_count);
GooTypeImpl* goo_type_system_create_impl(GooTypeContext* ctx, GooType* type, GooTrait* trait, GooType** type_args, int type_arg_count);
bool goo_type_system_type_implements_trait(GooTypeContext* ctx, GooType* type, GooTrait* trait, GooType*** out_type_args);

// Type checking core
GooType* goo_type_system_check_expr(GooTypeContext* ctx, GooAstNode* expr);
GooType* goo_type_system_check_stmt(GooTypeContext* ctx, GooAstNode* stmt);
bool goo_type_system_check_module(GooTypeContext* ctx, GooAstNode* module);

// Type checking for specific constructs
GooType* goo_type_system_check_var_decl(GooTypeContext* ctx, GooAstNode* var_decl);
GooType* goo_type_system_check_function_decl(GooTypeContext* ctx, GooAstNode* function_decl);
GooType* goo_type_system_check_call_expr(GooTypeContext* ctx, GooAstNode* call_expr);
GooType* goo_type_system_check_binary_expr(GooTypeContext* ctx, GooAstNode* binary_expr);
GooType* goo_type_system_check_unary_expr(GooTypeContext* ctx, GooAstNode* unary_expr);
GooType* goo_type_system_check_if_stmt(GooTypeContext* ctx, GooAstNode* if_stmt);
GooType* goo_type_system_check_for_stmt(GooTypeContext* ctx, GooAstNode* for_stmt);
GooType* goo_type_system_check_return_stmt(GooTypeContext* ctx, GooAstNode* return_stmt);
GooType* goo_type_system_check_channel_send(GooTypeContext* ctx, GooAstNode* send_expr);
GooType* goo_type_system_check_channel_recv(GooTypeContext* ctx, GooAstNode* recv_expr);

// Lifetime and region analysis
GooLifetime* goo_type_system_create_lifetime(GooTypeContext* ctx, const char* name, bool is_static);
GooRegion* goo_type_system_create_region(GooTypeContext* ctx, const char* name, GooLifetime* lifetime, bool is_mutable);
void goo_type_system_enter_scope(GooTypeContext* ctx);
void goo_type_system_exit_scope(GooTypeContext* ctx);
bool goo_type_system_check_borrow(GooTypeContext* ctx, GooRegion* region, bool is_mutable);
bool goo_type_system_release_borrow(GooTypeContext* ctx, GooRegion* region, bool is_mutable);
bool goo_type_system_lifetime_outlives(GooTypeContext* ctx, GooLifetime* a, GooLifetime* b);

// Utility functions
bool goo_type_system_is_subtype(GooTypeContext* ctx, GooType* sub, GooType* super);
bool goo_type_system_types_equal(GooTypeContext* ctx, GooType* a, GooType* b);
const char* goo_type_system_type_to_string(GooTypeContext* ctx, GooType* type, char* buffer, size_t buffer_size);
GooType* goo_type_system_resolve_type_var(GooTypeContext* ctx, GooTypeVar* var);
GooType* goo_type_system_resolve_type(GooTypeContext* ctx, GooType* type);

#endif /* GOO_TYPE_SYSTEM_H */ 