/**
 * @file        custom_esp_uart_thread.c
 * @brief       UART DATA 전송 모듈
 * @author      T0T
 * @date        2026-01-14
 * @version     1.0.0
 * 
 * @details     
 */

#include "custom_esp_uart_thread.h"

/**
 * @brief       custom_esp_uart_thread.c 파일 디버깅 여부
 * @details     custom_esp_uart_thread.c 파일에서 디버깅 Print 사용 여부를 정의합니다
 */
#define UART_DEBUG         DEBUG
// #define SPI_DEBUG         false

/**
 * @brief       custom_esp_uart_thread.c 디버깅 Tag
 * @details     Debug Print 시 custom_esp_spi_thread.c 파일을 구분하기 위한 Tag
 */
static const char *custom_esp_uart_TAG = "[@]custom_esp_uart_thread.c";

/**
 * @brief       UART 채널 설정 테이블
 * @details     UART 채널 설정 테이블 (딕셔너리처럼 사용)
 */
static const euccs uart_config_table[] = {
    [UART_CH_UPLOAD] = {
        .int_uart_port          = UPLOAD_LOG_UART_PORT,
        .int_baud_rate          = UPLOAD_LOG_PORT_BAUD_RATE_SEL,
        .uart_word_length       = UART_DATA_8_BITS,
        .uart_parity            = UART_PARITY_DISABLE,
        .uart_stop_bits         = UART_STOP_BITS_1,
        .uart_hw_flowcontrol    = UART_HW_FLOWCTRL_DISABLE,
        .uart_sclk              = UART_SCLK_DEFAULT,
    },
    [UART_CH_DEBUG] = {
        .int_uart_port          = DEBUG_UART_PORT,
        .int_baud_rate          = DEBUG_PORT_BAUD_RATE_SEL,
        .uart_word_length       = UART_DATA_8_BITS,
        .uart_parity            = UART_PARITY_DISABLE,
        .uart_stop_bits         = UART_STOP_BITS_1,
        .uart_hw_flowcontrol    = UART_HW_FLOWCTRL_DISABLE,
        .uart_sclk              = UART_SCLK_DEFAULT,
    },
};

/**
 * @brief       UART 수신 큐
 * @details     UART 수신 큐 (uint8_t**)
 */
static mpqs mpqs_uart_image_rx;

/**
 * @brief       custom_esp_uart_thread.c 파일에서 사용하는 Task Handle
 * @details     custom_uart_tx_thread의 Task Handle을 저장
 */
static volatile TaskHandle_t TaskHandle_custom_uart_tx_thread = NULL;

/**
 * @brief       UART 프레임 STX 패턴
 * @details     UART 프레임 STX 패턴 (이 파일 내부에서만 사용)
 */
static const uint8_t ui8_A_uart_stx_pattern[UART_STX_PATTERN_SIZE] = UART_STX_PATTERN;
/**
 * @brief       UART 프레임 ETX 패턴
 * @details     UART 프레임 ETX 패턴 (이 파일 내부에서만 사용)
 */
static const uint8_t ui8_A_uart_etx_pattern[UART_ETX_PATTERN_SIZE] = UART_ETX_PATTERN;

/**
 * @brief       UART 전송 중 여부
 * @details     UART 전송 중 여부 (현재 전송 중인지 여부)
 */
