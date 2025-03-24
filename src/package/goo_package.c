#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "goo_package.h"

// === Package Implementation ===

GooPackage* goo_package_create(const char* name, GooPackageVersion* version) {
    if (!name || !version) return NULL;
    
    GooPackage* package = (GooPackage*)malloc(sizeof(GooPackage));
    if (!package) return NULL;
    
    // Initialize the package structure
    package->name = strdup(name);
    package->description = NULL;
    package->author = NULL;
    package->license = NULL;
    package->repository = NULL;
    package->homepage = NULL;
    package->version = version;
    package->type = GOO_PACKAGE_LIBRARY;
    package->dependencies = NULL;
    package->dependency_count = 0;
    package->files = NULL;
    package->file_count = 0;
    package->tags = NULL;
    package->tag_count = 0;
    package->readme = NULL;
    package->path = NULL;
    
    return package;
}

void goo_package_destroy(GooPackage* package) {
    if (!package) return;
    
    // Free allocated strings
    free(package->name);
    free(package->description);
    free(package->author);
    free(package->license);
    free(package->repository);
    free(package->homepage);
    free(package->readme);
    free(package->path);
    
    // Free version
    if (package->version) {
        goo_version_destroy(package->version);
    }
    
    // Free dependencies
    for (int i = 0; i < package->dependency_count; i++) {
        goo_dependency_destroy(package->dependencies[i]);
    }
    free(package->dependencies);
    
    // Free files
    for (int i = 0; i < package->file_count; i++) {
        free(package->files[i]);
    }
    free(package->files);
    
    // Free tags
    for (int i = 0; i < package->tag_count; i++) {
        free(package->tags[i]);
    }
    free(package->tags);
    
    // Free the package itself
    free(package);
}

GooPackage* goo_package_load(const char* path) {
    if (!path) return NULL;
    
    FILE* file = fopen(path, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open package file: %s\n", path);
        return NULL;
    }
    
    // Read the file into a buffer
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = (char*)malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    size_t read_size = fread(buffer, 1, file_size, file);
    buffer[read_size] = '\0';
    fclose(file);
    
    // Parse the JSON content to create a package
    // Note: This is a placeholder for actual JSON parsing
    // In a real implementation, we would use a JSON parser library
    
    // For now, create a dummy package
    GooPackageVersion* version = goo_version_create(1, 0, 0, NULL, NULL);
    GooPackage* package = goo_package_create("loaded_package", version);
    
    // Set the path
    package->path = strdup(path);
    
    free(buffer);
    return package;
}

bool goo_package_save(GooPackage* package, const char* path) {
    if (!package || !path) return false;
    
    FILE* file = fopen(path, "w");
    if (!file) {
        fprintf(stderr, "Error: Unable to create package file: %s\n", path);
        return false;
    }
    
    // Serialize the package to JSON
    // Note: This is a placeholder for actual JSON serialization
    // In a real implementation, we would use a JSON generator library
    
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", package->name);
    
    if (package->description) {
        fprintf(file, "  \"description\": \"%s\",\n", package->description);
    }
    
    if (package->author) {
        fprintf(file, "  \"author\": \"%s\",\n", package->author);
    }
    
    if (package->license) {
        fprintf(file, "  \"license\": \"%s\",\n", package->license);
    }
    
    if (package->version) {
        char* version_str = goo_version_to_string(package->version);
        fprintf(file, "  \"version\": \"%s\",\n", version_str);
        free(version_str);
    }
    
    // Write dependencies if any
    if (package->dependency_count > 0) {
        fprintf(file, "  \"dependencies\": {\n");
        for (int i = 0; i < package->dependency_count; i++) {
            char* dep_str = goo_dependency_to_string(package->dependencies[i]);
            fprintf(file, "    \"%s\": \"%s\"%s\n", 
                    package->dependencies[i]->name, 
                    dep_str, 
                    (i < package->dependency_count - 1) ? "," : "");
            free(dep_str);
        }
        fprintf(file, "  },\n");
    }
    
    fprintf(file, "  \"type\": %d\n", package->type);
    fprintf(file, "}\n");
    
    fclose(file);
    return true;
}

