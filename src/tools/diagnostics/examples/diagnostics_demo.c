#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../diagnostics.h"
#include "../diagnostics_module.h"
#include "../error_catalog.h"
#include "../source_highlight.h"

// Sample Goo code to demonstrate diagnostics
const char* sample_code = 
    "func factorial(n: int) -> int {\n"
    "    if n <= 1 {\n"
    "        return 1\n"
    "    } else {\n"
    "        return n * factorial(n - 1)\n"
    "    }\n"
    "}\n"
    "\n"
    "func main() {\n"
    "    let x: int = \"hello\"  // Type mismatch error\n"
    "    let y = factorial(5)\n"
    "    \n"
    "    var numbers = [1, 2, 3, 4, 5]\n"
    "    \n"
    "    // Mutable borrowing error\n"
    "    let a = &mut numbers\n"
    "    let b = &mut numbers  // Error: cannot borrow 'numbers' as mutable more than once\n"
    "    \n"
    "    print(\"Factorial of 5 is: \", y)\n"
    "}\n";

int main(int argc, char** argv) {
    // Initialize the diagnostics system
    GooDiagnosticContext* ctx = goo_init_diagnostics(argc, argv);
    if (!ctx) {
        fprintf(stderr, "Failed to initialize diagnostics system\n");
        return 1;
    }
    
    // Initialize a dummy error catalog for this demo
    goo_error_catalog_register(
        "E0001",
        GOO_ERROR_CAT_TYPE,
        "Type mismatch",
        "This error occurs when a value of one type is used where a value of a different type is expected.\n"
        "Make sure the value being assigned matches the declared type of the variable.",
        NULL, // No example
        NULL  // No solution
    );
    
    goo_error_catalog_register(
        "E0002",
        GOO_ERROR_CAT_BORROW,
        "Multiple mutable borrows",
        "This error occurs when a variable is mutably borrowed more than once at the same time.\n"
        "In Goo, you can have either multiple immutable borrows or exactly one mutable borrow at a time.",
        NULL, // No example
        NULL  // No solution
    );
    
    // Create a diagnostic for type mismatch error
    GooDiagnostic* type_err = goo_diag_new(
        GOO_DIAG_ERROR,
        "example.goo",
        10,
        19,
        7,
        "Cannot assign a value of type 'string' to a variable of type 'int'"
    );
    
    // Set error code
    goo_diag_set_code(type_err, "E0001", NULL);
    
    // Add a child diagnostic with a note
    goo_diag_add_child(
        type_err, 
        GOO_DIAG_NOTE,
        "example.goo", 
        10, 
        9,
        1, 
        "Variable 'x' was declared with type 'int' here"
    );
    
    // Add a suggestion - using 0 for machine applicability
    goo_diag_add_suggestion(
        type_err,
        "example.goo", 
        10, 
        19,
        7, 
        "Consider using an integer literal instead",
        "5",
        0  // Assuming 0 means machine applicable
    );
    
    // Emit the diagnostic
    goo_diag_emit(ctx, type_err);
    
    // Report a borrowing error
    goo_diagnostics_report_error(
        ctx,
        "example.goo",
        sample_code,  // Pass the source code for highlighting
        16,
        15,
        14,
        "Cannot borrow 'numbers' as mutable more than once"
    );
    
    // Print all diagnostics with source highlighting
    GooHighlightOptions options = goo_highlight_options_default();
    options.context_lines = 2;  // Show 2 lines of context
    
    // Print all diagnostics
    goo_diag_print_all(ctx);
    
    // Highlight each diagnostic with source context
    printf("\nWith source code highlighting:\n\n");
    const GooDiagnostic** diags = malloc(ctx->diagnostics_count * sizeof(GooDiagnostic*));
    if (diags) {
        for (unsigned int i = 0; i < ctx->diagnostics_count; i++) {
            diags[i] = ctx->diagnostics[i];
        }
        
        // Print diagnostics with source code
        goo_print_highlighted_diagnostics(diags, ctx->diagnostics_count, sample_code, &options);
        
        free(diags);
    } else {
        // Just print the first diagnostic if we couldn't allocate memory
        if (ctx->diagnostics_count > 0) {
            goo_print_highlighted_source(ctx->diagnostics[0], sample_code, &options);
        }
    }
    
    // Print a summary
    goo_diagnostics_print_summary(ctx, NULL);
    
    // Clean up
    goo_cleanup_diagnostics(ctx);
    
    return 0;
} 