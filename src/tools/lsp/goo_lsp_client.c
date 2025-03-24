/**
 * @file goo_lsp_client.c
 * @brief Simple command-line client for testing the Goo LSP server
 *
 * This client allows sending LSP messages to the server and 
 * viewing responses, useful for testing and debugging.
 *
 * @copyright Copyright (c) 2023, Goo Language Authors
 * @license MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <json-c/json.h>

// Maximum length for header buffer
#define MAX_HEADER_LEN 4096
// Maximum length for content buffer
#define MAX_CONTENT_LEN (1024 * 1024)

// Global variables for the server process
static pid_t server_pid = -1;
static int server_stdin = -1;
static int server_stdout = -1;
static bool running = true;

/**
 * @brief Send a message to the server
 *
 * @param message The LSP message to send as a JSON string
 * @return true if the message was sent successfully, false otherwise
 */
static bool send_message(const char* message) {
    if (server_stdin < 0 || !message) {
        return false;
    }
    
    size_t length = strlen(message);
    char header[MAX_HEADER_LEN];
    
    // Format the header with Content-Length
    snprintf(header, MAX_HEADER_LEN, "Content-Length: %zu\r\n\r\n", length);
    
    // Write the header
    if (write(server_stdin, header, strlen(header)) < 0) {
        perror("Failed to write header");
        return false;
    }
    
    // Write the content
    if (write(server_stdin, message, length) < 0) {
        perror("Failed to write content");
        return false;
    }
    
    return true;
}

/**
 * @brief Read a message from the server
 *
 * @return A newly allocated string containing the message content, or NULL on error
 */
static char* read_message(void) {
    if (server_stdout < 0) {
        return NULL;
    }
    
    char header[MAX_HEADER_LEN];
    int header_pos = 0;
    int content_length = -1;
    bool header_complete = false;
    
    // Read the header
    while (!header_complete && header_pos < MAX_HEADER_LEN - 1) {
        char c;
        int result = read(server_stdout, &c, 1);
        
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking read with no data, try again
                usleep(10000); // Sleep for 10ms
                continue;
            }
            perror("Failed to read header");
            return NULL;
        } else if (result == 0) {
            // End of file
            return NULL;
        }
        
        header[header_pos++] = c;
        
        // Check for end of header (double CRLF)
        if (header_pos >= 4 && 
            header[header_pos - 4] == '\r' && 
            header[header_pos - 3] == '\n' && 
            header[header_pos - 2] == '\r' && 
            header[header_pos - 1] == '\n') {
            header_complete = true;
            header[header_pos] = '\0';
            
            // Extract the Content-Length
            char* content_length_str = strstr(header, "Content-Length: ");
            if (content_length_str) {
                content_length = atoi(content_length_str + 16);
            }
        }
    }
    
    if (!header_complete || content_length <= 0 || content_length > MAX_CONTENT_LEN) {
        fprintf(stderr, "Invalid header or content length: %d\n", content_length);
        return NULL;
    }
    
    // Allocate buffer for the content
    char* content = (char*)malloc(content_length + 1);
    if (!content) {
        perror("Failed to allocate memory");
        return NULL;
    }
    
    // Read the content
    int content_pos = 0;
    while (content_pos < content_length) {
        int result = read(server_stdout, content + content_pos, content_length - content_pos);
        
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Non-blocking read with no data, try again
                usleep(10000); // Sleep for 10ms
                continue;
            }
            perror("Failed to read content");
            free(content);
            return NULL;
        } else if (result == 0) {
            // End of file
            fprintf(stderr, "Unexpected end of file while reading content\n");
            free(content);
            return NULL;
        }
        
        content_pos += result;
    }
    
    content[content_length] = '\0';
    return content;
}

/**
 * @brief Start the LSP server
 *
 * @param server_path Path to the server executable
 * @param args Additional arguments to pass to the server
 * @return true if the server was started successfully, false otherwise
 */
