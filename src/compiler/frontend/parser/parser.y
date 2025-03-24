%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "goo.h"  // Include goo.h first for type definitions
#include "ast.h"
#include "ast_helpers.h"

// External functions from lexer
extern int yylex();
extern int line_num;
extern int col_num;
extern FILE* yyin;

// AST building context
GooAst* current_ast = NULL;

// Error handling
void yyerror(const char* msg) {
    fprintf(stderr, "Error at line %d, column %d: %s\n", line_num, col_num, msg);
}

%}

%union {
    int int_val;
    double float_val;
    bool bool_val;
    char* string_val;
    struct GooNode* node;
    struct GooAst* ast;
    int alloc_type;  // Using int instead of GooAllocatorType to avoid issues
}

/* Terminal tokens */
%token PACKAGE IMPORT FUNC VAR 
%token IF ELSE FOR RETURN
%token GO PARALLEL CHAN
%token COMPTIME BUILD SUPER
%token TRY RECOVER SUPERVISE
%token KERNEL USER MODULE
%token CAP SHARED PRIVATE
%token REFLECT ALLOCATOR
%token ALLOC FREE SCOPE
%token HEAP ARENA FIXED POOL BUMP CUSTOM
%token SIMD VECTOR ALIGNED MASK FUSED SAFE UNSAFE AUTO ARCH
%token AUTO_DETECT ALLOW_FALLBACK

%token INT_TYPE INT8_TYPE INT16_TYPE INT32_TYPE INT64_TYPE 
%token UINT_TYPE FLOAT32_TYPE FLOAT64_TYPE BOOL_TYPE STRING_TYPE
%token PUB SUB PUSH PULL REQ REP DEALER ROUTER PAIR
%token ARROW EQ NEQ LEQ GEQ AND OR
%token DECLARE_ASSIGN
%token SIMD VECTOR ALIGNED MASK FUSED AUTO ARCH

%token <int_val> INT_LITERAL
%token <float_val> FLOAT_LITERAL
%token <bool_val> BOOL_LITERAL
%token <string_val> STRING_LITERAL IDENTIFIER RANGE_LITERAL
%token <string_val> RANGE       /* Range literal (e.g., 1..10) */

/* Non-terminal types */
%type <node> program decl_list declaration
%type <node> package_decl import_decl function_decl kernel_func_decl user_func_decl
%type <node> var_decl channel_decl comptime_var_decl comptime_build_decl module_decl allocator_decl
%type <node> param_list param type_expr cap_type_expr allocator_options
%type <node> statement_list statement block_stmt if_stmt for_stmt
%type <node> go_stmt go_parallel_stmt go_parallel_options parallel_option_list parallel_option
%type <node> supervise_stmt supervise_block
%type <node> try_stmt recover_block
%type <node> return_stmt expr_stmt
%type <node> channel_send channel_recv
%type <node> expr binary_expr unary_expr literal identifier
%type <node> super_expr call_expr
%type <node> channel_pattern
%type <node> declaration_with_assignment
%type <node> alloc_expr free_expr scope_block optional_allocator
%type <alloc_type> allocator_type

/* Precedence (lowest to highest) */
%left OR
%left AND
%left EQ NEQ
%left '<' '>' LEQ GEQ
%left '+' '-'
%left '*' '/'
%right '!' UNARY_MINUS

%%

/* Grammar rules */

program
    : decl_list                { $$ = $1; }
    ;

decl_list
    : declaration              { $$ = $1; }
    | decl_list declaration    { 
                                 $1->next = $2; 
                                 $$ = $1; 
                               }
    ;

