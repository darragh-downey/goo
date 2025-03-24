#ifndef GOO_DEPENDENCY_H
#define GOO_DEPENDENCY_H

#include <stdbool.h>
#include "goo_package.h"

// Forward declarations
typedef struct GooDependencyGraph GooDependencyGraph;
typedef struct GooDependencyNode GooDependencyNode;
typedef struct GooDependencyResolver GooDependencyResolver;

// Dependency resolution result
typedef enum {
    GOO_RESOLVE_SUCCESS = 0,        // All dependencies resolved successfully
    GOO_RESOLVE_CONFLICT = 1,       // Version conflicts detected
    GOO_RESOLVE_MISSING = 2,        // Missing dependencies
    GOO_RESOLVE_CYCLE = 3,          // Cyclic dependencies detected
    GOO_RESOLVE_ERROR = 4           // Other error
} GooDependencyResult;

// Dependency graph node
struct GooDependencyNode {
    GooPackage* package;             // Package information
    GooDependencyNode** dependencies; // Dependencies of this package
    int dependency_count;            // Number of dependencies
    bool visited;                    // For cycle detection and traversal
    int depth;                       // Depth in the dependency tree
};

// Dependency graph structure
struct GooDependencyGraph {
    GooDependencyNode** nodes;       // All nodes in the graph
    int node_count;                  // Number of nodes
    GooDependencyNode* root;         // Root node (main package)
};

// Resolver configuration
typedef struct {
    char* cache_dir;                 // Package cache directory
    char* registry_url;              // Package registry URL
    bool offline_mode;               // Whether to operate offline
    GooResolveStrategy strategy;     // Resolution strategy
    int max_depth;                   // Maximum dependency depth
    bool dev_dependencies;           // Whether to include dev dependencies
    bool allow_prereleases;          // Whether to allow prerelease versions
} GooDependencyResolverConfig;

// Resolver structure
struct GooDependencyResolver {
    GooDependencyResolverConfig config;  // Configuration
    GooDependencyGraph* graph;           // Dependency graph
    GooPackage** resolved_packages;      // Resolved packages
    int resolved_count;                  // Number of resolved packages
    char** errors;                       // Error messages
    int error_count;                     // Number of errors
};

// === Dependency Resolver API ===

// Create a new dependency resolver
GooDependencyResolver* goo_dependency_resolver_create(GooDependencyResolverConfig* config);

// Destroy a dependency resolver
void goo_dependency_resolver_destroy(GooDependencyResolver* resolver);

// Resolve dependencies for a package
GooDependencyResult goo_dependency_resolver_resolve(GooDependencyResolver* resolver, GooPackage* package);

// Get resolved packages
GooPackage** goo_dependency_resolver_get_packages(GooDependencyResolver* resolver, int* count);

// Get error messages
const char** goo_dependency_resolver_get_errors(GooDependencyResolver* resolver, int* count);

// Create a dependency graph from a package
GooDependencyGraph* goo_dependency_graph_create(GooPackage* package);

// Destroy a dependency graph
void goo_dependency_graph_destroy(GooDependencyGraph* graph);

// Add a package to the dependency graph
GooDependencyNode* goo_dependency_graph_add_package(GooDependencyGraph* graph, GooPackage* package);

// Find a package in the dependency graph
GooDependencyNode* goo_dependency_graph_find_package(GooDependencyGraph* graph, const char* name);

// Detect cycles in the dependency graph
bool goo_dependency_graph_has_cycles(GooDependencyGraph* graph);

// Topological sort of the dependency graph
GooDependencyNode** goo_dependency_graph_topo_sort(GooDependencyGraph* graph, int* count);

// === Version Solver API ===

// Find the best version that satisfies all constraints
GooPackageVersion* goo_version_solve(GooPackageVersion** constraints, int constraint_count, 
                                   GooVersionType strategy);

// Check if a package version conflicts with existing dependencies
bool goo_version_conflicts(GooPackage* package, GooDependencyGraph* graph);

// Suggest version updates to resolve conflicts
GooPackageVersion** goo_version_suggest_updates(GooDependencyGraph* graph, int* count);

#endif // GOO_DEPENDENCY_H 