add_library(kernel_core OBJECT
    "main.c"

    "lib/console.c"
    "lib/kprintf.c"
    "lib/panic.c"
    "lib/delay.c"

    "shell/shell.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)