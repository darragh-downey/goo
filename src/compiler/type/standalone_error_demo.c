/**
 * standalone_error_demo.c
 * 
 * Completely standalone demonstration of type error reporting
 * with error codes and diagnostics
 * 
 * Copyright (c) 2024, Goo Language Project
 * Licensed under MIT License
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Error codes for type system errors
// These would normally be in a separate header file
#define GOO_ERR_TYPE_MISMATCH                "E0001" // General type mismatch
#define GOO_ERR_EXPECTED_FOUND               "E0002" // Expected X, found Y
#define GOO_ERR_INCOMPATIBLE_TYPES           "E0003" // Incompatible types in operation
#define GOO_ERR_TYPE_NOT_ASSIGNABLE          "E0004" // Type not assignable to target
#define GOO_ERR_TYPE_COMPARISON              "E0005" // Cannot compare these types
#define GOO_ERR_TYPE_INFERENCE               "E0006" // Cannot infer type
#define GOO_ERR_TYPE_CONDITION               "E0007" // Condition has wrong type
#define GOO_ERR_NON_BOOLEAN_CONDITION        "E0090" // Condition is not boolean

// Sample source code to demonstrate error reporting
const char* sample_source = 
"function add(a: int, b: int): int {\n"
"    return a + b;\n"
"}\n"
"\n"
"function main() {\n"
"    let x = 42;\n"
"    let y = \"hello\";\n"
"    let z = add(x, y);  // Type error: string passed where int expected\n"
"    \n"
"    if (z) {           // Type error: condition must be boolean\n"
"        print(z);\n"
"    }\n"
"}\n";

// Diagnostic severity levels
typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_HELP
} DiagLevel;

// Source location structure
typedef struct {
    const char* filename;
    int line;
    int column;
    int length;
} SourceLoc;

// Diagnostic structure
typedef struct Diagnostic {
    DiagLevel level;
    SourceLoc location;
    const char* message;
    const char* code;
    const char* explanation;
    struct Diagnostic* next;  // Linked list of diagnostics
} Diagnostic;

// Diagnostic context
typedef struct {
    Diagnostic* first;
    Diagnostic* last;
    int error_count;
    int warning_count;
} DiagContext;

// Create a new diagnostic context
DiagContext* create_context() {
    DiagContext* ctx = (DiagContext*)malloc(sizeof(DiagContext));
    ctx->first = NULL;
    ctx->last = NULL;
    ctx->error_count = 0;
    ctx->warning_count = 0;
    return ctx;
}

// Add a diagnostic to the context
void add_diagnostic(DiagContext* ctx, DiagLevel level, 
                   const char* filename, int line, int column, int length,
                   const char* message, const char* code, const char* explanation) {
    Diagnostic* diag = (Diagnostic*)malloc(sizeof(Diagnostic));
    
    diag->level = level;
    diag->location.filename = filename;
    diag->location.line = line;
    diag->location.column = column;
    diag->location.length = length;
    diag->message = strdup(message);
    diag->code = code ? strdup(code) : NULL;
    diag->explanation = explanation ? strdup(explanation) : NULL;
    diag->next = NULL;
    
    // Update counts
    if (level == DIAG_ERROR) {
        ctx->error_count++;
    } else if (level == DIAG_WARNING) {
        ctx->warning_count++;
    }
    
    // Add to linked list
    if (ctx->last) {
        ctx->last->next = diag;
        ctx->last = diag;
    } else {
        ctx->first = ctx->last = diag;
    }
}

// Find line in source code
const char* find_line(const char* source, int line_number) {
    const char* line_start = source;
    int current_line = 1;
    
    while (current_line < line_number && *line_start) {
        if (*line_start == '\n') {
            current_line++;
        }
        line_start++;
    }
    
    return line_start;
}

// Calculate line length
int line_length(const char* line) {
    const char* end = line;
    while (*end && *end != '\n') {
        end++;
    }
    return (int)(end - line);
}

// Print a source line with highlighting
void print_highlighted_line(const char* source, int line_number, int column, int length) {
    const char* line = find_line(source, line_number);
    int line_len = line_length(line);
    
    // Print line number and source line
    printf("%4d | ", line_number);
    for (int i = 0; i < line_len; i++) {
        putchar(line[i]);
    }
    printf("\n");
    
    // Print highlighting
    printf("     | ");
    for (int i = 1; i < column; i++) {
        putchar(' ');
    }
    
    for (int i = 0; i < length; i++) {
        putchar('^');
    }
    printf("\n");
}

// Print all diagnostics
void print_diagnostics(DiagContext* ctx, const char* source) {
    Diagnostic* diag = ctx->first;
    
    while (diag) {
        // Print diagnostic level and code if available
        switch (diag->level) {
            case DIAG_ERROR:
                printf("\x1b[1;31merror");  // Bold red for errors
                break;
            case DIAG_WARNING:
                printf("\x1b[1;33mwarning");  // Bold yellow for warnings
                break;
            case DIAG_NOTE:
                printf("\x1b[1;36mnote");     // Bold cyan for notes
                break;
            case DIAG_HELP:
                printf("\x1b[1;32mhelp");     // Bold green for help
                break;
        }
        
        if (diag->code) {
            printf("[%s]", diag->code);
        }
        
        // Print location and message
        printf("\x1b[0m: %s:%d:%d: %s\n", 
               diag->location.filename, 
               diag->location.line, 
               diag->location.column,
               diag->message);
        
        // Print source line with highlighting
        if (source) {
            print_highlighted_line(source, diag->location.line, diag->location.column, diag->location.length);
        }
        
        // Print explanation if available
        if (diag->explanation) {
            printf("     = %s\n", diag->explanation);
        }
        
        printf("\n");
        diag = diag->next;
    }
    
    // Print summary
    printf("%d error(s), %d warning(s) found.\n", 
           ctx->error_count, ctx->warning_count);
}

// Free diagnostic context
void free_diagnostic_context(DiagContext* ctx) {
    Diagnostic* diag = ctx->first;
    while (diag) {
        Diagnostic* next = diag->next;
        free((void*)diag->message);
        if (diag->code) free((void*)diag->code);
        if (diag->explanation) free((void*)diag->explanation);
        free(diag);
        diag = next;
    }
    free(ctx);
}

// Report a type mismatch error
void report_type_mismatch(DiagContext* ctx, const char* filename, 
                         int line, int column, int length,
                         const char* expected_type, const char* found_type) {
    char message[256];
    snprintf(message, sizeof(message), 
            "Type mismatch: expected '%s', found '%s'", 
            expected_type, found_type);
    
    const char* explanation = "Function arguments must match the parameter types.";
    
    add_diagnostic(ctx, DIAG_ERROR, filename, line, column, length, 
                  message, GOO_ERR_TYPE_MISMATCH, explanation);
}

// Report a non-boolean condition error
void report_non_boolean_condition(DiagContext* ctx, const char* filename,
                                 int line, int column, int length,
                                 const char* actual_type) {
    char message[256];
    snprintf(message, sizeof(message),
            "Non-boolean condition: '%s' used where a boolean is required",
            actual_type);
    
    const char* explanation = "Conditions in if statements must evaluate to a boolean value.";
    
    add_diagnostic(ctx, DIAG_ERROR, filename, line, column, length,
                  message, GOO_ERR_NON_BOOLEAN_CONDITION, explanation);
}

// Add a suggestion
void add_suggestion(DiagContext* ctx, const char* filename,
                   int line, int column, int length,
                   const char* message) {
    add_diagnostic(ctx, DIAG_HELP, filename, line, column, length,
                  message, NULL, NULL);
}

// Simulate applying type checking to the sample code
void run_type_checking_simulation() {
    DiagContext* ctx = create_context();
    const char* filename = "example.goo";
    
    printf("===== Goo Type Checking Demonstration =====\n\n");
    printf("Source code:\n\n");
    printf("%s\n", sample_source);
    
    // Simulate type checking and report errors
    
    // 1. Type mismatch in function call
    report_type_mismatch(ctx, filename, 8, 16, 1, "int", "string");
    
    // Add a suggestion for the type mismatch
    add_suggestion(ctx, filename, 8, 16, 1, 
                  "Try converting the string to an integer: to_int(y)");
    
    // 2. Non-boolean condition in if statement
    report_non_boolean_condition(ctx, filename, 10, 9, 1, "int");
    
    // Add a suggestion for the non-boolean condition
    add_suggestion(ctx, filename, 10, 9, 1,
                  "Try using a comparison: if (z != 0)");
    
    // Print all diagnostics
    printf("\n===== Type Checking Results =====\n\n");
    print_diagnostics(ctx, sample_source);
    
    // Free context
    free_diagnostic_context(ctx);
}

int main() {
    // Run the demonstration
    run_type_checking_simulation();
    
    return 0;
} 