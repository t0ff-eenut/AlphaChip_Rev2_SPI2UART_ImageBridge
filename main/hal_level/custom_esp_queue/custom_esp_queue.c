/**
 * @file        custom_esp_queue.c
 * @brief       ESP32 QUEUE 모듈
 * @author      T0T
 * @date        2026-01-07
 * @version     1.0.0
 * 
 * @details     ESP32에서 Queue를 생성하고 관리하기 위한 모듈
 */
#include "custom_esp_queue.h"

#define QUEUE_DEBUG         DEBUG
// #define QUEUE_DEBUG         false

static const char *custom_esp_queue_TAG = "[@]custom_esp_queue.c";

bool custom_mpqs_init(mpqs* input_p_mpqs, int input_i_length, int input_i_size, char* input_p_c_name){

    #define CUSTOM_MPQS_INIT_DEBUG         QUEUE_DEBUG

    #if CUSTOM_MPQS_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_init() - %s Mutex Queue 초기화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_c_name);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    if(input_p_mpqs == NULL){
        #if CUSTOM_MPQS_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_init() -input_p_mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    // Queue 생성
    input_p_mpqs->QueueHandle_queue = xQueueCreate(input_i_length, input_i_size);
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if CUSTOM_MPQS_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_init() - %s Queue 생성 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    // Mutex 생성
    input_p_mpqs->SemaphoreHandle_mutex = xSemaphoreCreateMutex();
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if CUSTOM_MPQS_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_init() - %s Mutex 생성 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    input_p_mpqs->p_c_name = input_p_c_name;
    
    #if CUSTOM_MPQS_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_init() - %s Mutex Queue 초기화 END\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_c_name);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    return true;
}

bool custom_mpqs_deinit(mpqs* input_p_mpqs, TickType_t input_TickType_timeout){

    #define CUSTOM_MPQS_DEINIT_DEBUG         QUEUE_DEBUG

    #if CUSTOM_MPQS_DEINIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s Mutex Queue 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    // NULL 체크
    if(input_p_mpqs == NULL){
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_deinit() -input_p_mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    // NULL 체크
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_deinit() - queue가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    // NULL 체크
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_deinit() - mutex가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    // ═══════════════════════════════════════════
    // Mutex 획득 (임계 영역 시작)
    // ═══════════════════════════════════════════
    if(xSemaphoreTake(input_p_mpqs->SemaphoreHandle_mutex, input_TickType_timeout) != pdTRUE){
        // Mutex 획득 실패 (타임아웃)
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_mpqs_deinit() - %s Mutex 획득 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif

        return false;
    }
    else{
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s Mutex 획득\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif

        // ═══════════════════════════════════════════
        // Queue 삭제
        // ═══════════════════════════════════════════
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s QueueHandle 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        
        vQueueDelete(input_p_mpqs->QueueHandle_queue);
        input_p_mpqs->QueueHandle_queue = NULL;

        // ═══════════════════════════════════════════
        // Mutex 해제 (임계 영역 종료)
        // ═══════════════════════════════════════════
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s Mutex 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);

        // ═══════════════════════════════════════════
        // Mutex 삭제
        // ═══════════════════════════════════════════
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s Mutex 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        vSemaphoreDelete(input_p_mpqs->SemaphoreHandle_mutex);
        input_p_mpqs->SemaphoreHandle_mutex = NULL;
        
        
        #if CUSTOM_MPQS_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_mpqs_deinit() - %s Mutex Queue 해제 END\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif

        input_p_mpqs->p_c_name = NULL;

        return true;
    }
}

cqrre custom_queue_safe_send(mpqs* input_p_mpqs, const void* input_p_v_data, TickType_t input_TickType_timeout){
    
    #define CUSTOM_QUEUE_SAFE_SEND_DEBUG         QUEUE_DEBUG
    // #define CUSTOM_QUEUE_SAFE_SEND_DEBUG         false

    cqrre cqrre_return = QUEUE_IS_ERROR;

    // NULL 체크
    if(input_p_mpqs == NULL){
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_send() - mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return MPQS_IS_NOT_READY;
    }
    // NULL 체크
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_send() - queue가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return QUEUE_IS_NOT_READY;
    }
    // NULL 체크
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_send() - mutex가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return MUTEX_IS_NOT_READY;
    }
    
    // ★ p_c_name NULL 체크 (deinit 된 Queue에 접근 방지)
    if(input_p_mpqs->p_c_name == NULL){
        return MPQS_IS_NOT_READY;
    }

    // ═══════════════════════════════════════════
    // Mutex 획득 (임계 영역 시작)
    // ═══════════════════════════════════════════
    if(xSemaphoreTake(input_p_mpqs->SemaphoreHandle_mutex, input_TickType_timeout) == pdTRUE){
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_send() - %s Mutex 획득\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif

        // 큐에 데이터 전송, Mutex 내부에서 타임아웃 0으로 설정 (이미 Mutex에서 대기함)
        BaseType_t send_result = xQueueSend(input_p_mpqs->QueueHandle_queue, input_p_v_data, 0);
        
        if(send_result == pdPASS){
            #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_send() - %s 전송 성공\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
            #endif
            cqrre_return = QUEUE_IS_READY;
        }
        else{
            // 큐가 가득 찬 경우
            #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_send() - %s 큐 가득 참\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
            #endif
            cqrre_return = QUEUE_IS_FULL;
        }

        // ═══════════════════════════════════════════
        // Mutex 해제 (임계 영역 종료)
        // ═══════════════════════════════════════════
        xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_send() - %s Mutex 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif
    }
    else{
        // Mutex 획득 실패 (타임아웃)
        #if CUSTOM_QUEUE_SAFE_SEND_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_send() - %s Mutex 획득 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif
        cqrre_return = MUTEX_IS_BUSY;
    }

    return cqrre_return;
}

