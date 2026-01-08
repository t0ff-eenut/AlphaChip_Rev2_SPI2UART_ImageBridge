/**
 * @file        custom_esp_gpio.h
 * @brief       ESP32의 GPIO Pin 할당 및 제어 헤더 파일
 * @author      T0T
 * @date        2025-12-30
 * @version     1.0.0
 * @details     이 파일은 ESP32의 GPIO 핀 할당과 LED Strip 제어 기능 구현에 필요한 메크로 및 헤더를 제공함.
 *              - SPI 핀 설정 값 메크로
 *              - UART 핀 설정 값 메크로
 *              - LED Strip 핀 설정 값 메크로 및 제어 기능 헤더
 * @todo        SPI Init 기능 구현해야함
 */
#ifndef CUSTOM_ESP_GPIO_H
#define CUSTOM_ESP_GPIO_H

#include "hw_level_top.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_strip.h"
#include "driver/rmt.h"

/*===========================================================================*/
/* 메크로 정의
/*===========================================================================*/
/*===========================================================================*/
/* LED STRIP 기능 활성화 설정                                                */
/*===========================================================================*/
#ifndef LED_STRIP_ENABLE
    /**
     * @brief       LED Strip 기능 활성화 (BackUP)
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 false 설정
     */
    #define LED_STRIP_ENABLE                false
#endif

/*===========================================================================*/
/* GPIO Pin Mapping                                                            */
/*===========================================================================*/
/**
 * @defgroup    GPIO_SETTING GPIO Pin 할당 그룹
 * @brief       GPIO 사용 핀 할당 정의
 * @details     
 * @todo        항목 수정
 * @{
 */
    /**
     * @brief       Debug UART 포트
     * @details     Debug UART 포트 설정
     */
    #define DEBUG_UART_PORT                     UART_NUM_1

    /**
     * @brief       Debug UART TXD 핀
     * @details     Debug UART TXD 핀 설정
     */
    #define DEBUG_TXD_GPIO_NUM                  GPIO_NUM_17

    /**
     * @brief       Debug UART RXD 핀
     * @details     Debug UART RXD 핀 설정
     */
    #define DEBUG_RXD_GPIO_NUM                  GPIO_NUM_18

    /**
     * @brief       Upload Log UART 포트
     * @details     Upload Log UART 포트 설정
     */
    #define UPLOAD_LOG_UART_PORT                UART_NUM_0

    /**
     * @brief       Upload Log UART TXD 핀
     * @details     Upload Log UART TXD 핀 설정
     */
    #define UPLOAD_LOG_TXD_GPIO_NUM             GPIO_NUM_43

    /**
     * @brief       Upload Log UART RXD 핀
     * @details     Upload Log UART RXD 핀 설정
     */
    #define UPLOAD_LOG_RXD_GPIO_NUM             GPIO_NUM_44

    /*===========================================================================*/
    /* LED Strip 설정                                                            */
    /*===========================================================================*/
    #if LED_STRIP_ENABLE
        /**
        * @defgroup    LED_STRIP_CONFIG LED Strip 제어 설정 값 그룹
        * @brief       LED Strip 제어 설정
        * @details     
        * @todo        항목 수정
        * @{
        */
            /**
            * @brief       LED Strip 제어용 GPIO 핀
            * @details     LED Strip 제어를 위한 신호 출력 핀 설정
            */
            #define LED_STRIP_GPIO_NUM              GPIO_NUM_38
            /**
            * @brief       LED Strip의 LED 개수
            * @details     1개의 LED 사용
            */
            #define LED_STRIP_LEN                   1
            /**
            * @brief       LED Strip RMT 해상도 (Hz)
            * @details     10 MHz - WS2812 타이밍 요구사항 충족
            */
            #define LED_STRIP_RESOLUTION_HZ         10 * 1000 * 1000
            /**
            * @brief       LED Strip DMA 사용 여부
            * @details     false - DMA 미사용
            */
            #define LED_STRIP_WITH_DMA              false
        /** @} */ // end of LED_STRIP_CONFIG
    #endif
/** @} */ // end of GPIO_SETTING

/*===========================================================================*/
/* 함수 선언                                                                  */
/*===========================================================================*/
/**
 * @defgroup    GPIO_FUNCTIONS GPIO 제어 함수 그룹
 * @brief       GPIO 및 PWM 제어 관련 함수들
 * @todo        항목 수정
 * @{
 */
    /**
    * @function    custom_gpio_init
    * @brief       GPIO 및 PWM 초기화
    * @param[in]   void
    * @return      bool    true: 초기화 성공, false: 초기화 실패
    * @details     다음 항목들을 초기화합니다:
    *              - GPIO 핀 모드 설정 (입력/출력)
    *              - SPI 입력 noise 방지 설정
    *              - UART 입력 noise 방지 설정
    *              - LED Strip 초기화 (활성화된 경우)
    * 
    * @note        시스템 부팅 시 한 번만 호출되어야 합니다
    */
    bool custom_gpio_init(void);

    #if LED_STRIP_ENABLE
        /**
        * @function    custom_gpio_set_led_strip_color
        * @brief       LED Strip의 색상 설정
        * @param[in]   input_ui32_red_value    Red 색상 값 (0 ~ 255)
        * @param[in]   input_ui32_green_value  Green 색상 값 (0 ~ 255)
        * @param[in]   input_ui32_blue_value   Blue 색상 값 (0 ~ 255)
        * @return      bool    true: 색상 변경 성공, false: 색상 변경 실패
        * @details     WS2812 등의 LED Strip의 RGB 색상을 설정합니다.
        *              각 색상은 0(꺼짐)부터 255(최대 밝기)까지 설정 가능합니다.
        * @note        LED_STRIP_ENABLE이 true일 때만 사용 가능
        * @code
        * // 빨간색 설정
        * custom_gpio_set_led_strip_color(255, 0, 0);
        * // 녹색 설정
        * custom_gpio_set_led_strip_color(0, 255, 0);
        * // 흰색 설정
        * custom_gpio_set_led_strip_color(255, 255, 255);
        * @endcode
        */
        bool custom_gpio_set_led_strip_color(const uint32_t input_ui32_red_value, const uint32_t input_ui32_green_value, const uint32_t input_ui32_blue_value);

        /**
        * @function    custom_gpio_clear_led_strip
        * @brief       LED Strip 끄기 (모든 LED를 검은색으로 설정)
        * @param[in]   void
        * @return      void
        * @details     LED Strip의 모든 LED를 끕니다 (RGB 모두 0으로 설정).
        * @note        LED_STRIP_ENABLE이 true일 때만 사용 가능
        */
        void custom_gpio_clear_led_strip(void);
    #endif

    /**
    * @function    custom_gpio_deinit
    * @brief       GPIO 및 PWM 해제
    * @param[in]   void
    * @return      bool    true: 해제 성공, false: 해제 실패
    * @details     초기화된 GPIO 및 PWM 리소스를 해제합니다.
    *              - PWM 타이머 정지
    *              - GPIO 핀 리셋
    *              - LED Strip 리소스 해제 (활성화된 경우)
    */
    bool custom_gpio_deinit(void);
/** @} */ // end of GPIO_FUNCTIONS
#endif
