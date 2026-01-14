/**
 * @file        project_top.h
 * @brief       iSENSOR 프로젝트의 전역 설정 헤더 파일
 * @author      T0T
 * @date        2025-08-21
 * @version     1.0.0
 * 
 * @details     이 파일은 iSENSOR 프로젝트의 매크로와 전역 설정을 정의함.
 *              - ANSI 색상 및 텍스트 스타일 매크로
 *              - Sleep 시간 단위 변환 매크로
 *              - 부팅 단계 열거형
 *              - 디버그 설정
 *              - ADC 설정
 *              - iSENSOR 관련 설정
 *              - 전역 변수 및 유틸리티 함수 선언
 */

#ifndef PROJECT_TOP_H
#define PROJECT_TOP_H

#include "esp_log.h"
#include "esp_sleep.h"

#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>
#include "esp_timer.h"
#include "esp_pm.h"

/*===========================================================================*/
/* 메크로 정의 */
/*===========================================================================*/
/**
 * @defgroup    ANSI_COLORS ANSI 텍스트 스타일 메크로 그룹
 * @brief       텍스트 스타일 설정
 * @details     텍스트 스타일을 설정하는 ANSI 이스케이프 시퀀스
                - 굵게
                - 밑줄
                - 반전
 * @code
 * printf(TEXT_BOLD COLOR_WHITE "내용 입력 \n" COLOR_RESET);
 * @endcode
 * @{
 */
    /**
    * @brief       Bold체 (굵은 글씨)
    * @details     콘솔 텍스트를 Bold체로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define TEXT_BOLD       "\033[1m"

    /**
    * @brief       밑줄 (Underline)
    * @details     콘솔 텍스트에 밑줄을 추가하는 ANSI 이스케이프 시퀀스
    */
    #define TEXT_UNDER_LINE "\033[4m"

    /**
    * @brief       반전 (Reverse Video)
    * @details     콘솔 텍스트의 전경색과 배경색을 반전시키는 ANSI 이스케이프 시퀀스
    * @todo        실제 사용 사례 확인 및 문서화
    */
    #define TEXT_REVERSE    "\033[7m"
/** @} */ // end of ANSI_STYLES

/**
 * @defgroup    ANSI_COLORS ANSI 텍스트 색상 메크로 그룹
 * @brief       텍스트 색상 설정
 * @details     텍스트 색상을 설정하는 ANSI 이스케이프 시퀀스
                - 검은색
                - 빨간색
                - 초록색
                - 노란색
                - 파란색
                - 보라색
                - 청록색
                - 흰색
 * @code
 * printf(TEXT_BOLD COLOR_WHITE "내용 입력 \n" COLOR_RESET);
 * @endcode
 * @{
 */
    /**
    * @brief       검은색 텍스트
    * @details     콘솔 텍스트를 검은색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_BLACK     "\033[30m"

    /**
    * @brief       빨간색 텍스트
    * @details     콘솔 텍스트를 빨간색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_RED       "\033[31m"

    /**
    * @brief       초록색 텍스트
    * @details     콘솔 텍스트를 초록색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_GREEN     "\033[32m"

    /**
    * @brief       노란색 텍스트
    * @details     콘솔 텍스트를 노란색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_YELLOW    "\033[33m"

    /**
    * @brief       파란색 텍스트
    * @details     콘솔 텍스트를 파란색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_BLUE      "\033[34m"

    /**
    * @brief       보라색 (마젠타) 텍스트
    * @details     콘솔 텍스트를 보라색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_MAGENTA   "\033[35m"

    /**
    * @brief       청록색 (시안) 텍스트
    * @details     콘솔 텍스트를 청록색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_CYAN      "\033[36m"

    /**
    * @brief       흰색 텍스트
    * @details     콘솔 텍스트를 흰색으로 설정하는 ANSI 이스케이프 시퀀스
    */
    #define COLOR_WHITE     "\033[37m"

    /**
    * @brief       텍스트 스타일 및 색상 초기화
    * @details     모든 ANSI 텍스트 속성을 기본값으로 리셋하는 이스케이프 시퀀스
    */
    #define COLOR_RESET     "\033[0m"
/** @} */ // end of ANSI_COLORS

// #ifndef MIN
//     #define MIN(a,b)    (( (a) < (b) ) ? (a) : (b))
// #endif

