#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../package/goo_package.h"
#include "../package/goo_package_manager.h"

// Define command functions
typedef int (*CommandFunction)(GooPackageManager* manager, int argc, char** argv);

// Structure to hold command information
typedef struct {
    const char* name;
    const char* description;
    CommandFunction function;
} Command;

// Command function prototypes
int cmd_init(GooPackageManager* manager, int argc, char** argv);
int cmd_install(GooPackageManager* manager, int argc, char** argv);
int cmd_uninstall(GooPackageManager* manager, int argc, char** argv);
int cmd_update(GooPackageManager* manager, int argc, char** argv);
int cmd_list(GooPackageManager* manager, int argc, char** argv);
int cmd_info(GooPackageManager* manager, int argc, char** argv);
int cmd_search(GooPackageManager* manager, int argc, char** argv);
int cmd_add(GooPackageManager* manager, int argc, char** argv);
int cmd_remove(GooPackageManager* manager, int argc, char** argv);
int cmd_publish(GooPackageManager* manager, int argc, char** argv);
int cmd_repo(GooPackageManager* manager, int argc, char** argv);
int cmd_help(GooPackageManager* manager, int argc, char** argv);

// Command list
Command commands[] = {
    {"init", "Initialize a new package", cmd_init},
    {"install", "Install all dependencies", cmd_install},
    {"uninstall", "Uninstall a package", cmd_uninstall},
    {"update", "Update dependencies", cmd_update},
    {"list", "List installed packages", cmd_list},
    {"info", "Show information about a package", cmd_info},
    {"search", "Search for packages", cmd_search},
    {"add", "Add a dependency", cmd_add},
    {"remove", "Remove a dependency", cmd_remove},
    {"publish", "Publish the package", cmd_publish},
    {"repo", "Manage repositories", cmd_repo},
    {"help", "Show help", cmd_help},
    {NULL, NULL, NULL}
};

// Print usage information
void print_usage() {
    printf("Goo Package Manager\n");
    printf("Usage: goo-package <command> [options]\n\n");
    printf("Commands:\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  %-10s %s\n", commands[i].name, commands[i].description);
    }
}

// Initialize a new package
int cmd_init(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package init <name> [version]\n");
        return 1;
    }
    
    const char* name = argv[0];
    const char* version = (argc > 1) ? argv[1] : "1.0.0";
    
    if (goo_package_manager_init(manager, name, version)) {
        printf("Initialized package %s@%s\n", name, version);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to initialize package\n");
        return 1;
    }
}

// Install all dependencies
int cmd_install(GooPackageManager* manager, int argc, char** argv) {
    // If arguments are provided, install specific packages
    if (argc > 0) {
        bool success = true;
        
        for (int i = 0; i < argc; i++) {
            // Check if version is specified
            char* name = argv[i];
            char* version = NULL;
            
            char* at_sign = strchr(name, '@');
            if (at_sign) {
                *at_sign = '\0';
                version = at_sign + 1;
            }
            
            printf("Installing %s%s%s...\n", name, version ? "@" : "", version ? version : "");
            
            if (!goo_package_manager_install_package(manager, name, version)) {
                fprintf(stderr, "Error: Failed to install %s\n", name);
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }
    
    // Otherwise, install all dependencies
    printf("Installing dependencies...\n");
    
    if (goo_package_manager_install(manager)) {
        printf("Dependencies installed successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to install dependencies\n");
        return 1;
    }
}

// Uninstall a package
int cmd_uninstall(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package uninstall <package> [package...]\n");
        return 1;
    }
    
    bool success = true;
    
    for (int i = 0; i < argc; i++) {
        printf("Uninstalling %s...\n", argv[i]);
        
        if (!goo_package_manager_uninstall(manager, argv[i])) {
            fprintf(stderr, "Error: Failed to uninstall %s\n", argv[i]);
            success = false;
        }
    }
    
    return success ? 0 : 1;
}

// Update dependencies
int cmd_update(GooPackageManager* manager, int argc, char** argv) {
    // If arguments are provided, update specific packages
    if (argc > 0) {
        bool success = true;
        
        for (int i = 0; i < argc; i++) {
            printf("Updating %s...\n", argv[i]);
            
            if (!goo_package_manager_update_package(manager, argv[i])) {
                fprintf(stderr, "Error: Failed to update %s\n", argv[i]);
                success = false;
            }
        }
        
        return success ? 0 : 1;
    }
    
    // Otherwise, update all dependencies
    printf("Updating dependencies...\n");
    
    if (goo_package_manager_update(manager)) {
        printf("Dependencies updated successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to update dependencies\n");
        return 1;
    }
}

// List installed packages
int cmd_list(GooPackageManager* manager, int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Installed packages:\n");
    
    int count;
    GooPackage** packages = goo_package_manager_list(manager, &count);
    
    if (!packages) {
        printf("No packages installed\n");
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        char* version_str = goo_version_to_string(packages[i]->version);
        printf("  %s@%s\n", packages[i]->name, version_str);
        free(version_str);
    }
    
    // Clean up
    for (int i = 0; i < count; i++) {
        goo_package_destroy(packages[i]);
    }
    free(packages);
    
    return 0;
}

// Show information about a package
int cmd_info(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package info <package>\n");
        return 1;
    }
    
    const char* name = argv[0];
    GooPackage* package = goo_package_manager_info(manager, name);
    
    if (!package) {
        fprintf(stderr, "Error: Package %s not found\n", name);
        return 1;
    }
    
    char* version_str = goo_version_to_string(package->version);
    
    printf("Package: %s@%s\n", package->name, version_str);
    printf("Description: %s\n", package->description ? package->description : "No description");
    printf("Author: %s\n", package->author ? package->author : "Unknown");
    printf("License: %s\n", package->license ? package->license : "Unknown");
    printf("Repository: %s\n", package->repository ? package->repository : "None");
    printf("Homepage: %s\n", package->homepage ? package->homepage : "None");
    
    if (package->dependency_count > 0) {
        printf("Dependencies:\n");
        for (int i = 0; i < package->dependency_count; i++) {
            char* dep_version = goo_version_to_string(package->dependencies[i]->version);
            printf("  %s@%s\n", package->dependencies[i]->name, dep_version);
            free(dep_version);
        }
    }
    
    free(version_str);
    goo_package_destroy(package);
    
    return 0;
}

