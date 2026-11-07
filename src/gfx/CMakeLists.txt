add_library(gfx OBJECT
    "font.c"
)
target_link_libraries(gfx PUBLIC kernel_flags)