/**
 * @defgroup    SLEEP_MACROS Sleep 시간 단위 매크로 그룹
 * @brief       Sleep 시간을 다양한 단위로 표현하기 위한 매크로
 * @details     이 매크로들은 숫자 앞에 곱하기 연산자를 사용하여 시간 단위를 변환합니다.
 * @note
 * @code
 * uint64_t delay_time;
 * delay_time = 100 us_sleep;    // 100 마이크로초
 * delay_time = 50 ms_sleep;     // 50 밀리초 = 50,000 마이크로초
 * delay_time = 2 s_sleep;       // 2초 = 2,000,000 마이크로초
 * delay_time = 5 m_sleep;       // 5분 = 300,000,000 마이크로초
 * delay_time = 1 h_sleep;       // 1시간 = 3,600,000,000 마이크로초
 * @endcode
 * @{
 */
    /**
    * @brief       나노초(ns) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 나노초(ns) 단위로 조절하기 위한 매크로
    *              1 ns = 0.001 us
    */
    #define ns_sleep    * ((1 us_sleep) / 1000)

    /**
    * @brief       마이크로초(us) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 마이크로초(us) 단위로 조절하기 위한 매크로
    *              기본 단위로 사용됨
    */
    #define us_sleep    * 1

    /**
    * @brief       밀리초(ms) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 밀리초(ms) 단위로 조절하기 위한 매크로
    *              1 ms = 1,000 us
    */
    #define ms_sleep    * (1000 us_sleep)

    /**
    * @brief       초(s) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 초(s) 단위로 조절하기 위한 매크로
    *              1 s = 1,000 ms = 1,000,000 us
    */
    #define s_sleep     * (1000 ms_sleep)

    /**
    * @brief       분(m) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 분(m) 단위로 조절하기 위한 매크로
    *              1 m = 60 s = 60,000,000 us
    */
    #define m_sleep     * (60 s_sleep)

    /**
    * @brief       시(h) 단위의 Sleep 시간 값
    * @details     Sleep 시간을 시(h) 단위로 조절하기 위한 매크로
    *              1 h = 60 m = 3,600,000,000 us
    */
    #define h_sleep     * (60 m_sleep)
/** @} */ // end of SLEEP_MACROS

/*===========================================================================*/
/* 열거형 정의 */
/*===========================================================================*/
/**
 * @enum        dble
 * @typedef     device_booting_level_enum
 * @brief       ESP32 부팅 단계 열거형
 * @details     ESP32 부팅 시 진행되는 각 단계를 정의한 열거형입니다.
 *              부팅 프로세스의 흐름을 제어하고 추적하는 데 사용됩니다.
 */
typedef enum device_booting_level_enum{
    BOOTING_LEVEL_INITIAL,              /* 0: 초기 설정 단계 - 하드웨어 및 시스템 초기화 */
    BOOTING_LEVEL_CHECK_CAUSE,          /* 1: 부팅 원인 확인 단계 - 웨이크업 원인 분석 및 분기 */
    BOOTING_LEVEL_APPLICATION_START,    /* 2: 애플리케이션 시작 단계 - 애플리케이션 동작 시작 */
    BOOTING_LEVEL_END,                  /* 3: 애플리케이션 종료 단계 - Deep Sleep 진입 준비 */
}dble;


