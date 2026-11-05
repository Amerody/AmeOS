add_library(kernel_core OBJECT
    "main.c"

    "exceptions.c"

    "kprintf.c"
    "panic.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)