#ifndef CUSTOM_ESP_QUEUE_H
#define CUSTOM_ESP_QUEUE_H

// HAL 모듈은 HW Level만 include (순환 참조 방지)
#include "hal_level_top.h"

#include "freertos/queue.h"
#include "freertos/semphr.h"

#define QUEUE_THREAD_STACK_SIZE (1024 * 4)

// #ifndef SEND_CALLBACK_BUFFER_SIZE
//     #define SEND_CALLBACK_BUFFER_SIZE       10
// #endif
// #ifndef RECEIVE_CALLBACK_BUFFER_SIZE
//     #define RECEIVE_CALLBACK_BUFFER_SIZE    10
// #endif
#ifndef ADC_BUFFER_SIZE
    #define ADC_BUFFER_SIZE                 128   // ADC 버퍼 크기
#endif

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
typedef enum queue_input_output_toggle_enum{
    QUEUE_INPUT,
    QUEUE_OUTPUT
} qiote;


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


// /**
//  * @struct      send_uart_queue_struct(suqs)
//  * @brief       Send Uart Queue Struct
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param uint16_t      ui16_adc_data
//  * @param uint16_t      ui16_voltage_data
//  * @param uint16_t      ui16_tp1
//  * @param uint8_t       ui8_tp2
//  * @param uint8_t       ui8_switch_status
//  * @param uint16_t*     A_ui16_adc_buf
//  * @param uint16_t*     A_ui16_adc_delta_buf
//  * @param bool*         A_b_occu_buf
//  * @param bool          b_occu_triger
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef struct send_uart_queue_struct{
//     uint16_t    ui16_adc_data;
//     uint16_t    ui16_voltage_data;
//     uint16_t    ui16_tp1;
//     uint8_t     ui8_tp2;
//     uint8_t     ui8_switch_status;
//     uint16_t*   A_ui16_adc_buf;
//     uint16_t*   A_ui16_adc_delta_buf;
//     bool*       A_b_occu_buf;
//     bool        b_occu_triger;
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//     float*      A_f_bandfilter_buf;
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// }suqs;

// 외부에서 사용되는 큐 핸들
// band.c
// extern QueueHandle_t QueueHandle_scs_band_send_callback;
// extern QueueHandle_t QueueHandle_rcs_band_receive_callback;
// extern QueueHandle_t QueueHandle_ui16_iSENSOR_adc_read;
// extern QueueHandle_t QueueHandle_ui16_iSENSOR_adc;
// extern QueueHandle_t QueueHandle_suqs_uart;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// extern QueueHandle_t QueueHandle_f_iSENSOR_bandfilter;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// /**
//  * @brief       Custom Queue Initial Function
//  * @attention   *주의사항 [const : 읽기 전용]
//  * @param[in]   void
//  * @return      bool    true : 초기화 성공, false : 초기화 실패
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_queue_init(void);

// /**
//  * @brief       Custom GPIO Initial Function
//  * @attention   *주의사항 [const : 읽기 전용]
//  * @param[in]   void
//  * @return      cqrre    QUEUE_IS_READY : 준비됨, QUEUE_IS_NOT_READY : 준비되지 않음, QUEUE_IS_EMPTY : 비어있음, QUEUE_IS_ERROR : 오류 
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// cqrre custom_check_queue_ready(char *queue_name, QueueHandle_t target_queue, qiote qiot_toggle);






/**
 * ═══════════════════════════════════════════════════════════════════════════════
 * @section     MUTEX 기반 Thread-Safe 큐 접근 함수
 * ═══════════════════════════════════════════════════════════════════════════════
 * @brief       Mutex를 사용하여 check + send/receive를 원자적으로 처리
 * @note        Race Condition 방지를 위해 이 함수들을 사용해야 함
 */

/**
 * @struct      mutex_protected_queue_struct(mpqs)
 * @brief       Mutex로 보호된 Queue Handle 구조체
 * @attention   *주의사항
 * @warning     *경고
 * @note        *참고사항
 *
 * @param QueueHandle_t       QueueHandle_queue
 * @param SemaphoreHandle_t   SemaphoreHandle_mutex
 * @param char*               p_c_name
 * 
 * @details     디테일 설명
 * @todo        todo
 * @bug         bug
 */
typedef struct mutex_protected_queue_struct {

    QueueHandle_t       QueueHandle_queue;          // 실제 큐 핸들
    SemaphoreHandle_t   SemaphoreHandle_mutex;          // 보호용 Mutex
    char*               p_c_name;           // 큐 이름 (디버깅용)
} mpqs;

/**
 * @brief       Mutex 보호 큐 초기화
 * @param[in]   input_p_mpqs      대상 mpqs 포인터
 * @param[in]   input_queue       기존 QueueHandle_t
 * @param[in]   input_p_c_name    큐 이름 문자열
 * @return      bool        true: 성공, false: 실패
 */
// bool custom_mpqs_init(mpqs* input_p_mpqs, QueueHandle_t input_queue, char* input_p_c_name);
bool custom_mpqs_init(mpqs* input_p_mpqs, int input_i_length, int input_i_size, char* input_p_c_name);

/**
 * @brief       Mutex 보호 큐 해제
 * @param[in]   p_mpqs      대상 mpqs 포인터
 * @param[in]   timeout     타임아웃 (ticks)
 */
bool custom_mpqs_deinit(mpqs* input_p_mpqs, TickType_t input_TickType_timeout);

/**
 * @brief       Thread-Safe 큐 송신 (Check + Send 원자적 처리)
 * @param[in]   p_mpqs      대상 mpqs 포인터
 * @param[in]   p_data      전송할 데이터 포인터
 * @param[in]   timeout     타임아웃 (ticks)
 * @return      cqrre       QUEUE_IS_READY: 성공, 그 외: 실패 사유
 */
cqrre custom_queue_safe_send(mpqs* input_p_mpqs, const void* input_p_v_data, TickType_t input_TickType_timeout);

/**
 * @brief       Thread-Safe 큐 수신 (Check + Receive 원자적 처리)
 * @param[in]   p_mpqs      대상 mpqs 포인터
 * @param[out]  p_data      수신할 데이터 포인터
 * @param[in]   timeout     타임아웃 (ticks)
 * @return      cqrre       QUEUE_IS_READY: 성공, 그 외: 실패 사유
 */
cqrre custom_queue_safe_receive(mpqs* input_p_mpqs, void* input_p_v_data, TickType_t input_TickType_timeout);

/**
 * @brief       Thread-Safe 큐 메시지 개수 확인
 * @param[in]   p_mpqs      대상 mpqs 포인터
 * @return      UBaseType_t 큐에 있는 메시지 개수
 */
UBaseType_t custom_queue_safe_messages_waiting(mpqs* p_mpqs);


// ═══════════════════════════════════════════
// Queue 내용 Reset
// ═══════════════════════════════════════════
/**
 * @brief       Thread-Safe 큐 내용 Reset
 * @param[in]   p_mpqs      대상 mpqs 포인터
 * @return      bool        true: 성공, false: 실패
 */
bool custom_mpqs_queue_reset(mpqs* input_p_mpqs);

#endif