declaration
    : package_decl             { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | import_decl              { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | function_decl            { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | kernel_func_decl         { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | user_func_decl           { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | var_decl                 { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | channel_decl             { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | comptime_var_decl        { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | comptime_build_decl      { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | comptime_simd_decl       { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | module_decl              { $$ = $1; goo_ast_add_node(current_ast, $$); }
    | allocator_decl           { $$ = $1; goo_ast_add_node(current_ast, $$); }
    ;

package_decl
    : PACKAGE IDENTIFIER       { $$ = (GooNode*)goo_ast_create_package_node($2, line_num, col_num); }
    ;

import_decl
    : IMPORT STRING_LITERAL    { $$ = (GooNode*)goo_ast_create_import_node($2, line_num, col_num); }
    ;

function_decl
    : FUNC IDENTIFIER '(' param_list ')' type_expr block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($2, $4, $6, $7, false, false, false, NULL, line_num, col_num); }
    | FUNC IDENTIFIER '(' param_list ')' block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($2, $4, NULL, $6, false, false, false, NULL, line_num, col_num); }
    | UNSAFE FUNC IDENTIFIER '(' param_list ')' type_expr block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, $7, $8, false, false, true, NULL, line_num, col_num); }
    | UNSAFE FUNC IDENTIFIER '(' param_list ')' block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, NULL, $7, false, false, true, NULL, line_num, col_num); }
    ;

kernel_func_decl
    : KERNEL FUNC IDENTIFIER '(' param_list ')' type_expr block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, $7, $8, true, false, false, NULL, line_num, col_num); }
    | KERNEL FUNC IDENTIFIER '(' param_list ')' block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, NULL, $7, true, false, false, NULL, line_num, col_num); }
    | KERNEL UNSAFE FUNC IDENTIFIER '(' param_list ')' type_expr block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($4, $6, $8, $9, true, false, true, NULL, line_num, col_num); }
    | KERNEL UNSAFE FUNC IDENTIFIER '(' param_list ')' block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($4, $6, NULL, $8, true, false, true, NULL, line_num, col_num); }
    ;

user_func_decl
    : USER FUNC IDENTIFIER '(' param_list ')' type_expr block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, $7, $8, false, true, false, NULL, line_num, col_num); }
    | USER FUNC IDENTIFIER '(' param_list ')' block_stmt 
                                { $$ = (GooNode*)goo_ast_create_function_node($3, $5, NULL, $7, false, true, false, NULL, line_num, col_num); }
    ;

param_list
    : /* empty */              { $$ = NULL; }
    | param                    { $$ = $1; }
    | param_list ',' param     { 
                                 $1->next = $3; 
                                 $$ = $1; 
                               }
    ;

param
    : IDENTIFIER type_expr     { $$ = (GooNode*)goo_ast_create_param_node($1, $2, false, false, 0, @$.first_line, @$.first_column); }
    | IDENTIFIER cap_type_expr { $$ = (GooNode*)goo_ast_create_param_node($1, $2, true, false, 0, @$.first_line, @$.first_column); }
    | ALLOCATOR IDENTIFIER allocator_type { $$ = (GooNode*)goo_ast_create_param_node($2, NULL, false, true, $3, @$.first_line, @$.first_column); }
    ;

type_expr
    : INT_TYPE                 { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_INT_TYPE, NULL, false, line_num, col_num); }
    | INT8_TYPE                { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_INT8_TYPE, NULL, false, line_num, col_num); }
    | INT16_TYPE               { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_INT16_TYPE, NULL, false, line_num, col_num); }
    | INT32_TYPE               { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_INT32_TYPE, NULL, false, line_num, col_num); }
    | INT64_TYPE               { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_INT64_TYPE, NULL, false, line_num, col_num); }
    | UINT_TYPE                { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_UINT_TYPE, NULL, false, line_num, col_num); }
    | FLOAT32_TYPE             { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_FLOAT32_TYPE, NULL, false, line_num, col_num); }
    | FLOAT64_TYPE             { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_FLOAT64_TYPE, NULL, false, line_num, col_num); }
    | BOOL_TYPE                { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_BOOL_TYPE, NULL, false, line_num, col_num); }
    | STRING_TYPE              { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_STRING_TYPE, NULL, false, line_num, col_num); }
    | CHAN '<' type_expr '>'   { $$ = (GooNode*)goo_ast_create_type_node(GOO_NODE_CHANNEL_DECL, $3, false, line_num, col_num); }
    ;

