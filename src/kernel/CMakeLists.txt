add_library(kernel_core OBJECT
    "main.c"

    # "exceptions.c"

    "lib/console.c"
    "lib/kprintf.c"
    "lib/panic.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)