bool goo_package_add_dependency(GooPackage* package, GooPackageDependency* dependency) {
    if (!package || !dependency) return false;
    
    // Check if dependency already exists
    for (int i = 0; i < package->dependency_count; i++) {
        if (strcmp(package->dependencies[i]->name, dependency->name) == 0) {
            // Replace the existing dependency
            goo_dependency_destroy(package->dependencies[i]);
            package->dependencies[i] = dependency;
            return true;
        }
    }
    
    // Add new dependency
    GooPackageDependency** new_deps = (GooPackageDependency**)realloc(
        package->dependencies, 
        (package->dependency_count + 1) * sizeof(GooPackageDependency*)
    );
    
    if (!new_deps) return false;
    
    package->dependencies = new_deps;
    package->dependencies[package->dependency_count] = dependency;
    package->dependency_count++;
    
    return true;
}

bool goo_package_remove_dependency(GooPackage* package, const char* name) {
    if (!package || !name) return false;
    
    int found_index = -1;
    
    // Find the dependency
    for (int i = 0; i < package->dependency_count; i++) {
        if (strcmp(package->dependencies[i]->name, name) == 0) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == -1) return false;  // Dependency not found
    
    // Free the dependency
    goo_dependency_destroy(package->dependencies[found_index]);
    
    // Shift remaining dependencies
    for (int i = found_index; i < package->dependency_count - 1; i++) {
        package->dependencies[i] = package->dependencies[i + 1];
    }
    
    // Resize the array
    package->dependency_count--;
    if (package->dependency_count == 0) {
        free(package->dependencies);
        package->dependencies = NULL;
    } else {
        GooPackageDependency** new_deps = (GooPackageDependency**)realloc(
            package->dependencies, 
            package->dependency_count * sizeof(GooPackageDependency*)
        );
        
        if (new_deps) {
            package->dependencies = new_deps;
        }
    }
    
    return true;
}

GooPackageDependency* goo_package_find_dependency(GooPackage* package, const char* name) {
    if (!package || !name) return NULL;
    
    for (int i = 0; i < package->dependency_count; i++) {
        if (strcmp(package->dependencies[i]->name, name) == 0) {
            return package->dependencies[i];
        }
    }
    
    return NULL;  // Dependency not found
}

bool goo_package_add_file(GooPackage* package, const char* file) {
    if (!package || !file) return false;
    
    // Check if file already exists
    for (int i = 0; i < package->file_count; i++) {
        if (strcmp(package->files[i], file) == 0) {
            return true;  // File already exists
        }
    }
    
    // Add new file
    char** new_files = (char**)realloc(
        package->files, 
        (package->file_count + 1) * sizeof(char*)
    );
    
    if (!new_files) return false;
    
    package->files = new_files;
    package->files[package->file_count] = strdup(file);
    package->file_count++;
    
    return true;
}

bool goo_package_add_tag(GooPackage* package, const char* tag) {
    if (!package || !tag) return false;
    
    // Check if tag already exists
    for (int i = 0; i < package->tag_count; i++) {
        if (strcmp(package->tags[i], tag) == 0) {
            return true;  // Tag already exists
        }
    }
    
    // Add new tag
    char** new_tags = (char**)realloc(
        package->tags, 
        (package->tag_count + 1) * sizeof(char*)
    );
    
    if (!new_tags) return false;
    
    package->tags = new_tags;
    package->tags[package->tag_count] = strdup(tag);
    package->tag_count++;
    
    return true;
}

// Set package metadata
void goo_package_set_description(GooPackage* package, const char* description) {
    if (!package) return;
    
    free(package->description);
    package->description = description ? strdup(description) : NULL;
}

void goo_package_set_author(GooPackage* package, const char* author) {
    if (!package) return;
    
    free(package->author);
    package->author = author ? strdup(author) : NULL;
}

void goo_package_set_license(GooPackage* package, const char* license) {
    if (!package) return;
    
    free(package->license);
    package->license = license ? strdup(license) : NULL;
}

void goo_package_set_repository(GooPackage* package, const char* repository) {
    if (!package) return;
    
    free(package->repository);
    package->repository = repository ? strdup(repository) : NULL;
}

void goo_package_set_homepage(GooPackage* package, const char* homepage) {
    if (!package) return;
    
    free(package->homepage);
    package->homepage = homepage ? strdup(homepage) : NULL;
}

