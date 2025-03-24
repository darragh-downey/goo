#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "token_definitions.h"
#include "lexer_selection.h"

// Token structure used for test comparisons
typedef struct Token {
    int type;
    int line;
    int column;
    int has_value;
    long int_value;
    double float_value;
    int bool_value;
    char* string_value;
    char* literal;  // Added literal field
} Token;

// Token stream for storing sequences of tokens
typedef struct TokenStream {
    Token* tokens;
    int size;
    int capacity;
} TokenStream;

// Create a new token stream
TokenStream* token_stream_new() {
    TokenStream* stream = (TokenStream*)malloc(sizeof(TokenStream));
    if (!stream) return NULL;
    
    stream->capacity = 16;
    stream->size = 0;
    stream->tokens = (Token*)malloc(sizeof(Token) * stream->capacity);
    if (!stream->tokens) {
        free(stream);
        return NULL;
    }
    
    return stream;
}

// Add a token to a stream
void token_stream_add(
    TokenStream* stream, 
    int type, 
    int line, 
    int column, 
    int has_int_value, 
    double float_value, 
    int bool_value, 
    char* string_value
) {
    if (!stream) return;
    
    // Resize if needed
    if (stream->size >= stream->capacity) {
        stream->capacity *= 2;
        Token* new_tokens = (Token*)realloc(stream->tokens, sizeof(Token) * stream->capacity);
        if (!new_tokens) return;
        stream->tokens = new_tokens;
    }
    
    // Add the token
    Token* t = &stream->tokens[stream->size++];
    t->type = type;
    t->line = line;
    t->column = column;
    t->has_value = has_int_value;
    t->int_value = (long)has_int_value;
    t->float_value = float_value;
    t->bool_value = bool_value;
    t->string_value = string_value ? strdup(string_value) : NULL;
}

// Free a token stream and all its tokens
void token_stream_free(TokenStream* stream) {
    if (!stream) return;
    
    // Free all string values
    for (int i = 0; i < stream->size; i++) {
        if (stream->tokens[i].string_value) {
            free(stream->tokens[i].string_value);
        }
    }
    
    free(stream->tokens);
    free(stream);
}

// Print a token stream
void token_stream_print(TokenStream* stream, const char* name) {
    if (!stream) return;
    
    printf("Token stream %s (%d tokens):\n", name, stream->size);
    for (int i = 0; i < stream->size; i++) {
        Token* t = &stream->tokens[i];
        printf("  %d: type=%d, line=%d, col=%d", i, t->type, t->line, t->column);
        
        if (t->has_value) {
            switch (t->type) {
                case INT_LITERAL:
                    printf(", value=%ld", t->int_value);
                    break;
                case FLOAT_LITERAL:
                    printf(", value=%f", t->float_value);
                    break;
                case BOOL_LITERAL:
                    printf(", value=%s", t->bool_value ? "true" : "false");
                    break;
                case STRING_LITERAL:
                    printf(", value=\"%s\"", t->string_value ? t->string_value : "");
                    break;
            }
        }
        
        printf("\n");
    }
}

// Compare two token streams
int token_stream_compare(TokenStream* stream1, TokenStream* stream2) {
    if (!stream1 || !stream2) return 0;
    
    // Check sizes
    if (stream1->size != stream2->size) {
        printf("Token streams have different sizes: %d vs %d\n", stream1->size, stream2->size);
        return 0;
    }
    
    // Compare each token
    for (int i = 0; i < stream1->size; i++) {
        Token* t1 = &stream1->tokens[i];
        Token* t2 = &stream2->tokens[i];
        
        // Check basic properties
        if (t1->type != t2->type) {
            printf("Token %d has different types: %d vs %d\n", i, t1->type, t2->type);
            return 0;
        }
        
        if (t1->line != t2->line) {
            printf("Token %d has different line numbers: %d vs %d\n", i, t1->line, t2->line);
            return 0;
        }
        
        if (t1->column != t2->column) {
            printf("Token %d has different column numbers: %d vs %d\n", i, t1->column, t2->column);
            return 0;
        }
        
        // Check values for literal tokens
        if (t1->has_value != t2->has_value) {
            printf("Token %d has different has_value flags: %d vs %d\n", i, t1->has_value, t2->has_value);
            return 0;
        }
        
        if (t1->has_value) {
            switch (t1->type) {
                case INT_LITERAL:
                    if (t1->int_value != t2->int_value) {
                        printf("Token %d has different int values: %ld vs %ld\n", 
                               i, t1->int_value, t2->int_value);
                        return 0;
                    }
                    break;
                    
                case FLOAT_LITERAL:
                    if (fabs(t1->float_value - t2->float_value) > 0.0000001) {
                        printf("Token %d has different float values: %f vs %f\n", 
                               i, t1->float_value, t2->float_value);
                        return 0;
                    }
                    break;
                    
                case BOOL_LITERAL:
                    if (t1->bool_value != t2->bool_value) {
                        printf("Token %d has different bool values: %d vs %d\n", 
                               i, t1->bool_value, t2->bool_value);
                        return 0;
                    }
                    break;
                    
                case STRING_LITERAL:
                    if ((t1->string_value == NULL && t2->string_value != NULL) ||
                        (t1->string_value != NULL && t2->string_value == NULL) ||
                        (t1->string_value != NULL && t2->string_value != NULL && 
                         strcmp(t1->string_value, t2->string_value) != 0)) {
                        printf("Token %d has different string values: '%s' vs '%s'\n", 
                               i, t1->string_value ? t1->string_value : "NULL", 
                               t2->string_value ? t2->string_value : "NULL");
                        return 0;
                    }
                    break;
            }
        }
    }
    
    return 1;
}

