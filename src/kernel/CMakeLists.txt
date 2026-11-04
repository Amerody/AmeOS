add_library(kernel_core OBJECT
    "kernel.c"

<<<<<<< HEAD
=======
    "exceptions.c"

>>>>>>> homeserver/Dev
    "kprintf.c"
    "panic.c"
)
target_link_libraries(kernel_core PUBLIC kernel_flags)