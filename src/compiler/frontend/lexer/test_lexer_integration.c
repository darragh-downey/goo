#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/goo_lexer.h"

// Function declarations
void tokenize_file(const char *filename);

// External declarations from the parser
extern int yyparse(void);
extern int yydebug;

// Token constants for Bison integration
#define PACKAGE 258
#define IMPORT 259
#define FUNC 260
#define VAR 261
#define SAFE 262
#define UNSAFE 263
#define IF 264
#define ELSE 265
#define FOR 266
#define RETURN 267
#define GO 268
#define PARALLEL 269
#define CHAN 270
#define COMPTIME 271
#define BUILD 272
#define SUPER 273
#define TRY 274
#define RECOVER 275
#define SUPERVISE 276
#define KERNEL 277
#define USER 278
#define MODULE 279
#define CAP 280
#define SHARED 281
#define PRIVATE 282
#define REFLECT 283
#define ALLOCATOR 284
#define ALLOC 285
#define FREE 286
#define SCOPE 287
#define HEAP 288
#define ARENA 289
#define FIXED 290
#define POOL 291
#define BUMP 292
#define CUSTOM 293
#define INT_TYPE 294
#define INT8_TYPE 295
#define INT16_TYPE 296
#define INT32_TYPE 297
#define INT64_TYPE 298
#define UINT_TYPE 299
#define FLOAT32_TYPE 300
#define FLOAT64_TYPE 301
#define BOOL_TYPE 302
#define STRING_TYPE 303
#define PUB 304
#define SUB 305
#define PUSH 306
#define PULL 307
#define REQ 308
#define REP 309
#define DEALER 310
#define ROUTER 311
#define PAIR 312
#define ARROW 313
#define EQ 314
#define NEQ 315
#define LEQ 316
#define GEQ 317
#define AND 318
#define OR 319
#define DECLARE_ASSIGN 320
#define INT_LITERAL 321
#define FLOAT_LITERAL 322
#define BOOL_LITERAL 323
#define STRING_LITERAL 324
#define IDENTIFIER 325
#define RANGE_LITERAL 326
#define RANGE 327
#define UNARY_MINUS 328
#define SIMD 329
#define VECTOR 330
#define ALIGNED 331
#define MASK 332
#define FUSED 333
#define AUTO 334
#define ARCH 335
#define AUTO_DETECT 336
#define ALLOW_FALLBACK 337

// Bison YYSTYPE union for semantic values
typedef union {
    int int_val;
    double float_val;
    bool bool_val;
    char* string_val;
    struct GooNode* node;
    struct GooAst* ast;
    int alloc_type;
} YYSTYPE;

// Global variables for yylval and yylloc
YYSTYPE yylval;
int yylineno = 1;

typedef struct {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;

YYLTYPE yylloc;

// Global variables for lexer integration
GooLexer *current_lexer = NULL;
GooToken current_token;
int token_consumed = 1;

// Implementation of yylex for Bison integration
int yylex(void) {
    // Return the current token if not consumed
    if (!token_consumed) {
        token_consumed = 1;
        return current_token.token_type;
    }
    
    // Free the previous token's resources
    goo_token_free(current_token);
    
    // Get the next token
    if (current_lexer == NULL) {
        fprintf(stderr, "Lexer not initialized\n");
        return 0; // EOF
    }
    
    current_token = goo_lexer_next_token(current_lexer);
    token_consumed = 1;
    
    // Update yylval based on token type
    switch (current_token.token_type) {
        case IDENTIFIER:
            if (current_token.string_value) {
                yylval.string_val = strdup(current_token.string_value);
            } else {
                yylval.string_val = NULL;
            }
            break;
            
        case STRING_LITERAL:
        case RANGE_LITERAL:
            if (current_token.string_value) {
                yylval.string_val = strdup(current_token.string_value);
            } else {
                yylval.string_val = NULL;
            }
            break;
            
        case INT_LITERAL:
            yylval.int_val = (int)current_token.int_value;
            break;
            
        case FLOAT_LITERAL:
            yylval.float_val = current_token.float_value;
            break;
            
        case BOOL_LITERAL:
            yylval.bool_val = current_token.bool_value;
            break;
    }
    
    // Update line and column information
    yylineno = current_token.line;
    yylloc.first_line = current_token.line;
    yylloc.first_column = current_token.column;
    yylloc.last_line = current_token.line;
    yylloc.last_column = current_token.column;
    
    // For debugging
    printf("Token: %s (%d) at line %d, column %d\n", 
           goo_token_get_name(current_token.token_type),
           current_token.token_type,
           current_token.line, 
           current_token.column);
    
    return current_token.token_type;
}

// Error handling for parser
void yyerror(const char *s) {
    fprintf(stderr, "Error: %s at line %d, column %d\n", 
            s, current_token.line, current_token.column);
}

// Read a file into a string
char *read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("Error opening file");
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate memory for the file contents
    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        perror("Memory allocation failed");
        return NULL;
    }
    
    // Read the file
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    return buffer;
}

