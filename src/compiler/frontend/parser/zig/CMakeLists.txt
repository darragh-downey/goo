# Zig build step for the parser
add_custom_target(parser
    COMMAND zig build-obj ${CMAKE_CURRENT_SOURCE_DIR}/parser.zig -I../../lexer/zig
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Zig parser module"
    SOURCES parser.zig ast.zig
    DEPENDS lexer
)

# Make sure the parser directory exists
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}) 