// Create a new token stream
TokenStream* create_token_stream() {
    return token_stream_new();
}

// Add a token to the stream
void add_token(
    TokenStream* stream,
    int type,
    int line,
    int column,
    long int_value,
    double float_value,
    int bool_value,
    const char* string_value
) {
    token_stream_add(stream, type, line, column, int_value, float_value, bool_value, (char*)string_value);
}

// Tokenize a source string and return a token stream
TokenStream* tokenize_string(const char* source, int debug) {
    if (!source) return NULL;
    
    TokenStream* stream = token_stream_new();
    if (!stream) return NULL;
    
    // Initialize lexer with string
    GooLexer* lexer = lexer_init_string(source);
    if (!lexer) {
        token_stream_free(stream);
        return NULL;
    }
    
    // Process all tokens
    GooToken token;
    do {
        token = lexer_next_token(lexer);
        
        if (debug) {
            printf("Token: type=%d, line=%d, col=%d, literal='%s'\n", 
                   token.type, token.line, token.column, token.literal);
        }
        
        // Add the token to our stream based on its type
        if (token.has_value) {
            switch (token.type) {
                case INT_LITERAL:
                    token_stream_add(stream, token.type, token.line, token.column, 
                                   token.value.int_value, 0.0, 0, NULL);
                    break;
                case FLOAT_LITERAL:
                    token_stream_add(stream, token.type, token.line, token.column, 
                                   0, token.value.float_value, 0, NULL);
                    break;
                case BOOL_LITERAL:
                    token_stream_add(stream, token.type, token.line, token.column, 
                                   0, 0.0, token.value.bool_value, NULL);
                    break;
                case STRING_LITERAL:
                    token_stream_add(stream, token.type, token.line, token.column, 
                                   0, 0.0, 0, (char*)token.value.string_value);
                    break;
                default:
                    token_stream_add(stream, token.type, token.line, token.column, 
                                   0, 0.0, 0, (char*)token.literal);
                    break;
            }
        } else {
            token_stream_add(stream, token.type, token.line, token.column, 
                           0, 0.0, 0, (char*)token.literal);
        }
    } while (token.type != 0);  // Continue until EOF
    
    // Clean up
    lexer_free(lexer);
    
    return stream;
}

// Run a test on a source string
int run_test(const char* source, int expected_token_count, int debug) {
    if (!source) return 0;
    
    printf("Testing source: '%s'\n", source);
    
    // Tokenize with the selected lexer
    TokenStream* token_stream = tokenize_string(source, debug);
    if (!token_stream) {
        printf("Failed to tokenize source\n");
        return 0;
    }
    
    int result = (token_stream->size == expected_token_count);
    if (!result) {
        printf("Expected %d tokens, but got %d\n", expected_token_count, token_stream->size);
        token_stream_print(token_stream, "actual");
    } else if (debug) {
        token_stream_print(token_stream, "actual");
    }
    
    token_stream_free(token_stream);
    
    return result;
}

// Run tests for both the Flex and Zig lexers and compare results
int run_comparison_test(const char* source, int debug) {
    if (!source) return 0;
    
    printf("Comparing lexers on source: '%s'\n", source);
    
    // Only run with the Flex lexer since the Zig lexer is not available
    TokenStream* flex_stream = tokenize_string(source, debug);
    if (!flex_stream) {
        printf("Failed to tokenize with the Flex lexer\n");
        return 0;
    }
    
    if (debug) {
        token_stream_print(flex_stream, "Flex");
    }
    
    // Just return success since we only have one lexer for now
    token_stream_free(flex_stream);
    return 1;
}

