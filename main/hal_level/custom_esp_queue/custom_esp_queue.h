/**
 * @file        custom_esp_queue.h
 * @brief       ESP32의 Queue 모듈 헤더 파일
 * @author      T0T
 * @date        2025-12-04
 * @version     1.0.0
 * 
 * @details     이 파일은 ESP32의 Queue 모듈을 구현한 헤더 파일임.
 *              Mutex를 사용하여 Thread-Safe한 Queue 접근을 제공
 */
#ifndef CUSTOM_ESP_QUEUE_H
#define CUSTOM_ESP_QUEUE_H

// HAL 모듈은 HW Level만 include (순환 참조 방지)
#include "hal_level_top.h"

#include "freertos/queue.h"
#include "freertos/semphr.h"

#define QUEUE_THREAD_STACK_SIZE (1024 * 4)

 /**
 * @enum        qiote
 * @typedef     queue_input_output_toggle_enum
 * @brief       Queue 입력/출력 토글
 * @note        Doxygen에 반영 안됨
 */
typedef enum queue_input_output_toggle_enum{
    QUEUE_INPUT,
    QUEUE_OUTPUT
} qiote;

 /**
 * @enum        cqrre
 * @typedef     check_queue_ready_return_enum
 * @brief       Queue 상태 확인 반환 enum
 * @note        Doxygen에 반영 안됨
 */
typedef enum check_queue_ready_return_enum{
    QUEUE_IS_READY,
    MPQS_IS_NOT_READY,
    QUEUE_IS_NOT_READY,
    MUTEX_IS_NOT_READY,
    MUTEX_IS_BUSY,
    QUEUE_IS_EMPTY,
    QUEUE_IS_FULL,
    QUEUE_IS_ERROR
} cqrre;

/**
 * @struct      mpqs
 * @typedef     mutex_protected_queue_struct
 * @brief       Mutex 보호된 Queue 구조체
 * @details     
 */
typedef struct mutex_protected_queue_struct {
    QueueHandle_t       QueueHandle_queue;      /**< 실제 Queue 핸들 */
    SemaphoreHandle_t   SemaphoreHandle_mutex;  /**< 보호용 Mutex */
    char*               p_c_name;               /**< 큐 이름 (디버깅용) */
} mpqs;

/**
 * @function    custom_nvs_handle_to_string
 * @brief       Mutex 보호 큐 초기화
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @param[in]   input_i_length    큐 길이
 * @param[in]   input_i_size      큐 크기
 * @param[in]   input_p_c_name    큐 이름 문자열
 * @return      bool        true: 성공, false: 실패
 */
bool custom_mpqs_init(mpqs* input_p_mpqs, int input_i_length, int input_i_size, char* input_p_c_name);

/**
 * @function    custom_mpqs_deinit
 * @brief       Mutex 보호 큐 해제
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @param[in]   input_TickType_timeout    타임아웃 (ticks)
 * @return      bool        true: 성공, false: 실패
 */
bool custom_mpqs_deinit(mpqs* input_p_mpqs, TickType_t input_TickType_timeout);

 /**
 * @function    custom_queue_safe_send
 * @brief       Thread-Safe 큐 송신 (Check + Send 원자적 처리)
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @param[in]   input_p_v_data    전송할 데이터 포인터
 * @param[in]   input_TickType_timeout    타임아웃 (ticks)
 * @return      cqrre       QUEUE_IS_READY: 성공, 그 외: 실패 사유
 */
cqrre custom_queue_safe_send(mpqs* input_p_mpqs, const void* input_p_v_data, TickType_t input_TickType_timeout);

 /**
 * @function    custom_queue_safe_receive
 * @brief       Thread-Safe 큐 수신 (Check + Receive 원자적 처리)
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @param[in]   input_p_v_data    수신할 데이터 포인터
 * @param[in]   input_TickType_timeout    타임아웃 (ticks)
 * @return      cqrre       QUEUE_IS_READY: 성공, 그 외: 실패 사유
 */
cqrre custom_queue_safe_receive(mpqs* input_p_mpqs, void* input_p_v_data, TickType_t input_TickType_timeout);

 /**
 * @function    custom_queue_safe_messages_waiting
 * @brief       Thread-Safe 큐 수신 (Check + Receive 원자적 처리)
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @return      UBaseType_t       큐에 있는 메시지 개수
 */
UBaseType_t custom_queue_safe_messages_waiting(mpqs* p_mpqs);

 /**
 * @function    custom_mpqs_queue_reset
 * @brief       큐 내용 Reset
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @return      bool        true: 성공, false: 실패
 */
bool custom_mpqs_queue_reset(mpqs* input_p_mpqs);

#endif