static bool start_server(const char* server_path, char* const args[]) {
    // Create pipes for communication
    int stdin_pipe[2];
    int stdout_pipe[2];
    
    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0) {
        perror("Failed to create pipes");
        return false;
    }
    
    // Fork the process
    server_pid = fork();
    
    if (server_pid < 0) {
        perror("Failed to fork");
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        return false;
    }
    
    if (server_pid == 0) {
        // Child process
        
        // Redirect stdin to read end of stdin_pipe
        if (dup2(stdin_pipe[0], STDIN_FILENO) < 0) {
            perror("Failed to redirect stdin");
            exit(EXIT_FAILURE);
        }
        
        // Redirect stdout to write end of stdout_pipe
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0) {
            perror("Failed to redirect stdout");
            exit(EXIT_FAILURE);
        }
        
        // Close unnecessary file descriptors
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        
        // Execute the server
        execvp(server_path, args);
        
        // If execvp returns, there was an error
        perror("Failed to execute server");
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    
    // Close unnecessary file descriptors
    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    
    // Set non-blocking mode for stdout
    int flags = fcntl(stdout_pipe[0], F_GETFL, 0);
    fcntl(stdout_pipe[0], F_SETFL, flags | O_NONBLOCK);
    
    // Store the file descriptors
    server_stdin = stdin_pipe[1];
    server_stdout = stdout_pipe[0];
    
    return true;
}

/**
 * @brief Stop the LSP server
 */
static void stop_server(void) {
    if (server_pid > 0) {
        // Send a shutdown request
        const char* shutdown_request = 
            "{\"jsonrpc\":\"2.0\",\"id\":999,\"method\":\"shutdown\"}";
        send_message(shutdown_request);
        
        // Send an exit notification
        const char* exit_notification = 
            "{\"jsonrpc\":\"2.0\",\"method\":\"exit\"}";
        send_message(exit_notification);
        
        // Wait for the process to exit
        int status;
        pid_t result = waitpid(server_pid, &status, WNOHANG);
        
        if (result == 0) {
            // Process still running, send SIGTERM
            kill(server_pid, SIGTERM);
            
            // Wait again
            sleep(1);
            result = waitpid(server_pid, &status, WNOHANG);
            
            if (result == 0) {
                // Process still running, send SIGKILL
                kill(server_pid, SIGKILL);
                waitpid(server_pid, &status, 0);
            }
        }
        
        server_pid = -1;
    }
    
    // Close file descriptors
    if (server_stdin >= 0) {
        close(server_stdin);
        server_stdin = -1;
    }
    
    if (server_stdout >= 0) {
        close(server_stdout);
        server_stdout = -1;
    }
}

/**
 * @brief Create an initialize request
 *
 * @param root_uri The root URI for the workspace
 * @return A newly allocated string containing the initialize request
 */
