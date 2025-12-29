/*
******************************************************************************
* File Name          : project_top.c
* Description        : 초기 부팅에 필요한 Define 및 전역 함수 및 변수 관리
******************************************************************************
* 프로젝트 전반에 걸쳐 사용되는 전역 변수, 상수, 공통 유틸리티 함수를 정의하고 구현
*
* 주요 기능:
* - 펌웨어의 동작 모드(MODE) 및 디버그 Print 옵션을 설정하는 매크로와 핵심 상수들을 포함
* - 여러 FreeRTOS Task 간의 종료 및 동기화를 위한 전역 플래그 변수를 정의
* - 로그 출력용 시간 문자열 생성, 부팅 원인 출력 등 디버깅에 유용한 헬퍼 함수들을 구현
******************************************************************************

******************************************************************************
* first update : 2025/08/21
******************************************************************************
* final update : 2025/11/27
******************************************************************************
*/

#include "project_top.h"

#define PROJECT_TOP_DEBUG         DEBUG
static const char *project_top_TAG  = "[@]project_top.c";

esp_sleep_wakeup_cause_t g_esp_sleep_wakeup_cause;
esp_err_t g_esp_err;
// bool b_deep_sleep_ready  = false;

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