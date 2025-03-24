#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../package/goo_package.h"
#include "../../package/goo_package_manager.h"
#include "../../package/goo_dependency.h"
#include "../../package/repository/goo_repository.h"

// Test package creation
void test_package_creation() {
    printf("Testing package creation...\n");
    
    // Create a package
    GooPackageVersion* version = goo_version_create(1, 0, 0, NULL, NULL);
    GooPackage* package = goo_package_create("test-package", version);
    
    assert(package != NULL);
    assert(strcmp(package->name, "test-package") == 0);
    assert(package->version != NULL);
    assert(package->version->major == 1);
    assert(package->version->minor == 0);
    assert(package->version->patch == 0);
    
    // Add a dependency
    GooPackageVersion* dep_version = goo_version_from_string("^2.0.0");
    GooPackageDependency* dependency = goo_dependency_create("test-dependency", dep_version);
    
    bool result = goo_package_add_dependency(package, dependency);
    assert(result == true);
    assert(package->dependency_count == 1);
    assert(strcmp(package->dependencies[0]->name, "test-dependency") == 0);
    
    // Set package metadata
    goo_package_set_description(package, "Test package description");
    goo_package_set_author(package, "Test Author");
    goo_package_set_license(package, "MIT");
    
    assert(strcmp(package->description, "Test package description") == 0);
    assert(strcmp(package->author, "Test Author") == 0);
    assert(strcmp(package->license, "MIT") == 0);
    
    // Save and load the package
    result = goo_package_save(package, "test_package.json");
    assert(result == true);
    
    GooPackage* loaded_package = goo_package_load("test_package.json");
    assert(loaded_package != NULL);
    assert(strcmp(loaded_package->name, "test-package") == 0);
    
    // Clean up
    goo_package_destroy(package);
    goo_package_destroy(loaded_package);
    remove("test_package.json");
    
    printf("Package creation test passed!\n");
}

// Test version handling
void test_version_handling() {
    printf("Testing version handling...\n");
    
    // Create versions
    GooPackageVersion* v1 = goo_version_from_string("1.2.3");
    GooPackageVersion* v2 = goo_version_from_string("1.2.4");
    GooPackageVersion* v3 = goo_version_from_string("2.0.0");
    GooPackageVersion* v4 = goo_version_from_string("1.2.3-alpha");
    GooPackageVersion* v5 = goo_version_from_string("^1.2.0");
    
    // Test version comparison
    assert(goo_version_compare(v1, v2) < 0);
    assert(goo_version_compare(v2, v1) > 0);
    assert(goo_version_compare(v2, v3) < 0);
    assert(goo_version_compare(v1, v4) > 0);  // Pre-release versions are lower
    
    // Test version constraints
    assert(goo_version_satisfies(v1, v1) == true);   // Exact match
    assert(goo_version_satisfies(v2, v5) == true);   // ^1.2.0 matches 1.2.4
    assert(goo_version_satisfies(v3, v5) == false);  // ^1.2.0 does not match 2.0.0
    
    // Clean up
    goo_version_destroy(v1);
    goo_version_destroy(v2);
    goo_version_destroy(v3);
    goo_version_destroy(v4);
    goo_version_destroy(v5);
    
    printf("Version handling test passed!\n");
}

