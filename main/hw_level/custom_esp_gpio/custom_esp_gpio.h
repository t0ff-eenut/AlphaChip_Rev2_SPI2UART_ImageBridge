#ifndef CUSTOM_ESP_GPIO_H
#define CUSTOM_ESP_GPIO_H

// #include "../../project_top.h"
#include "hw_level_top.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_strip.h"
#include "driver/rmt.h"

#define LED_STRIP_GPIO_NUM                  GPIO_NUM_38

#define DEBUG_UART_PORT                     UART_NUM_1
#define DEBUG_TXD_GPIO_NUM                  GPIO_NUM_17
#define DEBUG_RXD_GPIO_NUM                  GPIO_NUM_18

// UART0 기본 핀 (부팅 시 노이즈 방지를 위해 풀업 초기화 필요)
#define UPLOAD_LOG_UART_PORT                UART_NUM_0
#define UPLOAD_LOG_TXD_GPIO_NUM             GPIO_NUM_43
#define UPLOAD_LOG_RXD_GPIO_NUM             GPIO_NUM_44

#define LED_STRIP_LEN                       1
#define LED_STRIP_RESOLUTION_HZ             10 * 1000 * 1000
#define LED_STRIP_WITH_DMA                  false

/**
 * @brief       Custom GPIO Initial Function
 * @attention   *주의사항
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @warning     *경고
 * @note        *참고사항
 * 
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */
bool custom_gpio_init(void);

#if LED_STRIP_ENABLE
    /**
    * @brief       Custom GPIO Set LED Strip Color Function
    * @attention   *주의사항
    * @param[in]   uint32_t    input_ui32_red_value : Red 색상 정도(0 ~ 255)
    * @param[in]   uint32_t    input_ui32_green_value : Green 색상 정도(0 ~ 255)
    * @param[in]   uint32_t    input_ui32_blue_value : Blue 색상 정도(0 ~ 255)
    * @return      bool    true : LED Strip 색상 변경 성공, false : LED Strip 색상 변경 실패
    * @warning     *경고
    * @note        *참고사항
    * 
    * @details     디테일 설명
    * @todo        todo
    * @bug         bug
    */
    bool custom_gpio_set_led_strip_color(const uint32_t input_ui32_red_value, const uint32_t input_ui32_green_value, const uint32_t input_ui32_blue_value);
    void custom_gpio_clear_led_strip(void);
#endif

/**
 * @brief       Custom GPIO Set LED Strip Color Function
 * @attention   *주의사항
 * @param[in]   void
 * @return      void
 * @warning     *경고
 * @note        *참고사항
 * 
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */
bool custom_gpio_deinit(void);

#endif
