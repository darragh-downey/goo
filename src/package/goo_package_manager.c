#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "goo_package_manager.h"
#include "goo_dependency.h"
#include "repository/goo_repository.h"

// === Package Manager Implementation ===

GooPackageManager* goo_package_manager_create(GooPackageManagerConfig* config) {
    if (!config) return NULL;
    
    GooPackageManager* manager = (GooPackageManager*)malloc(sizeof(GooPackageManager));
    if (!manager) return NULL;
    
    // Copy configuration
    manager->config.package_file = config->package_file ? strdup(config->package_file) : strdup("package.json");
    manager->config.lock_file = config->lock_file ? strdup(config->lock_file) : strdup("package-lock.json");
    manager->config.install_dir = config->install_dir ? strdup(config->install_dir) : strdup("./node_modules");
    manager->config.cache_dir = config->cache_dir ? strdup(config->cache_dir) : strdup("./.goo-cache");
    manager->config.use_lock_file = config->use_lock_file;
    manager->config.resolve_strategy = config->resolve_strategy;
    manager->config.offline_mode = config->offline_mode;
    manager->config.concurrency = config->concurrency > 0 ? config->concurrency : 4;
    manager->config.verbose = config->verbose;
    manager->config.save_exact = config->save_exact;
    
    // Initialize other fields
    manager->repositories = NULL;
    manager->repository_count = 0;
    
    // Get current working directory
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        manager->working_dir = strdup(cwd);
    } else {
        manager->working_dir = strdup(".");
    }
    
    // Set up cache directory
    if (config->cache_dir) {
        manager->cache_dir = strdup(config->cache_dir);
    } else {
        // Default to ~/.goo/cache
        const char* home = getenv("HOME");
        if (home) {
            char cache_path[1024];
            snprintf(cache_path, sizeof(cache_path), "%s/.goo/cache", home);
            manager->cache_dir = strdup(cache_path);
        } else {
            manager->cache_dir = strdup("./.goo-cache");
        }
    }
    
    manager->current_package = NULL;
    manager->initialized = false;
    
    // Add default repositories
    
    // Local repository (installed packages)
    char local_repo_path[1024];
    snprintf(local_repo_path, sizeof(local_repo_path), "%s", manager->config.install_dir);
    goo_package_manager_add_repository(manager, local_repo_path, "local");
    
    // Cache repository
    goo_package_manager_add_repository(manager, manager->cache_dir, "cache");
    
    // Remote repository (central package registry)
    goo_package_manager_add_repository(manager, "https://registry.goolang.org", "central");
    
    return manager;
}

void goo_package_manager_destroy(GooPackageManager* manager) {
    if (!manager) return;
    
    // Free configuration strings
    free(manager->config.package_file);
    free(manager->config.lock_file);
    free(manager->config.install_dir);
    free(manager->config.cache_dir);
    
    // Free repositories
    for (int i = 0; i < manager->repository_count; i++) {
        goo_repository_destroy(manager->repositories[i]);
    }
    free(manager->repositories);
    
    // Free other strings
    free(manager->working_dir);
    free(manager->cache_dir);
    
    // Free current package
    if (manager->current_package) {
        goo_package_destroy(manager->current_package);
    }
    
    // Free the manager itself
    free(manager);
}

bool goo_package_manager_init(GooPackageManager* manager, const char* name, const char* version) {
    if (!manager || !name) return false;
    
    // Create a new package
    GooPackageVersion* ver = NULL;
    if (version) {
        ver = goo_version_from_string(version);
    } else {
        ver = goo_version_create(1, 0, 0, NULL, NULL);
    }
    
    if (!ver) return false;
    
    // Create the package
    GooPackage* package = goo_package_create(name, ver);
    if (!package) {
        goo_version_destroy(ver);
        return false;
    }
    
    // Set default metadata
    goo_package_set_description(package, "A Goo package");
    goo_package_set_license(package, "MIT");
    
    // Set the current package
    if (manager->current_package) {
        goo_package_destroy(manager->current_package);
    }
    manager->current_package = package;
    
    // Save the package
    bool result = goo_package_manager_save_current(manager);
    if (result) {
        manager->initialized = true;
    }
    
    return result;
}

