#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include "goo_repository.h"
#include "../goo_package.h"

// Memory buffer for HTTP responses
typedef struct {
    char* data;
    size_t size;
} MemoryBuffer;

// == Local Repository Implementation ==

// Initialize a local repository
static bool local_repo_init(GooPackageRepository* repo) {
    if (!repo || !repo->url) return false;
    
    // Check if the directory exists
    DIR* dir = opendir(repo->url);
    if (!dir) {
        // Try to create the directory
        int result = mkdir(repo->url, 0755);
        if (result != 0) {
            return false;
        }
    } else {
        closedir(dir);
    }
    
    return true;
}

// Clean up a local repository
static void local_repo_cleanup(GooPackageRepository* repo) {
    // Nothing to clean up for local repository
    (void)repo;
}

// Search for packages in a local repository
static GooRepositoryPackage* local_repo_search(GooPackageRepository* repo, const char* query, int* count) {
    if (!repo || !repo->url || !count) return NULL;
    
    DIR* dir = opendir(repo->url);
    if (!dir) return NULL;
    
    // Count the number of matching packages
    struct dirent* entry;
    *count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if it's a directory (each package is in its own directory)
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", repo->url, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // If query is NULL or empty, include all packages
            // Otherwise, check if the package name contains the query
            if (!query || !*query || strstr(entry->d_name, query) != NULL) {
                (*count)++;
            }
        }
    }
    
    // Allocate memory for the results
    GooRepositoryPackage* results = NULL;
    if (*count > 0) {
        results = (GooRepositoryPackage*)malloc(*count * sizeof(GooRepositoryPackage));
        if (!results) {
            closedir(dir);
            *count = 0;
            return NULL;
        }
    }
    
    // Reset directory and populate results
    rewinddir(dir);
    int index = 0;
    
    while ((entry = readdir(dir)) != NULL && index < *count) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if it's a directory
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", repo->url, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            // If query is NULL or empty, include all packages
            // Otherwise, check if the package name contains the query
            if (!query || !*query || strstr(entry->d_name, query) != NULL) {
                // Initialize package info
                results[index].name = strdup(entry->d_name);
                results[index].versions = NULL;
                results[index].version_count = 0;
                results[index].description = NULL;
                results[index].author = NULL;
                results[index].latest_version = NULL;
                results[index].repository_url = NULL;
                results[index].homepage = NULL;
                results[index].license = NULL;
                results[index].tags = NULL;
                results[index].tag_count = 0;
                results[index].downloads = 0;
                results[index].last_updated = NULL;
                
                // Load the latest version's metadata
                char latest_path[512];
                snprintf(latest_path, sizeof(latest_path), "%s/%s/latest.json", repo->url, entry->d_name);
                
                GooPackage* pkg = goo_package_load(latest_path);
                if (pkg) {
                    results[index].description = pkg->description ? strdup(pkg->description) : NULL;
                    results[index].author = pkg->author ? strdup(pkg->author) : NULL;
                    results[index].latest_version = pkg->version ? 
                                                 goo_version_to_string(pkg->version) : NULL;
                    results[index].repository_url = pkg->repository ? strdup(pkg->repository) : NULL;
                    results[index].homepage = pkg->homepage ? strdup(pkg->homepage) : NULL;
                    results[index].license = pkg->license ? strdup(pkg->license) : NULL;
                    
                    // Copy tags
                    if (pkg->tag_count > 0) {
                        results[index].tags = (char**)malloc(pkg->tag_count * sizeof(char*));
                        if (results[index].tags) {
                            results[index].tag_count = pkg->tag_count;
                            for (int i = 0; i < pkg->tag_count; i++) {
                                results[index].tags[i] = strdup(pkg->tags[i]);
                            }
                        }
                    }
                    
                    goo_package_destroy(pkg);
                }
                
                index++;
            }
        }
    }
    
    closedir(dir);
    return results;
}

