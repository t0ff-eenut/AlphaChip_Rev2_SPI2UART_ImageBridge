// CONFIG_FREERTOS_HZ  // Tick rate (Hz) = 1000 // 1ms 단위(1000Hz) 틱

// Enum
/**
 * @enum        queue_input_output_toggle(qiot)
 * @brief       Queue Input/Output Toggle enum
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param QUEUE_INPUT     0
 * @param QUEUE_OUTPUT    1
 * 
 * @see         
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */
// Struct
/**
 * @struct      send_uart_queue_struct(suqs)
 * @brief       Send Uart Queue Struct
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param uint16_t      ui16_adc_data
 * @param uint16_t      ui16_voltage_data
 * @param uint16_t      ui16_tp1
 * @param uint8_t       ui8_tp2
 * @param uint8_t       ui8_switch_status
 * @param uint16_t*     A_ui16_adc_buf
 * @param uint16_t*     A_ui16_adc_delta_buf
 * @param bool*         A_b_occu_buf
 * @param bool          b_occu_triger
 * 
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */
// 함수
/**
 * @brief       Custom Queue Initial Function
 * @attention   *주의사항 [const : 읽기 전용]
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @warning     *경고
 * @note        *참고사항
 * 
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */

#ifndef PROJECT_TOP_H
#define PROJECT_TOP_H

#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "esp_timer.h"
#include "esp_pm.h"

#define TEXT_BOLD       "\033[1m"
#define TEXT_UNDER_LINE "\033[4m"
#define TEXT_REVERSE    "\033[7m"

#define COLOR_BLACK     "\033[30m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_MAGENTA   "\033[35m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_WHITE     "\033[37m"
#define COLOR_RESET     "\033[0m"

#ifndef MIN
    #define MIN(a,b)    (( (a) < (b) ) ? (a) : (b))
#endif

#define ns_sleep    * ((1 us_sleep) / 1000)
#define us_sleep    * 1
#define ms_sleep    * (1000 us_sleep)
#define s_sleep     * (1000 ms_sleep)
#define m_sleep     * (60 s_sleep)
#define h_sleep     * (60 m_sleep)

/**
 * @enum        switch_booting_level_enum(sble)
 * @brief       Switch Booting Level Enum
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param SWITCH_INITIAL                        0   초기 설정 단계 진행
 * @param SWITCH_BOOTING_CHECK_CAUSE            1   부팅 원인 확인 단계 진행
 * @param SWITCH_BOOTING_iSENSOR_MODE_START     2   iSENSOR 모드 시작 단계 진행
 * @param SWITCH_BOOTING_END                    3   기기 종료 단계 진행
 * 
 * @see         
 * @details     ESP32 부팅 단계 순서 및 번호 정의
 * @todo        todo
 * @bug         bug
 */
typedef enum switch_booting_level_enum{
    SWITCH_INITIAL,                         // 초기 설정 단계 진행
    SWITCH_BOOTING_CHECK_CAUSE,             // 부팅 이유에 따라 동작 분기
    SWITCH_BOOTING_APPLICATION_START,      // Application 동작 시작
    SWITCH_BOOTING_END,                     // 기기 종료
}sble;

extern esp_sleep_wakeup_cause_t g_esp_sleep_wakeup_cause;
extern esp_err_t g_esp_err;

const char* custom_getRuntimeString(void);
void custom_wakeup_cause_print(esp_sleep_wakeup_cause_t input_wakeup_cause);
uint8_t custom_ui8_abs(uint8_t ui8_a, uint8_t ui8_b);
uint16_t custom_ui16_abs(uint16_t ui16_a, uint16_t ui16_b);
uint64_t custom_ui64_abs(uint64_t ui64_a, uint64_t ui64_b);
float custom_f_abs(float f_a, float f_b);
int custom_tick_to_delay(int input_i_tick);
int custom_ms_to_delay(int input_i_ms);





