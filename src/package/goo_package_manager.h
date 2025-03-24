#ifndef GOO_PACKAGE_MANAGER_H
#define GOO_PACKAGE_MANAGER_H

#include "goo_package.h"
#include "repository/goo_repository.h"

// Forward declarations
typedef struct GooPackageManager GooPackageManager;

// Package manager structure
struct GooPackageManager {
    GooPackageManagerConfig config;       // Configuration
    GooPackageRepository** repositories;  // Package repositories
    int repository_count;                 // Number of repositories
    char* working_dir;                    // Current working directory
    char* cache_dir;                      // Cache directory
    GooPackage* current_package;          // Current package
    bool initialized;                     // Whether the manager is initialized
};

// === Package Manager API ===

// Create a new package manager
GooPackageManager* goo_package_manager_create(GooPackageManagerConfig* config);

// Destroy a package manager
void goo_package_manager_destroy(GooPackageManager* manager);

// Initialize a new package
bool goo_package_manager_init(GooPackageManager* manager, const char* name, const char* version);

// Install dependencies
bool goo_package_manager_install(GooPackageManager* manager);

// Install a specific package
bool goo_package_manager_install_package(GooPackageManager* manager, const char* name, const char* version);

// Uninstall a package
bool goo_package_manager_uninstall(GooPackageManager* manager, const char* name);

// Update dependencies
bool goo_package_manager_update(GooPackageManager* manager);

// Update a specific package
bool goo_package_manager_update_package(GooPackageManager* manager, const char* name);

// List installed packages
GooPackage** goo_package_manager_list(GooPackageManager* manager, int* count);

// Get information about a package
GooPackage* goo_package_manager_info(GooPackageManager* manager, const char* name);

// Search for packages
GooPackage** goo_package_manager_search(GooPackageManager* manager, const char* query, int* count);

// Add a package repository
bool goo_package_manager_add_repository(GooPackageManager* manager, const char* url, const char* name);

// Remove a package repository
bool goo_package_manager_remove_repository(GooPackageManager* manager, const char* name);

// List package repositories
GooPackageRepository** goo_package_manager_list_repositories(GooPackageManager* manager, int* count);

// Create a lock file
bool goo_package_manager_create_lock(GooPackageManager* manager);

// Load a lock file
bool goo_package_manager_load_lock(GooPackageManager* manager);

// Load a package from the current directory
GooPackage* goo_package_manager_load_current(GooPackageManager* manager);

// Save the current package
bool goo_package_manager_save_current(GooPackageManager* manager);

// Add a dependency to the current package
bool goo_package_manager_add_dependency(GooPackageManager* manager, const char* name, const char* version);

// Remove a dependency from the current package
bool goo_package_manager_remove_dependency(GooPackageManager* manager, const char* name);

// Publish the current package
bool goo_package_manager_publish(GooPackageManager* manager, const char* repository_name);

#endif // GOO_PACKAGE_MANAGER_H 