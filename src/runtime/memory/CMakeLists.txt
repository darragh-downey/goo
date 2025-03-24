set(MEMORY_SOURCES
    memory_interface.c
    allocator.c
    scope_allocator.c
    region_allocator.c
    pool_allocator.c
)

# Create the memory component library
add_library(goo_memory STATIC ${MEMORY_SOURCES})

# Include directories
target_include_directories(goo_memory PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Link dependencies if any
target_link_libraries(goo_memory
    goo_runtime
) 