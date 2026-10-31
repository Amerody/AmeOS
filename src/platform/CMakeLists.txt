add_library(platform INTERFACE)
if (KERNEL_PLATFORM STREQUAL "rpi4")
    add_subdirectory("rpi4")
    target_link_libraries(platform INTERFACE 
        kernel_flags
        rpi4
    )
else() # qemu-virt
    add_subdirectory("qemu-virt")
    target_link_libraries(platform INTERFACE 
        kernel_flags
        qemu-virt
    )
endif()
