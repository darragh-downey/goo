/**
 * @file parser.h
 * @brief Minimal parser interface for lexer
 */

#ifndef GOO_PARSER_H
#define GOO_PARSER_H

/* Token definitions */
/* Language keywords */
#define PACKAGE     1
#define IMPORT      2
#define FUNC        3
#define VAR         4
#define UNSAFE      5
#define IF          6
#define ELSE        7
#define FOR         8
#define RETURN      9
#define GO          10
#define PARALLEL    11
#define CHAN        12
#define COMPTIME    13
#define BUILD       14
#define SUPER       15
#define TRY         16
#define RECOVER     17
#define SUPERVISE   18
#define KERNEL      19
#define USER        20
#define MODULE      21
#define CAP         22
#define SHARED      23
#define PRIVATE     24
#define REFLECT     25
#define ALLOCATOR   26
#define ALLOC       27
#define FREE        28
#define SCOPE       29
#define HEAP        30

/* Type keywords */
#define UINT_TYPE     31
#define FLOAT32_TYPE  32
#define FLOAT64_TYPE  33
#define BOOL_TYPE     34
#define STRING_TYPE   35

/* Messaging types */
#define PUB          36
#define SUB          37
#define PUSH         38
#define PULL         39
#define REQ          40
#define REP          41
#define DEALER       42
#define ROUTER       43
#define PAIR         44

/* Literals */
#define BOOL_LITERAL    45
#define INT_LITERAL     46
#define FLOAT_LITERAL   47
#define RANGE_LITERAL   48
#define STRING_LITERAL  49
#define IDENTIFIER      50

/* Operators and punctuation */
#define RANGE           51
#define DECLARE_ASSIGN  52
#define ARROW           53
#define EQ              54
#define NEQ             55
#define LEQ             56
#define GEQ             57
#define AND             58
#define OR              59

/* Token value union */
typedef union {
    int int_val;
    double float_val;
    int bool_val;
    char *str_val;
    void *ptr_val;
} YYSTYPE;

extern YYSTYPE yylval;

/* Function to report parser errors */
void yyerror(const char* msg);

#endif /* GOO_PARSER_H */ 