static char* create_initialize_request(const char* root_uri) {
    json_object* request = json_object_new_object();
    json_object* params = json_object_new_object();
    json_object* capabilities = json_object_new_object();
    
    // Add basic request fields
    json_object_object_add(request, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(request, "id", json_object_new_int(1));
    json_object_object_add(request, "method", json_object_new_string("initialize"));
    
    // Add parameters
    json_object_object_add(params, "processId", json_object_new_int(getpid()));
    
    if (root_uri) {
        json_object_object_add(params, "rootUri", json_object_new_string(root_uri));
    } else {
        json_object_object_add(params, "rootUri", NULL);
    }
    
    // Add capabilities (minimal)
    json_object_object_add(params, "capabilities", capabilities);
    
    // Add params to request
    json_object_object_add(request, "params", params);
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(request, JSON_C_TO_STRING_PRETTY);
    char* result = strdup(json_str);
    
    // Clean up
    json_object_put(request);
    
    return result;
}

/**
 * @brief Create an initialized notification
 *
 * @return A newly allocated string containing the initialized notification
 */
static char* create_initialized_notification(void) {
    json_object* notification = json_object_new_object();
    
    // Add basic notification fields
    json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(notification, "method", json_object_new_string("initialized"));
    json_object_object_add(notification, "params", json_object_new_object());
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(notification, JSON_C_TO_STRING_PRETTY);
    char* result = strdup(json_str);
    
    // Clean up
    json_object_put(notification);
    
    return result;
}

/**
 * @brief Create a document open notification
 *
 * @param uri The URI of the document
 * @param language_id The language ID (e.g., "goo")
 * @param version The document version
 * @param text The document text
 * @return A newly allocated string containing the notification
 */
static char* create_document_open_notification(
    const char* uri, 
    const char* language_id, 
    int version, 
    const char* text
) {
    json_object* notification = json_object_new_object();
    json_object* params = json_object_new_object();
    json_object* text_document = json_object_new_object();
    
    // Add basic notification fields
    json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(notification, "method", json_object_new_string("textDocument/didOpen"));
    
    // Add text document fields
    json_object_object_add(text_document, "uri", json_object_new_string(uri));
    json_object_object_add(text_document, "languageId", json_object_new_string(language_id));
    json_object_object_add(text_document, "version", json_object_new_int(version));
    json_object_object_add(text_document, "text", json_object_new_string(text));
    
    // Add params
    json_object_object_add(params, "textDocument", text_document);
    json_object_object_add(notification, "params", params);
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(notification, JSON_C_TO_STRING_PRETTY);
    char* result = strdup(json_str);
    
    // Clean up
    json_object_put(notification);
    
    return result;
}

/**
 * @brief Create a document change notification
 *
 * @param uri The URI of the document
 * @param version The document version
 * @param text The new document text
 * @return A newly allocated string containing the notification
 */
static char* create_document_change_notification(
    const char* uri, 
    int version, 
    const char* text
) {
    json_object* notification = json_object_new_object();
    json_object* params = json_object_new_object();
    json_object* text_document = json_object_new_object();
    json_object* content_changes = json_object_new_array();
    json_object* change = json_object_new_object();
    
    // Add basic notification fields
    json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(notification, "method", json_object_new_string("textDocument/didChange"));
    
    // Add text document fields
    json_object_object_add(text_document, "uri", json_object_new_string(uri));
    json_object_object_add(text_document, "version", json_object_new_int(version));
    
    // Add content change
    json_object_object_add(change, "text", json_object_new_string(text));
    json_object_array_add(content_changes, change);
    
    // Add params
    json_object_object_add(params, "textDocument", text_document);
    json_object_object_add(params, "contentChanges", content_changes);
    json_object_object_add(notification, "params", params);
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(notification, JSON_C_TO_STRING_PRETTY);
    char* result = strdup(json_str);
    
    // Clean up
    json_object_put(notification);
    
    return result;
}

/**
 * @brief Create a custom request
 *
 * @param id The request ID
 * @param method The request method
 * @param params_json The parameters as a JSON string (can be NULL)
 * @return A newly allocated string containing the request
 */
static char* create_custom_request(int id, const char* method, const char* params_json) {
    json_object* request = json_object_new_object();
    
    // Add basic request fields
    json_object_object_add(request, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(request, "id", json_object_new_int(id));
    json_object_object_add(request, "method", json_object_new_string(method));
    
    // Add parameters if provided
    if (params_json) {
        json_object* params = json_tokener_parse(params_json);
        if (params) {
            json_object_object_add(request, "params", params);
        } else {
            fprintf(stderr, "Failed to parse params JSON: %s\n", params_json);
            json_object_object_add(request, "params", json_object_new_object());
        }
    } else {
        json_object_object_add(request, "params", json_object_new_object());
    }
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(request, JSON_C_TO_STRING_PRETTY);
    char* result = strdup(json_str);
    
    // Clean up
    json_object_put(request);
    
    return result;
}

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [SERVER_PATH]\n\n", program_name);
    printf("Options:\n");
    printf("  -h              Show this help message\n");
    printf("  -r <root_uri>   Root URI for the workspace\n");
    printf("  -v              Enable verbose mode\n");
    printf("\n");
    printf("If SERVER_PATH is not specified, the program will look for 'goo-lsp' in PATH.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  help            Show this help message\n");
    printf("  exit            Exit the program\n");
    printf("  initialize      Send initialize request\n");
    printf("  initialized     Send initialized notification\n");
    printf("  open <uri> <file>   Open a document\n");
    printf("  change <uri> <version> <text>   Change a document\n");
    printf("  request <id> <method> [params]   Send a custom request\n");
    printf("  notify <method> [params]   Send a custom notification\n");
    printf("  raw <json>      Send raw JSON message\n");
}

/**
 * @brief Process a command entered by the user
 *
 * @param command The command string
 * @param root_uri The root URI for the workspace
 * @param verbose Whether to print verbose output
 * @return true if the program should continue, false if it should exit
 */
static bool process_command(char* command, const char* root_uri, bool verbose) {
    char* token = strtok(command, " \t\n");
    
    if (!token) {
        return true;
    }
    
    if (strcmp(token, "help") == 0) {
        print_usage("goo-lsp-client");
    } else if (strcmp(token, "exit") == 0) {
        return false;
    } else if (strcmp(token, "initialize") == 0) {
        // Send initialize request
        char* request = create_initialize_request(root_uri);
        if (verbose) {
            printf("Sending initialize request:\n%s\n", request);
        }
        send_message(request);
        free(request);
        
        // Wait for the response
        char* response = read_message();
        if (response) {
            printf("Received response:\n%s\n", response);
            free(response);
        } else {
            printf("No response received\n");
        }
    } else if (strcmp(token, "initialized") == 0) {
        // Send initialized notification
        char* notification = create_initialized_notification();
        if (verbose) {
            printf("Sending initialized notification:\n%s\n", notification);
        }
        send_message(notification);
        free(notification);
    } else if (strcmp(token, "open") == 0) {
        // Parse arguments
        char* uri = strtok(NULL, " \t\n");
        char* file_path = strtok(NULL, " \t\n");
        
        if (!uri || !file_path) {
            printf("Usage: open <uri> <file>\n");
            return true;
        }
        
        // Read the file
        FILE* file = fopen(file_path, "r");
        if (!file) {
            printf("Failed to open file: %s\n", file_path);
            return true;
        }
        
        // Get file size
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        // Read file content
        char* content = (char*)malloc(file_size + 1);
        if (!content) {
            printf("Failed to allocate memory for file content\n");
            fclose(file);
            return true;
        }
        
        size_t bytes_read = fread(content, 1, file_size, file);
        fclose(file);
        
        content[bytes_read] = '\0';
        
        // Send document open notification
        char* notification = create_document_open_notification(uri, "goo", 1, content);
        if (verbose) {
            printf("Sending document open notification:\n%s\n", notification);
        }
        send_message(notification);
        free(notification);
        free(content);
    } else if (strcmp(token, "change") == 0) {
        // Parse arguments
        char* uri = strtok(NULL, " \t\n");
        char* version_str = strtok(NULL, " \t\n");
        char* text = strtok(NULL, "");
        
        if (!uri || !version_str || !text) {
            printf("Usage: change <uri> <version> <text>\n");
            return true;
        }
        
        int version = atoi(version_str);
        
        // Send document change notification
        char* notification = create_document_change_notification(uri, version, text);
        if (verbose) {
            printf("Sending document change notification:\n%s\n", notification);
        }
        send_message(notification);
        free(notification);
    } else if (strcmp(token, "request") == 0) {
        // Parse arguments
        char* id_str = strtok(NULL, " \t\n");
        char* method = strtok(NULL, " \t\n");
        char* params = strtok(NULL, "");
        
        if (!id_str || !method) {
            printf("Usage: request <id> <method> [params]\n");
            return true;
        }
        
        int id = atoi(id_str);
        
        // Send custom request
        char* request = create_custom_request(id, method, params);
        if (verbose) {
            printf("Sending custom request:\n%s\n", request);
        }
        send_message(request);
        free(request);
        
        // Wait for the response
        char* response = read_message();
        if (response) {
            printf("Received response:\n%s\n", response);
            free(response);
        } else {
            printf("No response received\n");
        }
    } else if (strcmp(token, "notify") == 0) {
        // Parse arguments
        char* method = strtok(NULL, " \t\n");
        char* params = strtok(NULL, "");
        
        if (!method) {
            printf("Usage: notify <method> [params]\n");
            return true;
        }
        
        // Create a notification (no ID)
        json_object* notification = json_object_new_object();
        json_object_object_add(notification, "jsonrpc", json_object_new_string("2.0"));
        json_object_object_add(notification, "method", json_object_new_string(method));
        
        if (params) {
            json_object* params_obj = json_tokener_parse(params);
            if (params_obj) {
                json_object_object_add(notification, "params", params_obj);
            } else {
                fprintf(stderr, "Failed to parse params JSON: %s\n", params);
                json_object_object_add(notification, "params", json_object_new_object());
            }
        } else {
            json_object_object_add(notification, "params", json_object_new_object());
        }
        
        // Convert to string
        const char* json_str = json_object_to_json_string_ext(notification, JSON_C_TO_STRING_PRETTY);
        
        if (verbose) {
            printf("Sending custom notification:\n%s\n", json_str);
        }
        
        send_message(json_str);
        json_object_put(notification);
    } else if (strcmp(token, "raw") == 0) {
        // Get the raw JSON
        char* json = strtok(NULL, "");
        
        if (!json) {
            printf("Usage: raw <json>\n");
            return true;
        }
        
        // Send raw message
        if (verbose) {
            printf("Sending raw message:\n%s\n", json);
        }
        send_message(json);
        
        // Check if it's a request (has an ID)
        json_object* message = json_tokener_parse(json);
        if (message) {
            json_object* id_obj;
            if (json_object_object_get_ex(message, "id", &id_obj)) {
                // It's a request, wait for a response
                char* response = read_message();
                if (response) {
                    printf("Received response:\n%s\n", response);
                    free(response);
                } else {
                    printf("No response received\n");
                }
            }
            json_object_put(message);
        }
    } else {
        printf("Unknown command: %s\n", token);
    }
    
    return true;
}

/**
 * @brief Handle signals
 */
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        running = false;
    }
}