// #if CONFIG_IDF_TARGET_ESP32C3
//     #define ESP32C3_MINI            0
//     #define ESP32C3_SUPER_MINI      1
//     #define ESP32C3                 ESP32C3_SUPER_MINI
// #endif


#define DEBUG       true
#define PRINT_DELAY false
// #define UART_USE    true
#define UART_USE    false
#define DEBUG_DELAY_TIME_MS         100



#define NOTIFY_SHUTDOWN_BIT         (1 << 0) // 0번 비트를 종료 신호로 사용
#define NOTIFY_BUFFER_RESET_BIT     (1 << 8) // 8번 비트를 Buffer Reset 신호로 사용
// #define NOTIFY_ADC_BREAK_BIT        (1 << 16) // 16번 비트를 ADC Break 신호로 사용

// 메크로
// custom_esp_gpio.h
    // #if (CONFIG_IDF_TARGET_ESP32C3 && (ESP32C3 == ESP32C3_SUPER_MINI))
    //     #define LED_STRIP_ENABLE    false   // 
    // #else
    //     #define LED_STRIP_ENABLE    true   // 
    // #endif
    #define LED_STRIP_ENABLE    true   // 

// custom_esp_nvs.h
    // #define WATTING_NVS_RESET_TIMEOUT_US_INIT           2 * 1000 * 1000     // 2초간 재실 확인
    // #define WATTING_NVS_RESET_TIMEOUT_US_INIT           1 * 1000 * 1000     // 1초간 버튼 입력 확인
    // #define WATTING_NVS_RESET_TIMEOUT_US_INIT           1 s_sleep     // 1초간 버튼 입력 확인

// custom_esp_queue.h
    #define MUTEX_TIMEOUT_MS        10

// // custom_esp_adc_thread.c
//     #define ADC_RAW_ENABLE              true
//     #if (CONFIG_IDF_TARGET_ESP32C3 && (ESP32C3 == ESP32C3_SUPER_MINI))
//         #define ADC_HPF_ENABLE  false
//         #define ADC_BPF_ENABLE  false
//     #else
//         // #define ADC_HPF_ENABLE  true
//         // #define ADC_BPF_ENABLE  true
//         #define ADC_HPF_ENABLE  false
//         #define ADC_BPF_ENABLE  false
//     #endif
//     // #define ADC_SW_VOLTAGE_ENABLE       true
//     #define ADC_SW_VOLTAGE_ENABLE       false

//     // #define ADC_SW_HPF_ENABLE           true
//     // #define ADC_SW_BPF_ENABLE           true
//     #define ADC_SW_HPF_ENABLE           false
//     #define ADC_SW_BPF_ENABLE           false
//     #if (ADC_SW_HPF_ENABLE || ADC_SW_BPF_ENABLE)
//         #define SAMPLING_FREQ           100.0f  // 샘플링 주파수 (Hz) = 1000ms / ADC_SPEED_MS(10ms) = 100Hz
//         #define BUTTERWORTH_Q_FACTOR    0.707f  // Butterworth 특성을 위한 Q 값
//     #endif
//     #if ADC_SW_HPF_ENABLE
//         #define LOW_CUTOFF_FREQ         0.3f    // High-pass 필터의 차단 주파수 (Hz) - DC 성분 제거
//     #endif
//     #if ADC_SW_BPF_ENABLE
//         #define HIGH_CUTOFF_FREQ        4.0f    // Low-pass 필터의 차단 주파수 (Hz) - 고주파 노이즈 제거
//     #endif




//     #define ADC_READ_THREAD_STACK_SIZE                  (1024 * 4)
//     #define ADC_THREAD_STACK_SIZE                       (1024 * 4)
//     #define ADC_READ_BUFFER_SIZE                        WINDOW_SIZE * 2                 // ADC 버퍼 크기
//     // #define ADC_BUFFER_SIZE                             WINDOW_SIZE                 // ADC 버퍼 크기
//     #define ADC_SPEED_MS                                10                  // ADC 속도
//     // #define ADC_SPEED_MS                                500                  // ADC 속도

