# Main Makefile Modifications for Zig Lexer Integration

This document outlines the changes needed in the main Goo compiler Makefile to integrate the Zig lexer.

## Overview

The Zig lexer integration provides three main options for the build system:

1. **Fully integrated**: The main Makefile automatically chooses between Flex and Zig lexers
2. **Zig-only**: The build system uses only the Zig lexer
3. **Flex-only**: The build system continues to use only the Flex lexer

We recommend the fully integrated approach, which allows for easy comparison and fallback options.

## Steps to Modify the Main Makefile

### Step 1: Include the Lexer Makefile

At the beginning of the main Makefile, after variable definitions but before the main targets, add:

```makefile
# Include the lexer Makefile
include src/lexer/Makefile.compiler
```

### Step 2: Add the Lexer Variables

Find the object files, compiler flags, and clean targets sections in the main Makefile, and add:

```makefile
# Add lexer object files
OBJS += $(LEXER_OBJS)

# Add lexer compiler flags
CFLAGS += $(LEXER_CFLAGS)

# Add lexer clean targets
CLEAN_TARGETS += clean-lexer
```

### Step 3: Add Lexer as a Dependency

Find the main compiler target (often `goo`, `compiler`, or `all`) and add the lexer target as a dependency:

```makefile
goo: lexer $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)
```

### Step 4: Add Lexer Configuration Options

If you want to allow users to choose the lexer implementation at build time, add these lines to the top of the Makefile:

```makefile
# Lexer options (0 = Flex, 1 = Zig)
USE_ZIG_LEXER ?= 1
LEXER_DEBUG ?= 0
```

## Configuration Options

### Using the Flex Lexer

To use the Flex lexer:

```bash
make USE_ZIG_LEXER=0
```

### Using the Zig Lexer (Default)

To use the Zig lexer (which is the default):

```bash
make
# or explicitly:
make USE_ZIG_LEXER=1
```

### Enabling Debug Output

To enable debug output in the lexer:

```bash
make LEXER_DEBUG=1
```

## Using with CMake (Alternative Build System)

If your project uses CMake instead of Makefiles, add the following to your `CMakeLists.txt`:

```cmake
# Set lexer options
option(USE_ZIG_LEXER "Use Zig lexer instead of Flex" ON)
option(LEXER_DEBUG "Enable lexer debug output" OFF)

# Configure the lexer
if(USE_ZIG_LEXER)
    # Build Zig lexer
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/libgoolexer.a
        COMMAND cd ${CMAKE_SOURCE_DIR}/src/lexer/zig && zig build
        DEPENDS ${CMAKE_SOURCE_DIR}/src/lexer/zig/lexer.zig 
               ${CMAKE_SOURCE_DIR}/src/lexer/zig/token.zig
               ${CMAKE_SOURCE_DIR}/src/lexer/zig/lexer_bindings.zig
        COMMENT "Building Zig lexer"
    )

    # Copy header file
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/include/goo_lexer.h
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/src/lexer/zig/zig-out/include/goo_lexer.h
                ${CMAKE_SOURCE_DIR}/include/goo_lexer.h
        DEPENDS ${CMAKE_BINARY_DIR}/libgoolexer.a
        COMMENT "Copying Zig lexer header"
    )

    # Add Zig lexer and integration layer to the compiler
    add_library(goolexer STATIC IMPORTED)
    set_target_properties(goolexer PROPERTIES
        IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/src/lexer/zig/zig-out/lib/libgoolexer.a
    )
    
    # Compile the C integration file
    set(LEXER_SOURCES ${CMAKE_SOURCE_DIR}/src/lexer/zig_integration.c)
    
    # Add compiler definition
    add_definitions(-DUSE_ZIG_LEXER=1)
    
    # Link the Zig lexer
    list(APPEND COMPILER_LIBS goolexer)
else()
    # Use Flex lexer
    find_package(FLEX REQUIRED)
    FLEX_TARGET(GooLexer ${CMAKE_SOURCE_DIR}/src/lexer/lexer.l 
                ${CMAKE_BINARY_DIR}/lexer.c)
    
    # Add Flex lexer to the compiler
    set(LEXER_SOURCES ${FLEX_GooLexer_OUTPUTS})
    
    # Add compiler definition
    add_definitions(-DUSE_ZIG_LEXER=0)
endif()

# Add debug flag if requested
if(LEXER_DEBUG)
    add_definitions(-DLEXER_DEBUG=1)
endif()

# Add the lexer sources to the compiler
target_sources(goo PRIVATE ${LEXER_SOURCES})
```

## If Something Goes Wrong

If you encounter issues with the Zig lexer integration:

1. Verify that Zig is installed and available in the path
2. Check that the Zig lexer builds correctly with `make -C src/lexer/zig`
3. Try building with `LEXER_DEBUG=1` for more detailed output
4. Fall back to the Flex lexer with `USE_ZIG_LEXER=0` until the issue is resolved

## Next Steps After Integration

1. Run the full compiler test suite to verify the integration
2. Compare performance between the Flex and Zig lexers
3. Document any differences or improvements in behavior
4. Consider removing the Flex lexer dependency if no issues are found 