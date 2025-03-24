#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "goo_dependency.h"
#include "goo_package.h"

// === Dependency Resolver Implementation ===

GooDependencyResolver* goo_dependency_resolver_create(GooDependencyResolverConfig* config) {
    if (!config) return NULL;
    
    GooDependencyResolver* resolver = (GooDependencyResolver*)malloc(sizeof(GooDependencyResolver));
    if (!resolver) return NULL;
    
    // Copy configuration
    resolver->config.cache_dir = config->cache_dir ? strdup(config->cache_dir) : NULL;
    resolver->config.registry_url = config->registry_url ? strdup(config->registry_url) : NULL;
    resolver->config.offline_mode = config->offline_mode;
    resolver->config.strategy = config->strategy;
    resolver->config.max_depth = config->max_depth > 0 ? config->max_depth : 100; // Default max depth
    resolver->config.dev_dependencies = config->dev_dependencies;
    resolver->config.allow_prereleases = config->allow_prereleases;
    
    // Initialize other fields
    resolver->graph = NULL;
    resolver->resolved_packages = NULL;
    resolver->resolved_count = 0;
    resolver->errors = NULL;
    resolver->error_count = 0;
    
    return resolver;
}

void goo_dependency_resolver_destroy(GooDependencyResolver* resolver) {
    if (!resolver) return;
    
    // Free configuration strings
    free(resolver->config.cache_dir);
    free(resolver->config.registry_url);
    
    // Free graph
    if (resolver->graph) {
        goo_dependency_graph_destroy(resolver->graph);
    }
    
    // Free resolved packages (but don't destroy them)
    free(resolver->resolved_packages);
    
    // Free error messages
    for (int i = 0; i < resolver->error_count; i++) {
        free(resolver->errors[i]);
    }
    free(resolver->errors);
    
    // Free the resolver itself
    free(resolver);
}

// Helper function to add an error message to the resolver
static void add_error(GooDependencyResolver* resolver, const char* format, ...) {
    if (!resolver) return;
    
    // Format the error message
    va_list args;
    va_start(args, format);
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Allocate space for the new error
    char** new_errors = (char**)realloc(
        resolver->errors, 
        (resolver->error_count + 1) * sizeof(char*)
    );
    
    if (!new_errors) return;
    
    resolver->errors = new_errors;
    resolver->errors[resolver->error_count] = strdup(buffer);
    resolver->error_count++;
}

// Fetch a package from the registry or cache
static GooPackage* fetch_package(GooDependencyResolver* resolver, const char* name, GooPackageVersion* version) {
    if (!resolver || !name || !version) return NULL;
    
    // Check if we're in offline mode
    if (resolver->config.offline_mode) {
        // Try to load from cache
        char cache_path[512];
        snprintf(cache_path, sizeof(cache_path), "%s/%s-%s.json", 
                 resolver->config.cache_dir, name, version->raw_version);
        
        return goo_package_load(cache_path);
    }
    
    // In a real implementation, this would make a network request
    // to fetch the package from the registry
    
    // For now, just create a dummy package
    GooPackage* package = goo_package_create(name, goo_version_create(
        version->major, version->minor, version->patch, 
        version->prerelease, version->build
    ));
    
    // In a real implementation, we would populate the package
    // with its dependencies and other metadata
    
    return package;
}

GooDependencyResult goo_dependency_resolver_resolve(GooDependencyResolver* resolver, GooPackage* package) {
    if (!resolver || !package) return GOO_RESOLVE_ERROR;
    
    // Create a dependency graph for the package
    resolver->graph = goo_dependency_graph_create(package);
    if (!resolver->graph) {
        add_error(resolver, "Failed to create dependency graph for %s", package->name);
        return GOO_RESOLVE_ERROR;
    }
    
    // Check for cycles
    if (goo_dependency_graph_has_cycles(resolver->graph)) {
        add_error(resolver, "Cyclic dependencies detected in %s", package->name);
        return GOO_RESOLVE_CYCLE;
    }
    
    // Process the dependency graph
    bool success = process_dependency_graph(resolver);
    if (!success) {
        return resolver->error_count > 0 ? GOO_RESOLVE_CONFLICT : GOO_RESOLVE_ERROR;
    }
    
    // Success!
    return GOO_RESOLVE_SUCCESS;
}

// Helper function to process the dependency graph
static bool process_dependency_graph(GooDependencyResolver* resolver) {
    if (!resolver || !resolver->graph) return false;
    
    // Perform a topological sort of the graph
    int sorted_count;
    GooDependencyNode** sorted_nodes = goo_dependency_graph_topo_sort(resolver->graph, &sorted_count);
    if (!sorted_nodes) {
        add_error(resolver, "Failed to sort dependency graph");
        return false;
    }
    
    // Allocate space for resolved packages
    resolver->resolved_packages = (GooPackage**)malloc(sorted_count * sizeof(GooPackage*));
    if (!resolver->resolved_packages) {
        free(sorted_nodes);
        add_error(resolver, "Failed to allocate memory for resolved packages");
        return false;
    }
    
    // Process each node in topological order (dependencies first)
    bool success = true;
    resolver->resolved_count = 0;
    
    for (int i = 0; i < sorted_count; i++) {
        GooDependencyNode* node = sorted_nodes[i];
        
        // Skip the root package (it's already resolved)
        if (node == resolver->graph->root) {
            resolver->resolved_packages[resolver->resolved_count++] = node->package;
            continue;
        }
        
        // Check if the package conflicts with existing resolved packages
        if (goo_version_conflicts(node->package, resolver->graph)) {
            add_error(resolver, "Version conflict for package %s", node->package->name);
            success = false;
            continue;
        }
        
        // Add the package to the resolved list
        resolver->resolved_packages[resolver->resolved_count++] = node->package;
    }
    
    free(sorted_nodes);
    return success;
}

