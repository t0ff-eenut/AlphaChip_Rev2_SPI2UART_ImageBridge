#ifndef CUSTOM_ESP_GPIO_H
#define CUSTOM_ESP_GPIO_H

// #include "../../project_top.h"
#include "hw_level_top.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "led_strip.h"
#include "driver/rmt.h"

// #ifndef ADC_RAW_ENABLE
//     #define ADC_RAW_ENABLE          true   // 
// #endif
// #ifndef ADC_HPF_ENABLE
//     #define ADC_HPF_ENABLE      false   // 
// #endif
// #ifndef ADC_BPF_ENABLE
//     #define ADC_BPF_ENABLE      false   // 
// #endif

// #if CONFIG_IDF_TARGET_ESP32C3
//     #if ESP32C3 == ESP32C3_MINI
//         // GPIO 1은 CLK 혼선남

//         #define PIR_SIGNAL_OUTPUT_GPIO_NUM          GPIO_NUM_0          // 부팅 트리거    RTC핀
//         #if ADC_RAW_ENABLE
//             #define ADC_RAW_UNIT                        ADC_UNIT_1
//             #define ADC_RAW_CHANNEL                     ADC_CHANNEL_2       // GPIO2
//         #endif
//         #if ADC_HPF_ENABLE
//             #define ADC_HPF_UNIT                    ADC_UNIT_1
//             #define ADC_HPF_CHANNEL                 ADC_CHANNEL_3       // GPIO3
//         #endif
//         #if ADC_BPF_ENABLE
//             #define ADC_BPF_UNIT                    ADC_UNIT_1
//             #define ADC_BPF_CHANNEL                 ADC_CHANNEL_4       // GPIO4
//         #endif

//         #define LED_PWM_GPIO_NUM                    GPIO_NUM_5          // PWM을 출력할 GPIO 핀 번호 (원하는 핀으로 변경)

//         #ifndef LED_STRIP_ENABLE
//             #define LED_STRIP_ENABLE                false   // 
//         #endif
//         #if LED_STRIP_ENABLE
//             #define LED_STRIP_GPIO_NUM              GPIO_NUM_8
//         #else
//             #define BLUE_LED_PWM_GPIO_NUM           GPIO_NUM_8
//         #endif

//         #define DEBUG_UART_PORT                     UART_NUM_1
//         #define DEBUG_TXD_GPIO_NUM                  GPIO_NUM_7
//         #define DEBUG_RXD_GPIO_NUM                  GPIO_NUM_6

//         #define NVS_RESET_BUTTON_GPIO_NUM           GPIO_NUM_9          // BOOT 버튼
//         #define INTER_DEBUG_MODE_GPIO_NUM           GPIO_NUM_10         // DEBUG MODE

//         // UART0 기본 핀 (부팅 시 노이즈 방지를 위해 풀업 초기화 필요)
//         #define UPLOAD_LOG_TXD_GPIO_NUM             GPIO_NUM_21
//         #define UPLOAD_LOG_RXD_GPIO_NUM             GPIO_NUM_20

//         // 설정 가능 범위: 1 bit ~ 14 bit (일반적인 사용 범위)
//         // 해상도와 주파수의 관계: 해상도를 높이면 최대 주파수가 낮아집니다.
//         // 공식: $Frequency = \frac{Clock_Source_Freq}{2^{Resolution}}$
//         // ESP32-C3의 APB 클럭(80MHz)을 기준으로 할 때:
//         // 14비트 (16384 단계): 최대 약 4.88 kHz
//         // 13비트 (8192 단계): 최대 약 9.76 kHz
//         // 12비트 (4096 단계): 최대 약 19.53 kHz
//         // 10비트 (1024 단계): 최대 약 78.12 kHz
//         // 8비트 (256 단계): 최대 약 312.5 kHz

//     #elif ESP32C3 == ESP32C3_SUPER_MINI

//         #define PIR_SIGNAL_OUTPUT_GPIO_NUM          GPIO_NUM_0          // 부팅 트리거    RTC핀
//         #if ADC_RAW_ENABLE
//             #define ADC_RAW_UNIT                        ADC_UNIT_1
//             #define ADC_RAW_CHANNEL                     ADC_CHANNEL_3       // GPIO3
//         #endif
//         #if ADC_HPF_ENABLE
//             #define ADC_HPF_UNIT                    ADC_UNIT_1
//             #define ADC_HPF_CHANNEL                 ADC_CHANNEL_3       // GPIO3
//         #endif
//         #if ADC_BPF_ENABLE
//             #define ADC_BPF_UNIT                    ADC_UNIT_1
//             #define ADC_BPF_CHANNEL                 ADC_CHANNEL_4       // GPIO4
//         #endif

//         #define LED_PWM_GPIO_NUM                    GPIO_NUM_5          // PWM을 출력할 GPIO 핀 번호 (원하는 핀으로 변경)