/*===========================================================================*/
/* 변수 정의 */
/*===========================================================================*/
extern esp_sleep_wakeup_cause_t g_esp_sleep_wakeup_cause;
extern esp_err_t g_esp_err;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
/**
 * @defgroup    UTILITY_FUNCTIONS 유틸리티 함수 그룹
 * @brief       프로젝트 전체에서 사용되는 공통 유틸리티 함수들
 * @details     
 * @{
 */
    /**
    * @function    custom_getRuntimeString
    * @brief       시스템 부팅 이후 경과 시간을 문자열로 반환
    * @attention   반환되는 포인터는 static 버퍼를 가리키므로, 다음 호출 시 덮어쓰여질 수 있음
    * @param[in]   void
    * @return      const char* - "HH:MM:SS.UUUUUU" 형식의 런타임 문자열 (시:분:초.마이크로초)
    * @warning     반환된 문자열은 다음 함수 호출 시 변경될 수 있으므로, 장기 보관이 필요한 경우 복사 필요
    * @warning     스레드 안전하지 않음 (static 버퍼 사용)
    * @note        - esp_timer_get_time()을 사용하여 마이크로초 단위의 정밀도 제공
    *              - 최대 99시간 59분 59초까지 표시 가능 (HH 필드가 2자리)
    * @details     ESP32의 esp_timer_get_time() API를 사용하여 부팅 이후 경과된 시간을
    *              마이크로초 단위로 가져온 후, 이를 시:분:초.마이크로초 형식의 문자열로 변환합니다.
    *              내부적으로 32바이트 크기의 static 버퍼를 사용하여 문자열을 저장하며,
    *              snprintf를 통해 안전하게 포맷팅됩니다.
    *              
    *              출력 형식: "%02llu:%02llu:%02llu.%06llu"
    *              - 시간(HH): 2자리, 0으로 패딩
    *              - 분(MM): 2자리, 0으로 패딩
    *              - 초(SS): 2자리, 0으로 패딩
    *              - 마이크로초(UUUUUU): 6자리, 0으로 패딩
    * @code
    * const char* runtime = custom_getRuntimeString();
    * printf("Current runtime: %s\n", runtime);
    * // Output : [Current runtime: 01:23:45.678901]
    * @endcode
    * 
    * @todo        멀티스레드 환경에서 안전하게 사용할 수 있도록 스레드 로컬 스토리지 고려
    * @bug         없음
    */
    const char* custom_getRuntimeString(void);

    /**
    * @function    custom_wakeup_cause_print
    * @brief       ESP32 웨이크업 원인을 콘솔에 출력
    * @param[in]   input_wakeup_cause - 출력할 웨이크업 원인 (esp_sleep_wakeup_cause_t 타입)
    * @return      void
    * @note        디버깅 및 로깅 목적으로 사용
    *              각 웨이크업 원인에 대한 한글 설명과 함께 출력됨
    * @details     ESP32의 Deep Sleep 웨이크업 원인을 사람이 읽기 쉬운 형태로 출력합니다.
    *              입력된 웨이크업 원인 코드에 따라 적절한 설명 메시지를 콘솔에 출력합니다.
    *              지원하는 웨이크업 원인:
    *              - ESP_SLEEP_WAKEUP_UNDEFINED: 정의되지 않은 원인
    *              - ESP_SLEEP_WAKEUP_EXT0: 외부 인터럽트 0
    *              - ESP_SLEEP_WAKEUP_EXT1: 외부 인터럽트 1
    *              - ESP_SLEEP_WAKEUP_TIMER: 타이머
    *              - ESP_SLEEP_WAKEUP_TOUCHPAD: 터치패드
    *              - ESP_SLEEP_WAKEUP_ULP: ULP 코프로세서
    *              - ESP_SLEEP_WAKEUP_GPIO: GPIO
    *              - ESP_SLEEP_WAKEUP_UART: UART
    * @code
    * esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    * custom_wakeup_cause_print(cause);
    * @endcode
    */
    void custom_wakeup_cause_print(esp_sleep_wakeup_cause_t input_wakeup_cause);

    /**
    * @function    custom_ui8_abs
    * @brief       두 uint8_t 값의 절대 차이 계산
    * @param[in]   ui8_a - 첫 번째 값
    * @param[in]   ui8_b - 두 번째 값
    * @return      uint8_t - |ui8_a - ui8_b| 절대 차이값
    * @note        부호 없는 정수의 절대 차이를 안전하게 계산
    * @details     두 개의 uint8_t 값 중 큰 값에서 작은 값을 빼서 절대 차이를 반환합니다.
    *              부호 없는 정수 언더플로우를 방지하기 위해 조건문으로 처리합니다.
    * @code
    * uint8_t diff = custom_ui8_abs(100, 50);  // 결과: 50
    * uint8_t diff2 = custom_ui8_abs(30, 80);  // 결과: 50
    * @endcode
    */
    uint8_t custom_ui8_abs(uint8_t ui8_a, uint8_t ui8_b);

    /**
    * @function    custom_ui16_abs
    * @brief       두 uint16_t 값의 절대 차이 계산
    * @param[in]   ui16_a - 첫 번째 값
    * @param[in]   ui16_b - 두 번째 값
    * @return      uint16_t - |ui16_a - ui16_b| 절대 차이값
    * @note        부호 없는 정수의 절대 차이를 안전하게 계산
    * @details     두 개의 uint16_t 값 중 큰 값에서 작은 값을 빼서 절대 차이를 반환합니다.
    *              부호 없는 정수 언더플로우를 방지하기 위해 조건문으로 처리합니다.
    * @code
    * uint16_t diff = custom_ui16_abs(1000, 500);  // 결과: 500
    * uint16_t diff2 = custom_ui16_abs(300, 800);  // 결과: 500
    * @endcode
    */
    uint16_t custom_ui16_abs(uint16_t ui16_a, uint16_t ui16_b);

    /**
    * @function    custom_ui64_abs
    * @brief       두 uint64_t 값의 절대 차이 계산
    * @param[in]   ui64_a - 첫 번째 값
    * @param[in]   ui64_b - 두 번째 값
    * @return      uint64_t - |ui64_a - ui64_b| 절대 차이값
    * @note        부호 없는 정수의 절대 차이를 안전하게 계산
    *              타임스탬프 차이 계산 등에 유용
    * @details     두 개의 uint64_t 값 중 큰 값에서 작은 값을 빼서 절대 차이를 반환합니다.
    *              부호 없는 정수 언더플로우를 방지하기 위해 조건문으로 처리합니다.
    *              주로 esp_timer_get_time()의 마이크로초 타임스탬프 차이 계산에 사용됩니다.
    * @code
    * uint64_t time1 = esp_timer_get_time();
    * vTaskDelay(pdMS_TO_TICKS(100));
    * uint64_t time2 = esp_timer_get_time();
    * uint64_t elapsed = custom_ui64_abs(time2, time1);  // 약 100000us
    * @endcode
    */
    uint64_t custom_ui64_abs(uint64_t ui64_a, uint64_t ui64_b);

    /**
    * @function    custom_f_abs
    * @brief       두 float 값의 절대 차이 계산
    * @param[in]   f_a - 첫 번째 값
    * @param[in]   f_b - 두 번째 값
    * @return      float - |f_a - f_b| 절대 차이값
    * @note        부동소수점 값의 절대 차이를 계산
    *              센서 값 비교 등에 유용
    * @details     두 개의 float 값 중 큰 값에서 작은 값을 빼서 절대 차이를 반환합니다.
    *              부동소수점 연산의 특성상 매우 작은 오차가 발생할 수 있습니다.
    * @code
    * float diff = custom_f_abs(3.14f, 2.71f);  // 결과: 0.43
    * float diff2 = custom_f_abs(1.5f, 2.5f);   // 결과: 1.0
    * @endcode
    */
    float custom_f_abs(float f_a, float f_b);

    /**
    * @function    custom_tick_to_delay
    * @brief       FreeRTOS tick 값을 vTaskDelay 형식으로 변환
    * @param[in]   input_i_tick - 변환할 tick 값
    * @return      int - vTaskDelay에 사용 가능한 delay 값
    * @note        FreeRTOS의 tick 기반 지연 시간 계산에 사용
    * @details     입력된 tick 값을 FreeRTOS의 vTaskDelay() 함수에서 사용할 수 있는
    *              형식으로 변환합니다. configTICK_RATE_HZ 설정에 따라 실제 지연 시간이 결정됩니다.
    * @code
    * vTaskDelay(custom_tick_to_delay(100));
    * @endcode
    */
    int custom_tick_to_delay(int input_i_tick);

    /**
    * @function    custom_ms_to_delay
    * @brief       밀리초를 FreeRTOS delay 값으로 변환
    * @param[in]   input_i_ms - 변환할 밀리초 값
    * @return      int - vTaskDelay에 사용 가능한 delay 값
    * @note        밀리초 단위를 FreeRTOS tick 단위로 변환
    *              pdMS_TO_TICKS 매크로와 유사한 기능
    * @details     입력된 밀리초 값을 FreeRTOS의 vTaskDelay() 함수에서 사용할 수 있는
    *              tick 단위로 변환합니다. configTICK_RATE_HZ 설정값을 기반으로 계산됩니다.
    * @code
    * vTaskDelay(custom_ms_to_delay(1000));    // 1초
    * @endcode
    */
    int custom_ms_to_delay(int input_i_ms);
