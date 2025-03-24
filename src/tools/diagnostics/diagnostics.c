/**
 * @file diagnostics.c
 * @brief Implementation of the Goo diagnostics system
 */

#include "diagnostics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// ANSI color codes for terminal output
#define COLOR_RESET   "\x1b[0m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_BOLD    "\x1b[1m"

// Initial capacity for diagnostics array
#define INITIAL_DIAGNOSTICS_CAPACITY 16

// Private helper functions
static char* duplicate_string(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (result) {
        memcpy(result, str, len + 1);
    }
    return result;
}

static void ensure_capacity(GooDiagnosticContext* context) {
    if (context->diagnostics_count >= context->diagnostics_capacity) {
        size_t new_capacity = context->diagnostics_capacity * 2;
        GooDiagnostic** new_diagnostics = (GooDiagnostic**)realloc(
            context->diagnostics, 
            new_capacity * sizeof(GooDiagnostic*)
        );
        
        if (new_diagnostics) {
            context->diagnostics = new_diagnostics;
            context->diagnostics_capacity = new_capacity;
        }
    }
}

static const char* level_to_string(GooDiagnosticLevel level) {
    switch (level) {
        case GOO_DIAG_ERROR: return "error";
        case GOO_DIAG_WARNING: return "warning";
        case GOO_DIAG_NOTE: return "note";
        case GOO_DIAG_HELP: return "help";
        case GOO_DIAG_ICE: return "internal compiler error";
        default: return "unknown";
    }
}

static const char* level_to_colored_string(GooDiagnosticLevel level, bool use_color) {
    if (!use_color) {
        return level_to_string(level);
    }
    
    switch (level) {
        case GOO_DIAG_ERROR:
            return COLOR_BOLD COLOR_RED "error" COLOR_RESET;
        case GOO_DIAG_WARNING:
            return COLOR_BOLD COLOR_YELLOW "warning" COLOR_RESET;
        case GOO_DIAG_NOTE:
            return COLOR_BOLD COLOR_BLUE "note" COLOR_RESET;
        case GOO_DIAG_HELP:
            return COLOR_BOLD COLOR_CYAN "help" COLOR_RESET;
        case GOO_DIAG_ICE:
            return COLOR_BOLD COLOR_RED "internal compiler error" COLOR_RESET;
        default:
            return "unknown";
    }
}

// Public API implementation

GooDiagnosticContext* goo_diag_context_new(void) {
    GooDiagnosticContext* context = (GooDiagnosticContext*)malloc(sizeof(GooDiagnosticContext));
    if (!context) return NULL;
    
    context->diagnostics_capacity = INITIAL_DIAGNOSTICS_CAPACITY;
    context->diagnostics = (GooDiagnostic**)malloc(
        context->diagnostics_capacity * sizeof(GooDiagnostic*)
    );
    
    if (!context->diagnostics) {
        free(context);
        return NULL;
    }
    
    context->diagnostics_count = 0;
    context->treat_warnings_as_errors = false;
    context->json_output = false;
    context->colored_output = true;
    context->error_limit = 0;
    context->error_count = 0;
    context->warning_count = 0;
    
    return context;
}

void goo_diag_context_free(GooDiagnosticContext* context) {
    if (!context) return;
    
    // Free all diagnostics
    for (size_t i = 0; i < context->diagnostics_count; i++) {
        GooDiagnostic* diag = context->diagnostics[i];
        
        // Free children
        for (size_t j = 0; j < diag->children_count; j++) {
            free((void*)diag->children[j]->message);
            free(diag->children[j]);
        }
        free(diag->children);
        
        // Free suggestions
        for (size_t j = 0; j < diag->suggestions_count; j++) {
            free((void*)diag->suggestions[j]->message);
            free((void*)diag->suggestions[j]->suggested_replacement);
            free(diag->suggestions[j]);
        }
        free(diag->suggestions);
        
        // Free diagnostic itself
        free((void*)diag->message);
        free((void*)diag->code);
        free((void*)diag->explanation);
        free((void*)diag->primary_location.filename);
        free(diag);
    }
    
    free(context->diagnostics);
    free(context);
}

GooDiagnostic* goo_diag_new(
    GooDiagnosticLevel level,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message
) {
    GooDiagnostic* diag = (GooDiagnostic*)malloc(sizeof(GooDiagnostic));
    if (!diag) return NULL;
    
    diag->level = level;
    diag->primary_location.filename = duplicate_string(filename);
    diag->primary_location.line = line;
    diag->primary_location.column = column;
    diag->primary_location.length = length;
    diag->message = duplicate_string(message);
    
    diag->children = NULL;
    diag->children_count = 0;
    diag->suggestions = NULL;
    diag->suggestions_count = 0;
    diag->code = NULL;
    diag->explanation = NULL;
    
    return diag;
}

