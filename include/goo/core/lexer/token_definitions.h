/**
 * @file token_definitions.h
 * @brief Common token definitions for the Goo lexer
 * 
 * This header provides a unified set of token type definitions to be used
 * across the codebase, avoiding redefinitions and inconsistencies.
 */

#ifndef GOO_TOKEN_DEFINITIONS_H
#define GOO_TOKEN_DEFINITIONS_H

#include <stdint.h>

// Basic token types
#define ERROR_TOKEN 0
#define EOF_TOKEN 1
#define IDENTIFIER 2

// Literal token types
#define INT_LITERAL 3
#define FLOAT_LITERAL 4
#define STRING_LITERAL 5
#define BOOL_LITERAL 6
#define RANGE_LITERAL 7

// Operators
#define PLUS 10
#define MINUS 11
#define ASTERISK 12
#define SLASH 13
#define PERCENT 14
#define EQUAL 15
#define NOT_EQUAL 16
#define LESS_THAN 17
#define GREATER_THAN 18
#define LESS_EQUAL 19
#define GREATER_EQUAL 20
#define ASSIGN 21
#define PLUS_ASSIGN 22
#define MINUS_ASSIGN 23
#define ASTERISK_ASSIGN 24
#define SLASH_ASSIGN 25
#define LOGICAL_AND 26
#define LOGICAL_OR 27
#define LOGICAL_NOT 28
#define BITWISE_AND 29
#define BITWISE_OR 30
#define BITWISE_XOR 31
#define BITWISE_NOT 32
#define SHIFT_LEFT 33
#define SHIFT_RIGHT 34

// Punctuation
#define COMMA 40
#define SEMICOLON 41
#define COLON 42
#define DOT 43
#define LPAREN 44
#define RPAREN 45
#define LBRACE 46
#define RBRACE 47
#define LBRACKET 48
#define RBRACKET 49
#define RANGE 50
#define ARROW 51

// Keywords
#define FUNCTION 60
#define LET 61
#define CONST 62
#define IF 63
#define ELSE 64
#define RETURN 65
#define WHILE 66
#define FOR 67
#define TRUE 68
#define FALSE 69
#define IN 70
#define BREAK 71
#define CONTINUE 72
#define NULL_TOKEN 73
#define IMPORT 74
#define AS 75
#define TYPE 76
#define STRUCT 77
#define ENUM 78
#define INTERFACE 79
#define IMPLEMENT 80
#define MATCH 81
#define PUBLIC 82
#define PRIVATE 83
#define TRY 84
#define CATCH 85
#define THROW 86
#define DEFER 87
#define ASYNC 88
#define AWAIT 89
#define USING 90

// Additional tokens
#define PACKAGE 100
#define FUNC 101
#define VAR 102
#define UNSAFE 103
#define SAFE 104
#define GO 105
#define PARALLEL 106
#define CHAN 107
#define COMPTIME 108
#define BUILD 109
#define SUPER 110
#define RECOVER 111
#define SUPERVISE 112
#define KERNEL 113
#define USER 114
#define MODULE 115
#define CAP 116
#define SHARED 117
#define REFLECT 118
#define ALLOCATOR 119
#define ALLOC 120
#define FREE 121
#define SCOPE 122
#define HEAP 123
#define ARENA 124
#define FIXED 125
#define POOL 126
#define BUMP 127
#define CUSTOM 128
#define SIMD 129
#define VECTOR 130
#define ALIGNED 131
#define MASK 132
#define FUSED 133
#define AUTO 134
#define ARCH 135
#define AUTO_DETECT 136
#define ALLOW_FALLBACK 137

// Type tokens
#define INT_TYPE 140
#define INT8_TYPE 141
#define INT16_TYPE 142
#define INT32_TYPE 143
#define INT64_TYPE 144
#define UINT_TYPE 145
#define FLOAT32_TYPE 146
#define FLOAT64_TYPE 147
#define BOOL_TYPE 148
#define STRING_TYPE 149

// Common token structures

// Range value structure
typedef struct {
    int64_t start;
    int64_t end;
    int inclusive;  // 0 = false, 1 = true
} GooRangeValue;

// Token value union
typedef union {
    int64_t int_value;
    double float_value;
    int bool_value;          // 0 = false, 1 = true
    const char* string_value;
    GooRangeValue range_value;
} GooTokenValue;

// Token structure
typedef struct {
    int type;
    int line;
    int column;
    const char* literal;
    int has_value;           // 0 = no value, 1 = has value
    GooTokenValue value;
} GooToken;

// Opaque lexer type
typedef struct GooLexer GooLexer;

#endif /* GOO_TOKEN_DEFINITIONS_H */ 