cap_type_expr
    : CAP type_expr            { 
                                 GooTypeNode* type_node = (GooTypeNode*)$2;
                                 type_node->is_capability = true;
                                 $$ = $2;
                               }
    ;

var_decl
    : VAR IDENTIFIER type_expr '=' expr optional_allocator 
                                { $$ = (GooNode*)goo_ast_create_var_decl_node($2, $3, $5, false, false, $6, line_num, col_num); }
    | SAFE VAR IDENTIFIER type_expr '=' expr optional_allocator 
                                { $$ = (GooNode*)goo_ast_create_var_decl_node($3, $4, $6, true, false, $7, line_num, col_num); }
    ;

declaration_with_assignment
    : IDENTIFIER DECLARE_ASSIGN expr optional_allocator
                                { $$ = (GooNode*)goo_ast_create_var_decl_node($1, NULL, $3, false, false, $4, line_num, col_num); }
    ;

channel_decl
    : VAR IDENTIFIER channel_pattern CHAN '<' type_expr '>' 
                                { $$ = (GooNode*)goo_ast_create_channel_decl_node($2, (GooChannelPattern)$3, $6, NULL, line_num, col_num); }
    | VAR IDENTIFIER channel_pattern CHAN '<' type_expr '>' '@' STRING_LITERAL 
                                { $$ = (GooNode*)goo_ast_create_channel_decl_node($2, (GooChannelPattern)$3, $6, $9, line_num, col_num); }
    | VAR IDENTIFIER CAP channel_pattern CHAN '<' type_expr '>' 
                                { 
                                  GooChannelDeclNode* node = goo_ast_create_channel_decl_node($2, (GooChannelPattern)$4, $7, NULL, line_num, col_num);
                                  node->has_capability = true;
                                  $$ = (GooNode*)node;
                                }
    | VAR IDENTIFIER CAP channel_pattern CHAN '<' type_expr '>' '@' STRING_LITERAL 
                                { 
                                  GooChannelDeclNode* node = goo_ast_create_channel_decl_node($2, (GooChannelPattern)$4, $7, $10, line_num, col_num);
                                  node->has_capability = true;
                                  $$ = (GooNode*)node;
                                }
    ;

channel_pattern
    : /* empty */              { $$ = (GooNode*)GOO_CHAN_DEFAULT; }
    | PUB                      { $$ = (GooNode*)GOO_CHAN_PUB; }
    | SUB                      { $$ = (GooNode*)GOO_CHAN_SUB; }
    | PUSH                     { $$ = (GooNode*)GOO_CHAN_PUSH; }
    | PULL                     { $$ = (GooNode*)GOO_CHAN_PULL; }
    | REQ                      { $$ = (GooNode*)GOO_CHAN_REQ; }
    | REP                      { $$ = (GooNode*)GOO_CHAN_REP; }
    | DEALER                   { $$ = (GooNode*)GOO_CHAN_DEALER; }
    | ROUTER                   { $$ = (GooNode*)GOO_CHAN_ROUTER; }
    | PAIR                     { $$ = (GooNode*)GOO_CHAN_PAIR; }
    ;

comptime_var_decl
    : COMPTIME VAR IDENTIFIER type_expr '=' expr 
                                { $$ = (GooNode*)goo_ast_create_var_decl_node($3, $4, $6, false, true, NULL, line_num, col_num); }
    ;

comptime_build_decl
    : COMPTIME BUILD block_stmt 
                                { $$ = (GooNode*)goo_ast_create_comptime_build_node($3, line_num, col_num); }
    ;

comptime_simd_decl
    : COMPTIME SIMD '{' simd_decl_list '}'
                                { $$ = (GooNode*)goo_ast_create_comptime_simd_node($4, line_num, col_num); }
    ;

