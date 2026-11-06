add_library(platform INTERFACE)
add_subdirectory("rpi4")
target_link_libraries(platform INTERFACE 
    kernel_flags
    $<TARGET_OBJECTS:rpi4>
)