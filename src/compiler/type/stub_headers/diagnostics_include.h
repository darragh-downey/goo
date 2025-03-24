#ifndef DIAGNOSTICS_INCLUDE_STUB_H
#define DIAGNOSTICS_INCLUDE_STUB_H

// This is a stub header that includes just enough to compile with diagnostics
// but without LLVM dependencies

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Diagnostic severity levels
typedef enum {
    GOO_DIAG_ERROR,    // Error - prevents compilation
    GOO_DIAG_WARNING,  // Warning - does not prevent compilation
    GOO_DIAG_NOTE,     // Note - additional information
    GOO_DIAG_HELP      // Help - suggestion on how to fix
} GooDiagnosticLevel;

// Source location structure
typedef struct {
    const char* filename;    // Source filename
    unsigned int line;       // Line number (1-based)
    unsigned int column;     // Column number (1-based)
    unsigned int length;     // Length of the span
} GooSourceLocation;

// Diagnostic structure
typedef struct GooDiagnostic {
    GooDiagnosticLevel level;          // Error, warning, note, etc.
    GooSourceLocation primary_location; // Primary source location
    const char* message;               // Diagnostic message
    struct GooDiagnostic** children;   // Child diagnostics (like notes)
    unsigned int children_count;       // Number of child diagnostics
    void* suggestions;                 // Suggestions for fixing the issue
    unsigned int suggestions_count;    // Number of suggestions
    const char* code;                  // Error code (e.g., "E0001")
    const char* explanation;           // Detailed explanation
} GooDiagnostic;

// Forward declare the diagnostic context
typedef struct GooDiagnosticContext GooDiagnosticContext;

// Stub function declarations
GooDiagnosticContext* goo_diag_context_new(void);
void goo_diag_context_free(GooDiagnosticContext* context);
void goo_diag_emit(GooDiagnosticContext* context, GooDiagnostic* diagnostic);
void goo_diag_print_all(GooDiagnosticContext* context);
void goo_print_highlighted_source(const GooDiagnostic* diagnostic, const char* source_code, void* options);
GooDiagnostic* goo_diag_new(GooDiagnosticLevel level, const char* filename, 
                            unsigned int line, unsigned int column, 
                            unsigned int length, const char* message);
int goo_diag_error_count(GooDiagnosticContext* context);
int goo_diag_warning_count(GooDiagnosticContext* context);
void goo_diag_set_code(GooDiagnostic* diag, const char* code, const char* explanation);
void goo_diag_add_child(GooDiagnostic* diag, 
                        GooDiagnosticLevel level,
                        const char* filename, 
                        unsigned int line, 
                        unsigned int column,
                        unsigned int length, 
                        const char* message);
void goo_diag_add_suggestion(GooDiagnostic* diag,
                            const char* filename, 
                            unsigned int line, 
                            unsigned int column,
                            unsigned int length, 
                            const char* message,
                            const char* replacement,
                            int applicability);

// Higher-level diagnostic functions
void goo_diagnostics_report_error(GooDiagnosticContext* context,
                                const char* filename,
                                const char* source,
                                unsigned int line,
                                unsigned int column,
                                unsigned int length,
                                const char* message,
                                ...);

void goo_diagnostics_report_warning(GooDiagnosticContext* context,
                                   const char* filename,
                                   const char* source,
                                   unsigned int line,
                                   unsigned int column,
                                   unsigned int length,
                                   const char* message,
                                   ...);

bool goo_diagnostics_should_abort(GooDiagnosticContext* context);

// Initialize and cleanup diagnostics
GooDiagnosticContext* goo_init_diagnostics(int argc, char** argv);
void goo_cleanup_diagnostics(GooDiagnosticContext* ctx);

#endif // DIAGNOSTICS_INCLUDE_STUB_H 