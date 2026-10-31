add_library(aarch64 OBJECT
    "boot.S"
)
target_link_libraries(aarch64 PUBLIC kernel_flags)