/**
 * @file example.c
 * @brief Example program to demonstrate the Goo diagnostics system
 */

#include "diagnostics.h"
#include "error_catalog.h"
#include "source_highlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Example source code with errors
const char* example_source = 
    "fn factorial(n: int) -> int {\n"
    "    if n <= 1 {\n"
    "        return 1\n"       // Missing semicolon
    "    } else {\n"
    "        return n * factorial(n - 1);\n"
    "    }\n"
    "}\n"
    "\n"
    "fn main() {\n"
    "    let result: int = factorial(5);\n"
    "    println(\"Factorial of 5 is: {}\" result);\n"  // Missing comma
    "    \n"
    "    let value: int = \"not an integer\";\n"        // Type mismatch
    "    \n"
    "    let mut numbers = [1, 2, 3];\n"
    "    let ref1 = &numbers;\n"
    "    let ref2 = &mut numbers;\n"             // Borrow checker error
    "    \n"
    "    println(\"First number: {}\", ref1[0]);\n"
    "    println(\"Modified number: {}\", ref2[0]);\n"
    "}\n";

int main() {
    printf("Goo Diagnostics System Example\n");
    printf("===============================\n\n");
    
    // Initialize the error catalog
    if (!goo_error_catalog_init()) {
        fprintf(stderr, "Failed to initialize error catalog\n");
        return 1;
    }
    
    // Create a diagnostic context
    GooDiagnosticContext* ctx = goo_diag_context_new();
    if (!ctx) {
        fprintf(stderr, "Failed to create diagnostic context\n");
        goo_error_catalog_cleanup();
        return 1;
    }
    
    // Create some example diagnostics
    
    // 1. Missing semicolon
    GooDiagnostic* diag1 = goo_diag_new(
        GOO_DIAG_ERROR,
        "example.goo",
        3, 17, 1,
        "expected ';'"
    );
    
    goo_diag_add_child(
        diag1, GOO_DIAG_NOTE,
        "example.goo", 3, 17, 1,
        "statements must end with semicolons"
    );
    
    goo_diag_add_suggestion(
        diag1,
        "example.goo", 3, 17, 1,
        "add a semicolon",
        "        return 1;",
        GOO_APPLICABILITY_MACHINE_APPLICABLE
    );
    
    goo_diag_set_code(
        diag1, "E0001",
        "This error occurs when a statement doesn't end with a semicolon."
    );
    
    // 2. Missing comma in format string
    GooDiagnostic* diag2 = goo_diag_new(
        GOO_DIAG_ERROR,
        "example.goo",
        11, 42, 7,
        "expected ',' after format string"
    );
    
    goo_diag_add_suggestion(
        diag2,
        "example.goo", 11, 41, 8,
        "add a comma",
        "    println(\"Factorial of 5 is: {}\" , result);",
        GOO_APPLICABILITY_MACHINE_APPLICABLE
    );
    
    // 3. Type mismatch
    GooDiagnostic* diag3 = goo_diag_new(
        GOO_DIAG_ERROR,
        "example.goo",
        13, 25, 16,
        "mismatched types"
    );
    
    goo_diag_add_child(
        diag3, GOO_DIAG_NOTE,
        "example.goo", 13, 11, 3,
        "expected type 'int'"
    );
    
    goo_diag_add_child(
        diag3, GOO_DIAG_NOTE,
        "example.goo", 13, 25, 16,
        "found type 'string'"
    );
    
    goo_diag_add_suggestion(
        diag3,
        "example.goo", 13, 17, 25,
        "use an integer value instead",
        "    let value: int = 42;",
        GOO_APPLICABILITY_NOT_APPLICABLE
    );
    
    goo_diag_add_suggestion(
        diag3,
        "example.goo", 13, 17, 25,
        "or use the to_int function to convert",
        "    let value: int = to_int(\"not an integer\");",
        GOO_APPLICABILITY_MACHINE_APPLICABLE
    );
    
    goo_diag_set_code(
        diag3, "E0101",
        "This error occurs when a value of one type is used where a value of a different type is expected."
    );
    
    // 4. Borrow checker error
    GooDiagnostic* diag4 = goo_diag_new(
        GOO_DIAG_ERROR,
        "example.goo",
        17, 15, 12,
        "cannot borrow 'numbers' as mutable because it is also borrowed as immutable"
    );
    
    goo_diag_add_child(
        diag4, GOO_DIAG_NOTE,
        "example.goo", 16, 15, 8,
        "immutable borrow occurs here"
    );
    
    goo_diag_add_child(
        diag4, GOO_DIAG_NOTE,
        "example.goo", 17, 15, 12,
        "mutable borrow occurs here"
    );
    
    goo_diag_add_child(
        diag4, GOO_DIAG_NOTE,
        "example.goo", 19, 43, 6,
        "immutable borrow later used here"
    );
    
    goo_diag_add_suggestion(
        diag4,
        "example.goo", 16, 5, 0,
        "move the mutable borrow after the last use of the immutable borrow",
        "    let ref1 = &numbers;\n"
        "    println(\"First number: {}\", ref1[0]);\n"
        "    \n"
        "    let ref2 = &mut numbers;\n"
        "    println(\"Modified number: {}\", ref2[0]);",
        GOO_APPLICABILITY_NOT_APPLICABLE
    );
    
    goo_diag_set_code(
        diag4, "E0201",
        "This error occurs when trying to borrow a value as mutable while it's already borrowed as immutable."
    );
    
    // Emit the diagnostics
    goo_diag_emit(ctx, diag1);
    goo_diag_emit(ctx, diag2);
    goo_diag_emit(ctx, diag3);
    goo_diag_emit(ctx, diag4);
    
    // Print a summary
    printf("Generated %d diagnostics (%d errors, %d warnings)\n\n",
           goo_diag_error_count(ctx) + goo_diag_warning_count(ctx),
           goo_diag_error_count(ctx),
           goo_diag_warning_count(ctx));
    
    // Create an array of diagnostics for highlighting
    const GooDiagnostic* diagnostics[] = { diag1, diag2, diag3, diag4 };
    
    // Print highlighted source with diagnostics
    GooHighlightOptions options = goo_highlight_options_default();
    goo_print_highlighted_diagnostics(diagnostics, 4, example_source, &options);
    
    // Demonstrate --explain feature
    printf("\n\nDemonstrating --explain feature:\n");
    printf("=================================\n\n");
    
    goo_error_catalog_explain("E0101");
    
    // Clean up
    goo_diag_context_free(ctx);
    goo_error_catalog_cleanup();
    
    return 0;
} 