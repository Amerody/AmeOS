add_library(arch INTERFACE)
add_subdirectory("aarch64")
target_link_libraries(arch INTERFACE 
    kernel_flags
    $<TARGET_OBJECTS:aarch64>
)