add_library(kernel_core OBJECT
    "kernel.c"

    "kprintf.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)