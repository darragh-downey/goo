#ifndef GOO_PARSER_MODE_AWARE_H
#define GOO_PARSER_MODE_AWARE_H

#include <stddef.h>
#include <stdbool.h>
#include "goo_lexer.h"
#include "goo_parser.h"
#include "goo_file_detector.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a mode-aware parser for the Goo language.
 * This parser can handle both Go and Goo language syntax based on detection.
 *
 * @return A parser handle
 */
GooParserHandle gooModeAwareParserCreate(void);

/**
 * Parse source code with automatic language mode detection.
 * This function automatically detects whether the code is in Go or Goo mode
 * based on file extension and content markers.
 *
 * @param parser The parser handle
 * @param filename The name of the file being parsed (used for mode detection)
 * @param source The source code to parse
 * @return A parser result code
 */
GooParserResultCode gooModeAwareParseString(GooParserHandle parser, const char* filename, const char* source);

/**
 * Parse a file with automatic language mode detection.
 * This function automatically detects whether the code is in Go or Goo mode
 * based on file extension and content markers.
 *
 * @param parser The parser handle
 * @param filename The file to parse
 * @return A parser result code
 */
GooParserResultCode gooModeAwareParseFile(GooParserHandle parser, const char* filename);

/**
 * Get the language mode that was detected and used for parsing.
 *
 * @param parser The parser handle
 * @return The language mode (Go or Goo)
 */
GooLangMode gooParserGetDetectedMode(GooParserHandle parser);

/**
 * Set the default language mode for the parser to use when detection is inconclusive.
 *
 * @param parser The parser handle
 * @param mode The language mode to use as default
 */
void gooParserSetDefaultMode(GooParserHandle parser, GooLangMode mode);

/**
 * Force a specific language mode for parsing, bypassing detection.
 *
 * @param parser The parser handle
 * @param mode The language mode to use
 */
void gooParserForceMode(GooParserHandle parser, GooLangMode mode);

/**
 * Check if a node is using a Goo language extension.
 * This is useful for validating that Go-only files don't contain Goo features.
 *
 * @param node The AST node to check
 * @return true if the node uses Goo extensions, false otherwise
 */
bool gooAstNodeUsesExtensions(GooAstNodeHandle node);

#ifdef __cplusplus
}
#endif

#endif // GOO_PARSER_MODE_AWARE_H 