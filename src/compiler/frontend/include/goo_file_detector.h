#ifndef GOO_FILE_DETECTOR_H
#define GOO_FILE_DETECTOR_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Language mode flags
typedef enum {
    GOO_LANG_MODE_GOO = 0,   // Default Goo language mode
    GOO_LANG_MODE_GO = 1     // Go compatibility mode
} GooLangMode;

/**
 * Detects whether a file should be processed in Go or Goo mode.
 *
 * @param filename The name of the file to check
 * @param content The content of the file (can be NULL if not available)
 * @param content_length The length of the content
 * @return The language mode to use for processing the file
 */
GooLangMode goo_detect_file_mode(const char* filename, const char* content, size_t content_length);

/**
 * Sets the default language mode to use when detection is inconclusive.
 *
 * @param mode The language mode to use as default
 */
void goo_set_default_lang_mode(GooLangMode mode);

/**
 * Gets the name of the language mode as a string.
 *
 * @param mode The language mode
 * @return A string representation of the mode
 */
const char* goo_lang_mode_string(GooLangMode mode);

#ifdef __cplusplus
}
#endif

#endif // GOO_FILE_DETECTOR_H 