simd_decl_list
    : simd_decl                 { $$ = $1; }
    | simd_decl_list simd_decl  { 
                                  $1->next = $2; 
                                  $$ = $1; 
                                }
    ;

simd_decl
    : simd_type_decl            { $$ = $1; }
    | simd_op_decl              { $$ = $1; }
    | simd_config               { $$ = $1; }
    ;

simd_type_decl
    : VECTOR IDENTIFIER vector_data_type ALIGNED '(' INT_LITERAL ')' '{' simd_type_options '}' ';'
                                { 
                                  bool is_safe = true; // Safe by default
                                  GooSIMDType simd_type = GOO_SIMD_AUTO;
                                  
                                  // Process options from simd_type_options
                                  GooNode* options = $9;
                                  while (options) {
                                      // Here we would normally extract settings
                                      // For simplicity, we're defaulting
                                      options = options->next;
                                  }
                                  
                                  $$ = (GooNode*)goo_ast_create_simd_type_node(
                                      $2, (GooVectorDataType)$3, 0, simd_type, 
                                      is_safe, $6, line_num, col_num);
                                }
    ;

vector_data_type
    : INT8_TYPE                 { $$ = (GooNode*)GOO_VEC_INT8; }
    | UINT8_TYPE                { $$ = (GooNode*)GOO_VEC_UINT8; }
    | INT16_TYPE                { $$ = (GooNode*)GOO_VEC_INT16; }
    | UINT16_TYPE               { $$ = (GooNode*)GOO_VEC_UINT16; }
    | INT32_TYPE                { $$ = (GooNode*)GOO_VEC_INT32; }
    | UINT32_TYPE               { $$ = (GooNode*)GOO_VEC_UINT32; }
    | INT64_TYPE                { $$ = (GooNode*)GOO_VEC_INT64; }
    | UINT64_TYPE               { $$ = (GooNode*)GOO_VEC_UINT64; }
    | FLOAT32_TYPE              { $$ = (GooNode*)GOO_VEC_FLOAT; }
    | FLOAT64_TYPE              { $$ = (GooNode*)GOO_VEC_DOUBLE; }
    ;

simd_type_options
    : /* empty */               { $$ = NULL; }
    | simd_type_option          { $$ = $1; }
    | simd_type_options simd_type_option 
                                { 
                                  $1->next = $2; 
                                  $$ = $1; 
                                }
    ;

simd_type_option
    : UNSAFE ';'                { $$ = create_identifier_node("unsafe", line_num, col_num); }
    | SAFE ';'                  { $$ = create_identifier_node("safe", line_num, col_num); }
    | ARCH '=' simd_arch ';'    { $$ = $3; }
    ;

simd_arch
    : AUTO                      { $$ = create_identifier_node("auto", line_num, col_num); }
    | IDENTIFIER                { $$ = create_identifier_node($1, line_num, col_num); }
    ;

simd_op_decl
    : IDENTIFIER '=' vector_op vector_options ';'
                                {
                                  GooVectorOp op = GOO_VECTOR_ADD; // Default
                                  bool is_masked = false;
                                  bool is_fused = false;
                                  GooNode* vec_type = NULL;
                                  
                                  // Process options
                                  GooNode* options = $4;
                                  while (options) {
                                      // Here we would normally extract settings
                                      options = options->next;
                                  }
                                  
                                  $$ = (GooNode*)goo_ast_create_simd_op_node(
                                      $1, op, vec_type, is_masked, is_fused,
                                      line_num, col_num);
                                }
    ;

vector_op
    : IDENTIFIER                { $$ = create_identifier_node($1, line_num, col_num); }
    ;

vector_options
    : /* empty */               { $$ = NULL; }
    | vector_option             { $$ = $1; }
    | vector_options vector_option
                                {
                                  $1->next = $2;
                                  $$ = $1;
                                }
    ;

vector_option
    : MASK                     { $$ = create_identifier_node("mask", line_num, col_num); }
    | FUSED                    { $$ = create_identifier_node("fused", line_num, col_num); }
    | VECTOR '=' IDENTIFIER    { $$ = create_identifier_node($3, line_num, col_num); }
    ;