//         #ifndef LED_STRIP_ENABLE
//             #define LED_STRIP_ENABLE                false   // 
//         #endif
//         #if LED_STRIP_ENABLE
//             #define LED_STRIP_GPIO_NUM              GPIO_NUM_8
//         #else
//             #define BLUE_LED_PWM_GPIO_NUM           GPIO_NUM_8
//         #endif

//         #define DEBUG_UART_PORT                     UART_NUM_1
//         #define DEBUG_TXD_GPIO_NUM                  GPIO_NUM_7
//         #define DEBUG_RXD_GPIO_NUM                  GPIO_NUM_6

//         #define NVS_RESET_BUTTON_GPIO_NUM           GPIO_NUM_9          // BOOT 버튼
//         #define INTER_DEBUG_MODE_GPIO_NUM           GPIO_NUM_10         // DEBUG MODE

//         // UART0 기본 핀 (부팅 시 노이즈 방지를 위해 풀업 초기화 필요)
//         #define UPLOAD_LOG_UART_PORT                UART_NUM_0
//         #define UPLOAD_LOG_TXD_GPIO_NUM             GPIO_NUM_21
//         #define UPLOAD_LOG_RXD_GPIO_NUM             GPIO_NUM_20

//     #endif

// #else


// #endif

// #define PIR_SIGNAL_OUTPUT_GPIO_NUM          GPIO_NUM_0          // 부팅 트리거    RTC핀
// #if ADC_RAW_ENABLE
//     #define ADC_RAW_UNIT                        ADC_UNIT_1
//     #define ADC_RAW_CHANNEL                     ADC_CHANNEL_2       // GPIO2
// #endif
// #if ADC_HPF_ENABLE
//     #define ADC_HPF_UNIT                    ADC_UNIT_1
//     #define ADC_HPF_CHANNEL                 ADC_CHANNEL_3       // GPIO3
// #endif
// #if ADC_BPF_ENABLE
//     #define ADC_BPF_UNIT                    ADC_UNIT_1
//     #define ADC_BPF_CHANNEL                 ADC_CHANNEL_4       // GPIO4
// #endif

// #define LED_PWM_GPIO_NUM                    GPIO_NUM_5          // PWM을 출력할 GPIO 핀 번호 (원하는 핀으로 변경)

#ifndef LED_STRIP_ENABLE
    #define LED_STRIP_ENABLE                true   // 
#endif
#if LED_STRIP_ENABLE
    #define LED_STRIP_GPIO_NUM              GPIO_NUM_38
#else
    #define BLUE_LED_PWM_GPIO_NUM           GPIO_NUM_38
#endif

#define DEBUG_UART_PORT                     UART_NUM_1
#define DEBUG_TXD_GPIO_NUM                  GPIO_NUM_7
#define DEBUG_RXD_GPIO_NUM                  GPIO_NUM_6

#define NVS_RESET_BUTTON_GPIO_NUM           GPIO_NUM_9          // BOOT 버튼
#define INTER_DEBUG_MODE_GPIO_NUM           GPIO_NUM_10         // DEBUG MODE

// UART0 기본 핀 (부팅 시 노이즈 방지를 위해 풀업 초기화 필요)
#define UPLOAD_LOG_TXD_GPIO_NUM             GPIO_NUM_43
#define UPLOAD_LOG_RXD_GPIO_NUM             GPIO_NUM_44

// 설정 가능 범위: 1 bit ~ 14 bit (일반적인 사용 범위)
// 해상도와 주파수의 관계: 해상도를 높이면 최대 주파수가 낮아집니다.
// 공식: $Frequency = \frac{Clock_Source_Freq}{2^{Resolution}}$
// ESP32-C3의 APB 클럭(80MHz)을 기준으로 할 때:
// 14비트 (16384 단계): 최대 약 4.88 kHz
// 13비트 (8192 단계): 최대 약 9.76 kHz
// 12비트 (4096 단계): 최대 약 19.53 kHz
// 10비트 (1024 단계): 최대 약 78.12 kHz
// 8비트 (256 단계): 최대 약 312.5 kHz

#define LED_PWM_TIMER                   LEDC_TIMER_0
#define LED_PWM_MODE                    LEDC_LOW_SPEED_MODE // ESP32-C3는 Low Speed Mode만 지원합니다.
#define LED_PWM_CHANNEL                 LEDC_CHANNEL_0      // PWM 채널 (원하는 채널로 변경)(0~5)
#define LED_PWM_DUTY_RES                LEDC_TIMER_13_BIT   // 해상도: 13비트 (0 ~ 8191)    해상도(Bit)와 주파수(Frequency)는 반비례
#define LED_PWM_FREQUENCY               5 * 1000            // 주파수: 5 kHz

#if LED_STRIP_ENABLE
    #define LED_STRIP_LEN               1
    #define LED_STRIP_RESOLUTION_HZ     10 * 1000 * 1000
    #define LED_STRIP_WITH_DMA          false