// Main test function
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source_file> [--debug] [--tokenize-only]\n", argv[0]);
        return 1;
    }
    
    // Check for debug flag and tokenize-only mode
    int debug_mode = 0;
    int tokenize_only = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
            yydebug = 1;
        } else if (strcmp(argv[i], "--tokenize-only") == 0) {
            tokenize_only = 1;
        }
    }
    
    // Read the source file
    char *source = read_file_to_string(argv[1]);
    if (!source) {
        return 1;
    }
    
    // Initialize the Zig lexer
    current_lexer = goo_lexer_new(source);
    if (!current_lexer) {
        fprintf(stderr, "Failed to initialize lexer\n");
        free(source);
        return 1;
    }
    
    // Free the source string as the lexer has its own copy
    free(source);
    
    if (tokenize_only) {
        tokenize_file(argv[1]);
    } else {
        // Parse using the Zig lexer
        printf("Starting parse with Zig lexer...\n");
        int result = yyparse();
        printf("Parse %s. Result code: %d\n", 
               result == 0 ? "successful" : "failed", result);
    }
    
    // Cleanup
    goo_token_free(current_token);
    goo_lexer_free(current_lexer);
    
    return 0;
}

// Standalone tokenizer mode
void tokenize_file(const char *filename) {
    char *source = read_file_to_string(filename);
    if (!source) {
        return;
    }
    
    GooLexer *lexer = goo_lexer_new(source);
    free(source);
    
    if (!lexer) {
        fprintf(stderr, "Failed to initialize lexer\n");
        return;
    }
    
    printf("Tokens in %s:\n", filename);
    printf("--------------------\n");
    
    int token_count = 0;
    while (1) {
        GooToken token = goo_lexer_next_token(lexer);
        token_count++;
        
        printf("%3d: %-15s at line %3d, column %3d", 
               token_count, 
               goo_token_get_name(token.token_type),
               token.line, 
               token.column);
        
        // Print token value based on type
        switch (token.token_type) {
            case INT_LITERAL:
                printf("  Value: %ld\n", token.int_value);
                break;
                
            case FLOAT_LITERAL:
                printf("  Value: %f\n", token.float_value);
                break;
                
            case BOOL_LITERAL:
                printf("  Value: %s\n", token.bool_value ? "true" : "false");
                break;
                
            case STRING_LITERAL:
            case IDENTIFIER:
            case RANGE_LITERAL:
                if (token.string_value) {
                    printf("  Value: \"%s\"\n", token.string_value);
                } else {
                    printf("\n");
                }
                break;
                
            default:
                printf("\n");
                break;
        }
        
        // Free token resources
        goo_token_free(token);
        
        // Stop at EOF
        if (token.token_type == 0) { // EOF
            break;
        }
    }
    
    printf("--------------------\n");
    printf("Total tokens: %d\n", token_count);
    
    goo_lexer_free(lexer);
} 