/** @} */ // end of UTILITY_FUNCTIONS


/*===========================================================================*/
/* 환경 변수 그룹                                                            */
/*===========================================================================*/
/**
 * @defgroup    ENVIRONMENT_VARIABLES 환경 변수 그룹
 * @brief       프로젝트 전체에서 사용되는 환경 변수들
 * @details     
 * @{
 */
    /*===========================================================================*/
    /* 디버그 메세지 관련 설정                                                   */
    /*===========================================================================*/
    /**
    * @defgroup    DEBUG_SETTINGS 디버그 메세지 관련 설정 메크로 그룹
    * @brief       프로젝트 전체에서 사용되는 디버그 메세지 출력 관련 설정값
    * @details     
    * @{
    */
        /**
        * @brief       디버그 메세지 출력 활성화
        * @details     true로 설정 시 디버그 메시지 출력 활성화
        */
        #define DEBUG       true

        /**
        * @brief       디버그 메세지 출력 시 딜레이 활성화
        * @details     true로 설정 시 디버그 메시지 출력 시 딜레이
        */
        #if DEBUG
            #define PRINT_DELAY false
        #else
            #define PRINT_DELAY false
        #endif

        /**
        * @brief       디버그 모드에서의 딜레이 시간 (밀리초)
        * @details     디버그 메시지 출력 간격 조절용 딜레이 시간
        */
        #if (DEBUG && PRINT_DELAY)
            #define DEBUG_DELAY_TIME_MS         100
        #endif
    /** @} */ // end of DEBUG_SETTINGS

    /*===========================================================================*/
    /* Thread 이벤트 비트 정의                                                      */
    /*===========================================================================*/
    /**
    * @defgroup    Thread 이벤트 비트 정의 메크로 그룹
    * @brief       프로젝트 전체에서 사용되는 Thread 이벤트 비트 정의
    * @details     
    * @{
    */
        /**
        * @brief       Thread 종료 알림 비트
        * @details     이벤트 그룹에서 0번 비트를 종료 신호로 사용
        *              이 비트가 설정되면 Thread 종료 프로세스 시작
        */
        #define NOTIFY_SHUTDOWN_BIT         (1 << 0)

        /**
        * @brief       버퍼 리셋 알림 비트
        * @details     이벤트 그룹에서 8번 비트를 버퍼 리셋 신호로 사용
        *              이 비트가 설정되면 버퍼 초기화 수행
        */
        #define NOTIFY_BUFFER_RESET_BIT     (1 << 8)
        // #define NOTIFY_ADC_BREAK_BIT        (1 << 16) // 16번 비트를 ADC Break 신호로 사용
    /** @} */ // end of EVENT_GROUP_BITS

    /*===========================================================================*/
    /* custom_esp_gpio.h 설정 값                                                 */
    /*===========================================================================*/
    /**
    * @defgroup    GPIO_SETTINGS GPIO 설정 값 메크로 그룹
    * @brief       custom_esp_gpio.h 의 설정 값 정의
    * @details     
    * @{
    */
        /**
        * @brief       LED 스트립 기능 활성화
        * @details     ESP32C3 보드 타입에 따라 조건부로 설정됨
        *              - ESP32C3_SUPER_MINI: false (비활성화)
        *              - 기타 ESP32 보드: true (활성화)
        * @note        (참고) ESP32C3_SUPER_MINI의 경우 자동으로 False 설정됨
        */
        #if (CONFIG_IDF_TARGET_ESP32C3 && (ESP32C3 == ESP32C3_SUPER_MINI))
            #define LED_STRIP_ENABLE    false
        #else
            #define LED_STRIP_ENABLE    true
        #endif

    /** @} */ // end of GPIO_SETTINGS

    /*===========================================================================*/
    /* custom_esp_queue.h 설정 값                                                 */
    /*===========================================================================*/
    /**
    * @defgroup    QUEUE_SETTINGS 큐 설정 값 메크로 그룹
    * @brief       custom_esp_queue.h 의 설정 값 정의
    * @details     
    * @{
    */
        /**
        * @brief       뮤텍스 타임아웃 시간 (밀리초)
        * @details     FreeRTOS 뮤텍스 획득 시도할 때, 최대 대기 시간
        * @see         (Use) custom_esp_queue.h
        */
        #define MUTEX_TIMEOUT_MS        10
    /** @} */ // end of QUEUE_SETTINGS

    /*===========================================================================*/
    /* custom_esp_spi_thread.h 설정 값                                          */
    /*===========================================================================*/
    /**
    * @defgroup    SPI_SETTINGS SPI 설정 값 메크로 그룹
    * @brief       custom_esp_spi_thread.h 의 설정 값 정의
    * @details     
    * @{
    */
        // /**
        // * @brief       SPI 이미지 수신 버퍼 크기
        // * @details     SPI 이미지 수신 버퍼 크기 (ESP-IDF 최소 요구: > 128)
        // */
        // #define SPI_IMAGE_BUFFER_LENGTH                            128 * 10             // SPI 이미지 수신 버퍼 크기 (ESP-IDF 최소 요구: > 128)





    /** @} */ // end of SPI_SETTINGS

    /*===========================================================================*/
    /* custom_esp_uart_thread.h 설정 값                                          */
    /*===========================================================================*/
    /**
    * @defgroup    UART_SETTINGS UART 설정 값 메크로 그룹
    * @brief       custom_esp_uart_thread.h 의 설정 값 정의
    * @details     
    * @{
    */
        /**
        * @def         UART_BUFFER_SIZE
        * @brief       UART 버퍼 크기
        * @details     UART 버퍼 크기 (ESP-IDF 최소 요구: > 128)
        */
        #define UART_BUFFER_SIZE                            256             // UART 버퍼 크기 (ESP-IDF 최소 요구: > 128)
    /** @} */ // end of UART_SETTINGS
/** @} */ // end of ENVIRONMENT_VARIABLES

#endif // PROJECT_TOP_H