simd_config
    : ARCH '=' IDENTIFIER ';'  { $$ = create_identifier_node($3, line_num, col_num); }
    | AUTO_DETECT '=' BOOL_LITERAL ';'  
                               { $$ = create_identifier_node($3 ? "true" : "false", line_num, col_num); }
    | ALLOW_FALLBACK '=' BOOL_LITERAL ';'
                               { $$ = create_identifier_node($3 ? "true" : "false", line_num, col_num); }
    ;

module_decl
    : MODULE IDENTIFIER '{' decl_list '}'
                                { $$ = (GooNode*)goo_ast_create_module_decl_node($2, $4, line_num, col_num); }
    ;

allocator_decl
    : ALLOCATOR IDENTIFIER allocator_type allocator_options
                                { (yyval.node) = (GooNode*)goo_ast_create_allocator_decl_node((yyvsp[(2) - (4)].string_val), (GooAllocatorType)(yyvsp[(3) - (4)].alloc_type), (yyvsp[(4) - (4)].node), line_num, col_num); }
    ;

allocator_type
    : HEAP
    { (yyval.alloc_type) = GOO_ALLOC_HEAP; }
    | ARENA
    { (yyval.alloc_type) = GOO_ALLOC_ARENA; }
    | FIXED
    { (yyval.alloc_type) = GOO_ALLOC_FIXED; }
    | POOL
    { (yyval.alloc_type) = GOO_ALLOC_POOL; }
    | BUMP
    { (yyval.alloc_type) = GOO_ALLOC_BUMP; }
    | CUSTOM
    { (yyval.alloc_type) = GOO_ALLOC_CUSTOM; }
    ;

allocator_options
    : /* empty */              { $$ = NULL; }
    | '{' expr_stmt '}'        { $$ = $2; }
    ;

optional_allocator
    : /* empty */              { $$ = NULL; }
    | '[' ALLOCATOR ':' IDENTIFIER ']'
                                { $$ = create_identifier_node($4, @$.first_line, @$.first_column); }
    ;

statement_list
    : statement                { $$ = $1; }
    | statement_list statement { 
                                 $1->next = $2; 
                                 $$ = $1; 
                               }
    ;

statement
    : block_stmt               { $$ = $1; }
    | if_stmt                  { $$ = $1; }
    | for_stmt                 { $$ = $1; }
    | go_stmt                  { $$ = $1; }
    | go_parallel_stmt         { $$ = $1; }
    | supervise_stmt           { $$ = $1; }
    | try_stmt                 { $$ = $1; }
    | return_stmt              { $$ = $1; }
    | var_decl                 { $$ = $1; }
    | declaration_with_assignment { $$ = $1; }
    | channel_decl             { $$ = $1; }
    | expr_stmt                { $$ = $1; }
    | alloc_expr               { $$ = $1; }
    | free_expr                { $$ = $1; }
    | scope_block              { $$ = $1; }
    | allocator_decl           { $$ = $1; }
    ;

block_stmt
    : '{' statement_list '}'   { $$ = (GooNode*)goo_ast_create_block_stmt_node($2, line_num, col_num); }
    | '{' '}'                  { $$ = (GooNode*)goo_ast_create_block_stmt_node(NULL, line_num, col_num); }
    ;

if_stmt
    : IF expr block_stmt                   { $$ = (GooNode*)goo_ast_create_if_stmt_node($2, $3, NULL, line_num, col_num); }
    | IF expr block_stmt ELSE block_stmt   { $$ = (GooNode*)goo_ast_create_if_stmt_node($2, $3, $5, line_num, col_num); }
    | IF expr block_stmt ELSE if_stmt      { $$ = (GooNode*)goo_ast_create_if_stmt_node($2, $3, $5, line_num, col_num); }
    ;

