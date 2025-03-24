# Zig build step for the lexer
add_custom_target(lexer
    COMMAND zig build-obj ${CMAKE_CURRENT_SOURCE_DIR}/lexer.zig
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Zig lexer module"
    SOURCES lexer.zig token.zig keywords.zig
)

# Make sure the lexer directory exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}) 