void goo_diag_add_child(
    GooDiagnostic* diag, 
    GooDiagnosticLevel level,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message
) {
    if (!diag) return;
    
    GooDiagnostic* child = goo_diag_new(
        level, filename, line, column, length, message
    );
    
    if (!child) return;
    
    // Resize children array
    size_t new_count = diag->children_count + 1;
    GooDiagnostic** new_children = (GooDiagnostic**)realloc(
        diag->children, new_count * sizeof(GooDiagnostic*)
    );
    
    if (!new_children) {
        free((void*)child->message);
        free((void*)child->primary_location.filename);
        free(child);
        return;
    }
    
    diag->children = new_children;
    diag->children[diag->children_count] = child;
    diag->children_count = new_count;
}

void goo_diag_add_suggestion(
    GooDiagnostic* diag,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* message,
    const char* replacement,
    GooSuggestionApplicability applicability
) {
    if (!diag) return;
    
    GooSuggestion* suggestion = (GooSuggestion*)malloc(sizeof(GooSuggestion));
    if (!suggestion) return;
    
    suggestion->location.filename = duplicate_string(filename);
    suggestion->location.line = line;
    suggestion->location.column = column;
    suggestion->location.length = length;
    suggestion->message = duplicate_string(message);
    suggestion->suggested_replacement = duplicate_string(replacement);
    suggestion->applicability = applicability;
    
    // Resize suggestions array
    size_t new_count = diag->suggestions_count + 1;
    GooSuggestion** new_suggestions = (GooSuggestion**)realloc(
        diag->suggestions, new_count * sizeof(GooSuggestion*)
    );
    
    if (!new_suggestions) {
        free((void*)suggestion->message);
        free((void*)suggestion->suggested_replacement);
        free((void*)suggestion->location.filename);
        free(suggestion);
        return;
    }
    
    diag->suggestions = new_suggestions;
    diag->suggestions[diag->suggestions_count] = suggestion;
    diag->suggestions_count = new_count;
}

void goo_diag_set_code(
    GooDiagnostic* diag,
    const char* code,
    const char* explanation
) {
    if (!diag) return;
    
    if (diag->code) {
        free((void*)diag->code);
    }
    
    if (diag->explanation) {
        free((void*)diag->explanation);
    }
    
    diag->code = duplicate_string(code);
    diag->explanation = duplicate_string(explanation);
}

void goo_diag_emit(GooDiagnosticContext* context, GooDiagnostic* diag) {
    if (!context || !diag) return;
    
    // Update error/warning count
    if (diag->level == GOO_DIAG_ERROR || diag->level == GOO_DIAG_ICE) {
        context->error_count++;
    } else if (diag->level == GOO_DIAG_WARNING) {
        if (context->treat_warnings_as_errors) {
            context->error_count++;
        } else {
            context->warning_count++;
        }
    }
    
    // Check error limit
    if (context->error_limit > 0 && context->error_count > context->error_limit) {
        return;
    }
    
    // Add diagnostic to the context
    ensure_capacity(context);
    context->diagnostics[context->diagnostics_count++] = diag;
    
    // Print diagnostic immediately unless we're in json mode
    if (!context->json_output) {
        // Print main diagnostic
        if (diag->primary_location.filename) {
            fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                diag->primary_location.filename, 
                diag->primary_location.line, 
                diag->primary_location.column,
                level_to_colored_string(diag->level, context->colored_output),
                diag->message);
        } else {
            fprintf(stderr, "%s: %s\n", 
                level_to_colored_string(diag->level, context->colored_output),
                diag->message);
        }
        
        // Print children
        for (size_t i = 0; i < diag->children_count; i++) {
            GooDiagnostic* child = diag->children[i];
            if (child->primary_location.filename) {
                fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                    child->primary_location.filename, 
                    child->primary_location.line, 
                    child->primary_location.column,
                    level_to_colored_string(child->level, context->colored_output),
                    child->message);
            } else {
                fprintf(stderr, "%s: %s\n", 
                    level_to_colored_string(child->level, context->colored_output),
                    child->message);
            }
        }
        
        // Print suggestions
        for (size_t i = 0; i < diag->suggestions_count; i++) {
            GooSuggestion* suggestion = diag->suggestions[i];
            if (suggestion->location.filename) {
                fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                    suggestion->location.filename, 
                    suggestion->location.line, 
                    suggestion->location.column,
                    level_to_colored_string(GOO_DIAG_HELP, context->colored_output),
                    suggestion->message);
                
                // Show suggested replacement if it exists
                if (suggestion->suggested_replacement) {
                    fprintf(stderr, "%s\n", suggestion->suggested_replacement);
                }
            } else {
                fprintf(stderr, "%s: %s\n", 
                    level_to_colored_string(GOO_DIAG_HELP, context->colored_output),
                    suggestion->message);
                
                // Show suggested replacement if it exists
                if (suggestion->suggested_replacement) {
                    fprintf(stderr, "%s\n", suggestion->suggested_replacement);
                }
            }
        }
        
        // Print separator
        fprintf(stderr, "\n");
    }
}