for_stmt
    : FOR expr block_stmt      { $$ = (GooNode*)goo_ast_create_for_stmt_node($2, NULL, NULL, $3, false, line_num, col_num); }
    | FOR RANGE_LITERAL block_stmt { $$ = (GooNode*)goo_ast_create_for_stmt_node(create_range_literal_node($2, @2.first_line, @2.first_column), NULL, NULL, $3, true, line_num, col_num); }
    ;

go_stmt
    : GO expr                  { $$ = (GooNode*)goo_ast_create_go_stmt_node($2, line_num, col_num); }
    ;

go_parallel_stmt
    : GO PARALLEL block_stmt   { $$ = (GooNode*)goo_ast_create_go_parallel_node($3, NULL, line_num, col_num); }
    | GO PARALLEL go_parallel_options block_stmt { $$ = (GooNode*)goo_ast_create_go_parallel_node($4, $3, line_num, col_num); }
    ;

go_parallel_options
    : '[' parallel_option_list ']' { $$ = $2; }
    ;

parallel_option_list
    : parallel_option                      { $$ = $1; }
    | parallel_option_list ',' parallel_option { $$ = append_node($1, $3); }
    ;

parallel_option
    : RANGE ':' RANGE                           { $$ = create_range_option_node($3, @$.first_line, @$.first_column); }
    | SHARED ':' IDENTIFIER                     { $$ = create_shared_var_node($3, @$.first_line, @$.first_column); }
    | PRIVATE ':' IDENTIFIER                    { $$ = create_private_var_node($3, @$.first_line, @$.first_column); }
    ;

supervise_stmt
    : SUPERVISE block_stmt     { $$ = (GooNode*)goo_ast_create_supervise_stmt_node($2, line_num, col_num); }
    | SUPERVISE GO expr        { $$ = (GooNode*)goo_ast_create_supervise_stmt_node($3, line_num, col_num); }
    ;

supervise_block
    : SUPERVISE '{' statement_list '}'     { $$ = (GooNode*)goo_ast_create_supervise_stmt_node($3, line_num, col_num); }
    ;

try_stmt
    : TRY expr '!' IDENTIFIER  { $$ = (GooNode*)goo_ast_create_try_stmt_node($2, $4, NULL, line_num, col_num); }
    | TRY block_stmt recover_block { $$ = (GooNode*)goo_ast_create_try_stmt_node($2, NULL, $3, line_num, col_num); }
    | TRY expr recover_block   { $$ = (GooNode*)goo_ast_create_try_stmt_node($2, NULL, $3, line_num, col_num); }
    ;

recover_block
    : RECOVER block_stmt       { $$ = $2; }
    | RECOVER '(' IDENTIFIER ')' block_stmt { $$ = $5; /* TODO: Handle error parameter */ }
    ;

return_stmt
    : RETURN expr              { $$ = (GooNode*)goo_ast_create_return_stmt_node($2, line_num, col_num); }
    | RETURN                   { $$ = (GooNode*)goo_ast_create_return_stmt_node(NULL, line_num, col_num); }
    ;

expr_stmt
    : expr                     { $$ = $1; }
    | channel_send             { $$ = $1; }
    | channel_recv             { $$ = $1; }
    ;

channel_send
    : ARROW expr '=' expr      { $$ = (GooNode*)goo_ast_create_channel_send_node($2, $4, line_num, col_num); }
    ;

channel_recv
    : IDENTIFIER DECLARE_ASSIGN ARROW expr  
                                { 
                                  // Create a variable declaration with the value from a channel receive
                                  GooChannelRecvNode* recv = goo_ast_create_channel_recv_node($4, line_num, col_num);
                                  $$ = (GooNode*)goo_ast_create_var_decl_node($1, NULL, (GooNode*)recv, false, false, NULL, line_num, col_num);
                                }
    | ARROW expr               { $$ = (GooNode*)goo_ast_create_channel_recv_node($2, line_num, col_num); }
    ;