bool goo_package_manager_install(GooPackageManager* manager) {
    if (!manager) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Resolve dependencies
    GooDependencyResolverConfig resolver_config = {
        .cache_dir = manager->cache_dir,
        .registry_url = "https://registry.goolang.org",
        .offline_mode = manager->config.offline_mode,
        .strategy = manager->config.resolve_strategy,
        .max_depth = 100,
        .dev_dependencies = true,
        .allow_prereleases = false
    };
    
    GooDependencyResolver* resolver = goo_dependency_resolver_create(&resolver_config);
    if (!resolver) return false;
    
    // Resolve dependencies
    GooDependencyResult result = goo_dependency_resolver_resolve(resolver, manager->current_package);
    if (result != GOO_RESOLVE_SUCCESS) {
        int error_count;
        const char** errors = goo_dependency_resolver_get_errors(resolver, &error_count);
        for (int i = 0; i < error_count; i++) {
            fprintf(stderr, "Error: %s\n", errors[i]);
        }
        
        goo_dependency_resolver_destroy(resolver);
        return false;
    }
    
    // Get resolved packages
    int package_count;
    GooPackage** packages = goo_dependency_resolver_get_packages(resolver, &package_count);
    
    // Install each package
    bool success = true;
    for (int i = 0; i < package_count; i++) {
        GooPackage* pkg = packages[i];
        
        // Skip the root package
        if (strcmp(pkg->name, manager->current_package->name) == 0) {
            continue;
        }
        
        // Install the package
        char pkg_dir[1024];
        snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", manager->config.install_dir, pkg->name);
        
        // Create the package directory
        struct stat st = {0};
        if (stat(pkg_dir, &st) == -1) {
            if (mkdir(pkg_dir, 0755) != 0) {
                fprintf(stderr, "Error: Failed to create directory %s\n", pkg_dir);
                success = false;
                continue;
            }
        }
        
        // Save the package.json
        char pkg_file[1024];
        snprintf(pkg_file, sizeof(pkg_file), "%s/package.json", pkg_dir);
        if (!goo_package_save(pkg, pkg_file)) {
            fprintf(stderr, "Error: Failed to save package %s\n", pkg->name);
            success = false;
            continue;
        }
        
        if (manager->config.verbose) {
            char* version_str = goo_version_to_string(pkg->version);
            printf("Installed %s@%s\n", pkg->name, version_str);
            free(version_str);
        }
    }
    
    // Create lock file
    if (success) {
        success = goo_package_manager_create_lock(manager);
    }
    
    goo_dependency_resolver_destroy(resolver);
    
    return success;
}

