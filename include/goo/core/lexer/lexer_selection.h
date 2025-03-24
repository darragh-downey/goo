/**
 * @file lexer_selection.h
 * @brief Header to select between the Flex and Zig lexers
 * 
 * This header file provides the interface for selecting between the
 * Flex-based lexer and the Zig-based lexer at compile time based on
 * the USE_ZIG_LEXER preprocessor definition.
 */

#ifndef LEXER_SELECTION_H
#define LEXER_SELECTION_H

#include <stdio.h>
#include <stdint.h>
#include "token_definitions.h"

// Token type definitions
#ifdef USE_ZIG_LEXER
// Use the Zig lexer token definitions
#include <goo_lexer.h>
#else
// We're already using the token definitions from token_definitions.h
#endif

// Lexer initialization and cleanup
GooLexer* lexer_init_file(const char* filename);
GooLexer* lexer_init_string(const char* input);
void lexer_free(GooLexer* lexer);

// Token operations
GooToken lexer_next_token(GooLexer* lexer);

// Error handling
void lexer_set_error_callback(void (*callback)(const char* msg, int line, int col));

// For Zig lexer integration
#ifdef USE_ZIG_LEXER
void lexer_update_position(int line, int column);
GooLexer* lexer_get_current(void);
void lexer_set_token(GooToken token);
#endif

#endif /* LEXER_SELECTION_H */ 