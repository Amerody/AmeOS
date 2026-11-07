add_library(drivers OBJECT
    "console_uart.c"
    "console_fb.c"
)
target_link_libraries(drivers PUBLIC kernel_flags)