GooPackage** goo_dependency_resolver_get_packages(GooDependencyResolver* resolver, int* count) {
    if (!resolver || !count) return NULL;
    
    *count = resolver->resolved_count;
    return resolver->resolved_packages;
}

const char** goo_dependency_resolver_get_errors(GooDependencyResolver* resolver, int* count) {
    if (!resolver || !count) return NULL;
    
    *count = resolver->error_count;
    return (const char**)resolver->errors;
}

// === Dependency Graph Implementation ===

GooDependencyGraph* goo_dependency_graph_create(GooPackage* package) {
    if (!package) return NULL;
    
    GooDependencyGraph* graph = (GooDependencyGraph*)malloc(sizeof(GooDependencyGraph));
    if (!graph) return NULL;
    
    // Initialize the graph
    graph->nodes = NULL;
    graph->node_count = 0;
    
    // Create the root node
    graph->root = goo_dependency_graph_add_package(graph, package);
    if (!graph->root) {
        free(graph);
        return NULL;
    }
    
    // Recursively add dependencies
    if (!build_dependency_graph(graph, graph->root, 0)) {
        goo_dependency_graph_destroy(graph);
        return NULL;
    }
    
    return graph;
}

// Helper function to recursively build the dependency graph
static bool build_dependency_graph(GooDependencyGraph* graph, GooDependencyNode* node, int depth) {
    if (!graph || !node || depth > 100) return false; // Prevent infinite recursion
    
    // Mark the node as visited
    node->visited = true;
    node->depth = depth;
    
    // Process each dependency
    for (int i = 0; i < node->package->dependency_count; i++) {
        GooPackageDependency* dep = node->package->dependencies[i];
        
        // Skip optional dependencies
        if (dep->optional) continue;
        
        // Find the dependency in the graph
        GooDependencyNode* dep_node = goo_dependency_graph_find_package(graph, dep->name);
        
        if (!dep_node) {
            // Dependency not found, need to fetch and add it
            GooPackage* dep_package = fetch_package(NULL, dep->name, dep->version);
            if (!dep_package) {
                // Failed to fetch the package
                return false;
            }
            
            // Add the package to the graph
            dep_node = goo_dependency_graph_add_package(graph, dep_package);
            if (!dep_node) {
                goo_package_destroy(dep_package);
                return false;
            }
            
            // Recursively build the dependency's dependencies
            if (!build_dependency_graph(graph, dep_node, depth + 1)) {
                return false;
            }
        }
        
        // Add the dependency to the node's dependencies
        GooDependencyNode** new_deps = (GooDependencyNode**)realloc(
            node->dependencies, 
            (node->dependency_count + 1) * sizeof(GooDependencyNode*)
        );
        
        if (!new_deps) return false;
        
        node->dependencies = new_deps;
        node->dependencies[node->dependency_count++] = dep_node;
    }
    
    return true;
}

void goo_dependency_graph_destroy(GooDependencyGraph* graph) {
    if (!graph) return;
    
    // Free all nodes
    for (int i = 0; i < graph->node_count; i++) {
        GooDependencyNode* node = graph->nodes[i];
        
        // Free dependencies array (but not the nodes themselves)
        free(node->dependencies);
        
        // Free the node (but not the package)
        free(node);
    }
    
    // Free the nodes array
    free(graph->nodes);
    
    // Free the graph itself
    free(graph);
}

GooDependencyNode* goo_dependency_graph_add_package(GooDependencyGraph* graph, GooPackage* package) {
    if (!graph || !package) return NULL;
    
    // Check if the package is already in the graph
    for (int i = 0; i < graph->node_count; i++) {
        if (strcmp(graph->nodes[i]->package->name, package->name) == 0) {
            return graph->nodes[i];
        }
    }
    
    // Create a new node
    GooDependencyNode* node = (GooDependencyNode*)malloc(sizeof(GooDependencyNode));
    if (!node) return NULL;
    
    // Initialize the node
    node->package = package;
    node->dependencies = NULL;
    node->dependency_count = 0;
    node->visited = false;
    node->depth = 0;
    
    // Add the node to the graph
    GooDependencyNode** new_nodes = (GooDependencyNode**)realloc(
        graph->nodes, 
        (graph->node_count + 1) * sizeof(GooDependencyNode*)
    );
    
    if (!new_nodes) {
        free(node);
        return NULL;
    }
    
    graph->nodes = new_nodes;
    graph->nodes[graph->node_count++] = node;
    
    return node;
}