// Get information about a package in a local repository
static GooRepositoryPackage* local_repo_get_info(GooPackageRepository* repo, const char* name) {
    if (!repo || !repo->url || !name) return NULL;
    
    // Check if the package directory exists
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", repo->url, name);
    
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return NULL;
    }
    
    // Allocate memory for the package info
    GooRepositoryPackage* package = (GooRepositoryPackage*)malloc(sizeof(GooRepositoryPackage));
    if (!package) return NULL;
    
    // Initialize package info
    package->name = strdup(name);
    package->versions = NULL;
    package->version_count = 0;
    package->description = NULL;
    package->author = NULL;
    package->latest_version = NULL;
    package->repository_url = NULL;
    package->homepage = NULL;
    package->license = NULL;
    package->tags = NULL;
    package->tag_count = 0;
    package->downloads = 0;
    package->last_updated = NULL;
    
    // Load the latest version's metadata
    char latest_path[512];
    snprintf(latest_path, sizeof(latest_path), "%s/%s/latest.json", repo->url, name);
    
    GooPackage* pkg = goo_package_load(latest_path);
    if (pkg) {
        package->description = pkg->description ? strdup(pkg->description) : NULL;
        package->author = pkg->author ? strdup(pkg->author) : NULL;
        package->latest_version = pkg->version ? 
                               goo_version_to_string(pkg->version) : NULL;
        package->repository_url = pkg->repository ? strdup(pkg->repository) : NULL;
        package->homepage = pkg->homepage ? strdup(pkg->homepage) : NULL;
        package->license = pkg->license ? strdup(pkg->license) : NULL;
        
        // Copy tags
        if (pkg->tag_count > 0) {
            package->tags = (char**)malloc(pkg->tag_count * sizeof(char*));
            if (package->tags) {
                package->tag_count = pkg->tag_count;
                for (int i = 0; i < pkg->tag_count; i++) {
                    package->tags[i] = strdup(pkg->tags[i]);
                }
            }
        }
        
        goo_package_destroy(pkg);
    }
    
    // Get available versions
    DIR* dir = opendir(path);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            // Check if it's a file ending with .json and not "latest.json"
            if (strstr(entry->d_name, ".json") && strcmp(entry->d_name, "latest.json") != 0) {
                // Extract version from filename (e.g., "1.0.0.json" -> "1.0.0")
                char* version = strdup(entry->d_name);
                char* dot = strrchr(version, '.');
                if (dot) *dot = '\0';
                
                // Add to versions array
                char** new_versions = (char**)realloc(
                    package->versions, 
                    (package->version_count + 1) * sizeof(char*)
                );
                
                if (new_versions) {
                    package->versions = new_versions;
                    package->versions[package->version_count++] = version;
                } else {
                    free(version);
                }
            }
        }
        
        closedir(dir);
    }
    
    return package;
}

// Get available versions of a package in a local repository
static char** local_repo_get_versions(GooPackageRepository* repo, const char* name, int* count) {
    if (!repo || !repo->url || !name || !count) return NULL;
    
    // Check if the package directory exists
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", repo->url, name);
    
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        *count = 0;
        return NULL;
    }
    
    // Get available versions
    DIR* dir = opendir(path);
    if (!dir) {
        *count = 0;
        return NULL;
    }
    
    // Count versions
    struct dirent* entry;
    *count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Check if it's a file ending with .json and not "latest.json"
        if (strstr(entry->d_name, ".json") && strcmp(entry->d_name, "latest.json") != 0) {
            (*count)++;
        }
    }
    
    // Allocate memory for versions
    char** versions = NULL;
    if (*count > 0) {
        versions = (char**)malloc(*count * sizeof(char*));
        if (!versions) {
            closedir(dir);
            *count = 0;
            return NULL;
        }
    }
    
    // Reset directory and populate versions
    rewinddir(dir);
    int index = 0;
    
    while ((entry = readdir(dir)) != NULL && index < *count) {
        // Check if it's a file ending with .json and not "latest.json"
        if (strstr(entry->d_name, ".json") && strcmp(entry->d_name, "latest.json") != 0) {
            // Extract version from filename (e.g., "1.0.0.json" -> "1.0.0")
            char* version = strdup(entry->d_name);
            char* dot = strrchr(version, '.');
            if (dot) *dot = '\0';
            
            versions[index++] = version;
        }
    }
    
    closedir(dir);
    return versions;
}

// Fetch a package from a local repository
static GooPackage* local_repo_fetch_package(GooPackageRepository* repo, const char* name, const char* version) {
    if (!repo || !repo->url || !name) return NULL;
    
    // Determine the package file path
    char path[512];
    if (!version || strcmp(version, "latest") == 0) {
        snprintf(path, sizeof(path), "%s/%s/latest.json", repo->url, name);
    } else {
        snprintf(path, sizeof(path), "%s/%s/%s.json", repo->url, name, version);
    }
    
    // Load the package
    return goo_package_load(path);
}

