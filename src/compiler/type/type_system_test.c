/**
 * type_system_test.c
 * 
 * Test program for the enhanced type system of the Goo programming language
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "goo_type_system.h"
#include "../../../include/goo_diagnostics.h"

// Test structure to track test results
typedef struct {
    int passed;
    int failed;
    int total;
} TestResults;

// Helper function to print test result
void print_test_result(const char* test_name, bool passed) {
    printf("Test %-50s %s\n", test_name, passed ? "PASSED" : "FAILED");
}

// Helper function to update test results
void update_results(TestResults* results, bool passed) {
    results->total++;
    if (passed) {
        results->passed++;
    } else {
        results->failed++;
    }
}

// Helper function to run a test and update results
void run_test(const char* test_name, bool (*test_func)(void), TestResults* results) {
    bool passed = test_func();
    print_test_result(test_name, passed);
    update_results(results, passed);
}

// Test basic type creation
bool test_basic_type_creation() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* uint_type = goo_type_system_create_int_type(ctx, GOO_INT_64, false);
    GooType* float_type = goo_type_system_create_float_type(ctx, GOO_FLOAT_64);
    GooType* bool_type = goo_type_system_create_bool_type(ctx);
    GooType* char_type = goo_type_system_create_char_type(ctx);
    GooType* string_type = goo_type_system_create_string_type(ctx);
    
    bool success = (int_type != NULL && uint_type != NULL && float_type != NULL && 
                   bool_type != NULL && char_type != NULL && string_type != NULL);
    
    success = success && int_type->kind == GOO_TYPE_INT && int_type->int_info.width == GOO_INT_32 && 
              int_type->int_info.is_signed == true;
    
    success = success && uint_type->kind == GOO_TYPE_INT && uint_type->int_info.width == GOO_INT_64 && 
              uint_type->int_info.is_signed == false;
    
    success = success && float_type->kind == GOO_TYPE_FLOAT && 
              float_type->float_info.precision == GOO_FLOAT_64;
    
    success = success && bool_type->kind == GOO_TYPE_BOOL;
    success = success && char_type->kind == GOO_TYPE_CHAR;
    success = success && string_type->kind == GOO_TYPE_STRING;
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test composite type creation
bool test_composite_type_creation() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* array_type = goo_type_system_create_array_type(ctx, int_type, 10);
    GooType* slice_type = goo_type_system_create_slice_type(ctx, int_type);
    
    GooType* elem_types[3] = {
        int_type,
        goo_type_system_create_string_type(ctx),
        goo_type_system_create_bool_type(ctx)
    };
    
    GooType* tuple_type = goo_type_system_create_tuple_type(ctx, elem_types, 3);
    
    const char* field_names[2] = {"x", "y"};
    GooType* field_types[2] = {
        goo_type_system_create_int_type(ctx, GOO_INT_32, true),
        goo_type_system_create_int_type(ctx, GOO_INT_32, true)
    };
    
    GooType* struct_type = goo_type_system_create_struct_type(ctx, "Point", field_names, field_types, 2);
    
    const char* variant_names[3] = {"Red", "Green", "Blue"};
    GooType* enum_type = goo_type_system_create_enum_type(ctx, "Color", variant_names, NULL, 3);
    
    bool success = (array_type != NULL && slice_type != NULL && tuple_type != NULL && 
                   struct_type != NULL && enum_type != NULL);
    
    success = success && array_type->kind == GOO_TYPE_ARRAY && 
              goo_type_system_types_equal(ctx, array_type->array_info.element_type, int_type) &&
              array_type->array_info.size == 10;
    
    success = success && slice_type->kind == GOO_TYPE_SLICE && 
              goo_type_system_types_equal(ctx, slice_type->slice_info.element_type, int_type);
    
    success = success && tuple_type->kind == GOO_TYPE_TUPLE && 
              tuple_type->tuple_info.element_count == 3 &&
              goo_type_system_types_equal(ctx, tuple_type->tuple_info.element_types[0], int_type);
    
    success = success && struct_type->kind == GOO_TYPE_STRUCT && 
              strcmp(struct_type->struct_info.name, "Point") == 0 &&
              struct_type->struct_info.field_count == 2 &&
              strcmp(struct_type->struct_info.field_names[0], "x") == 0;
    
    success = success && enum_type->kind == GOO_TYPE_ENUM && 
              strcmp(enum_type->enum_info.name, "Color") == 0 &&
              enum_type->enum_info.variant_count == 3 &&
              strcmp(enum_type->enum_info.variant_names[0], "Red") == 0;
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test function and reference types
bool test_function_and_ref_types() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* string_type = goo_type_system_create_string_type(ctx);
    
    GooType* param_types[2] = {int_type, string_type};
    GooType* func_type = goo_type_system_create_function_type(ctx, int_type, param_types, 2, false, false);
    
    GooLifetime* lifetime = goo_type_system_create_lifetime(ctx, "a", false);
    GooType* ref_type = goo_type_system_create_ref_type(ctx, int_type, lifetime, false);
    GooType* mut_ref_type = goo_type_system_create_ref_type(ctx, int_type, lifetime, true);
    
    bool success = (func_type != NULL && ref_type != NULL && mut_ref_type != NULL);
    
    success = success && func_type->kind == GOO_TYPE_FUNCTION && 
              func_type->function_info.param_count == 2 &&
              goo_type_system_types_equal(ctx, func_type->function_info.return_type, int_type) &&
              goo_type_system_types_equal(ctx, func_type->function_info.param_types[0], int_type) &&
              goo_type_system_types_equal(ctx, func_type->function_info.param_types[1], string_type);
    
    success = success && ref_type->kind == GOO_TYPE_REF && 
              goo_type_system_types_equal(ctx, ref_type->ref_info.referenced_type, int_type) &&
              ref_type->ref_info.lifetime == lifetime;
    
    success = success && mut_ref_type->kind == GOO_TYPE_MUT_REF && 
              goo_type_system_types_equal(ctx, mut_ref_type->ref_info.referenced_type, int_type) &&
              mut_ref_type->ref_info.lifetime == lifetime;
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test type variable and unification
bool test_type_variables_and_unification() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooTypeVar* var1 = goo_type_system_create_type_var(ctx);
    GooTypeVar* var2 = goo_type_system_create_type_var(ctx);
    
    GooType* var_type1 = (GooType*)malloc(sizeof(GooType));
    var_type1->kind = GOO_TYPE_VAR;
    var_type1->type_var = var1;
    
    GooType* var_type2 = (GooType*)malloc(sizeof(GooType));
    var_type2->kind = GOO_TYPE_VAR;
    var_type2->type_var = var2;
    
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    
    bool success = (var1 != NULL && var2 != NULL && var1->id != var2->id);
    
    // Unify var1 with int_type
    success = success && goo_type_system_unify(ctx, var_type1, int_type);
    
    // Verify var1 is resolved to int_type
    success = success && var1->resolved_type != NULL && 
              var1->resolved_type->kind == GOO_TYPE_INT &&
              var1->resolved_type->int_info.width == GOO_INT_32;
    
    // Unify var2 with var1
    success = success && goo_type_system_unify(ctx, var_type2, var_type1);
    
    // Verify var2 is now also resolved to int_type
    success = success && var2->resolved_type != NULL && 
              var2->resolved_type->kind == GOO_TYPE_INT &&
              var2->resolved_type->int_info.width == GOO_INT_32;
    
    // Clean up manually allocated types
    free(var_type1);
    free(var_type2);
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test type conversion and subtyping
bool test_type_conversion_and_subtyping() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooType* i32_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* i64_type = goo_type_system_create_int_type(ctx, GOO_INT_64, true);
    GooType* u32_type = goo_type_system_create_int_type(ctx, GOO_INT_32, false);
    GooType* f32_type = goo_type_system_create_float_type(ctx, GOO_FLOAT_32);
    GooType* f64_type = goo_type_system_create_float_type(ctx, GOO_FLOAT_64);
    
    bool success = true;
    
    // i32 is a subtype of i64
    success = success && goo_type_system_is_subtype(ctx, i32_type, i64_type);
    
    // i64 is not a subtype of i32
    success = success && !goo_type_system_is_subtype(ctx, i64_type, i32_type);
    
    // u32 is not a subtype of i32 (different signedness)
    success = success && !goo_type_system_is_subtype(ctx, u32_type, i32_type);
    
    // f32 is a subtype of f64
    success = success && goo_type_system_is_subtype(ctx, f32_type, f64_type);
    
    // f64 is not a subtype of f32
    success = success && !goo_type_system_is_subtype(ctx, f64_type, f32_type);
    
    // Test array and slice subtyping
    GooType* i32_array = goo_type_system_create_array_type(ctx, i32_type, 10);
    GooType* i64_array = goo_type_system_create_array_type(ctx, i64_type, 10);
    
    // Arrays are invariant, so i32_array is not a subtype of i64_array
    success = success && !goo_type_system_is_subtype(ctx, i32_array, i64_array);
    
    GooType* i32_slice = goo_type_system_create_slice_type(ctx, i32_type);
    GooType* i64_slice = goo_type_system_create_slice_type(ctx, i64_type);
    
    // Slices are covariant, so i32_slice should be a subtype of i64_slice
    success = success && goo_type_system_is_subtype(ctx, i32_slice, i64_slice);
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test trait system
bool test_trait_system() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    // Create a simple trait with methods
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* string_type = goo_type_system_create_string_type(ctx);
    
    GooType* method_types[2];
    
    // First method: to_string() -> string
    method_types[0] = goo_type_system_create_function_type(ctx, string_type, NULL, 0, false, false);
    
    // Second method: hash() -> i32
    method_types[1] = goo_type_system_create_function_type(ctx, int_type, NULL, 0, false, false);
    
    const char* method_names[2] = {"to_string", "hash"};
    
    GooTrait* hashable_trait = goo_type_system_create_trait(ctx, "Hashable", method_names, method_types, 2);
    
    // Create a struct type and implement the trait
    const char* point_fields[2] = {"x", "y"};
    GooType* point_field_types[2] = {int_type, int_type};
    
    GooType* point_type = goo_type_system_create_struct_type(
        ctx, "Point", point_fields, point_field_types, 2);
    
    GooTypeImpl* point_impl = goo_type_system_create_impl(ctx, point_type, hashable_trait, NULL, 0);
    goo_type_system_add_method_impl(ctx, point_impl, "to_string");
    goo_type_system_add_method_impl(ctx, point_impl, "hash");
    
    // Check if Point implements Hashable
    bool success = goo_type_system_type_implements_trait(ctx, point_type, hashable_trait, NULL);
    
    // Create a type variable with a trait constraint
    GooTypeVar* type_var = goo_type_system_create_type_var(ctx);
    goo_type_system_add_trait_constraint(ctx, type_var, hashable_trait);
    
    GooType* var_type = (GooType*)malloc(sizeof(GooType));
    var_type->kind = GOO_TYPE_VAR;
    var_type->type_var = type_var;
    
    // Test unification with trait constraints
    success = success && goo_type_system_unify(ctx, var_type, point_type);
    
    // Create another struct that doesn't implement Hashable
    GooType* no_impl_type = goo_type_system_create_struct_type(
        ctx, "NoImpl", point_fields, point_field_types, 2);
    
    // Check that it doesn't implement Hashable
    success = success && !goo_type_system_type_implements_trait(ctx, no_impl_type, hashable_trait, NULL);
    
    // Create a trait object type
    GooType* trait_obj_type = (GooType*)malloc(sizeof(GooType));
    trait_obj_type->kind = GOO_TYPE_TRAIT_OBJECT;
    trait_obj_type->trait_object_info.trait = hashable_trait;
    trait_obj_type->trait_object_info.lifetime = NULL;
    
    // Check if point_type is a subtype of the trait object type
    success = success && goo_type_system_is_subtype(ctx, point_type, trait_obj_type);
    
    // Clean up manually allocated types
    free(var_type);
    free(trait_obj_type);
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Test type to string conversion
bool test_type_to_string() {
    GooDiagnosticContext* diag_ctx = goo_diagnostic_context_create();
    GooTypeContext* ctx = goo_type_system_create(diag_ctx);
    
    GooType* int_type = goo_type_system_create_int_type(ctx, GOO_INT_32, true);
    GooType* uint_type = goo_type_system_create_int_type(ctx, GOO_INT_64, false);
    GooType* float_type = goo_type_system_create_float_type(ctx, GOO_FLOAT_64);
    GooType* bool_type = goo_type_system_create_bool_type(ctx);
    GooType* string_type = goo_type_system_create_string_type(ctx);
    
    GooType* array_type = goo_type_system_create_array_type(ctx, int_type, 10);
    
    GooType* param_types[2] = {int_type, string_type};
    GooType* func_type = goo_type_system_create_function_type(ctx, bool_type, param_types, 2, false, false);
    
    char buffer[256];
    
    goo_type_system_type_to_string(ctx, int_type, buffer, sizeof(buffer));
    bool success = strcmp(buffer, "i32") == 0;
    
    goo_type_system_type_to_string(ctx, uint_type, buffer, sizeof(buffer));
    success = success && strcmp(buffer, "u64") == 0;
    
    goo_type_system_type_to_string(ctx, array_type, buffer, sizeof(buffer));
    success = success && strcmp(buffer, "[i32; 10]") == 0;
    
    goo_type_system_type_to_string(ctx, func_type, buffer, sizeof(buffer));
    success = success && strcmp(buffer, "fn(i32, string) -> bool") == 0;
    
    goo_type_system_destroy(ctx);
    goo_diagnostic_context_destroy(diag_ctx);
    
    return success;
}

// Main function to run all tests
int main(int argc, char** argv) {
    printf("Goo Type System Tests\n");
    printf("=====================\n");
    
    TestResults results = {0, 0, 0};
    
    run_test("Basic Type Creation", test_basic_type_creation, &results);
    run_test("Composite Type Creation", test_composite_type_creation, &results);
    run_test("Function and Reference Types", test_function_and_ref_types, &results);
    run_test("Type Variables and Unification", test_type_variables_and_unification, &results);
    run_test("Type Conversion and Subtyping", test_type_conversion_and_subtyping, &results);
    run_test("Trait System", test_trait_system, &results);
    run_test("Type to String Conversion", test_type_to_string, &results);
    
    printf("\nTest Summary: %d tests, %d passed, %d failed\n", 
          results.total, results.passed, results.failed);
    
    return results.failed == 0 ? 0 : 1;
} 