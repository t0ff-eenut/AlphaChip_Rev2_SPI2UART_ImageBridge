/**
 * @file        project_top.c
 * @brief       프로젝트 전역 설정 코드 파일
 * @author      T0T
 * @date        2025-12-19
 * @version     1.0.0
 * 
 * @details     이 파일은 프로젝트 전반에 걸쳐 사용되는 전역 변수, 상수, 공통 유틸리티 함수를 정의하고 구현
 *              - 펌웨어의 동작 모드(MODE) 및 디버그 Print 옵션을 설정하는 매크로와 핵심 상수들을 포함
 *              - FreeRTOS Task 간의 종료 및 동기화를 위한 전역 플래그 변수를 정의
 *              - 로그 출력용 시간 문자열 생성, 부팅 원인 출력 등 디버깅에 유용한 헬퍼 함수들을 구현
 */
#include "project_top.h"

/**
 * @brief       project_top.c 파일 디버깅 여부
 * @details     project_top.c 파일에서 디버깅 Print 사용 여부를 정의합니다
 */
#define PROJECT_TOP_DEBUG         DEBUG

/**
 * @brief       project_top.c 디버깅 Tag
 * @details     Debug Print 시 project_top.c 파일을 구분하기 위한 Tag
 */
static const char *project_top_TAG  = "[@]project_top.c";

/*===========================================================================*/
/* 변수 정의
/*===========================================================================*/
/**
 * @brief       ESP32 웨이크업 원인 저장 전역 변수
 * @note        가능한 웨이크업 원인 값:
 *              | 값 | 설명 |
 *              |----|----|
 *              | ESP_SLEEP_WAKEUP_UNDEFINED | 정의되지 않은 원인 (첫 부팅 또는 리셋) |
 *              | ESP_SLEEP_WAKEUP_EXT0 | EXT0 웨이크업 (단일 RTC GPIO) |
 *              | ESP_SLEEP_WAKEUP_EXT1 | EXT1 웨이크업 (다중 RTC GPIO) |
 *              | ESP_SLEEP_WAKEUP_TIMER | 타이머 웨이크업 |
 *              | ESP_SLEEP_WAKEUP_TOUCHPAD | 터치패드 웨이크업 |
 *              | ESP_SLEEP_WAKEUP_ULP | ULP 코프로세서 웨이크업 |
 *              | ESP_SLEEP_WAKEUP_GPIO | GPIO 웨이크업 (라이트 슬립 전용) |
 *              | ESP_SLEEP_WAKEUP_UART | UART 웨이크업 (라이트 슬립 전용) |
 * @details     ESP32가 Deep Sleep에서 깨어난 원인을 저장하는 전역 변수.
 *              esp_sleep_get_wakeup_cause() 함수로 값을 가져와 저장함.
 */
esp_sleep_wakeup_cause_t g_esp_sleep_wakeup_cause;

/**
 * @brief       ESP-IDF API 에러 코드 저장 전역 변수
 * @details     ESP-IDF 함수 호출 결과를 저장하는 전역 변수
 *              에러 처리 및 디버깅 목적으로 사용됨
 * @note        - ESP_OK(0)이면 성공, 그 외의 값은 에러 코드
 *              - esp_err_to_name() 함수로 에러 이름 문자열 확인 가능
 * @todo        각 파일별로 변수 선언으로 변경하기
 */
esp_err_t g_esp_err;
// bool b_deep_sleep_ready  = false;

/*===========================================================================*/
/* 함수 정의
/*===========================================================================*/
const char* custom_getRuntimeString(void){
    static char buffer[32];
    uint64_t us = esp_timer_get_time(); // 부팅 이후 경과 시간 (마이크로초)
    uint64_t total_seconds = us / 1000000ULL;
    uint64_t hours   = total_seconds / 3600;
    uint64_t minutes = (total_seconds % 3600) / 60;
    uint64_t seconds = total_seconds % 60;
    uint64_t rem_us  = us % 1000000ULL;

    snprintf(buffer, sizeof(buffer), "%02llu:%02llu:%02llu.%06llu", hours, minutes, seconds, rem_us);

    return buffer;
}

void custom_wakeup_cause_print(esp_sleep_wakeup_cause_t input_esp_sleep_wakeup_cause){
    #if PROJECT_TOP_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s wakeup_cause_print() - Booting 이유 : ", custom_getRuntimeString(), project_top_TAG);
    #endif
    switch (input_esp_sleep_wakeup_cause) {
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            #if PROJECT_TOP_DEBUG
            printf("전원 켜짐 또는 리셋 (정의되지 않음)\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_TIMER:
            #if PROJECT_TOP_DEBUG
            printf("타이머 만료로 기기 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_EXT0:
            #if PROJECT_TOP_DEBUG
            printf("RTC_IO 단일 핀 외부 신호로 깨어남 (EXT0)\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_EXT1:
            #if PROJECT_TOP_DEBUG
            printf("RTC_CNTL 여러 핀 외부 신호로 깨어남 (EXT1)\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_GPIO:
            #if PROJECT_TOP_DEBUG
            printf("일반 GPIO 신호로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            #if PROJECT_TOP_DEBUG
            printf("터치센서 감지로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_ULP:
            #if PROJECT_TOP_DEBUG
            printf("ULP 코프로세서 실행 완료로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_UART:
            #if PROJECT_TOP_DEBUG
            printf("UART 데이터 수신으로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_WIFI:
            #if PROJECT_TOP_DEBUG
            printf("Wi-Fi 이벤트로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_COCPU:
            #if PROJECT_TOP_DEBUG
            printf("코프로세서 이벤트로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
            #if PROJECT_TOP_DEBUG
            printf("코프로세서 트랩 발생으로 깨어남\n" COLOR_RESET);
            #endif
            break;

        case ESP_SLEEP_WAKEUP_BT:
            #if PROJECT_TOP_DEBUG
            printf("블루투스 이벤트로 깨어남\n" COLOR_RESET);
            #endif
            break;

        default:
            #if PROJECT_TOP_DEBUG
            printf("알 수 없는 원인으로 깨어남\n" COLOR_RESET);
            #endif
            break;
    } // end switch
}
uint8_t custom_ui8_abs(uint8_t ui8_a, uint8_t ui8_b){
    return (ui8_a > ui8_b) ? ui8_a - ui8_b : ui8_b - ui8_a;
}

uint16_t custom_ui16_abs(uint16_t ui16_a, uint16_t ui16_b){
    return (ui16_a > ui16_b) ? ui16_a - ui16_b : ui16_b - ui16_a;
}

uint64_t custom_ui64_abs(uint64_t ui64_a, uint64_t ui64_b){
    return (ui64_a > ui64_b) ? ui64_a - ui64_b : ui64_b - ui64_a;
}

float custom_f_abs(float f_a, float f_b){
    return (f_a > f_b) ? f_a - f_b : f_b - f_a;
}


int custom_tick_to_delay(int input_i_tick){
    return pdTICKS_TO_MS(input_i_tick);
}
int custom_ms_to_delay(int input_i_ms){
    return pdMS_TO_TICKS(input_i_ms);
}