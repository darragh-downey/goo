/**
 * @file goo_lsp_server_tests.c
 * @brief Integration tests for the Goo LSP server
 *
 * @copyright Copyright (c) 2023, Goo Language Authors
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <json-c/json.h>

#include "../goo_lsp_server.h"
#include "../goo_lsp_protocol.h"

#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("TEST FAILED: %s\n", message); \
            printf("  At %s:%d\n", __FILE__, __LINE__); \
            passed = false; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(a, b, message) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            printf("TEST FAILED: %s\n", message); \
            printf("  Expected: \"%s\"\n", (b)); \
            printf("  Got: \"%s\"\n", (a)); \
            printf("  At %s:%d\n", __FILE__, __LINE__); \
            passed = false; \
        } \
    } while (0)

#define TEST_ASSERT_JSON_HAS_MEMBER(obj, member, message) \
    do { \
        if (!json_object_object_get_ex((obj), (member), NULL)) { \
            printf("TEST FAILED: %s\n", message); \
            printf("  JSON object doesn't have member: \"%s\"\n", (member)); \
            printf("  At %s:%d\n", __FILE__, __LINE__); \
            passed = false; \
        } \
    } while (0)

/**
 * @brief Test server creation and initialization
 */
static bool test_server_creation(void) {
    bool passed = true;
    
    // Create server with default config
    GooLspServer* server = goo_lsp_server_create();
    TEST_ASSERT(server != NULL, "Failed to create server with default config");
    
    // Get the config
    const GooLspServerConfig* config = goo_lsp_server_get_config(server);
    TEST_ASSERT(config != NULL, "Failed to get server config");
    TEST_ASSERT(config->diagnostics_enabled, "Diagnostics should be enabled by default");
    TEST_ASSERT(config->hover_enabled, "Hover info should be enabled by default");
    
    // Create a custom config
    GooLspServerConfig custom_config = {
        .working_dir = "/tmp",
        .diagnostics_enabled = false,
        .hover_enabled = true,
        .completion_enabled = true,
        .definition_enabled = false,
        .references_enabled = true,
        .formatting_enabled = false,
        .symbols_enabled = true,
        .highlight_enabled = false,
        .rename_enabled = true,
        .signature_help_enabled = false,
        .verbose = true
    };
    
    // Create server with custom config
    GooLspServer* custom_server = goo_lsp_server_create_with_config(&custom_config);
    TEST_ASSERT(custom_server != NULL, "Failed to create server with custom config");
    
    // Get the config
    const GooLspServerConfig* custom_config_get = goo_lsp_server_get_config(custom_server);
    TEST_ASSERT(custom_config_get != NULL, "Failed to get custom server config");
    TEST_ASSERT_STR_EQ(custom_config_get->working_dir, "/tmp", "Working dir should match");
    TEST_ASSERT(custom_config_get->diagnostics_enabled == false, "Diagnostics setting should match");
    TEST_ASSERT(custom_config_get->hover_enabled == true, "Hover setting should match");
    TEST_ASSERT(custom_config_get->definition_enabled == false, "Definition setting should match");
    TEST_ASSERT(custom_config_get->verbose == true, "Verbose setting should match");
    
    // Update the config
    GooLspServerConfig updated_config = {
        .working_dir = "/var/tmp",
        .diagnostics_enabled = true,
        .hover_enabled = false,
        .completion_enabled = false,
        .definition_enabled = true,
        .references_enabled = false,
        .formatting_enabled = true,
        .symbols_enabled = false,
        .highlight_enabled = true,
        .rename_enabled = false,
        .signature_help_enabled = true,
        .verbose = false
    };
    
    bool updated = goo_lsp_server_update_config(custom_server, &updated_config);
    TEST_ASSERT(updated, "Failed to update server config");
    
    // Get the updated config
    const GooLspServerConfig* updated_config_get = goo_lsp_server_get_config(custom_server);
    TEST_ASSERT(updated_config_get != NULL, "Failed to get updated server config");
    TEST_ASSERT_STR_EQ(updated_config_get->working_dir, "/var/tmp", "Updated working dir should match");
    TEST_ASSERT(updated_config_get->diagnostics_enabled == true, "Updated diagnostics setting should match");
    TEST_ASSERT(updated_config_get->hover_enabled == false, "Updated hover setting should match");
    TEST_ASSERT(updated_config_get->definition_enabled == true, "Updated definition setting should match");
    TEST_ASSERT(updated_config_get->verbose == false, "Updated verbose setting should match");
    
    // Clean up
    goo_lsp_server_destroy(server);
    goo_lsp_server_destroy(custom_server);
    
    return passed;
}

/**
 * @brief Test server initialization message handling
 */
