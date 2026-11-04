add_library(aarch64 OBJECT
    "boot.S"
<<<<<<< HEAD

    "timer.c"
=======
    "vectors.S"

    # "timer.c"
    "gic.c"
>>>>>>> homeserver/Dev
)
target_link_libraries(aarch64 PUBLIC kernel_flags)