// Test dependency resolution
void test_dependency_resolution() {
    printf("Testing dependency resolution...\n");
    
    // Create a dependency from string
    GooPackageDependency* dep1 = goo_dependency_from_string("test-dep@^1.0.0");
    assert(dep1 != NULL);
    assert(strcmp(dep1->name, "test-dep") == 0);
    assert(dep1->version != NULL);
    assert(dep1->version->type == GOO_VERSION_CARET);
    
    // Convert dependency to string
    char* dep_str = goo_dependency_to_string(dep1);
    assert(dep_str != NULL);
    assert(strstr(dep_str, "test-dep") != NULL);
    
    // Create test packages
    GooPackageVersion* v1 = goo_version_create(1, 0, 0, NULL, NULL);
    GooPackage* pkg1 = goo_package_create("root-package", v1);
    
    GooPackageVersion* v2 = goo_version_create(1, 1, 0, NULL, NULL);
    GooPackage* pkg2 = goo_package_create("dep-a", v2);
    
    GooPackageVersion* v3 = goo_version_create(2, 0, 0, NULL, NULL);
    GooPackage* pkg3 = goo_package_create("dep-b", v3);
    
    // Add dependencies
    GooPackageDependency* dep2 = goo_dependency_create("dep-a", goo_version_from_string("^1.0.0"));
    GooPackageDependency* dep3 = goo_dependency_create("dep-b", goo_version_from_string("^2.0.0"));
    
    goo_package_add_dependency(pkg1, dep2);
    goo_package_add_dependency(pkg1, dep3);
    
    // Create a dependency graph
    GooDependencyGraph* graph = goo_dependency_graph_create(pkg1);
    assert(graph != NULL);
    
    // Check for cycles
    assert(goo_dependency_graph_has_cycles(graph) == false);
    
    // Clean up
    goo_dependency_destroy(dep1);
    free(dep_str);
    goo_dependency_graph_destroy(graph);
    goo_package_destroy(pkg1);
    goo_package_destroy(pkg2);
    goo_package_destroy(pkg3);
    
    printf("Dependency resolution test passed!\n");
}

// Test repository operations
void test_repository_operations() {
    printf("Testing repository operations...\n");
    
    // Create a local repository
    GooPackageRepository* repo = goo_repository_create_local("test-repo", "./test-repo");
    assert(repo != NULL);
    assert(strcmp(repo->name, "test-repo") == 0);
    assert(repo->type == GOO_REPO_LOCAL);
    
    // Initialize the repository
    bool result = goo_repository_init(repo);
    assert(result == true);
    
    // Create a test package
    GooPackageVersion* version = goo_version_create(1, 0, 0, NULL, NULL);
    GooPackage* package = goo_package_create("repo-test-package", version);
    goo_package_set_description(package, "Repository test package");
    
    // Publish the package to the repository
    result = goo_repository_publish_package(repo, package);
    assert(result == true);
    
    // Clean up
    goo_repository_destroy(repo);
    goo_package_destroy(package);
    
    // Remove test repository directory
    system("rm -rf ./test-repo");
    
    printf("Repository operations test passed!\n");
}

// Test package manager
void test_package_manager() {
    printf("Testing package manager...\n");
    
    // Create a package manager configuration
    GooPackageManagerConfig config;
    memset(&config, 0, sizeof(config));
    config.package_file = "test-package.json";
    config.lock_file = "test-package-lock.json";
    config.install_dir = "./test-node_modules";
    config.cache_dir = "./test-cache";
    config.use_lock_file = true;
    config.resolve_strategy = GOO_RESOLVE_NEWEST;
    config.offline_mode = true;
    config.concurrency = 1;
    config.verbose = true;
    
    // Create a package manager
    GooPackageManager* manager = goo_package_manager_create(&config);
    assert(manager != NULL);
    
    // Initialize a new package
    bool result = goo_package_manager_init(manager, "test-project", "1.0.0");
    assert(result == true);
    
    // Add a dependency
    result = goo_package_manager_add_dependency(manager, "test-dependency", "^1.0.0");
    assert(result == true);
    
    // Save the current package
    result = goo_package_manager_save_current(manager);
    assert(result == true);
    
    // Clean up
    goo_package_manager_destroy(manager);
    
    // Remove test files
    remove("test-package.json");
    remove("test-package-lock.json");
    system("rm -rf ./test-node_modules");
    system("rm -rf ./test-cache");
    
    printf("Package manager test passed!\n");
}

int main() {
    printf("Running package management tests...\n");
    
    test_package_creation();
    test_version_handling();
    test_dependency_resolution();
    test_repository_operations();
    test_package_manager();
    
    printf("All package management tests passed!\n");
    return 0;
} 