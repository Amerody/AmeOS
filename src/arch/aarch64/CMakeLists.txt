add_library(aarch64 OBJECT
    "boot.S"
    "vectors.S"

    "timer.c"
    
    "exceptions.c"

    "arch.c"

    "irq.c"
)
target_link_libraries(aarch64 PUBLIC kernel_flags)