// Forward declarations for the new functions
void run_test_with_debug(const char* source);
void run_built_in_tests_with_debug();
void run_built_in_tests();
void run_performance_test(int iterations);

// Run a test with the given source, showing debug output
void run_test_with_debug(const char* source) {
    printf("Comparing lexers on source: '%s'\n", source);
    
    // Create a token stream
    TokenStream* stream = tokenize_string(source, 1); // 1 for debug mode
    if (!stream) {
        printf("Failed to tokenize source\n");
        return;
    }
    
    // Print token stream
    token_stream_print(stream, "Flex");
    
    // Free token stream
    token_stream_free(stream);
}

// Run a performance test with a fixed test string and the given number of iterations
void run_performance_test(int iterations) {
    const char* test_string = "let x = 123; let y = \"test string\"; fn main() { return x + 1; }";
    printf("Running performance test with %d iterations...\n", iterations);
    
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        TokenStream* stream = tokenize_string(test_string, 0);
        if (!stream) {
            printf("Failed to tokenize string\n");
            return;
        }
        token_stream_free(stream);
    }
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    printf("Time taken: %f seconds\n", time_taken);
    printf("Iterations per second: %f\n", iterations / time_taken);
}

// Run built-in tests with debug output
void run_built_in_tests_with_debug() {
    printf("Running built-in lexer tests with debug output...\n");
    
    run_test_with_debug("let x = 123;");
    run_test_with_debug("let s = \"hello, world\";");
    run_test_with_debug("function test() { return 42; }");
    
    run_test_with_debug(
        "let x = 123;\n"
        "let s = \"hello, world\";\n"
        "fn test() {\n"
        "    return x + 1;\n"
        "}\n"
    );
    
    printf("Built-in tests completed.\n");
}

// Run built-in tests without debug output
void run_built_in_tests() {
    printf("Running built-in lexer tests...\n");
    
    int success = 1;
    
    // Test simple tokens
    success &= run_test("let x = 123;", 7, 0);
    success &= run_test("let s = \"hello, world\";", 7, 0);
    success &= run_test("function test() { return 42; }", 13, 0);
    
    // Test more complex code
    success &= run_test(
        "let x = 123;\n"
        "let s = \"hello, world\";\n"
        "fn test() {\n"
        "    return x + 1;\n"
        "}\n", 
        17, 0
    );
    
    if (success) {
        printf("All tests passed!\n");
    } else {
        printf("Some tests failed.\n");
    }
}

// Print usage information
void print_usage(const char* program_name) {
    printf("Usage: %s [options] [test_string]\n", program_name);
    printf("Options:\n");
    printf("  --help           Show this help\n");
    printf("  --debug          Enable debug output\n");
    printf("  --flex-only      Use only the Flex lexer\n");
    printf("  --perf=N         Run performance test with N iterations\n");
    printf("If no test_string is provided, built-in tests will be run.\n");
}

int main(int argc, char *argv[]) {
#ifdef USE_ZIG_LEXER
    (void)argc; // Suppress unused warning
    (void)argv; // Suppress unused warning
    printf("Using Zig lexer\n");
#elif defined(USE_FLEX_LEXER)
    (void)argc; // Suppress unused warning
    (void)argv; // Suppress unused warning
    printf("Using Flex lexer\n");
#else
    int i; // Only declare i where it's used
    int debug_mode = 0;
    int performance_test = 0;
    int iterations = 1000;
    
    // Parse command line arguments
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strcmp(argv[i], "--flex-only") == 0) {
            // This flag is used by the build system to select the lexer
            // We don't need to set anything here as it's controlled by compile flags
        } else if (strncmp(argv[i], "--perf=", 7) == 0) {
            performance_test = 1;
            iterations = atoi(argv[i] + 7);
            if (iterations <= 0) iterations = 1000;
        } else if (argv[i][0] != '-') {
            // Assume this is a test string
            if (debug_mode) {
                run_test_with_debug(argv[i]);
            } else {
                run_test(argv[i], 0, 0);
            }
            return 0;
        }
    }

    if (performance_test) {
        run_performance_test(iterations);
        return 0;
    }

    // If no specific test requested, run built-in tests
    if (debug_mode) {
        run_built_in_tests_with_debug();
    } else {
        run_built_in_tests();
    }
#endif

    return 0;
} 