static bool test_initialize_message(void) {
    bool passed = true;
    
    // Create server
    GooLspServer* server = goo_lsp_server_create();
    TEST_ASSERT(server != NULL, "Failed to create server");
    
    // Start server
    bool started = goo_lsp_server_start(server);
    TEST_ASSERT(started, "Failed to start server");
    
    // Create initialize message
    json_object* message = json_object_new_object();
    json_object_object_add(message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(message, "id", json_object_new_int(1));
    json_object_object_add(message, "method", json_object_new_string("initialize"));
    
    // Create params
    json_object* params = json_object_new_object();
    
    // Create capabilities
    json_object* capabilities = json_object_new_object();
    json_object_object_add(params, "capabilities", capabilities);
    
    // Set root URI
    json_object_object_add(params, "rootUri", json_object_new_string("file:///test/project"));
    
    // Add params to message
    json_object_object_add(message, "params", params);
    
    // Process the message
    json_object* response = goo_lsp_server_process_message(server, message);
    TEST_ASSERT(response != NULL, "Initialize response should not be NULL");
    
    // Check response structure
    json_object* jsonrpc;
    json_object* id;
    json_object* result;
    
    bool has_jsonrpc = json_object_object_get_ex(response, "jsonrpc", &jsonrpc);
    bool has_id = json_object_object_get_ex(response, "id", &id);
    bool has_result = json_object_object_get_ex(response, "result", &result);
    
    TEST_ASSERT(has_jsonrpc, "Response should have jsonrpc field");
    TEST_ASSERT(has_id, "Response should have id field");
    TEST_ASSERT(has_result, "Response should have result field");
    
    // Check result structure
    TEST_ASSERT_JSON_HAS_MEMBER(result, "capabilities", "Initialize result should have capabilities");
    TEST_ASSERT_JSON_HAS_MEMBER(result, "serverInfo", "Initialize result should have serverInfo");
    
    // Check server info
    json_object* server_info;
    bool has_server_info = json_object_object_get_ex(result, "serverInfo", &server_info);
    TEST_ASSERT(has_server_info, "Result should have serverInfo");
    TEST_ASSERT_JSON_HAS_MEMBER(server_info, "name", "ServerInfo should have name");
    TEST_ASSERT_JSON_HAS_MEMBER(server_info, "version", "ServerInfo should have version");
    
    // Check capabilities
    json_object* server_capabilities;
    bool has_capabilities = json_object_object_get_ex(result, "capabilities", &server_capabilities);
    TEST_ASSERT(has_capabilities, "Result should have capabilities");
    
    // Clean up
    json_object_put(message);
    json_object_put(response);
    goo_lsp_server_shutdown(server);
    goo_lsp_server_destroy(server);
    
    return passed;
}

/**
 * @brief Test document synchronization
 */
static bool test_document_sync(void) {
    bool passed = true;
    
    // Create server
    GooLspServer* server = goo_lsp_server_create();
    TEST_ASSERT(server != NULL, "Failed to create server");
    
    // Start server
    bool started = goo_lsp_server_start(server);
    TEST_ASSERT(started, "Failed to start server");
    
    // Initialize server
    json_object* init_message = json_object_new_object();
    json_object_object_add(init_message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(init_message, "id", json_object_new_int(1));
    json_object_object_add(init_message, "method", json_object_new_string("initialize"));
    json_object_object_add(init_message, "params", json_object_new_object());
    
    json_object* init_response = goo_lsp_server_process_message(server, init_message);
    json_object_put(init_message);
    json_object_put(init_response);
    
    // Send initialized notification
    json_object* initialized_message = json_object_new_object();
    json_object_object_add(initialized_message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(initialized_message, "method", json_object_new_string("initialized"));
    json_object_object_add(initialized_message, "params", json_object_new_object());
    
    json_object* initialized_response = goo_lsp_server_process_message(server, initialized_message);
    json_object_put(initialized_message);
    TEST_ASSERT(initialized_response == NULL, "Initialized notification should not have a response");
    
    // Send document open notification
    json_object* open_message = json_object_new_object();
    json_object_object_add(open_message, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(open_message, "method", json_object_new_string("textDocument/didOpen"));
    
    json_object* open_params = json_object_new_object();
    json_object* text_document = json_object_new_object();
    
    json_object_object_add(text_document, "uri", json_object_new_string("file:///test/file.goo"));
    json_object_object_add(text_document, "languageId", json_object_new_string("goo"));
    json_object_object_add(text_document, "version", json_object_new_int(1));
    json_object_object_add(text_document, "text", json_object_new_string("fn main() {\n    println(\"Hello, world!\");\n}\n"));
    
    json_object_object_add(open_params, "textDocument", text_document);
    json_object_object_add(open_message, "params", open_params);
    
    json_object* open_response = goo_lsp_server_process_message(server, open_message);
    json_object_put(open_message);
    TEST_ASSERT(open_response == NULL, "Document open notification should not have a response");
    
    // Clean up
    goo_lsp_server_shutdown(server);
    goo_lsp_server_destroy(server);
    
    return passed;
}

int main(void) {
    int tests_passed = 0;
    int tests_failed = 0;
    
    printf("Running Goo LSP server tests...\n");
    
    // Run tests
    if (test_server_creation()) {
        printf("✅ Server creation test passed\n");
        tests_passed++;
    } else {
        printf("❌ Server creation test failed\n");
        tests_failed++;
    }
    
    if (test_initialize_message()) {
        printf("✅ Initialize message test passed\n");
        tests_passed++;
    } else {
        printf("❌ Initialize message test failed\n");
        tests_failed++;
    }
    
    if (test_document_sync()) {
        printf("✅ Document sync test passed\n");
        tests_passed++;
    } else {
        printf("❌ Document sync test failed\n");
        tests_failed++;
    }
    
    // Print results
    printf("\nTest Results: %d passed, %d failed\n", tests_passed, tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
} 