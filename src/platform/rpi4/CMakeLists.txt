add_library(rpi4 OBJECT
    "platform.c"

    "uart/uart.c"
    "uart/uart_mini.c"
    "uart/uart_pl011.c"

    "auxiliary.c"

    "gpio/gpio.c"

    "gic400/gic.c"
    "gic400/hal_irq.c"
)
target_link_libraries(rpi4 PUBLIC kernel_flags)
