add_library(drivers OBJECT
    "console_uart.c"
)
target_link_libraries(drivers PUBLIC kernel_flags)