void goo_report_error(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
) {
    va_list args;
    va_start(args, fmt);
    
    // Format the message
    char buffer[1024]; // Reasonable size for most error messages
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    va_end(args);
    
    // Create and emit diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_ERROR, filename, line, column, length, buffer
    );
    
    if (diag) {
        goo_diag_emit(context, diag);
    }
}

void goo_report_warning(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
) {
    va_list args;
    va_start(args, fmt);
    
    // Format the message
    char buffer[1024]; // Reasonable size for most warning messages
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    va_end(args);
    
    // Create and emit diagnostic
    GooDiagnosticLevel level = context->treat_warnings_as_errors ? 
                               GOO_DIAG_ERROR : GOO_DIAG_WARNING;
    
    GooDiagnostic* diag = goo_diag_new(
        level, filename, line, column, length, buffer
    );
    
    if (diag) {
        goo_diag_emit(context, diag);
    }
}

void goo_report_note(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
) {
    va_list args;
    va_start(args, fmt);
    
    // Format the message
    char buffer[1024]; // Reasonable size for most note messages
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    va_end(args);
    
    // Create and emit diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_NOTE, filename, line, column, length, buffer
    );
    
    if (diag) {
        goo_diag_emit(context, diag);
    }
}

void goo_report_help(
    GooDiagnosticContext* context,
    const char* filename, 
    unsigned int line, 
    unsigned int column,
    unsigned int length, 
    const char* fmt, 
    ...
) {
    va_list args;
    va_start(args, fmt);
    
    // Format the message
    char buffer[1024]; // Reasonable size for most help messages
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    va_end(args);
    
    // Create and emit diagnostic
    GooDiagnostic* diag = goo_diag_new(
        GOO_DIAG_HELP, filename, line, column, length, buffer
    );
    
    if (diag) {
        goo_diag_emit(context, diag);
    }
}

void goo_diag_print_all(GooDiagnosticContext* context) {
    if (!context) return;
    
    for (size_t i = 0; i < context->diagnostics_count; i++) {
        GooDiagnostic* diag = context->diagnostics[i];
        
        // Print main diagnostic
        if (diag->primary_location.filename) {
            fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                diag->primary_location.filename, 
                diag->primary_location.line, 
                diag->primary_location.column,
                level_to_colored_string(diag->level, context->colored_output),
                diag->message);
        } else {
            fprintf(stderr, "%s: %s\n", 
                level_to_colored_string(diag->level, context->colored_output),
                diag->message);
        }
        
        // Print children
        for (size_t j = 0; j < diag->children_count; j++) {
            GooDiagnostic* child = diag->children[j];
            if (child->primary_location.filename) {
                fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                    child->primary_location.filename, 
                    child->primary_location.line, 
                    child->primary_location.column,
                    level_to_colored_string(child->level, context->colored_output),
                    child->message);
            } else {
                fprintf(stderr, "%s: %s\n", 
                    level_to_colored_string(child->level, context->colored_output),
                    child->message);
            }
        }
        
        // Print suggestions
        for (size_t j = 0; j < diag->suggestions_count; j++) {
            GooSuggestion* suggestion = diag->suggestions[j];
            if (suggestion->location.filename) {
                fprintf(stderr, "%s:%u:%u: %s: %s\n", 
                    suggestion->location.filename, 
                    suggestion->location.line, 
                    suggestion->location.column,
                    level_to_colored_string(GOO_DIAG_HELP, context->colored_output),
                    suggestion->message);
                
                // Show suggested replacement if it exists
                if (suggestion->suggested_replacement) {
                    fprintf(stderr, "%s\n", suggestion->suggested_replacement);
                }
            } else {
                fprintf(stderr, "%s: %s\n", 
                    level_to_colored_string(GOO_DIAG_HELP, context->colored_output),
                    suggestion->message);
                
                // Show suggested replacement if it exists
                if (suggestion->suggested_replacement) {
                    fprintf(stderr, "%s\n", suggestion->suggested_replacement);
                }
            }
        }
        
        // Print separator
        fprintf(stderr, "\n");
    }
    
    // Print summary
    if (context->error_count > 0 || context->warning_count > 0) {
        fprintf(stderr, "Summary: ");
        
        if (context->error_count > 0) {
            fprintf(stderr, "%d %s", 
                context->error_count, 
                context->error_count == 1 ? "error" : "errors");
                
            if (context->warning_count > 0) {
                fprintf(stderr, ", ");
            }
        }
        
        if (context->warning_count > 0) {
            fprintf(stderr, "%d %s", 
                context->warning_count, 
                context->warning_count == 1 ? "warning" : "warnings");
        }
        
        fprintf(stderr, "\n");
    }
}