GooDependencyNode* goo_dependency_graph_find_package(GooDependencyGraph* graph, const char* name) {
    if (!graph || !name) return NULL;
    
    for (int i = 0; i < graph->node_count; i++) {
        if (strcmp(graph->nodes[i]->package->name, name) == 0) {
            return graph->nodes[i];
        }
    }
    
    return NULL;
}

bool goo_dependency_graph_has_cycles(GooDependencyGraph* graph) {
    if (!graph) return false;
    
    // Reset visited flags
    for (int i = 0; i < graph->node_count; i++) {
        graph->nodes[i]->visited = false;
    }
    
    // DFS cycle detection starting from the root
    return has_cycle_dfs(graph->root);
}

// Helper function for cycle detection using DFS
static bool has_cycle_dfs(GooDependencyNode* node) {
    if (!node) return false;
    
    // If we've already fully explored this node, no cycles here
    if (node->visited == 2) return false;
    
    // If we're in the process of exploring this node, we found a cycle
    if (node->visited == 1) return true;
    
    // Mark the node as being explored
    node->visited = 1;
    
    // Check all dependencies
    for (int i = 0; i < node->dependency_count; i++) {
        if (has_cycle_dfs(node->dependencies[i])) {
            return true;
        }
    }
    
    // Mark the node as fully explored
    node->visited = 2;
    
    return false;
}

GooDependencyNode** goo_dependency_graph_topo_sort(GooDependencyGraph* graph, int* count) {
    if (!graph || !count) return NULL;
    
    // Allocate space for the sorted nodes
    GooDependencyNode** sorted = (GooDependencyNode**)malloc(graph->node_count * sizeof(GooDependencyNode*));
    if (!sorted) return NULL;
    
    // Reset visited flags
    for (int i = 0; i < graph->node_count; i++) {
        graph->nodes[i]->visited = false;
    }
    
    // Perform topological sort
    *count = 0;
    for (int i = 0; i < graph->node_count; i++) {
        if (!graph->nodes[i]->visited) {
            topo_sort_dfs(graph->nodes[i], sorted, count);
        }
    }
    
    return sorted;
}

// Helper function for topological sort using DFS
static void topo_sort_dfs(GooDependencyNode* node, GooDependencyNode** sorted, int* count) {
    if (!node || node->visited) return;
    
    // Mark the node as visited
    node->visited = true;
    
    // Visit all dependencies first
    for (int i = 0; i < node->dependency_count; i++) {
        if (!node->dependencies[i]->visited) {
            topo_sort_dfs(node->dependencies[i], sorted, count);
        }
    }
    
    // Add the node to the sorted list
    sorted[(*count)++] = node;
}

// === Version Solver Implementation ===

GooPackageVersion* goo_version_solve(GooPackageVersion** constraints, int constraint_count, 
                                   GooVersionType strategy) {
    if (!constraints || constraint_count <= 0) return NULL;
    
    // For a single constraint, return a copy of it
    if (constraint_count == 1) {
        return goo_version_create(
            constraints[0]->major,
            constraints[0]->minor,
            constraints[0]->patch,
            constraints[0]->prerelease,
            constraints[0]->build
        );
    }
    
    // For multiple constraints, find a version that satisfies all of them
    
    // First, find the highest minimum version (greatest lower bound)
    GooPackageVersion* min_version = goo_version_create(0, 0, 0, NULL, NULL);
    
    for (int i = 0; i < constraint_count; i++) {
        if (goo_version_compare(constraints[i], min_version) > 0) {
            min_version->major = constraints[i]->major;
            min_version->minor = constraints[i]->minor;
            min_version->patch = constraints[i]->patch;
            
            free(min_version->prerelease);
            min_version->prerelease = constraints[i]->prerelease ? 
                                     strdup(constraints[i]->prerelease) : NULL;
            
            free(min_version->build);
            min_version->build = constraints[i]->build ? 
                               strdup(constraints[i]->build) : NULL;
        }
    }
    
    // Check if the minimum version satisfies all constraints
    bool satisfies_all = true;
    for (int i = 0; i < constraint_count; i++) {
        if (!goo_version_satisfies(min_version, constraints[i])) {
            satisfies_all = false;
            break;
        }
    }
    
    if (!satisfies_all) {
        // No solution found
        goo_version_destroy(min_version);
        return NULL;
    }
    
    return min_version;
}

bool goo_version_conflicts(GooPackage* package, GooDependencyGraph* graph) {
    if (!package || !graph) return false;
    
    // Check if the package is already in the graph
    GooDependencyNode* existing = goo_dependency_graph_find_package(graph, package->name);
    if (!existing) return false; // Not in the graph, no conflict
    
    // Check if the versions conflict
    return goo_version_compare(package->version, existing->package->version) != 0;
}

GooPackageVersion** goo_version_suggest_updates(GooDependencyGraph* graph, int* count) {
    if (!graph || !count) return NULL;
    
    // In a real implementation, this would use a constraint solver to
    // suggest updates to resolve version conflicts
    
    // For now, just return an empty array
    *count = 0;
    return NULL;
} 