// Search for packages
int cmd_search(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package search <query>\n");
        return 1;
    }
    
    const char* query = argv[0];
    printf("Searching for packages matching '%s'...\n", query);
    
    int count;
    GooPackage** packages = goo_package_manager_search(manager, query, &count);
    
    if (!packages || count == 0) {
        printf("No packages found matching '%s'\n", query);
        return 0;
    }
    
    printf("Found %d package(s):\n", count);
    
    for (int i = 0; i < count; i++) {
        char* version_str = goo_version_to_string(packages[i]->version);
        printf("  %s@%s\n", packages[i]->name, version_str);
        if (packages[i]->description) {
            printf("    %s\n", packages[i]->description);
        }
        free(version_str);
    }
    
    // Clean up
    for (int i = 0; i < count; i++) {
        goo_package_destroy(packages[i]);
    }
    free(packages);
    
    return 0;
}

// Add a dependency
int cmd_add(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package add <package> [version]\n");
        return 1;
    }
    
    const char* name = argv[0];
    const char* version = (argc > 1) ? argv[1] : NULL;
    
    if (goo_package_manager_add_dependency(manager, name, version)) {
        printf("Added dependency %s%s%s\n", name, version ? "@" : "", version ? version : "");
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to add dependency %s\n", name);
        return 1;
    }
}

// Remove a dependency
int cmd_remove(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        printf("Usage: goo-package remove <package>\n");
        return 1;
    }
    
    const char* name = argv[0];
    
    if (goo_package_manager_remove_dependency(manager, name)) {
        printf("Removed dependency %s\n", name);
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to remove dependency %s\n", name);
        return 1;
    }
}

// Publish the package
int cmd_publish(GooPackageManager* manager, int argc, char** argv) {
    const char* repository = (argc > 0) ? argv[0] : "central";
    
    printf("Publishing package to %s repository...\n", repository);
    
    if (goo_package_manager_publish(manager, repository)) {
        printf("Package published successfully\n");
        return 0;
    } else {
        fprintf(stderr, "Error: Failed to publish package\n");
        return 1;
    }
}