void goo_diag_print_json(GooDiagnosticContext* context) {
    if (!context) return;
    
    // Print JSON array opening
    printf("[\n");
    
    // Print each diagnostic as a JSON object
    for (size_t i = 0; i < context->diagnostics_count; i++) {
        GooDiagnostic* diag = context->diagnostics[i];
        
        printf("  {\n");
        printf("    \"level\": \"%s\",\n", level_to_string(diag->level));
        printf("    \"message\": \"%s\",\n", diag->message);
        
        // Location
        if (diag->primary_location.filename) {
            printf("    \"location\": {\n");
            printf("      \"file\": \"%s\",\n", diag->primary_location.filename);
            printf("      \"line\": %u,\n", diag->primary_location.line);
            printf("      \"column\": %u,\n", diag->primary_location.column);
            printf("      \"length\": %u\n", diag->primary_location.length);
            printf("    },\n");
        }
        
        // Code and explanation
        if (diag->code) {
            printf("    \"code\": \"%s\",\n", diag->code);
        }
        
        if (diag->explanation) {
            printf("    \"explanation\": \"%s\",\n", diag->explanation);
        }
        
        // Children
        if (diag->children_count > 0) {
            printf("    \"children\": [\n");
            for (size_t j = 0; j < diag->children_count; j++) {
                GooDiagnostic* child = diag->children[j];
                
                printf("      {\n");
                printf("        \"level\": \"%s\",\n", level_to_string(child->level));
                printf("        \"message\": \"%s\"", child->message);
                
                // Location
                if (child->primary_location.filename) {
                    printf(",\n        \"location\": {\n");
                    printf("          \"file\": \"%s\",\n", child->primary_location.filename);
                    printf("          \"line\": %u,\n", child->primary_location.line);
                    printf("          \"column\": %u,\n", child->primary_location.column);
                    printf("          \"length\": %u\n", child->primary_location.length);
                    printf("        }\n");
                } else {
                    printf("\n");
                }
                
                printf("      }%s\n", j < diag->children_count - 1 ? "," : "");
            }
            printf("    ],\n");
        }
        
        // Suggestions
        if (diag->suggestions_count > 0) {
            printf("    \"suggestions\": [\n");
            for (size_t j = 0; j < diag->suggestions_count; j++) {
                GooSuggestion* suggestion = diag->suggestions[j];
                
                printf("      {\n");
                printf("        \"message\": \"%s\",\n", suggestion->message);
                
                // Location
                if (suggestion->location.filename) {
                    printf("        \"location\": {\n");
                    printf("          \"file\": \"%s\",\n", suggestion->location.filename);
                    printf("          \"line\": %u,\n", suggestion->location.line);
                    printf("          \"column\": %u,\n", suggestion->location.column);
                    printf("          \"length\": %u\n", suggestion->location.length);
                    printf("        },\n");
                }
                
                // Replacement
                if (suggestion->suggested_replacement) {
                    printf("        \"replacement\": \"%s\",\n", suggestion->suggested_replacement);
                }
                
                // Applicability
                printf("        \"applicability\": %d\n", suggestion->applicability);
                
                printf("      }%s\n", j < diag->suggestions_count - 1 ? "," : "");
            }
            printf("    ]\n");
        } else {
            // Remove trailing comma if no suggestions
            printf("    \"suggestions\": []\n");
        }
        
        printf("  }%s\n", i < context->diagnostics_count - 1 ? "," : "");
    }
    
    // Print JSON array closing
    printf("]\n");
}

bool goo_diag_has_errors(GooDiagnosticContext* context) {
    if (!context) return false;
    return context->error_count > 0;
}

int goo_diag_error_count(GooDiagnosticContext* context) {
    if (!context) return 0;
    return context->error_count;
}

int goo_diag_warning_count(GooDiagnosticContext* context) {
    if (!context) return 0;
    return context->warning_count;
} 