#include <string.h>
#include <ctype.h>
#include "../include/goo_file_detector.h"

// Default language mode
static GooLangMode default_lang_mode = GOO_LANG_MODE_GOO;

// Checks file extension
static GooLangMode detect_mode_from_extension(const char* filename) {
    if (filename == NULL) {
        return default_lang_mode;
    }
    
    // Find the last dot in the filename
    const char* dot = strrchr(filename, '.');
    if (dot == NULL) {
        return default_lang_mode; // No extension
    }
    
    // Check if it's a Go file
    if (strcmp(dot, ".go") == 0) {
        return GOO_LANG_MODE_GO;
    }
    
    // Check if it's a Goo file
    if (strcmp(dot, ".goo") == 0) {
        return GOO_LANG_MODE_GOO;
    }
    
    return default_lang_mode; // Unknown extension
}

// Checks for Goo marker in content
static GooLangMode detect_mode_from_content(const char* content, size_t content_length) {
    if (content == NULL || content_length == 0) {
        return default_lang_mode;
    }
    
    // Look for "goo:enable" or "goo:mode" in the first 100 lines
    // This would typically be in a comment at the top of the file
    const char* goo_marker = "goo:enable";
    const char* goo_mode_marker = "goo:mode";
    const char* go_mode_marker = "go:build";
    
    // Simple check, in a real implementation you'd want to ensure this is in a comment
    if (memmem(content, content_length < 2000 ? content_length : 2000, goo_marker, strlen(goo_marker)) != NULL) {
        return GOO_LANG_MODE_GOO;
    }
    
    // Check for goo:mode=goo or goo:mode=go
    const char* mode_pos = memmem(content, content_length < 2000 ? content_length : 2000, goo_mode_marker, strlen(goo_mode_marker));
    if (mode_pos != NULL) {
        // Look for the = after the marker
        const char* equals_pos = strchr(mode_pos, '=');
        if (equals_pos != NULL && equals_pos - mode_pos < 20) { // Ensure it's right after the marker
            // Check what follows the equals sign
            const char* value_start = equals_pos + 1;
            while (isspace(*value_start)) {
                value_start++;
            }
            
            if (strncmp(value_start, "goo", 3) == 0) {
                return GOO_LANG_MODE_GOO;
            }
            if (strncmp(value_start, "go", 2) == 0) {
                return GOO_LANG_MODE_GO;
            }
        }
    }
    
    // Check if the file has Go build constraints, suggesting it's a Go file
    if (memmem(content, content_length < 2000 ? content_length : 2000, go_mode_marker, strlen(go_mode_marker)) != NULL) {
        return GOO_LANG_MODE_GO;
    }
    
    return default_lang_mode;
}

GooLangMode goo_detect_file_mode(const char* filename, const char* content, size_t content_length) {
    // First check the extension
    GooLangMode ext_mode = detect_mode_from_extension(filename);
    
    // If we have content, also check it for markers
    if (content != NULL && content_length > 0) {
        GooLangMode content_mode = detect_mode_from_content(content, content_length);
        
        // If the content has an explicit marker, it overrides the extension
        if (content_mode != default_lang_mode) {
            return content_mode;
        }
    }
    
    return ext_mode;
}

void goo_set_default_lang_mode(GooLangMode mode) {
    default_lang_mode = mode;
}

const char* goo_lang_mode_string(GooLangMode mode) {
    switch (mode) {
        case GOO_LANG_MODE_GOO:
            return "Goo";
        case GOO_LANG_MODE_GO:
            return "Go";
        default:
            return "Unknown";
    }
} 