// Manage repositories
int cmd_repo(GooPackageManager* manager, int argc, char** argv) {
    if (argc < 1) {
        // List repositories
        printf("Repositories:\n");
        
        int count;
        GooPackageRepository** repositories = goo_package_manager_list_repositories(manager, &count);
        
        for (int i = 0; i < count; i++) {
            printf("  %s: %s (%s)\n", repositories[i]->name, repositories[i]->url,
                   repositories[i]->enabled ? "enabled" : "disabled");
        }
        
        return 0;
    }
    
    const char* action = argv[0];
    
    if (strcmp(action, "add") == 0) {
        // Add a repository
        if (argc < 3) {
            printf("Usage: goo-package repo add <name> <url>\n");
            return 1;
        }
        
        const char* name = argv[1];
        const char* url = argv[2];
        
        if (goo_package_manager_add_repository(manager, url, name)) {
            printf("Added repository %s (%s)\n", name, url);
            return 0;
        } else {
            fprintf(stderr, "Error: Failed to add repository %s\n", name);
            return 1;
        }
    } else if (strcmp(action, "remove") == 0) {
        // Remove a repository
        if (argc < 2) {
            printf("Usage: goo-package repo remove <name>\n");
            return 1;
        }
        
        const char* name = argv[1];
        
        if (goo_package_manager_remove_repository(manager, name)) {
            printf("Removed repository %s\n", name);
            return 0;
        } else {
            fprintf(stderr, "Error: Failed to remove repository %s\n", name);
            return 1;
        }
    } else {
        fprintf(stderr, "Error: Unknown repository action: %s\n", action);
        printf("Usage: goo-package repo [add|remove] ...\n");
        return 1;
    }
}

// Show help
int cmd_help(GooPackageManager* manager, int argc, char** argv) {
    (void)manager;
    
    if (argc < 1) {
        print_usage();
        return 0;
    }
    
    const char* command_name = argv[0];
    
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, command_name) == 0) {
            printf("%s: %s\n", commands[i].name, commands[i].description);
            
            if (strcmp(command_name, "init") == 0) {
                printf("Usage: goo-package init <name> [version]\n");
                printf("  Initialize a new package with the given name and version\n");
            } else if (strcmp(command_name, "install") == 0) {
                printf("Usage: goo-package install [package] [package...]\n");
                printf("  Install all dependencies or specific packages\n");
            } else if (strcmp(command_name, "uninstall") == 0) {
                printf("Usage: goo-package uninstall <package> [package...]\n");
                printf("  Uninstall one or more packages\n");
            } else if (strcmp(command_name, "update") == 0) {
                printf("Usage: goo-package update [package] [package...]\n");
                printf("  Update all dependencies or specific packages\n");
            } else if (strcmp(command_name, "list") == 0) {
                printf("Usage: goo-package list\n");
                printf("  List installed packages\n");
            } else if (strcmp(command_name, "info") == 0) {
                printf("Usage: goo-package info <package>\n");
                printf("  Show information about a package\n");
            } else if (strcmp(command_name, "search") == 0) {
                printf("Usage: goo-package search <query>\n");
                printf("  Search for packages matching the query\n");
            } else if (strcmp(command_name, "add") == 0) {
                printf("Usage: goo-package add <package> [version]\n");
                printf("  Add a dependency to the current package\n");
            } else if (strcmp(command_name, "remove") == 0) {
                printf("Usage: goo-package remove <package>\n");
                printf("  Remove a dependency from the current package\n");
            } else if (strcmp(command_name, "publish") == 0) {
                printf("Usage: goo-package publish [repository]\n");
                printf("  Publish the current package to a repository\n");
            } else if (strcmp(command_name, "repo") == 0) {
                printf("Usage: goo-package repo [add|remove] ...\n");
                printf("  Manage package repositories\n");
                printf("  goo-package repo - List repositories\n");
                printf("  goo-package repo add <name> <url> - Add a repository\n");
                printf("  goo-package repo remove <name> - Remove a repository\n");
            }
            
            return 0;
        }
    }
    
    fprintf(stderr, "Error: Unknown command: %s\n", command_name);
    print_usage();
    return 1;
}

int main(int argc, char** argv) {
    // Check for arguments
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Get the command
    const char* command_name = argv[1];
    
    // Find the command
    Command* command = NULL;
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(commands[i].name, command_name) == 0) {
            command = &commands[i];
            break;
        }
    }
    
    if (!command) {
        fprintf(stderr, "Error: Unknown command: %s\n", command_name);
        print_usage();
        return 1;
    }
    
    // Create the package manager
    GooPackageManagerConfig config;
    memset(&config, 0, sizeof(config));
    config.package_file = "package.json";
    config.lock_file = "package-lock.json";
    config.install_dir = "./node_modules";
    config.use_lock_file = true;
    config.resolve_strategy = GOO_RESOLVE_NEWEST;
    config.verbose = true;
    
    GooPackageManager* manager = goo_package_manager_create(&config);
    if (!manager) {
        fprintf(stderr, "Error: Failed to create package manager\n");
        return 1;
    }
    
    // Execute the command
    int result = command->function(manager, argc - 2, argv + 2);
    
    // Clean up
    goo_package_manager_destroy(manager);
    
    return result;
} 