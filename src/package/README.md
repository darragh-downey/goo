# Goo Package Management System

The Goo Package Management System is a comprehensive solution for managing packages, dependencies, and repositories for the Goo programming language. It provides a complete set of tools for package creation, distribution, and consumption.

## Features

- **Package Definition**: Define packages with metadata, dependencies, and version constraints
- **Dependency Resolution**: Resolve dependencies with support for version ranges and constraints
- **Package Installation**: Install packages and their dependencies
- **Repository Management**: Support for local, remote, and Git repositories
- **Version Management**: Semantic versioning support with ranges and constraints
- **Lockfile Support**: Lock dependency versions for reproducible builds
- **Command Line Interface**: Easy-to-use CLI for managing packages

## Components

The package management system consists of several key components:

1. **Core Package API** (`goo_package.h/c`): Define and manipulate packages
2. **Dependency Resolution** (`goo_dependency.h/c`): Resolve package dependencies
3. **Repository Management** (`goo_repository.h/c`): Interact with package repositories
4. **Package Manager** (`goo_package_manager.h/c`): High-level API for package management
5. **CLI Tool** (`goo_package_cli.c`): Command line interface for package management

## Building

To build the package management system:

```bash
cd src/package
make
```

This will build:
- The package management library (`libgoo_package.a`)
- The package management tests (`goo_package_test`)
- The package management CLI tool (`goo-package`)

## Testing

To run the tests:

```bash
make test
```

## Usage

### Command Line Interface

The `goo-package` command line tool provides a simple interface for managing packages:

```bash
# Initialize a new package
goo-package init my-package 1.0.0

# Add a dependency
goo-package add some-dependency ^1.2.3

# Install dependencies
goo-package install

# Update dependencies
goo-package update

# List installed packages
goo-package list

# Search for packages
goo-package search keyword

# Publish a package
goo-package publish
```

### Programmatic API

The package management system can also be used programmatically:

```c
#include "goo_package.h"
#include "goo_package_manager.h"

// Create a package manager
GooPackageManagerConfig config = {
    .package_file = "package.json",
    .lock_file = "package-lock.json",
    .install_dir = "./node_modules",
    .use_lock_file = true,
    .resolve_strategy = GOO_RESOLVE_NEWEST,
    .verbose = true
};

GooPackageManager* manager = goo_package_manager_create(&config);

// Initialize a new package
goo_package_manager_init(manager, "my-package", "1.0.0");

// Add a dependency
goo_package_manager_add_dependency(manager, "some-dependency", "^1.2.3");

// Install dependencies
goo_package_manager_install(manager);

// Clean up
goo_package_manager_destroy(manager);
```

## Package File Format

The package file (`package.json`) is a JSON file that defines a package:

```json
{
  "name": "my-package",
  "version": "1.0.0",
  "description": "My awesome package",
  "author": "John Doe",
  "license": "MIT",
  "repository": "https://github.com/johndoe/my-package",
  "homepage": "https://my-package.example.com",
  "dependencies": {
    "some-dependency": "^1.2.3",
    "another-dependency": "~2.0.0"
  }
}
```

## Version Constraints

The package management system supports various version constraints:

- **Exact version**: `1.2.3`
- **Caret range**: `^1.2.3` (compatible with 1.2.3 or later, but less than 2.0.0)
- **Tilde range**: `~1.2.3` (compatible with 1.2.3 or later, but less than 1.3.0)
- **Latest version**: `latest`

## Repository Support

The package management system supports different types of repositories:

- **Local repositories**: For local development and testing
- **Remote repositories**: For centralized package distribution
- **Git repositories**: For version-controlled packages

## Integration with Goo Language

The package management system is designed to integrate seamlessly with the Goo language:

- Automatic dependency resolution during compilation
- Package-aware module system
- Language server integration for IDE support
- Build system integration

## Future Enhancements

- Enhanced security features (package signing, checksums)
- Workspaces for monorepo support
- Plugin system for extensibility
- Binary package support
- Development/Production dependency management 