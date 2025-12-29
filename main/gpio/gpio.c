#include "gpio/gpio.h"

void gpio_init(void){
    gpio_reset_pin(GPIO_ISR_SIGNAL);
    gpio_set_direction(GPIO_ISR_SIGNAL, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_ISR_SIGNAL, 0);
    return 0;
}

void gpio_isr_on(void){
    gpio_set_level(GPIO_ISR_SIGNAL, 1);
    printf("SPI_GPIO ON\n");
}

void gpio_isr_off(void){
    gpio_set_level(GPIO_ISR_SIGNAL, 0);
    printf("SPI_GPIO OFF\n");
}