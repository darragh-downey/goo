#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/goo_parser_mode_aware.h"
#include "../include/goo_file_detector.h"

// Sample Go code
const char* go_code = 
    "package main\n\n"
    "import \"fmt\"\n\n"
    "func main() {\n"
    "    fmt.Println(\"Hello, Go!\")\n"
    "}\n";

// Sample Goo code with extensions
const char* goo_code = 
    "package main\n\n"
    "import \"fmt\"\n\n"
    "enum Status {\n"
    "    SUCCESS\n"
    "    ERROR\n"
    "    PENDING\n"
    "}\n\n"
    "func main() {\n"
    "    status := Status.SUCCESS\n"
    "    match status {\n"
    "        Status.SUCCESS => fmt.Println(\"Success!\")\n"
    "        Status.ERROR => fmt.Println(\"Error!\")\n"
    "        _ => fmt.Println(\"Unknown status\")\n"
    "    }\n"
    "}\n";

// Sample Go code with Goo extensions (should fail in Go mode)
const char* go_with_extensions = 
    "package main\n\n"
    "import \"fmt\"\n\n"
    "enum Status {\n"
    "    SUCCESS\n"
    "    ERROR\n"
    "    PENDING\n"
    "}\n\n"
    "func main() {\n"
    "    fmt.Println(\"Hello, Go with extensions!\")\n"
    "}\n";

void test_go_mode_detection() {
    printf("Testing Go mode detection...\n");
    
    GooLangMode mode = goo_detect_file_mode("test.go", go_code, strlen(go_code));
    if (mode == GOO_LANG_MODE_GO) {
        printf("  PASS: Detected .go file correctly as Go mode\n");
    } else {
        printf("  FAIL: Did not detect .go file correctly\n");
    }
    
    mode = goo_detect_file_mode("test.goo", go_code, strlen(go_code));
    if (mode == GOO_LANG_MODE_GOO) {
        printf("  PASS: Detected .goo file correctly as Goo mode\n");
    } else {
        printf("  FAIL: Did not detect .goo file correctly\n");
    }
    
    // Test with content override
    const char* content_override = "// goo:mode=go\npackage main";
    mode = goo_detect_file_mode("test.goo", content_override, strlen(content_override));
    if (mode == GOO_LANG_MODE_GO) {
        printf("  PASS: Detected goo:mode=go marker correctly\n");
    } else {
        printf("  FAIL: Did not detect goo:mode=go marker correctly\n");
    }
}

void test_go_parsing() {
    printf("Testing Go code parsing...\n");
    
    GooParserHandle parser = gooModeAwareParserCreate();
    if (parser == NULL) {
        printf("  FAIL: Could not create parser\n");
        return;
    }
    
    // Force Go mode
    gooParserForceMode(parser, GOO_LANG_MODE_GO);
    
    // Parse Go code
    GooParserResultCode result = gooModeAwareParseString(parser, "test.go", go_code);
    if (result == GOO_PARSER_SUCCESS) {
        printf("  PASS: Successfully parsed Go code in Go mode\n");
    } else {
        printf("  FAIL: Failed to parse Go code in Go mode\n");
    }
    
    // Try to parse Goo code with extensions in Go mode
    result = gooModeAwareParseString(parser, "test.go", goo_code);
    if (result != GOO_PARSER_SUCCESS) {
        printf("  PASS: Correctly rejected Goo extensions in Go mode\n");
    } else {
        printf("  FAIL: Did not reject Goo extensions in Go mode\n");
    }
    
    gooParserDestroy(parser);
}

void test_goo_parsing() {
    printf("Testing Goo code parsing...\n");
    
    GooParserHandle parser = gooModeAwareParserCreate();
    if (parser == NULL) {
        printf("  FAIL: Could not create parser\n");
        return;
    }
    
    // Force Goo mode
    gooParserForceMode(parser, GOO_LANG_MODE_GOO);
    
    // Parse Go code in Goo mode
    GooParserResultCode result = gooModeAwareParseString(parser, "test.goo", go_code);
    if (result == GOO_PARSER_SUCCESS) {
        printf("  PASS: Successfully parsed Go code in Goo mode\n");
    } else {
        printf("  FAIL: Failed to parse Go code in Goo mode\n");
    }
    
    // Parse Goo code with extensions
    result = gooModeAwareParseString(parser, "test.goo", goo_code);
    if (result == GOO_PARSER_SUCCESS) {
        printf("  PASS: Successfully parsed Goo code with extensions\n");
    } else {
        printf("  FAIL: Failed to parse Goo code with extensions\n");
    }
    
    gooParserDestroy(parser);
}

void test_automatic_mode_detection() {
    printf("Testing automatic mode detection...\n");
    
    GooParserHandle parser = gooModeAwareParserCreate();
    if (parser == NULL) {
        printf("  FAIL: Could not create parser\n");
        return;
    }
    
    // Parse Go file with automatic mode detection
    GooParserResultCode result = gooModeAwareParseString(parser, "test.go", go_code);
    if (result == GOO_PARSER_SUCCESS && gooParserGetDetectedMode(parser) == GOO_LANG_MODE_GO) {
        printf("  PASS: Successfully parsed Go code with automatic detection\n");
    } else {
        printf("  FAIL: Failed to parse Go code with automatic detection\n");
    }
    
    // Parse Goo file with automatic mode detection
    result = gooModeAwareParseString(parser, "test.goo", goo_code);
    if (result == GOO_PARSER_SUCCESS && gooParserGetDetectedMode(parser) == GOO_LANG_MODE_GOO) {
        printf("  PASS: Successfully parsed Goo code with automatic detection\n");
    } else {
        printf("  FAIL: Failed to parse Goo code with automatic detection\n");
    }
    
    // Test content-based detection override
    const char* goo_code_with_go_marker = "// goo:mode=go\n\npackage main\n\nfunc main() {}\n";
    result = gooModeAwareParseString(parser, "test.goo", goo_code_with_go_marker);
    if (result == GOO_PARSER_SUCCESS && gooParserGetDetectedMode(parser) == GOO_LANG_MODE_GO) {
        printf("  PASS: Content marker overrode file extension\n");
    } else {
        printf("  FAIL: Content marker did not override file extension\n");
    }
    
    gooParserDestroy(parser);
}

int main() {
    printf("Mode-aware parser tests\n");
    printf("=======================\n\n");
    
    test_go_mode_detection();
    printf("\n");
    
    test_go_parsing();
    printf("\n");
    
    test_goo_parsing();
    printf("\n");
    
    test_automatic_mode_detection();
    printf("\n");
    
    printf("Tests completed.\n");
    
    return 0;
} 