// Publish a package to a local repository
static bool local_repo_publish_package(GooPackageRepository* repo, GooPackage* package) {
    if (!repo || !repo->url || !package || !package->name || !package->version) return false;
    
    // Ensure the package directory exists
    char pkg_dir[512];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", repo->url, package->name);
    
    DIR* dir = opendir(pkg_dir);
    if (!dir) {
        // Create the directory
        int result = mkdir(pkg_dir, 0755);
        if (result != 0) {
            return false;
        }
    } else {
        closedir(dir);
    }
    
    // Save the package to the version-specific file
    char* version_str = goo_version_to_string(package->version);
    if (!version_str) return false;
    
    char version_path[512];
    snprintf(version_path, sizeof(version_path), "%s/%s/%s.json", 
             repo->url, package->name, version_str);
    
    bool result = goo_package_save(package, version_path);
    
    // Also save as the latest version
    if (result) {
        char latest_path[512];
        snprintf(latest_path, sizeof(latest_path), "%s/%s/latest.json", 
                 repo->url, package->name);
        
        result = goo_package_save(package, latest_path);
    }
    
    free(version_str);
    return result;
}

// == Remote Repository Implementation ==

// Callback function for libcurl to write data to a memory buffer
static size_t curl_write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t real_size = size * nmemb;
    MemoryBuffer* mem = (MemoryBuffer*)userp;
    
    char* ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;
    
    return real_size;
}

// Initialize a remote repository
static bool remote_repo_init(GooPackageRepository* repo) {
    if (!repo || !repo->url) return false;
    
    // Initialize libcurl
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    // Set up the custom data
    repo->custom_data = curl;
    
    return true;
}

// Clean up a remote repository
static void remote_repo_cleanup(GooPackageRepository* repo) {
    if (!repo || !repo->custom_data) return;
    
    // Clean up libcurl
    CURL* curl = (CURL*)repo->custom_data;
    curl_easy_cleanup(curl);
    repo->custom_data = NULL;
}

// Helper function to make HTTP requests
static char* make_http_request(GooPackageRepository* repo, const char* endpoint) {
    if (!repo || !repo->custom_data || !endpoint) return NULL;
    
    CURL* curl = (CURL*)repo->custom_data;
    CURLcode res;
    MemoryBuffer chunk;
    
    chunk.data = malloc(1);
    chunk.size = 0;
    
    // Build the URL
    char url[512];
    snprintf(url, sizeof(url), "%s%s", repo->url, endpoint);
    
    // Set up the request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "goo-package-manager/1.0");
    
    // Perform the request
    res = curl_easy_perform(curl);
    
    // Check for errors
    if (res != CURLE_OK) {
        free(chunk.data);
        return NULL;
    }
    
    return chunk.data;
}

// Search for packages in a remote repository
static GooRepositoryPackage* remote_repo_search(GooPackageRepository* repo, const char* query, int* count) {
    if (!repo || !count) return NULL;
    
    // Build the search endpoint
    char endpoint[512];
    if (query && *query) {
        snprintf(endpoint, sizeof(endpoint), "/api/search?q=%s", query);
    } else {
        strcpy(endpoint, "/api/packages");
    }
    
    // Make the request
    char* response = make_http_request(repo, endpoint);
    if (!response) {
        *count = 0;
        return NULL;
    }
    
    // Parse the JSON response
    // In a real implementation, this would use a JSON parser library
    
    // For now, just create a dummy result
    *count = 1;
    GooRepositoryPackage* results = (GooRepositoryPackage*)malloc(sizeof(GooRepositoryPackage));
    if (!results) {
        free(response);
        *count = 0;
        return NULL;
    }
    
    results[0].name = strdup("example-package");
    results[0].versions = (char**)malloc(sizeof(char*));
    results[0].versions[0] = strdup("1.0.0");
    results[0].version_count = 1;
    results[0].description = strdup("Example package from remote repository");
    results[0].author = strdup("Remote Author");
    results[0].latest_version = strdup("1.0.0");
    results[0].repository_url = strdup("https://github.com/example/repo");
    results[0].homepage = strdup("https://example.com");
    results[0].license = strdup("MIT");
    results[0].tags = (char**)malloc(2 * sizeof(char*));
    results[0].tags[0] = strdup("example");
    results[0].tags[1] = strdup("remote");
    results[0].tag_count = 2;
    results[0].downloads = 1000;
    results[0].last_updated = strdup("2023-01-01");
    
    free(response);
    return results;
}