// custom_esp_uart_thread.h
    #define UART_BUFFER_SIZE                            256             // UART 버퍼 크기 (ESP-IDF 최소 요구: > 128)

// // iSENSOR_memory.h
//     #define WINDOW_SIZE                                 300
//     // #define WATTING_OCCUPANCY_TIMEOUT_US_INIT           5 s_sleep       // 5초간 재실 확인
//     #define WATTING_OCCUPANCY_TIMEOUT_US_INIT           3 s_sleep       // 3초간 재실 확인
//     #define TP1_OCCUPANCY_INIT                          70             // TP1_INIT < 4095
//     #define TP1_RECHECK_INIT                            500            // TP1_INIT < 4095
//     #define TP2_INIT                                    20              // TP2_INIT < WINDOW_SIZE

//     #define LED_DIMMING_WORK_TIME_MS_INIT               500             // 0.5초간 Dimming
//     #define LED_DIMMING_STEP_TIME_MS_INIT               5               // Dimming 해상도 = (LED_DIMMING_WORK_TIME_MS_INIT / LED_DIMMING_STEP_TIME_MS_INIT) 단계
//     #define LED_DIMMING_DELAY_TIME_MS_INIT              500             // Dimming 유지 시간

//     #define LED_MAX_PERCENTAGE_INIT                     100             // %
//     #define LED_DIMMING_PERCENTAGE_INIT                 20              // %
//     #define LED_MIN_PERCENTAGE_INIT                     0               // %

//     #define OCCUPANCY_ON_COUNT_LIMIT_INIT               5
//     #define TP_ERROR_COUNT_LIMIT_INIT                   2

//     #define SLEEP_TIME_INIT                             10 s_sleep      // 10초간 DeepSleep

// // iSENSOR_Mode.c
//     #define ISENSOR_BACKGROUND_THREAD_STACK_SIZE        (1024 * 6)
//     #define ISENSOR_MODE_THREAD_STACK_SIZE              (1024 * 4)
//     #define BLINK_OCCUPANCY_RED_LED_TIME_US             100 ms_sleep




// main.c
    // #define DEFAULT_SLEEP_TIME                          5 s_sleep
    // #define SYNCHRONIZE_SLEEP_TIME                      10 s_sleep
    // Sensor
        // #define NONE_SLEEP_TIME                         -1
    // // Switch
    //     #define CYCLE_SLEEP_TIME                        1 s_sleep


////////////////////////////////
    // main.c
    // #define DEFAULT_SLEEP_TIME                          1 s_sleep 






// // band.c
//     #define SENSOR_MAC                                  {0x02, 0x20, 0x30, 0x40, 0x50, 0x01}
//     #define SWITCH_CONTROLLER_MAC                       {0x02, 0x20, 0x30, 0x40, 0x50, 0x02}

//     #define TRY_SEND_TIMEOUT_US                         2 * 1000 * 1000     // 데이터 전송 3초동안 시도
//     #define WATTING_SEND_CALLBACK_TIMEOUT_US            1 * 1000 * 1000     // SEND CALLBACK을 위한 1초 대기
//     #if MODE == SENSOR
//         #define WATTING_RECEIVE_CALLBACK_TIMEOUT_US     6 * 1000 * 1000     // RECEIVE CALLBACK을 위한 3초 대기
//     #endif
//     // #if MODE == SWITCH
//     //     #define WATTING_RECEIVE_CALLBACK_TIMEOUT_US     4 * 1000 * 1000     // RECEIVE CALLBACK을 위한 1초 대기
//     //     // #define WATTING_RECEIVE_CALLBACK_TIMEOUT_US     300 * 1000     // RECEIVE CALLBACK을 위한 0.3초 대기
//     // #endif
#endif // MAIN_TOP_H