alloc_expr
    : ALLOC type_expr optional_allocator { $$ = (GooNode*)goo_ast_create_alloc_expr_node($2, NULL, $3, line_num, col_num); }
    | ALLOC type_expr '[' expr ']' optional_allocator { $$ = (GooNode*)goo_ast_create_alloc_expr_node($2, $4, $6, line_num, col_num); }
    ;

free_expr
    : FREE expr optional_allocator { $$ = (GooNode*)goo_ast_create_free_expr_node($2, $3, line_num, col_num); }
    ;

scope_block
    : SCOPE '(' IDENTIFIER ')' block_stmt { $$ = (GooNode*)goo_ast_create_scope_block_node(create_identifier_node($3, @3.first_line, @3.first_column), $5, line_num, col_num); }
    ;

expr
    : binary_expr              { $$ = $1; }
    | unary_expr               { $$ = $1; }
    | literal                  { $$ = $1; }
    | identifier               { $$ = $1; }
    | super_expr               { $$ = $1; }
    | call_expr                { $$ = $1; }
    | '(' expr ')'             { $$ = $2; }
    | RANGE_LITERAL            { $$ = create_range_literal_node($1, @$.first_line, @$.first_column); }
    ;

binary_expr
    : expr '+' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '+', $3, line_num, col_num); }
    | expr '-' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '-', $3, line_num, col_num); }
    | expr '*' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '*', $3, line_num, col_num); }
    | expr '/' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '/', $3, line_num, col_num); }
    | expr '<' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '<', $3, line_num, col_num); }
    | expr '>' expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, '>', $3, line_num, col_num); }
    | expr EQ expr             { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, EQ, $3, line_num, col_num); }
    | expr NEQ expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, NEQ, $3, line_num, col_num); }
    | expr LEQ expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, LEQ, $3, line_num, col_num); }
    | expr GEQ expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, GEQ, $3, line_num, col_num); }
    | expr AND expr            { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, AND, $3, line_num, col_num); }
    | expr OR expr             { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, OR, $3, line_num, col_num); }
    | expr RANGE expr          { $$ = (GooNode*)goo_ast_create_binary_expr_node($1, RANGE, $3, line_num, col_num); }
    ;

unary_expr
    : '!' expr                 { $$ = (GooNode*)goo_ast_create_unary_expr_node('!', $2, line_num, col_num); }
    | '-' expr %prec UNARY_MINUS 
                               { $$ = (GooNode*)goo_ast_create_unary_expr_node('-', $2, line_num, col_num); }
    ;

literal
    : INT_LITERAL              { $$ = (GooNode*)goo_ast_create_int_literal_node($1, @$.first_line, @$.first_column); }
    | FLOAT_LITERAL            { $$ = (GooNode*)goo_ast_create_float_literal_node($1, @$.first_line, @$.first_column); }
    | BOOL_LITERAL             { $$ = (GooNode*)goo_ast_create_bool_literal_node($1, @$.first_line, @$.first_column); }
    | STRING_LITERAL           { $$ = (GooNode*)goo_ast_create_string_literal_node($1, @$.first_line, @$.first_column); }
    ;

identifier
    : IDENTIFIER               { $$ = create_identifier_node($1, @$.first_line, @$.first_column); }
    ;

super_expr
    : SUPER expr               { $$ = (GooNode*)goo_ast_create_super_expr_node($2, line_num, col_num); }
    ;

call_expr
    : identifier '(' ')'       { $$ = (GooNode*)goo_ast_create_call_expr_node($1, NULL, line_num, col_num); }
    | identifier '(' expr ')'  { $$ = (GooNode*)goo_ast_create_call_expr_node($1, $3, line_num, col_num); }
    ;

%%

// Initialize parser and set up the AST
GooAst* goo_parse_file(GooContext* ctx, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return NULL;
    }
    
    yyin = file;
    current_ast = goo_ast_create(filename);
    
    if (yyparse() != 0) {
        goo_ast_free(current_ast);
        current_ast = NULL;
    }
    
    fclose(file);
    
    return current_ast;
} 