void goo_package_set_readme(GooPackage* package, const char* readme) {
    if (!package) return;
    
    free(package->readme);
    package->readme = readme ? strdup(readme) : NULL;
}

// === Version Implementation ===

GooPackageVersion* goo_version_create(int major, int minor, int patch, const char* prerelease, const char* build) {
    GooPackageVersion* version = (GooPackageVersion*)malloc(sizeof(GooPackageVersion));
    if (!version) return NULL;
    
    version->major = major;
    version->minor = minor;
    version->patch = patch;
    version->prerelease = prerelease ? strdup(prerelease) : NULL;
    version->build = build ? strdup(build) : NULL;
    version->type = GOO_VERSION_EXACT;
    
    // Generate raw version string
    char buffer[128];
    sprintf(buffer, "%d.%d.%d", major, minor, patch);
    
    if (prerelease) {
        strcat(buffer, "-");
        strcat(buffer, prerelease);
    }
    
    if (build) {
        strcat(buffer, "+");
        strcat(buffer, build);
    }
    
    version->raw_version = strdup(buffer);
    
    return version;
}

GooPackageVersion* goo_version_from_string(const char* version_str) {
    if (!version_str) return NULL;
    
    // Parse version string
    int major = 0, minor = 0, patch = 0;
    char prerelease[64] = {0};
    char build[64] = {0};
    GooVersionType type = GOO_VERSION_EXACT;
    
    // Check for prefixes indicating the version type
    if (version_str[0] == '^') {
        type = GOO_VERSION_CARET;
        version_str++; // Skip the prefix
    } else if (version_str[0] == '~') {
        type = GOO_VERSION_TILDE;
        version_str++; // Skip the prefix
    } else if (strcmp(version_str, "latest") == 0) {
        // Create a version object for "latest"
        GooPackageVersion* version = goo_version_create(0, 0, 0, NULL, NULL);
        if (version) {
            version->type = GOO_VERSION_LATEST;
            free(version->raw_version);
            version->raw_version = strdup("latest");
        }
        return version;
    }
    
    // Parse the version components
    int result = sscanf(version_str, "%d.%d.%d", &major, &minor, &patch);
    if (result < 1) {
        return NULL; // Invalid version format
    }
    
    // Look for prerelease and build metadata
    const char* prerelease_start = strchr(version_str, '-');
    const char* build_start = strchr(version_str, '+');
    
    if (prerelease_start) {
        size_t prerelease_len;
        if (build_start) {
            prerelease_len = build_start - prerelease_start - 1;
        } else {
            prerelease_len = strlen(prerelease_start + 1);
        }
        
        if (prerelease_len < sizeof(prerelease)) {
            strncpy(prerelease, prerelease_start + 1, prerelease_len);
            prerelease[prerelease_len] = '\0';
        }
    }
    
    if (build_start) {
        strncpy(build, build_start + 1, sizeof(build) - 1);
    }
    
    // Create and return the version object
    GooPackageVersion* version = goo_version_create(major, minor, patch, 
                                                  *prerelease ? prerelease : NULL, 
                                                  *build ? build : NULL);
    if (version) {
        version->type = type;
        free(version->raw_version);
        version->raw_version = strdup(version_str);
    }
    
    return version;
}

char* goo_version_to_string(GooPackageVersion* version) {
    if (!version) return NULL;
    
    return strdup(version->raw_version);
}

int goo_version_compare(GooPackageVersion* v1, GooPackageVersion* v2) {
    if (!v1 || !v2) return 0;
    
    // Compare major version
    if (v1->major > v2->major) return 1;
    if (v1->major < v2->major) return -1;
    
    // Compare minor version
    if (v1->minor > v2->minor) return 1;
    if (v1->minor < v2->minor) return -1;
    
    // Compare patch version
    if (v1->patch > v2->patch) return 1;
    if (v1->patch < v2->patch) return -1;
    
    // If we get here, the numerical parts are equal
    // Pre-release versions have lower precedence than regular versions
    if (!v1->prerelease && v2->prerelease) return 1;
    if (v1->prerelease && !v2->prerelease) return -1;
    
    // If both have pre-release versions, compare them lexically
    if (v1->prerelease && v2->prerelease) {
        return strcmp(v1->prerelease, v2->prerelease);
    }
    
    // Versions are equal
    return 0;
}

