add_library(lib OBJECT
    "string.c"
    "memory.c"
)
target_link_libraries(lib PUBLIC kernel_flags)