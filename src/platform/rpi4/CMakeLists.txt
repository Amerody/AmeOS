set(RPI4_MODULES
    gpio
    aux
    uart
)

add_library(gpio OBJECT
    "gpio.c"
)
target_link_libraries(gpio PUBLIC kernel_flags)

add_library(aux OBJECT
    "auxiliary.c"
)
target_link_libraries(aux PUBLIC kernel_flags)

add_library(uart OBJECT
    "uart/uart.c"
    "uart/uart_mini.c"
    "uart/uart_pl011.c"
)
target_link_libraries(uart PUBLIC kernel_flags)

add_library(rpi4 INTERFACE)
foreach(module IN LISTS RPI4_MODULES)
    target_sources(rpi4 INTERFACE
        $<TARGET_OBJECTS:${module}>
    )
endforeach()
target_link_libraries(rpi4 INTERFACE kernel_flags)

# add_library(rpi4 STATIC
#     # gpio
#     "gpio.c"

#     # aux
#     "auxiliary.c"

#     # uart
#     "uart/uart.c"
#     "uart/uart_mini.c"
#     "uart/uart_pl011.c"
# )
# target_link_libraries(rpi4 PUBLIC kernel_flags)