// Get information about a package in a remote repository
static GooRepositoryPackage* remote_repo_get_info(GooPackageRepository* repo, const char* name) {
    if (!repo || !name) return NULL;
    
    // Build the package info endpoint
    char endpoint[512];
    snprintf(endpoint, sizeof(endpoint), "/api/packages/%s", name);
    
    // Make the request
    char* response = make_http_request(repo, endpoint);
    if (!response) {
        return NULL;
    }
    
    // Parse the JSON response
    // In a real implementation, this would use a JSON parser library
    
    // For now, just create a dummy result
    GooRepositoryPackage* package = (GooRepositoryPackage*)malloc(sizeof(GooRepositoryPackage));
    if (!package) {
        free(response);
        return NULL;
    }
    
    package->name = strdup(name);
    package->versions = (char**)malloc(sizeof(char*));
    package->versions[0] = strdup("1.0.0");
    package->version_count = 1;
    package->description = strdup("Example package from remote repository");
    package->author = strdup("Remote Author");
    package->latest_version = strdup("1.0.0");
    package->repository_url = strdup("https://github.com/example/repo");
    package->homepage = strdup("https://example.com");
    package->license = strdup("MIT");
    package->tags = (char**)malloc(2 * sizeof(char*));
    package->tags[0] = strdup("example");
    package->tags[1] = strdup("remote");
    package->tag_count = 2;
    package->downloads = 1000;
    package->last_updated = strdup("2023-01-01");
    
    free(response);
    return package;
}

// Get available versions of a package in a remote repository
static char** remote_repo_get_versions(GooPackageRepository* repo, const char* name, int* count) {
    if (!repo || !name || !count) return NULL;
    
    // Build the versions endpoint
    char endpoint[512];
    snprintf(endpoint, sizeof(endpoint), "/api/packages/%s/versions", name);
    
    // Make the request
    char* response = make_http_request(repo, endpoint);
    if (!response) {
        *count = 0;
        return NULL;
    }
    
    // Parse the JSON response
    // In a real implementation, this would use a JSON parser library
    
    // For now, just create a dummy result
    *count = 1;
    char** versions = (char**)malloc(sizeof(char*));
    if (!versions) {
        free(response);
        *count = 0;
        return NULL;
    }
    
    versions[0] = strdup("1.0.0");
    
    free(response);
    return versions;
}

// Fetch a package from a remote repository
static GooPackage* remote_repo_fetch_package(GooPackageRepository* repo, const char* name, const char* version) {
    if (!repo || !name) return NULL;
    
    // Build the package fetch endpoint
    char endpoint[512];
    if (!version || strcmp(version, "latest") == 0) {
        snprintf(endpoint, sizeof(endpoint), "/api/packages/%s/latest", name);
    } else {
        snprintf(endpoint, sizeof(endpoint), "/api/packages/%s/%s", name, version);
    }
    
    // Make the request
    char* response = make_http_request(repo, endpoint);
    if (!response) {
        return NULL;
    }
    
    // Parse the JSON response
    // In a real implementation, this would use a JSON parser library
    
    // For now, just create a dummy package
    GooPackageVersion* pkg_version = goo_version_create(1, 0, 0, NULL, NULL);
    GooPackage* package = goo_package_create(name, pkg_version);
    
    goo_package_set_description(package, "Example package from remote repository");
    goo_package_set_author(package, "Remote Author");
    goo_package_set_license(package, "MIT");
    goo_package_set_repository(package, "https://github.com/example/repo");
    goo_package_set_homepage(package, "https://example.com");
    
    goo_package_add_tag(package, "example");
    goo_package_add_tag(package, "remote");
    
    free(response);
    return package;
}

// Publish a package to a remote repository
static bool remote_repo_publish_package(GooPackageRepository* repo, GooPackage* package) {
    if (!repo || !package) return NULL;
    
    // In a real implementation, this would upload the package to the remote repository
    // using HTTP POST or similar
    
    // For now, just return success
    return true;
}

// === Repository API Implementation ===