bool goo_package_manager_install_package(GooPackageManager* manager, const char* name, const char* version) {
    if (!manager || !name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Create a version constraint
    GooPackageVersion* ver = NULL;
    if (version) {
        ver = goo_version_from_string(version);
    } else {
        ver = goo_version_from_string("latest");
    }
    
    if (!ver) return false;
    
    // Create the dependency
    GooPackageDependency* dep = goo_dependency_create(name, ver);
    if (!dep) {
        goo_version_destroy(ver);
        return false;
    }
    
    // Add the dependency to the current package
    bool added = goo_package_add_dependency(manager->current_package, dep);
    if (!added) {
        goo_dependency_destroy(dep);
        return false;
    }
    
    // Save the current package
    bool saved = goo_package_manager_save_current(manager);
    if (!saved) return false;
    
    // Install dependencies
    return goo_package_manager_install(manager);
}

bool goo_package_manager_uninstall(GooPackageManager* manager, const char* name) {
    if (!manager || !name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Remove the dependency from the current package
    bool removed = goo_package_remove_dependency(manager->current_package, name);
    if (!removed) {
        fprintf(stderr, "Error: Package %s is not a dependency\n", name);
        return false;
    }
    
    // Save the current package
    bool saved = goo_package_manager_save_current(manager);
    if (!saved) return false;
    
    // Remove the package directory
    char pkg_dir[1024];
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", manager->config.install_dir, name);
    
    // Remove the directory (recursively in a real implementation)
    // For now, just try to remove the package.json file
    char pkg_file[1024];
    snprintf(pkg_file, sizeof(pkg_file), "%s/package.json", pkg_dir);
    
    if (remove(pkg_file) != 0) {
        fprintf(stderr, "Warning: Failed to remove package file %s\n", pkg_file);
    }
    
    if (rmdir(pkg_dir) != 0) {
        fprintf(stderr, "Warning: Failed to remove package directory %s\n", pkg_dir);
    }
    
    if (manager->config.verbose) {
        printf("Uninstalled %s\n", name);
    }
    
    // Update lock file
    return goo_package_manager_create_lock(manager);
}

bool goo_package_manager_update(GooPackageManager* manager) {
    if (!manager) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // For now, just reinstall dependencies
    // In a real implementation, this would check for newer versions
    return goo_package_manager_install(manager);
}

bool goo_package_manager_update_package(GooPackageManager* manager, const char* name) {
    if (!manager || !name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Find the dependency
    GooPackageDependency* dep = goo_package_find_dependency(manager->current_package, name);
    if (!dep) {
        fprintf(stderr, "Error: Package %s is not a dependency\n", name);
        return false;
    }
    
    // Update the dependency to "latest"
    if (dep->version) {
        goo_version_destroy(dep->version);
    }
    dep->version = goo_version_from_string("latest");
    
    // Save the current package
    bool saved = goo_package_manager_save_current(manager);
    if (!saved) return false;
    
    // Reinstall dependencies
    return goo_package_manager_install(manager);
}

GooPackage** goo_package_manager_list(GooPackageManager* manager, int* count) {
    if (!manager || !count) return NULL;
    
    // Initialize count
    *count = 0;
    
    // Get the local repository
    GooPackageRepository* local_repo = NULL;
    for (int i = 0; i < manager->repository_count; i++) {
        if (strcmp(manager->repositories[i]->name, "local") == 0) {
            local_repo = manager->repositories[i];
            break;
        }
    }
    
    if (!local_repo) return NULL;
    
    // Search for all packages in the local repository
    GooRepositoryPackage* repo_packages = goo_repository_search(local_repo, NULL, count);
    if (!repo_packages || *count == 0) return NULL;
    
    // Convert to GooPackage objects
    GooPackage** packages = (GooPackage**)malloc(*count * sizeof(GooPackage*));
    if (!packages) {
        for (int i = 0; i < *count; i++) {
            goo_repository_package_free(&repo_packages[i]);
        }
        free(repo_packages);
        *count = 0;
        return NULL;
    }
    
    int actual_count = 0;
    for (int i = 0; i < *count; i++) {
        // Skip the current package
        if (manager->current_package && 
            strcmp(repo_packages[i].name, manager->current_package->name) == 0) {
            continue;
        }
        
        // Fetch the package
        GooPackage* pkg = goo_repository_fetch_package(local_repo, 
                                                     repo_packages[i].name, 
                                                     repo_packages[i].latest_version);
        if (pkg) {
            packages[actual_count++] = pkg;
        }
    }
    
    // Free repository packages
    for (int i = 0; i < *count; i++) {
        goo_repository_package_free(&repo_packages[i]);
    }
    free(repo_packages);
    
    *count = actual_count;
    return packages;
}

GooPackage* goo_package_manager_info(GooPackageManager* manager, const char* name) {
    if (!manager || !name) return NULL;
    
    // Check each repository in order of priority
    for (int i = 0; i < manager->repository_count; i++) {
        GooRepositoryPackage* info = goo_repository_get_info(manager->repositories[i], name);
        if (info) {
            // Found the package, fetch it
            GooPackage* pkg = goo_repository_fetch_package(manager->repositories[i], 
                                                         name, 
                                                         info->latest_version);
            
            goo_repository_package_free(info);
            free(info);
            
            return pkg;
        }
    }
    
    return NULL;
}

GooPackage** goo_package_manager_search(GooPackageManager* manager, const char* query, int* count) {
    if (!manager || !count) return NULL;
    
    // Initialize count
    *count = 0;
    
    // Search in all repositories
    // In a real implementation, this would merge results from all repositories
    
    // For now, just search in the central repository
    GooPackageRepository* central_repo = NULL;
    for (int i = 0; i < manager->repository_count; i++) {
        if (strcmp(manager->repositories[i]->name, "central") == 0) {
            central_repo = manager->repositories[i];
            break;
        }
    }
    
    if (!central_repo) return NULL;
    
    // Search for packages
    GooRepositoryPackage* repo_packages = goo_repository_search(central_repo, query, count);
    if (!repo_packages || *count == 0) return NULL;
    
    // Convert to GooPackage objects
    GooPackage** packages = (GooPackage**)malloc(*count * sizeof(GooPackage*));
    if (!packages) {
        for (int i = 0; i < *count; i++) {
            goo_repository_package_free(&repo_packages[i]);
        }
        free(repo_packages);
        *count = 0;
        return NULL;
    }
    
    int actual_count = 0;
    for (int i = 0; i < *count; i++) {
        // Fetch the package
        GooPackage* pkg = goo_repository_fetch_package(central_repo, 
                                                     repo_packages[i].name, 
                                                     repo_packages[i].latest_version);
        if (pkg) {
            packages[actual_count++] = pkg;
        }
    }
    
    // Free repository packages
    for (int i = 0; i < *count; i++) {
        goo_repository_package_free(&repo_packages[i]);
    }
    free(repo_packages);
    
    *count = actual_count;
    return packages;
}

bool goo_package_manager_add_repository(GooPackageManager* manager, const char* url, const char* name) {
    if (!manager || !url || !name) return false;
    
    // Check if the repository already exists
    for (int i = 0; i < manager->repository_count; i++) {
        if (strcmp(manager->repositories[i]->name, name) == 0) {
            // Repository already exists
            return true;
        }
    }
    
    // Determine the repository type
    GooRepositoryType type;
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        type = GOO_REPO_REMOTE;
    } else if (strncmp(url, "git://", 6) == 0 || strstr(url, ".git") != NULL) {
        type = GOO_REPO_GIT;
    } else {
        type = GOO_REPO_LOCAL;
    }
    
    // Create the repository
    GooPackageRepository* repo = goo_repository_create(name, url, type);
    if (!repo) return false;
    
    // Initialize the repository
    if (!goo_repository_init(repo)) {
        goo_repository_destroy(repo);
        return false;
    }
    
    // Add to the list of repositories
    GooPackageRepository** new_repos = (GooPackageRepository**)realloc(
        manager->repositories, 
        (manager->repository_count + 1) * sizeof(GooPackageRepository*)
    );
    
    if (!new_repos) {
        goo_repository_destroy(repo);
        return false;
    }
    
    manager->repositories = new_repos;
    manager->repositories[manager->repository_count] = repo;
    manager->repository_count++;
    
    return true;
}

bool goo_package_manager_remove_repository(GooPackageManager* manager, const char* name) {
    if (!manager || !name) return false;
    
    int found_index = -1;
    
    // Find the repository
    for (int i = 0; i < manager->repository_count; i++) {
        if (strcmp(manager->repositories[i]->name, name) == 0) {
            found_index = i;
            break;
        }
    }
    
    if (found_index == -1) return false;  // Repository not found
    
    // Free the repository
    goo_repository_destroy(manager->repositories[found_index]);
    
    // Shift remaining repositories
    for (int i = found_index; i < manager->repository_count - 1; i++) {
        manager->repositories[i] = manager->repositories[i + 1];
    }
    
    // Resize the array
    manager->repository_count--;
    if (manager->repository_count == 0) {
        free(manager->repositories);
        manager->repositories = NULL;
    } else {
        GooPackageRepository** new_repos = (GooPackageRepository**)realloc(
            manager->repositories, 
            manager->repository_count * sizeof(GooPackageRepository*)
        );
        
        if (new_repos) {
            manager->repositories = new_repos;
        }
    }
    
    return true;
}

GooPackageRepository** goo_package_manager_list_repositories(GooPackageManager* manager, int* count) {
    if (!manager || !count) return NULL;
    
    *count = manager->repository_count;
    return manager->repositories;
}

bool goo_package_manager_create_lock(GooPackageManager* manager) {
    if (!manager) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // In a real implementation, this would:
    // 1. Get a complete list of all installed packages and their versions
    // 2. Create a lock file with exact versions
    
    // For now, just create a simple lock file with the current dependencies
    
    FILE* lock_file = fopen(manager->config.lock_file, "w");
    if (!lock_file) {
        fprintf(stderr, "Error: Failed to create lock file %s\n", manager->config.lock_file);
        return false;
    }
    
    fprintf(lock_file, "{\n");
    fprintf(lock_file, "  \"name\": \"%s\",\n", manager->current_package->name);
    
    if (manager->current_package->version) {
        char* version_str = goo_version_to_string(manager->current_package->version);
        fprintf(lock_file, "  \"version\": \"%s\",\n", version_str);
        free(version_str);
    }
    
    // Write dependencies
    fprintf(lock_file, "  \"dependencies\": {\n");
    
    for (int i = 0; i < manager->current_package->dependency_count; i++) {
        GooPackageDependency* dep = manager->current_package->dependencies[i];
        char* version_str = goo_version_to_string(dep->version);
        
        fprintf(lock_file, "    \"%s\": \"%s\"%s\n", 
                dep->name, 
                version_str, 
                (i < manager->current_package->dependency_count - 1) ? "," : "");
        
        free(version_str);
    }
    
    fprintf(lock_file, "  }\n");
    fprintf(lock_file, "}\n");
    
    fclose(lock_file);
    
    return true;
}

bool goo_package_manager_load_lock(GooPackageManager* manager) {
    if (!manager) return false;
    
    // In a real implementation, this would parse the lock file and
    // ensure that all dependencies match the locked versions
    
    // For now, just check if the lock file exists
    FILE* lock_file = fopen(manager->config.lock_file, "r");
    if (!lock_file) {
        // Lock file doesn't exist
        return false;
    }
    
    fclose(lock_file);
    return true;
}

GooPackage* goo_package_manager_load_current(GooPackageManager* manager) {
    if (!manager) return NULL;
    
    // Try to load the package file
    char package_path[1024];
    snprintf(package_path, sizeof(package_path), "%s/%s", 
             manager->working_dir, manager->config.package_file);
    
    return goo_package_load(package_path);
}

bool goo_package_manager_save_current(GooPackageManager* manager) {
    if (!manager || !manager->current_package) return false;
    
    // Save the package file
    char package_path[1024];
    snprintf(package_path, sizeof(package_path), "%s/%s", 
             manager->working_dir, manager->config.package_file);
    
    return goo_package_save(manager->current_package, package_path);
}

bool goo_package_manager_add_dependency(GooPackageManager* manager, const char* name, const char* version) {
    if (!manager || !name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Create a version constraint
    GooPackageVersion* ver = NULL;
    if (version) {
        ver = goo_version_from_string(version);
    } else {
        ver = goo_version_from_string("latest");
    }
    
    if (!ver) return false;
    
    // Create the dependency
    GooPackageDependency* dep = goo_dependency_create(name, ver);
    if (!dep) {
        goo_version_destroy(ver);
        return false;
    }
    
    // Add the dependency to the current package
    bool added = goo_package_add_dependency(manager->current_package, dep);
    if (!added) {
        goo_dependency_destroy(dep);
        return false;
    }
    
    // Save the current package
    return goo_package_manager_save_current(manager);
}

bool goo_package_manager_remove_dependency(GooPackageManager* manager, const char* name) {
    if (!manager || !name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Remove the dependency from the current package
    bool removed = goo_package_remove_dependency(manager->current_package, name);
    if (!removed) return false;
    
    // Save the current package
    return goo_package_manager_save_current(manager);
}

bool goo_package_manager_publish(GooPackageManager* manager, const char* repository_name) {
    if (!manager || !repository_name) return false;
    
    // Load the current package if not already loaded
    if (!manager->current_package) {
        manager->current_package = goo_package_manager_load_current(manager);
        if (!manager->current_package) return false;
    }
    
    // Find the repository
    GooPackageRepository* repo = NULL;
    for (int i = 0; i < manager->repository_count; i++) {
        if (strcmp(manager->repositories[i]->name, repository_name) == 0) {
            repo = manager->repositories[i];
            break;
        }
    }
    
    if (!repo) {
        fprintf(stderr, "Error: Repository %s not found\n", repository_name);
        return false;
    }
    
    // Publish the package
    return goo_repository_publish_package(repo, manager->current_package);
} 