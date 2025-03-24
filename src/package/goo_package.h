#ifndef GOO_PACKAGE_H
#define GOO_PACKAGE_H

#include <stdbool.h>
#include <stdint.h>

// Forward declarations
typedef struct GooPackage GooPackage;
typedef struct GooPackageManager GooPackageManager;
typedef struct GooPackageDependency GooPackageDependency;
typedef struct GooPackageVersion GooPackageVersion;
typedef struct GooPackageRepository GooPackageRepository;

// Package version types
typedef enum {
    GOO_VERSION_EXACT = 0,    // Exact version (e.g., "1.2.3")
    GOO_VERSION_RANGE = 1,    // Version range (e.g., ">=1.2.0 <2.0.0")
    GOO_VERSION_CARET = 2,    // Caret range (e.g., "^1.2.3")
    GOO_VERSION_TILDE = 3,    // Tilde range (e.g., "~1.2.3")
    GOO_VERSION_LATEST = 4,   // Latest version
    GOO_VERSION_LOCAL = 5     // Local package
} GooVersionType;

// Package types
typedef enum {
    GOO_PACKAGE_LIBRARY = 0,  // Library package
    GOO_PACKAGE_APPLICATION = 1, // Application package
    GOO_PACKAGE_TOOL = 2,     // Tool package
    GOO_PACKAGE_META = 3      // Meta package (just dependencies)
} GooPackageType;

// Dependency resolution strategies
typedef enum {
    GOO_RESOLVE_NEWEST = 0,   // Use newest compatible version
    GOO_RESOLVE_OLDEST = 1,   // Use oldest compatible version
    GOO_RESOLVE_LOCKED = 2    // Use versions from lock file
} GooResolveStrategy;

// Package structure
struct GooPackage {
    char* name;                     // Package name
    char* description;              // Package description
    char* author;                   // Package author
    char* license;                  // Package license
    char* repository;               // Package repository URL
    char* homepage;                 // Package homepage URL
    GooPackageVersion* version;     // Package version
    GooPackageType type;            // Package type
    GooPackageDependency** dependencies; // Package dependencies
    int dependency_count;           // Number of dependencies
    char** files;                   // Package files
    int file_count;                 // Number of files
    char** tags;                    // Package tags
    int tag_count;                  // Number of tags
    char* readme;                   // README content
    char* path;                     // Local path to package (if installed)
};

// Version structure
struct GooPackageVersion {
    int major;                      // Major version
    int minor;                      // Minor version
    int patch;                      // Patch version
    char* prerelease;               // Prerelease identifier
    char* build;                    // Build metadata
    GooVersionType type;            // Version type
    char* raw_version;              // Raw version string
};

// Dependency structure
struct GooPackageDependency {
    char* name;                     // Package name
    GooPackageVersion* version;     // Version constraint
    bool optional;                  // Whether this dependency is optional
    bool development;               // Whether this is a development dependency
    char* source;                   // Source of the dependency (e.g., git URL)
};

// Package manager configuration
typedef struct {
    char* package_file;             // Path to package.json or equivalent
    char* lock_file;                // Path to lockfile
    char* install_dir;              // Installation directory
    char* cache_dir;                // Cache directory
    bool use_lock_file;             // Whether to use the lock file
    GooResolveStrategy resolve_strategy; // Dependency resolution strategy
    bool offline_mode;              // Whether to operate offline
    int concurrency;                // Maximum concurrent operations
    bool verbose;                   // Verbose output
    bool save_exact;                // Save exact versions instead of ranges
} GooPackageManagerConfig;

// === Package API ===

// Create a new package
GooPackage* goo_package_create(const char* name, GooPackageVersion* version);

// Destroy a package
void goo_package_destroy(GooPackage* package);

// Load a package from a package file
GooPackage* goo_package_load(const char* path);

// Save a package to a package file
bool goo_package_save(GooPackage* package, const char* path);

// Add a dependency to a package
bool goo_package_add_dependency(GooPackage* package, GooPackageDependency* dependency);

// Remove a dependency from a package
bool goo_package_remove_dependency(GooPackage* package, const char* name);

// Find a dependency in a package
GooPackageDependency* goo_package_find_dependency(GooPackage* package, const char* name);

// Add a file to a package
bool goo_package_add_file(GooPackage* package, const char* file);

// Add a tag to a package
bool goo_package_add_tag(GooPackage* package, const char* tag);

// Set package metadata
void goo_package_set_description(GooPackage* package, const char* description);
void goo_package_set_author(GooPackage* package, const char* author);
void goo_package_set_license(GooPackage* package, const char* license);
void goo_package_set_repository(GooPackage* package, const char* repository);
void goo_package_set_homepage(GooPackage* package, const char* homepage);
void goo_package_set_readme(GooPackage* package, const char* readme);

// === Version API ===

// Create a new version
GooPackageVersion* goo_version_create(int major, int minor, int patch, const char* prerelease, const char* build);

// Create a version from a string
GooPackageVersion* goo_version_from_string(const char* version_str);

// Convert a version to a string
char* goo_version_to_string(GooPackageVersion* version);

// Compare versions
int goo_version_compare(GooPackageVersion* v1, GooPackageVersion* v2);

// Check if a version satisfies a constraint
bool goo_version_satisfies(GooPackageVersion* version, GooPackageVersion* constraint);

// Destroy a version
void goo_version_destroy(GooPackageVersion* version);

// === Dependency API ===

// Create a new dependency
GooPackageDependency* goo_dependency_create(const char* name, GooPackageVersion* version);

// Create a dependency from a string
GooPackageDependency* goo_dependency_from_string(const char* dependency_str);

// Convert a dependency to a string
char* goo_dependency_to_string(GooPackageDependency* dependency);

// Destroy a dependency
void goo_dependency_destroy(GooPackageDependency* dependency);

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

#endif // GOO_PACKAGE_H 