/**
 * @brief Main entry point
 */
int main(int argc, char** argv) {
    char* server_path = "goo-lsp";
    char* root_uri = NULL;
    bool verbose = false;
    
    // Parse command-line arguments
    int opt;
    while ((opt = getopt(argc, argv, "hr:v")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
                
            case 'r':
                root_uri = optarg;
                break;
                
            case 'v':
                verbose = true;
                break;
                
            default:
                fprintf(stderr, "Try '%s -h' for more information.\n", argv[0]);
                return EXIT_FAILURE;
        }
    }
    
    // If server path is provided, use it
    if (optind < argc) {
        server_path = argv[optind];
    }
    
    // Set up signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Start the server
    printf("Starting LSP server: %s\n", server_path);
    
    // Prepare arguments for the server
    char* args[3];
    args[0] = server_path;
    args[1] = verbose ? "--verbose" : NULL;
    args[2] = NULL;
    
    if (!start_server(server_path, args)) {
        fprintf(stderr, "Failed to start the server\n");
        return EXIT_FAILURE;
    }
    
    printf("LSP client ready. Type 'help' for available commands.\n");
    
    // Main loop
    char buffer[4096];
    while (running) {
        printf("> ");
        fflush(stdout);
        
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            break;
        }
        
        if (!process_command(buffer, root_uri, verbose)) {
            break;
        }
    }
    
    // Stop the server
    printf("Stopping LSP server...\n");
    stop_server();
    
    return EXIT_SUCCESS;
} 