bool goo_version_satisfies(GooPackageVersion* version, GooPackageVersion* constraint) {
    if (!version || !constraint) return false;
    
    // Handle special version types
    switch (constraint->type) {
        case GOO_VERSION_EXACT:
            // Exact version match required
            return goo_version_compare(version, constraint) == 0;
            
        case GOO_VERSION_CARET:
            // ^1.2.3 means >=1.2.3 <2.0.0
            if (version->major != constraint->major) return false;
            if (version->major == 0) {
                // For 0.x.y, ^0.x.y means >=0.x.y <0.(x+1).0
                if (version->minor < constraint->minor) return false;
                if (version->minor > constraint->minor) return true;
                return version->patch >= constraint->patch;
            } else {
                // For x.y.z where x > 0, ^x.y.z means >=x.y.z <(x+1).0.0
                if (version->minor < constraint->minor) return false;
                if (version->minor > constraint->minor) return true;
                return version->patch >= constraint->patch;
            }
            
        case GOO_VERSION_TILDE:
            // ~1.2.3 means >=1.2.3 <1.3.0
            if (version->major != constraint->major) return false;
            if (version->minor != constraint->minor) return false;
            return version->patch >= constraint->patch;
            
        case GOO_VERSION_LATEST:
            // Latest version always satisfies "latest" constraint
            return true;
            
        case GOO_VERSION_RANGE:
            // Range handling would require more complex logic
            // This is a placeholder for an actual range check
            return false;
            
        case GOO_VERSION_LOCAL:
            // Local package versions always match themselves
            return true;
    }
    
    return false;
}

void goo_version_destroy(GooPackageVersion* version) {
    if (!version) return;
    
    free(version->prerelease);
    free(version->build);
    free(version->raw_version);
    free(version);
}

// === Dependency Implementation ===

GooPackageDependency* goo_dependency_create(const char* name, GooPackageVersion* version) {
    if (!name) return NULL;
    
    GooPackageDependency* dependency = (GooPackageDependency*)malloc(sizeof(GooPackageDependency));
    if (!dependency) return NULL;
    
    dependency->name = strdup(name);
    dependency->version = version;
    dependency->optional = false;
    dependency->development = false;
    dependency->source = NULL;
    
    return dependency;
}

GooPackageDependency* goo_dependency_from_string(const char* dependency_str) {
    if (!dependency_str) return NULL;
    
    char name[256] = {0};
    char version_str[128] = {0};
    
    // Parse the dependency string (format: "name@version")
    const char* at_sign = strchr(dependency_str, '@');
    if (at_sign) {
        size_t name_len = at_sign - dependency_str;
        if (name_len < sizeof(name)) {
            strncpy(name, dependency_str, name_len);
            name[name_len] = '\0';
            
            strncpy(version_str, at_sign + 1, sizeof(version_str) - 1);
        }
    } else {
        // No version specified, use the entire string as the name
        strncpy(name, dependency_str, sizeof(name) - 1);
    }
    
    // Create the version object if a version was specified
    GooPackageVersion* version = NULL;
    if (*version_str) {
        version = goo_version_from_string(version_str);
    } else {
        // Default to latest version if none specified
        version = goo_version_from_string("latest");
    }
    
    // Create and return the dependency
    return goo_dependency_create(name, version);
}

char* goo_dependency_to_string(GooPackageDependency* dependency) {
    if (!dependency) return NULL;
    
    char* version_str = dependency->version ? 
                       goo_version_to_string(dependency->version) : 
                       strdup("latest");
    
    size_t len = strlen(dependency->name) + strlen(version_str) + 2; // +2 for '@' and '\0'
    char* result = (char*)malloc(len);
    
    if (result) {
        sprintf(result, "%s@%s", dependency->name, version_str);
    }
    
    free(version_str);
    return result;
}

void goo_dependency_destroy(GooPackageDependency* dependency) {
    if (!dependency) return;
    
    free(dependency->name);
    free(dependency->source);
    
    if (dependency->version) {
        goo_version_destroy(dependency->version);
    }
    
    free(dependency);
} 