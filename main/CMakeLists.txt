# idf_component_register(SRCS "tusb_console_main.c"
#                        INCLUDE_DIRS .)
idf_component_register(SRCS "queue/queue.h" "queue/queue.c" "gpio/gpio.h" "gpio/gpio.c" "spi/spi.h" "spi/spi.c" "uart/uart.h" "uart/uart.c" "tusb_console_main_module.c"
                       INCLUDE_DIRS .)
