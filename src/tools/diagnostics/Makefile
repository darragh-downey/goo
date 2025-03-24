# Makefile for Goo Diagnostics System

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
AR = ar
ARFLAGS = rcs

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

# Targets
LIB_TARGET = $(LIB_DIR)/libgoo_diagnostics.a
EXAMPLE_TARGET = $(BIN_DIR)/diagnostics_example
INTEGRATION_TARGET = $(BIN_DIR)/diagnostics_integration

# Source files
LIB_SRCS = diagnostics.c error_catalog.c source_highlight.c diagnostics_module.c
EXAMPLE_SRCS = example.c
INTEGRATION_SRCS = integration.c

# Object files
LIB_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
EXAMPLE_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(EXAMPLE_SRCS))
INTEGRATION_OBJS = $(patsubst %.c,$(OBJ_DIR)/%.o,$(INTEGRATION_SRCS))

# Default target
all: directories $(LIB_TARGET) $(EXAMPLE_TARGET) $(INTEGRATION_TARGET)

# Create build directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(BIN_DIR)

# Build the diagnostics library
$(LIB_TARGET): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

# Build the example program
$(EXAMPLE_TARGET): $(EXAMPLE_OBJS) $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -lgoo_diagnostics

# Build the integration example
$(INTEGRATION_TARGET): $(INTEGRATION_OBJS) $(LIB_TARGET)
	$(CC) $(CFLAGS) -o $@ $< -L$(LIB_DIR) -lgoo_diagnostics

# Compile library source files
$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Run the example program
run-example: $(EXAMPLE_TARGET)
	./$(EXAMPLE_TARGET)

# Run the integration example
run-integration: $(INTEGRATION_TARGET)
	./$(INTEGRATION_TARGET)

# Run Zig build
zig-build:
	zig build

# Run Zig tests
zig-test:
	zig build test

# Install
install: $(LIB_TARGET)
	@mkdir -p ../../../lib
	@mkdir -p ../../../include/tools/diagnostics
	cp $(LIB_TARGET) ../../../lib/
	cp *.h ../../../include/tools/diagnostics/

.PHONY: all clean directories run-example run-integration zig-build zig-test install 