#else
    #define BLUE_LED_PWM_TIMER          LEDC_TIMER_0
    #define BLUE_LED_PWM_MODE           LEDC_LOW_SPEED_MODE // ESP32-C3는 Low Speed Mode만 지원합니다.
    #define BLUE_LED_PWM_CHANNEL        LEDC_CHANNEL_1      // PWM 채널 (원하는 채널로 변경)(0~5)
    #define BLUE_LED_PWM_DUTY_RES       LEDC_TIMER_13_BIT   // 해상도: 13비트 (0 ~ 8191)    해상도(Bit)와 주파수(Frequency)는 반비례
    #define BLUE_LED_PWM_FREQUENCY      5 * 1000            // 주파수: 5 kHz
#endif

/**
 * @struct      esp_pwm_channel_config_struct(epccs)
 * @brief       ESP PWM Channel Config Struct
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param gpio_num_t      gpio_num
 * @param ledc_mode_t     ledc_mode_num
 * @param ledc_timer_t    ledc_timer_num
 * @param ledc_channel_t  ledc_channel_num
 * @param ledc_timer_bit_t    ledc_timer_bit
 * @param uint32_t        ui32_frequency
 * @param ledc_clk_cfg_t  ledc_clk_cfg_num
 * @param ledc_intr_type_t    ledc_intr_type_num
 * 
 * @details     ESP32 PWM 채널 설정 구조체
 * @todo        todo
 * @bug         bug
 */
typedef struct esp_pwm_channel_config_struct{
    gpio_num_t          gpio_num;           // GPIO 핀 번호
    ledc_mode_t         ledc_mode_num;      // 속도 모드
    ledc_timer_t        ledc_timer_num;     // LEDC 타이머
    ledc_channel_t      ledc_channel_num;   // LEDC 채널
    ledc_timer_bit_t    ledc_timer_bit;     // 듀티 해상도
    uint32_t            ui32_frequency;     // 주파수 (Hz)
    ledc_clk_cfg_t      ledc_clk_cfg_num;   // 클럭 설정
    ledc_intr_type_t    ledc_intr_type_num; // 인터럽트 설정
} epccs;

/**
 * @brief PWM 채널 식별자 enum
 * @details LED_PWM (GPIO5)과 LED_BLUE (GPIO8) 두 채널을 하나의 API로 제어하기 위함
 */
 /**
 * @enum        esp_pwm_channel_id_enum(epcie)
 * @brief       ESP PWM Channel ID Enum
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param PWM_CH_LED_PWM            0   LED PWM 채널
 * @param PWM_CH_BLUE_LED           1   Blue LED 채널
 * @param PWM_CH_MAX                2   PWM 채널 개수
 * 
 * @see         
 * @details     PWM 채널 식별자 enum
 * @todo        todo
 * @bug         bug
 */
typedef enum esp_pwm_channel_id_enum{
    PWM_CH_LED_PWM,         ///< GPIO5 - 외부 LED PWM 제어용
#if !LED_STRIP_ENABLE
    PWM_CH_BLUE_LED,        ///< GPIO8 - Blue LED (LED_STRIP 비활성화시)
#endif
    PWM_CH_MAX              ///< PWM 채널 개수
} epcie;

ledc_channel_t get_ledc_channel_num(epcie input_epcie);

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

/**
 * @brief       지정된 PWM 채널의 최대 Duty 값 반환
 * @param[in]   input_epcie  PWM 채널 ID
 * @return      uint32_t    최대 Duty 값 (13비트 해상도시 8191)
 */
uint32_t custom_pwm_get_max_duty(epcie input_epcie);

/**
 * @brief       지정된 PWM 채널의 주파수 반환
 * @param[in]   input_epcie  PWM 채널 ID (PWM_CH_LED_PWM, PWM_CH_BLUE_LED)
 * @return      uint32_t    현재 주파수 (Hz)
 */
uint32_t custom_pwm_get_frequency(epcie input_epcie);

/**
 * @brief PWM 듀티 사이클을 변경합니다.
 * 
 * @param input_epcie PWM 채널 ID
 * @param intput_ui32_duty 듀티 값 (0 ~ max_duty). 
 *             13비트 해상도이므로 8191이 100%입니다.
 *             예: 50% 듀티 = 4095
 */
/**
 * @brief       지정된 PWM 채널의 Duty 값 설정
 * @param[in]   input_epcie  PWM 채널 ID
 * @param[in]   intput_ui32_duty        Duty 값 (0 ~ max_duty)
 */
void custom_pwm_set_duty(epcie input_epcie, uint32_t intput_ui32_duty);

/**
 * @brief       지정된 PWM 채널의 현재 Duty 값 반환
 * @param[in]   input_epcie  PWM 채널 ID
 * @return      uint32_t    현재 Duty 값
 */
uint32_t custom_pwm_get_duty(epcie input_epcie);

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

bool custom_gpio_pir_output_on_check(void);

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
