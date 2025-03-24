#ifndef GOO_REPOSITORY_H
#define GOO_REPOSITORY_H

#include <stdbool.h>
#include "../goo_package.h"

// Forward declarations
typedef struct GooPackageRepository GooPackageRepository;
typedef struct GooRepositoryPackage GooRepositoryPackage;

// Repository types
typedef enum {
    GOO_REPO_LOCAL = 0,       // Local directory repository
    GOO_REPO_REMOTE = 1,      // Remote HTTP repository
    GOO_REPO_GIT = 2,         // Git repository
    GOO_REPO_CUSTOM = 3       // Custom repository type
} GooRepositoryType;

// Repository package information
struct GooRepositoryPackage {
    char* name;                // Package name
    char** versions;           // Available versions
    int version_count;         // Number of versions
    char* description;         // Package description
    char* author;              // Package author
    char* latest_version;      // Latest version
    char* repository_url;      // Source repository URL
    char* homepage;            // Homepage URL
    char* license;             // License
    char** tags;               // Tags
    int tag_count;             // Number of tags
    int downloads;             // Download count
    char* last_updated;        // Last updated timestamp
};

// Repository structure
struct GooPackageRepository {
    char* name;                // Repository name
    char* url;                 // Repository URL
    GooRepositoryType type;    // Repository type
    bool enabled;              // Whether the repository is enabled
    int priority;              // Repository priority (lower = higher priority)
    void* custom_data;         // Custom repository data
    
    // Repository operations
    bool (*init)(GooPackageRepository* repo);
    void (*cleanup)(GooPackageRepository* repo);
    GooRepositoryPackage* (*search)(GooPackageRepository* repo, const char* query, int* count);
    GooRepositoryPackage* (*get_info)(GooPackageRepository* repo, const char* name);
    char** (*get_versions)(GooPackageRepository* repo, const char* name, int* count);
    GooPackage* (*fetch_package)(GooPackageRepository* repo, const char* name, const char* version);
    bool (*publish_package)(GooPackageRepository* repo, GooPackage* package);
};

// === Repository API ===

// Create a new repository
GooPackageRepository* goo_repository_create(const char* name, const char* url, GooRepositoryType type);

// Destroy a repository
void goo_repository_destroy(GooPackageRepository* repo);

// Initialize a repository
bool goo_repository_init(GooPackageRepository* repo);

// Clean up a repository
void goo_repository_cleanup(GooPackageRepository* repo);

// Search for packages in a repository
GooRepositoryPackage* goo_repository_search(GooPackageRepository* repo, const char* query, int* count);

// Get information about a package in a repository
GooRepositoryPackage* goo_repository_get_info(GooPackageRepository* repo, const char* name);

// Get available versions of a package in a repository
char** goo_repository_get_versions(GooPackageRepository* repo, const char* name, int* count);

// Fetch a package from a repository
GooPackage* goo_repository_fetch_package(GooPackageRepository* repo, const char* name, const char* version);

// Publish a package to a repository
bool goo_repository_publish_package(GooPackageRepository* repo, GooPackage* package);

// Free a repository package
void goo_repository_package_free(GooRepositoryPackage* package);

// === Repository Factory ===

// Create a local repository
GooPackageRepository* goo_repository_create_local(const char* name, const char* path);

// Create a remote repository
GooPackageRepository* goo_repository_create_remote(const char* name, const char* url);

// Create a git repository
GooPackageRepository* goo_repository_create_git(const char* name, const char* url);

#endif // GOO_REPOSITORY_H 