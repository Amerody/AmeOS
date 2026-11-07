add_library(rpi4 OBJECT
    "platform.c"

    "uart/uart.c"
    "uart/uart_mini.c"
    "uart/uart_pl011.c"

    "auxiliary/auxiliary.c"

    "gpio/gpio.c"

    "gic400/gic.c"
    "gic400/hal_irq.c"

    "mailbox/mailbox.c"
    
    "framebuffer/fb.c"
)
target_link_libraries(rpi4 PUBLIC kernel_flags)

target_include_directories(rpi4
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}        # src/platform/rpi4 -> "gpio/gpio.h", "soc.h"
        ${CMAKE_CURRENT_SOURCE_DIR}/..     # src/platform      -> "common/mmio.h"
)