cqrre custom_queue_safe_receive(mpqs* input_p_mpqs, void* input_p_v_data, TickType_t input_TickType_timeout){

    #define QUEUE_SAFE_RECEIVE_DEBUG         QUEUE_DEBUG
    // #define QUEUE_SAFE_RECEIVE_DEBUG         false

    cqrre cqrre_return = QUEUE_IS_ERROR;

    // NULL 체크
    if(input_p_mpqs == NULL){
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return MPQS_IS_NOT_READY;
    }
    // NULL 체크
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - queue가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return QUEUE_IS_NOT_READY;
    }
    // NULL 체크
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mutex가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return MUTEX_IS_NOT_READY;
    }

    // ★ p_c_name NULL 체크 (deinit 된 Queue에 접근 방지)
    if(input_p_mpqs->p_c_name == NULL){
        return MPQS_IS_NOT_READY;
    }

    // ═══════════════════════════════════════════
    // Mutex 획득 (임계 영역 시작)
    // ═══════════════════════════════════════════
    if(xSemaphoreTake(input_p_mpqs->SemaphoreHandle_mutex, input_TickType_timeout) == pdTRUE){
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_receive() - %s Mutex 획득\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif

        // 큐가 비어있는지 먼저 확인
        UBaseType_t UBaseType_messages_waiting = uxQueueMessagesWaiting(input_p_mpqs->QueueHandle_queue);
        
        if(UBaseType_messages_waiting == 0){
            #if QUEUE_SAFE_RECEIVE_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - %s 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
            #endif
            cqrre_return = QUEUE_IS_EMPTY;
        }
        else{
            // 큐에서 데이터 수신, Mutex 내부에서 타임아웃 0으로 설정
            BaseType_t BaseType_receive_result = xQueueReceive(input_p_mpqs->QueueHandle_queue, input_p_v_data, 0);
            
            if(BaseType_receive_result == pdPASS){
                #if QUEUE_SAFE_RECEIVE_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_receive() - %s 수신 성공\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
                #endif
                cqrre_return = QUEUE_IS_READY;
            }
            else{
                #if QUEUE_SAFE_RECEIVE_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - %s 수신 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
                #endif
                cqrre_return = QUEUE_IS_ERROR;
            }
        }

        // ═══════════════════════════════════════════
        // Mutex 해제 (임계 영역 종료)
        // ═══════════════════════════════════════════
        xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_queue_safe_receive() - %s Mutex 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif
    }
    else{
        // Mutex 획득 실패 (타임아웃)
        #if QUEUE_SAFE_RECEIVE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - %s Mutex 획득 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG, input_p_mpqs->p_c_name);
        #endif
        cqrre_return = MUTEX_IS_BUSY;
    }

    return cqrre_return;
}

UBaseType_t custom_queue_safe_messages_waiting(mpqs* input_p_mpqs){

    #define CUSTOM_QUEUE_SAFE_MESSAGES_WAITING_DEBUG         QUEUE_DEBUG

    UBaseType_t count = 0;

    // if(input_p_mpqs == NULL || input_p_mpqs->QueueHandle_queue == NULL || input_p_mpqs->SemaphoreHandle_mutex == NULL || input_p_mpqs->p_c_name == NULL){
    //     return 0;
    // }

    // NULL 체크
    if(input_p_mpqs == NULL){
        #if CUSTOM_QUEUE_SAFE_MESSAGES_WAITING_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return 0;
    }
    // NULL 체크
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if CUSTOM_QUEUE_SAFE_MESSAGES_WAITING_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - queue가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return 0;
    }
    // NULL 체크
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if CUSTOM_QUEUE_SAFE_MESSAGES_WAITING_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mutex가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return 0;
    }

    // ★ p_c_name NULL 체크 (deinit 된 Queue에 접근 방지)
    if(input_p_mpqs->p_c_name == NULL){
        return 0;
    }



    if(xSemaphoreTake(input_p_mpqs->SemaphoreHandle_mutex, portMAX_DELAY) == pdTRUE){
        count = uxQueueMessagesWaiting(input_p_mpqs->QueueHandle_queue);
        xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);
    }

    return count;
}

bool custom_mpqs_queue_reset(mpqs* input_p_mpqs){

    #define CUSTOM_MPQS_QUEUE_RESET_DEBUG         QUEUE_DEBUG
    
    // NULL 체크
    if(input_p_mpqs == NULL){
        #if CUSTOM_MPQS_QUEUE_RESET_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mpqs가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return false;
    }
    // NULL 체크
    if(input_p_mpqs->QueueHandle_queue == NULL){
        #if CUSTOM_MPQS_QUEUE_RESET_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - queue가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return false;
    }
    // NULL 체크
    if(input_p_mpqs->SemaphoreHandle_mutex == NULL){
        #if CUSTOM_MPQS_QUEUE_RESET_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - mutex가 NULL\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
        #endif
        return false;
    }

    // ★ p_c_name NULL 체크 (deinit 된 Queue에 접근 방지)
    if(input_p_mpqs->p_c_name == NULL){
        return false;
    }

    if(xSemaphoreTake(input_p_mpqs->SemaphoreHandle_mutex, portMAX_DELAY) == pdTRUE){
        if(!xQueueReset(input_p_mpqs->QueueHandle_queue)){
            #if CUSTOM_MPQS_QUEUE_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_queue_safe_receive() - queue reset 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_queue_TAG);
            #endif
            xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);
            return false;
        }        
        xSemaphoreGive(input_p_mpqs->SemaphoreHandle_mutex);
    }
    return true;
}