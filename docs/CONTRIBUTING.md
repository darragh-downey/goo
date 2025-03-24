# Contributing to Goo

Thank you for your interest in contributing to the Goo programming language! This document outlines the process for contributing to the project.

## Code of Conduct

Please be respectful and considerate of others when contributing to the project. We aim to foster an inclusive and welcoming community.

## Getting Started

1. Fork the repository
2. Clone your fork
3. Add the upstream repository as a remote
4. Create a new branch for your changes

## Development Environment Setup

1. Install Zig 0.11.0 or later
2. Install a C compiler (GCC, Clang, or MSVC)
3. Install Make

## Building and Testing

Before submitting a pull request, ensure that your changes build and pass all tests:

```bash
./build_and_test.sh
```

## Coding Conventions

### Zig Code

- Follow the Zig style guide
- Use meaningful variable and function names
- Write clear comments for complex logic
- Include docstrings for public functions

### C API

- Follow the C99 standard
- Prefix all functions with `goo_`
- Use snake_case for function and variable names
- Include Doxygen-style comments for all public API functions

## Pull Request Process

1. Update the README.md or documentation with details of changes if appropriate
2. Update the ROADMAP.md file if your changes complete a roadmap item
3. Create a pull request with a clear title and description
4. Address any feedback from code reviewers

## Commit Messages

Please use clear and descriptive commit messages:

- Start with a concise summary line (50 characters or less)
- Follow with a detailed description if necessary
- Reference issues or pull requests when appropriate

## Adding Features

If you're adding a new feature, please first open an issue to discuss the feature. This ensures that your work aligns with the project's goals and roadmap.

## Reporting Bugs

When reporting bugs, please include:

- A clear and descriptive title
- Steps to reproduce the bug
- Expected behavior
- Actual behavior
- Environment information (OS, Zig version, etc.)

## License

By contributing to Goo, you agree that your contributions will be licensed under the project's MIT license. 