GooPackageRepository* goo_repository_create(const char* name, const char* url, GooRepositoryType type) {
    if (!name || !url) return NULL;
    
    GooPackageRepository* repo = (GooPackageRepository*)malloc(sizeof(GooPackageRepository));
    if (!repo) return NULL;
    
    // Initialize the repository
    repo->name = strdup(name);
    repo->url = strdup(url);
    repo->type = type;
    repo->enabled = true;
    repo->priority = 0;
    repo->custom_data = NULL;
    
    // Set up the repository operations based on type
    switch (type) {
        case GOO_REPO_LOCAL:
            repo->init = local_repo_init;
            repo->cleanup = local_repo_cleanup;
            repo->search = local_repo_search;
            repo->get_info = local_repo_get_info;
            repo->get_versions = local_repo_get_versions;
            repo->fetch_package = local_repo_fetch_package;
            repo->publish_package = local_repo_publish_package;
            break;
            
        case GOO_REPO_REMOTE:
            repo->init = remote_repo_init;
            repo->cleanup = remote_repo_cleanup;
            repo->search = remote_repo_search;
            repo->get_info = remote_repo_get_info;
            repo->get_versions = remote_repo_get_versions;
            repo->fetch_package = remote_repo_fetch_package;
            repo->publish_package = remote_repo_publish_package;
            break;
            
        case GOO_REPO_GIT:
            // Git repository operations not implemented yet
            free(repo->name);
            free(repo->url);
            free(repo);
            return NULL;
            
        case GOO_REPO_CUSTOM:
            // Custom repository operations must be set by the caller
            repo->init = NULL;
            repo->cleanup = NULL;
            repo->search = NULL;
            repo->get_info = NULL;
            repo->get_versions = NULL;
            repo->fetch_package = NULL;
            repo->publish_package = NULL;
            break;
    }
    
    return repo;
}

void goo_repository_destroy(GooPackageRepository* repo) {
    if (!repo) return;
    
    // Clean up the repository
    if (repo->cleanup) {
        repo->cleanup(repo);
    }
    
    // Free allocated strings
    free(repo->name);
    free(repo->url);
    
    // Free the repository itself
    free(repo);
}

bool goo_repository_init(GooPackageRepository* repo) {
    if (!repo || !repo->init) return false;
    
    return repo->init(repo);
}

void goo_repository_cleanup(GooPackageRepository* repo) {
    if (!repo || !repo->cleanup) return;
    
    repo->cleanup(repo);
}

GooRepositoryPackage* goo_repository_search(GooPackageRepository* repo, const char* query, int* count) {
    if (!repo || !repo->search || !count) return NULL;
    
    return repo->search(repo, query, count);
}

GooRepositoryPackage* goo_repository_get_info(GooPackageRepository* repo, const char* name) {
    if (!repo || !repo->get_info || !name) return NULL;
    
    return repo->get_info(repo, name);
}

char** goo_repository_get_versions(GooPackageRepository* repo, const char* name, int* count) {
    if (!repo || !repo->get_versions || !name || !count) return NULL;
    
    return repo->get_versions(repo, name, count);
}

GooPackage* goo_repository_fetch_package(GooPackageRepository* repo, const char* name, const char* version) {
    if (!repo || !repo->fetch_package || !name) return NULL;
    
    return repo->fetch_package(repo, name, version);
}

bool goo_repository_publish_package(GooPackageRepository* repo, GooPackage* package) {
    if (!repo || !repo->publish_package || !package) return false;
    
    return repo->publish_package(repo, package);
}

void goo_repository_package_free(GooRepositoryPackage* package) {
    if (!package) return;
    
    // Free strings
    free(package->name);
    free(package->description);
    free(package->author);
    free(package->latest_version);
    free(package->repository_url);
    free(package->homepage);
    free(package->license);
    free(package->last_updated);
    
    // Free versions
    for (int i = 0; i < package->version_count; i++) {
        free(package->versions[i]);
    }
    free(package->versions);
    
    // Free tags
    for (int i = 0; i < package->tag_count; i++) {
        free(package->tags[i]);
    }
    free(package->tags);
}

// === Repository Factory Implementation ===

GooPackageRepository* goo_repository_create_local(const char* name, const char* path) {
    return goo_repository_create(name, path, GOO_REPO_LOCAL);
}

GooPackageRepository* goo_repository_create_remote(const char* name, const char* url) {
    return goo_repository_create(name, url, GOO_REPO_REMOTE);
}

GooPackageRepository* goo_repository_create_git(const char* name, const char* url) {
    return goo_repository_create(name, url, GOO_REPO_GIT);
} 