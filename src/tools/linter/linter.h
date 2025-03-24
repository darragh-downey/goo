/**
 * @file linter.h
 * @brief Static code analysis for the Goo programming language
 * 
 * This file defines the interfaces for static code analysis in the Goo
 * compiler, providing code quality checks and best practice enforcement.
 */

#ifndef GOO_LINTER_H
#define GOO_LINTER_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Linter rule severity levels
 */
typedef enum {
    GOO_LINT_ERROR,       // Definite problems that should be fixed
    GOO_LINT_WARNING,     // Potential problems or style issues
    GOO_LINT_INFO,        // Informational recommendations
    GOO_LINT_HINT         // Stylistic suggestions
} GooLintSeverity;

/**
 * Linter rule categories
 */
typedef enum {
    GOO_LINT_CORRECTNESS,    // Code correctness issues
    GOO_LINT_PERFORMANCE,    // Performance issues
    GOO_LINT_STYLE,          // Code style issues
    GOO_LINT_COMPLEXITY,     // Code complexity issues
    GOO_LINT_SECURITY,       // Security issues
    GOO_LINT_COMPATIBILITY,  // Compatibility issues
    GOO_LINT_MAINTAINABILITY // Maintainability issues
} GooLintCategory;

/**
 * Linter diagnostic location
 */
typedef struct {
    const char* file;       // Source file path
    unsigned int line;      // Line number (1-based)
    unsigned int column;    // Column number (1-based)
    unsigned int length;    // Length of the affected code
} GooLintLocation;

/**
 * Linter diagnostic message
 */
typedef struct {
    const char* rule_id;        // Unique rule identifier (e.g., "no-unused-vars")
    GooLintSeverity severity;   // Severity level
    GooLintCategory category;   // Rule category
    GooLintLocation location;   // Code location
    const char* message;        // Human-readable message
    const char* suggestion;     // Fix suggestion (may be NULL)
} GooLintDiagnostic;

/**
 * Linter rule configuration
 */
typedef struct {
    const char* rule_id;        // Unique rule identifier
    GooLintSeverity severity;   // Desired severity level (or -1 to disable)
} GooLintRuleConfig;

/**
 * Linter configuration
 */
typedef struct {
    GooLintRuleConfig* rules;   // Array of rule configurations
    unsigned int rule_count;    // Number of rules in the array
    const char** ignore_files;  // Patterns of files to ignore
    unsigned int ignore_count;  // Number of ignore patterns
    bool treat_warnings_as_errors; // Treat all warnings as errors
} GooLintConfig;

/**
 * Linter context
 */
typedef struct {
    GooLintConfig config;       // Linter configuration
    GooLintDiagnostic* diagnostics; // Array of diagnostics
    unsigned int diagnostic_count;   // Number of diagnostics
    unsigned int diagnostic_capacity; // Capacity of diagnostics array
} GooLintContext;

/**
 * Create a new linter context with default configuration
 */
GooLintContext* goo_lint_context_new(void);

/**
 * Free a linter context
 */
void goo_lint_context_free(GooLintContext* context);

/**
 * Load linter configuration from a file
 * 
 * @param context Linter context to configure
 * @param config_file Path to configuration file
 * @return true on success, false on error
 */
bool goo_lint_load_config(GooLintContext* context, const char* config_file);

/**
 * Configure a linter rule
 * 
 * @param context Linter context
 * @param rule_id Rule identifier
 * @param severity Severity level (-1 to disable the rule)
 * @return true on success, false on error
 */
bool goo_lint_configure_rule(GooLintContext* context, const char* rule_id, GooLintSeverity severity);

/**
 * Add a file pattern to ignore
 * 
 * @param context Linter context
 * @param pattern Glob pattern for files to ignore
 * @return true on success, false on error
 */
bool goo_lint_add_ignore(GooLintContext* context, const char* pattern);

/**
 * Lint a source file
 * 
 * @param context Linter context
 * @param filename Source file path
 * @param source Source code (if NULL, file will be read from disk)
 * @return Number of issues found
 */
int goo_lint_file(GooLintContext* context, const char* filename, const char* source);

/**
 * Get a diagnostic at a specific index
 * 
 * @param context Linter context
 * @param index Diagnostic index
 * @return Pointer to diagnostic or NULL if index is invalid
 */
const GooLintDiagnostic* goo_lint_get_diagnostic(const GooLintContext* context, unsigned int index);

/**
 * Get the total number of diagnostics
 * 
 * @param context Linter context
 * @return Number of diagnostics
 */
unsigned int goo_lint_diagnostic_count(const GooLintContext* context);

/**
 * Count diagnostics of a specific severity level
 * 
 * @param context Linter context
 * @param severity Severity level to count
 * @return Number of diagnostics with the specified severity
 */
unsigned int goo_lint_count_by_severity(const GooLintContext* context, GooLintSeverity severity);

/**
 * Print diagnostics to stdout
 * 
 * @param context Linter context
 * @param colored Use colored output if true
 */
void goo_lint_print_diagnostics(const GooLintContext* context, bool colored);

/**
 * Export diagnostics to JSON
 * 
 * @param context Linter context
 * @param output_file Output file path (or NULL for stdout)
 * @return true on success, false on error
 */
bool goo_lint_export_json(const GooLintContext* context, const char* output_file);

/**
 * Add a custom diagnostic
 * 
 * @param context Linter context
 * @param rule_id Rule identifier
 * @param severity Severity level
 * @param category Rule category
 * @param file Source file path
 * @param line Line number (1-based)
 * @param column Column number (1-based)
 * @param length Length of the affected code
 * @param message Human-readable message
 * @param suggestion Fix suggestion (may be NULL)
 * @return true on success, false on error
 */
bool goo_lint_add_diagnostic(
    GooLintContext* context,
    const char* rule_id,
    GooLintSeverity severity,
    GooLintCategory category,
    const char* file,
    unsigned int line,
    unsigned int column,
    unsigned int length,
    const char* message,
    const char* suggestion
);

#ifdef __cplusplus
}
#endif

#endif /* GOO_LINTER_H */ 