static volatile bool b_uart_working = false;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
bool custom_uart_init(void){

    #define CUSTOM_UART_INIT_DEBUG         UART_DEBUG

    static int i_xTaskCreate_return_value = 0;

    static uart_config_t uart_config;
    // UART 설정
    uart_config.baud_rate   = uart_config_table[UART_CH_DEBUG].int_baud_rate;           // 보레이트 설정
    uart_config.data_bits   = uart_config_table[UART_CH_DEBUG].uart_word_length;        // 데이터 비트 설정
    uart_config.parity      = uart_config_table[UART_CH_DEBUG].uart_parity;             // 페리티 설정
    uart_config.stop_bits   = uart_config_table[UART_CH_DEBUG].uart_stop_bits;          // Stop Bit 설정
    uart_config.flow_ctrl   = uart_config_table[UART_CH_DEBUG].uart_hw_flowcontrol;     // Flow Control 설정
    uart_config.source_clk  = uart_config_table[UART_CH_DEBUG].uart_sclk;

    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART 파라미터 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(uart_param_config(DEBUG_UART_PORT, &uart_config) != ESP_OK){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART 파라미터 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }

    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART 핀 설정 (TX=GPIO%d, RX=GPIO%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, DEBUG_TXD_GPIO_NUM, DEBUG_RXD_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(uart_set_pin(DEBUG_UART_PORT, DEBUG_TXD_GPIO_NUM, DEBUG_RXD_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART 핀 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }

    // // ★ 기존 UART 드라이버가 설치되어 있으면 먼저 삭제 (콘솔 UART와의 충돌 방지)
    // if(uart_is_driver_installed(DEBUG_UART_PORT)){
    //     #if CUSTOM_UART_INIT_DEBUG
    //     printf("[%s] "COLOR_YELLOW"[경고-WARN]\t %s custom_uart_init() - 기존 UART 드라이버 감지, 삭제 후 재설치\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    //     #endif
    //     uart_driver_delete(DEBUG_UART_PORT);
    // }

    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART 드라이버 설치\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    // if(uart_driver_install(DEBUG_UART_PORT, UART_BUFFER_SIZE * 2, 0, 0, NULL, 0) != ESP_OK){
    if(uart_driver_install(DEBUG_UART_PORT, UART_BUFFER_SIZE, 0, 0, NULL, 0) != ESP_OK){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART 드라이버 설치 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }

    // // ★ GPIO 핀 설정
    // #if CUSTOM_UART_INIT_DEBUG
    // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART GPIO 핀 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    // #endif
    // if(uart_set_pin(DEBUG_UART_PORT, DEBUG_TXD_GPIO_NUM, DEBUG_RXD_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK){
    //     #if CUSTOM_UART_INIT_DEBUG
    //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART GPIO 핀 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    //     #endif
    //     return false;
    // }

    // // ★ UART 드라이버 설치 (TX 버퍼: 4096+256, RX 버퍼: 256)
    // #if CUSTOM_UART_INIT_DEBUG
    // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART 드라이버 설치\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    // #endif
    // if(uart_driver_install(DEBUG_UART_PORT, 256, UART_MAX_PAYLOAD_SIZE + 256, 0, NULL, 0) != ESP_OK){
    //     #if CUSTOM_UART_INIT_DEBUG
    //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART 드라이버 설치 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    //     #endif
    //     return false;
    // }

    if(!custom_mpqs_init(&mpqs_uart_image_rx, UART_IMAGE_MAX_COUNT, sizeof(void*), "mpqs_uart_image_rx")){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - mpqs_uart_image_rx Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    i_xTaskCreate_return_value = xTaskCreate(custom_uart_tx_thread, "UART_TX_THREAD", UART_TX_THREAD_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &TaskHandle_custom_uart_tx_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART_TX_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }

    return true;
}

// APP에서 사용
cqrre custom_set_uart_image(ruifas input_ruifas){
    #define CUSTOM_SET_UART_IMAGE_DEBUG         UART_DEBUG

    // rsuis_value.cqrre_value = QUEUE_IS_ERROR;
    ruifas* ruifas_value = (ruifas*)pvPortMalloc(sizeof(ruifas));
    if(ruifas_value == NULL){
        // 메모리 할당 실패 처리
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - ruifas_value 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return QUEUE_IS_ERROR;
    }
    ruifas_value->ui8_height = input_ruifas.ui8_height;
    ruifas_value->ui8_width = input_ruifas.ui8_width;
    ruifas_value->A_ui8_uart_frame_buf = (uint8_t**)pvPortMalloc(ruifas_value->ui8_height * sizeof(uint8_t*));
    if(ruifas_value->A_ui8_uart_frame_buf == NULL){
        // 메모리 할당 실패 처리
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        vPortFree(ruifas_value);
        return QUEUE_IS_ERROR;
    }
    // 각 행에 대해 열(column) 할당
    for(int i = 0; i < ruifas_value->ui8_height; i++) {
        ruifas_value->A_ui8_uart_frame_buf[i] = (uint8_t*)pvPortMalloc(ruifas_value->ui8_width * sizeof(uint8_t));
        if(ruifas_value->A_ui8_uart_frame_buf[i] == NULL) {
            // 이전에 할당한 메모리 해제 후 오류 처리
            for(int j = 0; j < i; j++) {
                vPortFree(ruifas_value->A_ui8_uart_frame_buf[j]);
            }
            vPortFree(ruifas_value->A_ui8_uart_frame_buf);
            vPortFree(ruifas_value);
            #if CUSTOM_SET_UART_IMAGE_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            return QUEUE_IS_ERROR;
        }
        memset(ruifas_value->A_ui8_uart_frame_buf[i], 0, ruifas_value->ui8_width * sizeof(uint8_t));
    }

    // ★ 각 행의 실제 데이터를 복사 (포인터 배열이 아닌 실제 데이터!)
    for(int i = 0; i < ruifas_value->ui8_height; i++) {
        memcpy(ruifas_value->A_ui8_uart_frame_buf[i], 
               input_ruifas.A_ui8_uart_frame_buf[i], 
               ruifas_value->ui8_width * sizeof(uint8_t));
    }
    cqrre cqrre_value = custom_queue_safe_send(&mpqs_uart_image_rx, &ruifas_value, 1);


    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    // input_rsuis->cqrre_value = custom_queue_safe_send(&mpqs_uart_image_rx, &rsuis_value, 1);
    if(cqrre_value == QUEUE_IS_READY){
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_set_uart_image() - Queue 전송 성공\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
    }
    else{
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - Queue 전송 실패 Error Num : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, cqrre_value);
        #endif
    }

    return cqrre_value;
}

// APP에서 사용
rguis custom_get_uart_image(void){
    #define CUSTOM_GET_UART_IMAGE_DEBUG         UART_DEBUG

    rguis rguis_value = {QUEUE_IS_ERROR, 0, 0, {0,}};
    ruifas* ruifas_value = NULL;

    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    rguis_value.cqrre_value = custom_queue_safe_receive(&mpqs_uart_image_rx, &ruifas_value, 1);
    if(rguis_value.cqrre_value == QUEUE_IS_READY){

        rguis_value.ui8_height = ruifas_value->ui8_height;
        rguis_value.ui8_width = ruifas_value->ui8_width;
        for(int i = 0; i < rguis_value.ui8_height; i++) {
            memcpy(rguis_value.A_ui8_uart_frame_buf[i], 
                ruifas_value->A_ui8_uart_frame_buf[i], 
                rguis_value.ui8_width * sizeof(uint8_t));
        }

        for(int j = 0; j < ruifas_value->ui8_height; j++) {
            vPortFree(ruifas_value->A_ui8_uart_frame_buf[j]);
        }
        vPortFree(ruifas_value->A_ui8_uart_frame_buf);
        vPortFree(ruifas_value);
    }
    return rguis_value;
}

bool custom_get_uart_working(void){
    return b_uart_working;
}

void custom_set_uart_working(bool input_b_working){
    b_uart_working = input_b_working;
}

// 3. Checksum Process
// uint8_t checksum(utfs input_utfs_value) {
//   uint8_t ui8_checksum = 0;

//   for (uint8_t ui8_index = 0; ui8_index < UART_STX_PATTERN_SIZE; ui8_index++) {
//     ui8_checksum ^= input_utfs_value.A_ui8_start_marker[ui8_index]; // XOR 체크섬
//   }
//   ui8_checksum ^= input_utfs_value.ui8_width; // XOR 체크섬
//   ui8_checksum ^= input_utfs_value.ui8_height; // XOR 체크섬
//   for (uint8_t ui8_index = 0; ui8_index < input_utfs_value.ui8_width * input_utfs_value.ui8_height; ui8_index++) {
//     ui8_checksum ^= input_utfs_value.A_ui8_uart_frame_buf[ui8_index]; // XOR 체크섬
//   }

//   return ui8_checksum;
// }

// 체크섬 계산 함수 (간단한 Sum 방식)
static uint16_t custom_calculate_checksum(uint8_t* input_ui8_p_data, uint16_t input_ui16_length){
    uint32_t ui32_sum = 0;

    for(uint16_t i = 0; i < input_ui16_length; i++){
        ui32_sum += input_ui8_p_data[i];
    }

    return (uint16_t)(ui32_sum & 0xFFFF);
}

// 데이터 준비 및 직렬화 함수
// static bool custom_prepare_n_serialize_data(utrs* input_utrs_p_request, utfs* input_utfs_value){
static bool custom_prepare_n_serialize_data(ruifas* input_ruifas_value, utfs* input_utfs_value){
    
    #define CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG         UART_DEBUG

    // 직렬화
    input_utfs_value->ui8_height = input_ruifas_value->ui8_height;
    input_utfs_value->ui8_width = input_ruifas_value->ui8_width;

    for (uint8_t y = 0; y < input_ruifas_value->ui8_height; y++) {
        memcpy(&input_utfs_value->A_ui8_uart_frame_buf[y * input_ruifas_value->ui8_width], 
                input_ruifas_value->A_ui8_uart_frame_buf[y], 
                input_ruifas_value->ui8_width);
    }
    // 준비
    // STX 패턴 복사
    memcpy(input_utfs_value->ui8_stx, ui8_A_uart_stx_pattern, UART_STX_PATTERN_SIZE);

    // 체크섬 계산 (STX(3) + data_type(1) + data_length(2) + payload까지)
    uint8_t* ui8_checksum_data = (uint8_t*)input_utfs_value;
    uint16_t ui16_checksum_length = UART_STX_PATTERN_SIZE + UART_FRAME_HEIGHT_PACKIT_SIZE + UART_FRAME_WIDTH_PACKIT_SIZE + (input_ruifas_value->ui8_height * input_ruifas_value->ui8_width);
    input_utfs_value->ui16_checksum = custom_calculate_checksum(ui8_checksum_data, ui16_checksum_length);
    #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_prepare_n_serialize_data() - 페이로드 크기: %d, 체크섬: 0x%04X\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, (input_utfs_value->ui8_height * input_utfs_value->ui8_width), input_utfs_value->ui16_checksum);
    #endif

    // ETX 패턴 복사
    memcpy(input_utfs_value->ui8_etx, ui8_A_uart_etx_pattern, UART_ETX_PATTERN_SIZE);

    return true;
}

void custom_uart_tx_thread(void *arg){
    
    #define CUSTOM_UART_TX_THREAD_DEBUG         UART_DEBUG
    // #define CUSTOM_UART_TX_THREAD_DEBUG         false

    static upsle upsle_switch_level = QUEUE_WAIT;
    cqrre cqrre_receive_result;
    ruifas* ruifas_value = NULL;
    
    // // static utrs utrs_uart_tx_request;
    // static utdte utdte_data_type;
    static utfs utfs_uart_tx_frame;

    while(true){
        // Queue 존재 Check
        if(mpqs_uart_image_rx.QueueHandle_queue == NULL){
            #if CUSTOM_UART_TX_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s custom_uart_tx_thread() - mpqs_uart_image_rx 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
        }
        else{
            switch(upsle_switch_level){
                case QUEUE_WAIT:
                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 1.QUEUE_WAIT [Queue 대기]\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif
                    
                    // b_uart_transmitting = false;  // 유휴 상태
                    
                    // ★ Mutex 보호 큐 수신 (Thread-Safe)
                    // cqrre cqrre_receive_result = custom_queue_safe_receive(&mpqs_uart_tx_request, &utrs_uart_tx_request, pdMS_TO_TICKS(100));
                    // cqrre cqrre_receive_result = custom_queue_safe_receive(&mpqs_uart_tx_request, &utdte_data_type, custom_ms_to_delay(100));
                    // ★ Mutex 보호 큐 수신 (Thread-Safe)
                    cqrre_receive_result = custom_queue_safe_receive(&mpqs_uart_image_rx, &ruifas_value, 1);
                    if(cqrre_receive_result != QUEUE_IS_READY){
                        // Timeout 또는 비어있음 - 계속 대기
                        custom_set_uart_working(false);
                        break;
                    }
                    custom_set_uart_working(true);

                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 1.QUEUE_WAIT -> 2.DATA_PREPARE\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif
                    
                    // b_uart_transmitting = true;  // 전송 시작!
                    upsle_switch_level = DATA_PREPARE;

                case DATA_PREPARE:
                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 2.DATA_PREPARE [데이터 준비 및 직렬화]\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif

                    // if(!custom_prepare_n_serialize_data(&utrs_uart_tx_request, &utfs_uart_tx_frame)){
                    if(!custom_prepare_n_serialize_data(ruifas_value, &utfs_uart_tx_frame)){
                        #if CUSTOM_UART_TX_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - 데이터 준비 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                        #endif

                        for(int j = 0; j < ruifas_value->ui8_height; j++) {
                            vPortFree(ruifas_value->A_ui8_uart_frame_buf[j]);
                        }
                        vPortFree(ruifas_value->A_ui8_uart_frame_buf);
                        vPortFree(ruifas_value);

                        upsle_switch_level = QUEUE_WAIT;
                        break;
                    }

                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 2.DATA_PREPARE -> 3.UART_TRANSMIT\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif
                    upsle_switch_level = UART_TRANSMIT;

                case UART_TRANSMIT:
                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 3.UART_TRANSMIT [UART 전송]\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif

                    // 전송할 전체 크기 계산
                    // uint16_t total_size = 3 + 1 + 2 + utfs_uart_tx_frame.data_length + 2 + 3; // STX(3) + type + length + payload + checksum + ETX(3)
                    
                    // // UART로 전송
                    // int written = uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame, total_size);
                    
                    // if(written != total_size){
                    //     #if CUSTOM_UART_TX_THREAD_DEBUG
                    //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - UART 전송 실패 (전송: %d/%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, written, total_size);
                    //     #endif
                    // }
                    // else{
                    //     #if CUSTOM_UART_TX_THREAD_DEBUG
                    //     printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_uart_tx_thread() - UART 전송 완료 (크기: %d bytes)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, total_size);
                    //     #endif
                    // }

                    // 전송할 전체 크기 계산 (디버그 표시용)
                    uint16_t ui16_total_size = UART_DEFAULT_TOTAL_SIZE + (utfs_uart_tx_frame.ui8_height * utfs_uart_tx_frame.ui8_width);
                    // UART로 순차 전송 (구조체 패딩/배열 간격 문제 해결)
                    int i_written = 0;
                    // 1. STX (3 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.ui8_stx, UART_STX_PATTERN_SIZE);
                    
                    // 2. Data Type (1 byte)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui8_height, UART_FRAME_HEIGHT_PACKIT_SIZE);
                    
                    // 3. Data Length (2 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui8_width, UART_FRAME_WIDTH_PACKIT_SIZE);
                    
                    // 4. Payload (Variable Length)
                    // if(utfs_uart_tx_frame.ui16_data_length > 0){
                    //     i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.A_ui8_uart_frame_buf, (utfs_uart_tx_frame.ui8_height * utfs_uart_tx_frame.ui8_width));
                    // }
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.A_ui8_uart_frame_buf, (utfs_uart_tx_frame.ui8_height * utfs_uart_tx_frame.ui8_width));
                    
                    // 5. Checksum (2 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui16_checksum, UART_CHECKSUM_PACKIT_SIZE);
                    
                    // 6. ETX (3 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.ui8_etx, UART_ETX_PATTERN_SIZE);
                    
                    if(i_written != ui16_total_size){
                        #if CUSTOM_UART_TX_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - UART 전송 실패 (전송: %d/%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, i_written, ui16_total_size);
                        #endif
                    }
                    else{
                        #if CUSTOM_UART_TX_THREAD_DEBUG

                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_uart_tx_thread() - UART 전송 완료 (크기: %d bytes)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, ui16_total_size);
                        #endif
                    }

                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 3.UART_TRANSMIT -> 1.QUEUE_WAIT\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif
                    upsle_switch_level = QUEUE_WAIT;
                    break;

                default:
                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - Switch Level ERROR\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif
                    upsle_switch_level = QUEUE_WAIT;
                    break;
            } // END Switch
        } // END if(QueueHandle_utrs_uart_tx_request == NULL) else

        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, 0, &ulNotificationValue, custom_ms_to_delay(10)) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_UART_TX_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - UART TX Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                break; // 루프 탈출
            }
        }

        vTaskDelay(1);
    } // END while(true)

    TaskHandle_custom_uart_tx_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
}