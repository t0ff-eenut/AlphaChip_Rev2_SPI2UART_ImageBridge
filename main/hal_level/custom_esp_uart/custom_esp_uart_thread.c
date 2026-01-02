/*
******************************************************************************
* File Name          : custom_uart_thread.c
* Description        : UART DATA TRANSMISSION MODULE
******************************************************************************
* UART를 통해 isensor_memory의 버퍼 및 설정값을 PC로 전송하는 모듈
* Queue를 통해 전송 요청을 받아서 데이터를 직렬화하여 전송
* uart_init 후 uart_tx_thread 실행
******************************************************************************

******************************************************************************
* first update : 2025/12/04
******************************************************************************
* final update : 2025/12/04
******************************************************************************
*/

#include "custom_esp_uart_thread.h"
#include "iSENSOR_memory.h"
#include <string.h>

// #define UART_DEBUG         DEBUG
#define UART_DEBUG         false

static const char *custom_esp_uart_TAG = "[@]custom_esp_uart.c";

static volatile TaskHandle_t TaskHandle_custom_uart_tx_thread = NULL;
static volatile TaskHandle_t TaskHandle_custom_uart_rx_thread = NULL;

// ═══════════════════════════════════════════════════════════════════════════════
// Mutex 보호 큐 (Thread-Safe Queue)
// ═══════════════════════════════════════════════════════════════════════════════
static mpqs mpqs_uart_tx_request;  // UART TX 요청 큐 (utrs)
// static mpqs mpqs_uart_rx_request;  // UART RX 요청 큐 (미정)

// UART 프레임 STX/ETX 패턴 (이 파일 내부에서만 사용)
static const uint8_t ui8_A_uart_stx_pattern[UART_STX_PATTERN_SIZE] = UART_STX_PATTERN;
static const uint8_t ui8_A_uart_etx_pattern[UART_ETX_PATTERN_SIZE] = UART_ETX_PATTERN;

// PWM 채널 설정 테이블 (딕셔너리처럼 사용)
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
#if !LED_STRIP_ENABLE
    [UART_CH_DEBUG] = {
        .int_uart_port          = DEBUG_UART_PORT,
        .int_baud_rate          = DEBUG_PORT_BAUD_RATE_SEL,
        .uart_word_length       = UART_DATA_8_BITS,
        .uart_parity            = UART_PARITY_DISABLE,
        .uart_stop_bits         = UART_STOP_BITS_1,
        .uart_hw_flowcontrol    = UART_HW_FLOWCTRL_DISABLE,
        .uart_sclk              = UART_SCLK_DEFAULT,
    },
#endif
};

static volatile bool b_uart_working = false;  // 현재 전송 중인지 여부

// // 백업용 원본 큐 핸들 (내부 사용)
// static QueueHandle_t QueueHandle_utrs_uart_tx_request = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    // ★ Mutex 보호 큐 초기화 (Queue에는 utdte만 저장, utfs 프레임은 TX 스레드 내에서 로컬 생성)
    if(!custom_mpqs_init(&mpqs_uart_tx_request, UART_BUFFER_SIZE, sizeof(utdte), "mpqs_uart_tx_request")){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - mpqs_uart_tx_request Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    // Thread 생성
    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART_TX_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
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
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART_TX_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }

/////////////////////////// RX //////////////////////////////////////////////////////////////////////////////////////////

    // ★ 부팅 시 축적된 RX 노이즈 데이터 제거
    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - RX 버퍼 초기화 (노이즈 제거)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    uart_flush_input(DEBUG_UART_PORT);

    // Thread 생성
    #if CUSTOM_UART_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_init() - UART_RX_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    i_xTaskCreate_return_value = xTaskCreate(custom_uart_rx_thread, "UART_RX_THREAD", UART_RX_THREAD_STACK_SIZE, NULL, configMAX_PRIORITIES - 3, &TaskHandle_custom_uart_rx_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART_RX_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_uart_deinit();
        return false;
    }
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - UART_RX_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
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

bool custom_running_iSENSOR_uart_tx_thread(void){
    return TaskHandle_custom_uart_tx_thread != NULL;
}

bool custom_running_iSENSOR_uart_rx_thread(void){
    return TaskHandle_custom_uart_rx_thread != NULL;
}

// iSENSOR_Mode에서 사용
bool custom_request_uart_tx(utdte input_utdte){
    #define CUSTOM_REQUEST_UART_TX_DEBUG         UART_DEBUG

    // utrs utrs_request;
    // utrs_request.utdte_data_type    = input_utdte;
    // utrs_request.p_void_data_ptr    = NULL;
    // utrs_request.ui16_data_size     = 0;

    // ★ Mutex 보호 큐 송신 (Thread-Safe)
    // cqrre cqrre_send_result = custom_queue_safe_send(&mpqs_uart_tx_request, &utrs_request, pdMS_TO_TICKS(10));
    cqrre cqrre_send_result = custom_queue_safe_send(&mpqs_uart_tx_request, &input_utdte, custom_ms_to_delay(10));
    
    if(cqrre_send_result != QUEUE_IS_READY){
        #if CUSTOM_REQUEST_UART_TX_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_request_uart_tx() - mpqs_uart_tx_request 전송 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, cqrre_send_result);
        #endif
        return false;
    }

    #if CUSTOM_REQUEST_UART_TX_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_request_uart_tx() - UART TX 요청 전송 완료 (타입: %d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_utdte);
    #endif
    return true;
}

bool custom_get_uart_working(void){
    return b_uart_working;
}

void custom_set_uart_working(bool input_b_working){
    b_uart_working = input_b_working;
}

// 체크섬 계산 함수 (간단한 Sum 방식)
static uint16_t custom_calculate_checksum(uint8_t* input_ui8_p_data, uint16_t input_ui16_length){
    uint32_t ui32_sum = 0;
    for(uint16_t i = 0; i < input_ui16_length; i++){
        ui32_sum += input_ui8_p_data[i];
    }
    return (uint16_t)(ui32_sum & 0xFFFF);
}

// 데이터 준비 및 직렬화 함수
// static bool custom_prepare_n_serialize_data(utrs* input_utrs_p_request, utfs* input_utfs_p_frame){
static bool custom_prepare_n_serialize_data(utdte input_utdte_data_type, utfs* input_utfs_p_frame){
    
    #define CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG         UART_DEBUG

    // STX 패턴 복사 (3 bytes)
    memcpy(input_utfs_p_frame->ui8_stx, ui8_A_uart_stx_pattern, UART_STX_PATTERN_SIZE);
    // input_utfs_p_frame->ui8_data_type = input_utrs_p_request->utdte_data_type;
    input_utfs_p_frame->ui8_data_type = input_utdte_data_type;
    // ETX 패턴 복사 (3 bytes)
    memcpy(input_utfs_p_frame->ui8_etx, ui8_A_uart_etx_pattern, UART_ETX_PATTERN_SIZE);

    uint16_t ui16_payload_index = 0;

    switch(input_utdte_data_type){
        case UART_TX_ADC_RAW_BUFFER:
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - ADC 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            for(int i = 0; i < WINDOW_SIZE; i++){
                uint16_t ui16_value = custom_read_adc_raw_buffer(i);
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(ui16_value >> 8);
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(ui16_value & 0xFF);
            }
            break;

        case UART_TX_ADC_RAW_VOLTAGE_BUFFER:
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - Voltage 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            for(int i = 0; i < WINDOW_SIZE; i++){
                uint16_t ui16_value = custom_read_adc_raw_voltage_buffer(i);
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(ui16_value >> 8);
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(ui16_value & 0xFF);
            }
            break;

        case UART_TX_ADC_SW_HPF_BUFFER:  // SW HPF 버퍼
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - SW HPF 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            for(int i = 0; i < WINDOW_SIZE; i++){
                float f_value = custom_read_adc_sw_hpf_buffer(i);  // SW HPF
                memcpy(&input_utfs_p_frame->ui8_payload[ui16_payload_index], &f_value, sizeof(float));
                ui16_payload_index += sizeof(float);
            }
            break;

        case UART_TX_ADC_SW_BPF_BUFFER:  // SW BPF 버퍼
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - SW BPF 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            for(int i = 0; i < WINDOW_SIZE; i++){
                float f_value = custom_read_adc_sw_bpf_buffer(i);  // SW BPF
                memcpy(&input_utfs_p_frame->ui8_payload[ui16_payload_index], &f_value, sizeof(float));
                ui16_payload_index += sizeof(float);
            }
            break;

        case UART_TX_HPF_BUFFER:  // HW HPF 버퍼 (RAW ADC)
            #if ADC_CHANNEL_HPF_ENABLE
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - HW HPF 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                for(int i = 0; i < WINDOW_SIZE; i++){
                    uint16_t value = custom_read_hpf_buffer(i);  // HW HPF RAW
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value >> 8);
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value & 0xFF);
                }
            #else
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - UART_TX_HPF_BUFFER : ADC_CHANNEL_HPF 비활성화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
            #endif
            break;
        case UART_TX_HPF_VOLTAGE_BUFFER:  // HW HPF 버퍼 (Voltage)
            #if ADC_CHANNEL_HPF_ENABLE
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - HW HPF 버퍼(Voltage) 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                for(int i = 0; i < WINDOW_SIZE; i++){
                    uint16_t value = custom_read_hpf_voltage_buffer(i);  // HW HPF Voltage
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value >> 8);
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value & 0xFF);
                }
            #else
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - UART_TX_HPF_VOLTAGE_BUFFER : ADC_CHANNEL_HPF 비활성화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
            #endif
            break;



        case UART_TX_BPF_BUFFER:  // HW BPF 버퍼 (RAW ADC)
            #if ADC_CHANNEL_BPF_ENABLE
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - HW BPF 버퍼 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                for(int i = 0; i < WINDOW_SIZE; i++){
                    uint16_t value = custom_read_bpf_buffer(i);  // HW BPF RAW
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value >> 8);
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value & 0xFF);
                }
            #else
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - UART_TX_BPF_BUFFER : ADC_CHANNEL_BPF 비활성화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
            #endif
            break;

        case UART_TX_BPF_VOLTAGE_BUFFER:  // HW BPF 버퍼 (Voltage)
            #if ADC_CHANNEL_BPF_ENABLE
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - HW BPF 버퍼(Voltage) 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                for(int i = 0; i < WINDOW_SIZE; i++){
                    uint16_t value = custom_read_bpf_voltage_buffer(i);  // HW BPF Voltage
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value >> 8);
                    input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(value & 0xFF);
                }
            #else
                #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - UART_TX_BPF_VOLTAGE_BUFFER : ADC_CHANNEL_BPF 비활성화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
            #endif
            break;

        case UART_TX_SETTINGS:
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_prepare_n_serialize_data() - 설정값 데이터 준비\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            // TP1 Occupancy (uint16_t)
            uint16_t tp1_occupancy = custom_get_occupancy_tp1();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(tp1_occupancy >> 8);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(tp1_occupancy & 0xFF);
            
            // TP1 Recheck (uint16_t)
            uint16_t tp1_recheck = custom_get_recheck_tp1();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(tp1_recheck >> 8);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(tp1_recheck & 0xFF);
            
            // TP2 (uint64_t)
            uint64_t tp2 = custom_get_adc_tp2();
            for(int i = 7; i >= 0; i--){
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)((tp2 >> (i * 8)) & 0xFF);
            }
            
            // LED 설정값들
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = custom_get_led_max_percentage();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = custom_get_led_min_percentage();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = custom_get_led_dimming_percentage();
            
            // LED dimming step time (uint32_t)
            uint32_t led_dimming_step_time_ms = custom_get_led_dimming_step_time_ms();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_step_time_ms >> 24);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_step_time_ms >> 16);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_step_time_ms >> 8);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_step_time_ms & 0xFF);
            
            // LED dimming work time (uint32_t)
            uint32_t led_dimming_work_time_ms = custom_get_led_dimming_work_time_ms();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_work_time_ms >> 24);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_work_time_ms >> 16);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_work_time_ms >> 8);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_work_time_ms & 0xFF);
            
            // LED dimming delay time (uint32_t)
            uint32_t led_dimming_delay_time_ms = custom_get_led_dimming_delay_time_ms();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_delay_time_ms >> 24);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_delay_time_ms >> 16);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_delay_time_ms >> 8);
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)(led_dimming_delay_time_ms & 0xFF);
            
            // Occupancy timeout (uint64_t)
            uint64_t timeout = custom_get_wating_occupancy_timeout_us();
            for(int i = 7; i >= 0; i--){
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)((timeout >> (i * 8)) & 0xFF);
            }
            
            // Sleep time (uint64_t)
            uint64_t sleep_time = custom_get_sleep_time();
            for(int i = 7; i >= 0; i--){
                input_utfs_p_frame->ui8_payload[ui16_payload_index++] = (uint8_t)((sleep_time >> (i * 8)) & 0xFF);
            }
            
            // Occupancy status (bool -> uint8_t, 1 byte)
            bool b_occupancy = custom_get_occupancy();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = b_occupancy ? 1 : 0;
            
            // PIR Output status (bool -> uint8_t, 1 byte)
            bool b_pir_output = custom_get_pir_output();
            input_utfs_p_frame->ui8_payload[ui16_payload_index++] = b_pir_output ? 1 : 0;
            
            // ★ 디버그: Occupancy 및 PIR Output 상태 출력
            printf("[%s] "COLOR_CYAN"[DEBUG]\t %s - UART TX Settings: Occupancy=%d, PIR_Output=%d\n" COLOR_RESET, 
                   custom_getRuntimeString(), custom_esp_uart_TAG, b_occupancy, b_pir_output);
            break;

        case UART_TX_ALL_DATA:
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s custom_prepare_n_serialize_data() - 모든 데이터 전송은 페이로드 크기 초과 가능\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            // 페이로드 크기 제한으로 인해 여러 번 나눠 전송 필요
            return false;

        default:
            #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_prepare_n_serialize_data() - 알 수 없는 데이터 타입: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_utdte_data_type);
            #endif

            return false;
    }

    input_utfs_p_frame->ui16_data_length = ui16_payload_index;

    // 체크섬 계산 (STX(3) + data_type(1) + data_length(2) + payload까지)
    uint8_t* ui8_checksum_data = (uint8_t*)input_utfs_p_frame;
    uint16_t ui16_checksum_length = 3 + 1 + 2 + ui16_payload_index; // STX(3) + data_type + data_length + payload
    input_utfs_p_frame->ui16_checksum = custom_calculate_checksum(ui8_checksum_data, ui16_checksum_length);

    #if CUSTOM_PREPARE_AND_SERIALIZE_DATA_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_prepare_n_serialize_data() - 페이로드 크기: %d, 체크섬: 0x%04X\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, ui16_payload_index, input_utfs_p_frame->ui16_checksum);
    #endif

    return true;
}

void custom_uart_tx_thread(void *arg){
    #define CUSTOM_UART_TX_THREAD_DEBUG         UART_DEBUG
    // #define CUSTOM_UART_TX_THREAD_DEBUG         false

    static upsle upsle_switch_level = QUEUE_WAIT;
    // static utrs utrs_uart_tx_request;
    static utdte utdte_data_type;
    static utfs utfs_uart_tx_frame;

    while(true){
        // Queue 존재 Check
        if(mpqs_uart_tx_request.QueueHandle_queue == NULL){
            #if CUSTOM_UART_TX_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s custom_uart_tx_thread() - mpqs_uart_tx_request 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
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
                    cqrre cqrre_receive_result = custom_queue_safe_receive(&mpqs_uart_tx_request, &utdte_data_type, custom_ms_to_delay(100));
                    if(cqrre_receive_result != QUEUE_IS_READY){
                        // Timeout 또는 비어있음 - 계속 대기
                        custom_set_uart_working(false);
                        break;
                    }
                    custom_set_uart_working(true);

                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 1.QUEUE_WAIT -> 2.DATA_PREPARE (타입: %d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, utdte_data_type);
                    #endif
                    
                    // b_uart_transmitting = true;  // 전송 시작!
                    upsle_switch_level = DATA_PREPARE;

                case DATA_PREPARE:
                    #if CUSTOM_UART_TX_THREAD_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_tx_thread() - 2.DATA_PREPARE [데이터 준비 및 직렬화]\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                    #endif

                    // if(!custom_prepare_n_serialize_data(&utrs_uart_tx_request, &utfs_uart_tx_frame)){
                    if(!custom_prepare_n_serialize_data(utdte_data_type, &utfs_uart_tx_frame)){
                        #if CUSTOM_UART_TX_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - 데이터 준비 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                        #endif
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
                    uint16_t ui16_total_size = 3 + 1 + 2 + utfs_uart_tx_frame.ui16_data_length + 2 + 3; 
                    // UART로 순차 전송 (구조체 패딩/배열 간격 문제 해결)
                    int i_written = 0;
                    // 1. STX (3 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.ui8_stx, UART_STX_PATTERN_SIZE);
                    
                    // 2. Data Type (1 byte)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui8_data_type, 1);
                    
                    // 3. Data Length (2 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui16_data_length, 2);
                    
                    // 4. Payload (Variable Length)
                    if(utfs_uart_tx_frame.ui16_data_length > 0){
                        i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.ui8_payload, utfs_uart_tx_frame.ui16_data_length);
                    }
                    
                    // 5. Checksum (2 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)&utfs_uart_tx_frame.ui16_checksum, 2);
                    
                    // 6. ETX (3 bytes)
                    i_written += uart_write_bytes(DEBUG_UART_PORT, (const char*)utfs_uart_tx_frame.ui8_etx, UART_ETX_PATTERN_SIZE);
                    
                    if(i_written != ui16_total_size){
                        #if CUSTOM_UART_TX_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_tx_thread() - UART 전송 실패 (전송: %d/%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, i_written, ui16_total_size);
                        #endif
                    }
                    else{
                        #if CUSTOM_UART_TX_THREAD_DEBUG
                        printf("[%s] [3단계:UART TX] Frame sent. Data Type: %d, Size: %d\n", custom_getRuntimeString(), utfs_uart_tx_frame.ui8_data_type, ui16_total_size);
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

// Main.c에서 사용
// bool custom_uart_wait_all_tx_done(uint32_t input_ui32_timeout_ms){
bool custom_uart_wait_all_tx_done(void){

    #define CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG  UART_DEBUG
    
    // TickType_t start_tick = xTaskGetTickCount();
    // TickType_t timeout_ticks = custom_ms_to_delay(input_ui32_timeout_ms);
    
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_wait_all_tx_done() - [1단계] 모든 UART 전송 완료 체크 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    // ═══════════════════════════════════════════
    // 단계 1: Queue가 비었는지 확인
    // ═══════════════════════════════════════════
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_wait_all_tx_done() - 남은 Queue 갯수 확인\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    while(custom_queue_safe_messages_waiting(&mpqs_uart_tx_request) > 0){
        #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_uart_wait_all_tx_done() - 남은 Queue 갯수 : %u개\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, custom_queue_safe_messages_waiting(&mpqs_uart_tx_request));
        #endif
        vTaskDelay(custom_ms_to_delay(10));
    }
    
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_uart_wait_all_tx_done() - Queue 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    // ═══════════════════════════════════════════
    // 단계 2: Thread가 현재 전송 중이 아닌지 확인
    // ═══════════════════════════════════════════
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_wait_all_tx_done() - [2단계] UART 전송 중인지 판단 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    while(custom_get_uart_working()){
        #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
        printf("[%s] "COLOR_YELLOW"[대기-WAIT]\t %s custom_uart_wait_all_tx_done() - Thread 전송 중...\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        vTaskDelay(custom_ms_to_delay(10));
    }
    
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_uart_wait_all_tx_done() - UART 전송 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    // ═══════════════════════════════════════════
    // 단계 3: UART TX FIFO가 완전히 비었는지 확인
    // ═══════════════════════════════════════════
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_wait_all_tx_done() - [단계3] UART HW FIFO 확인 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    if(uart_wait_tx_done(DEBUG_UART_PORT, portMAX_DELAY) != ESP_OK){
        #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_wait_all_tx_done() - [단계3] uart_wait_tx_done() 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return false;
    }
    
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_uart_wait_all_tx_done() - UART TX FIFO 비어있음 확인\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    vTaskDelay(custom_ms_to_delay(10));  // 10ms 추가 대기
    
    #if CUSTOM_UART_WAIT_ALL_TX_DONE_DEBUG
    printf("[%s] "COLOR_GREEN"[완료-DONE]\t %s custom_uart_wait_all_tx_done() - ✅ 모든 UART 전송 완전히 완료!\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    
    return true;
}

//////////// RX //////////////////////

/**
 * @brief 명령 핸들러
 */
static void custom_rx_command_handler(cte input_cte, uint8_t *input_p_ui8_payload, uint16_t input_ui16_payload_len){

    #define CUSTOM_RX_COMMAND_HANDLER_DEBUG  UART_DEBUG
    
    switch(input_cte){
        case CMD_SET_TP1: {
            if(input_ui16_payload_len != 2){
                #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_rx_command_handler() - CMD_SET_TP1 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
                #endif
                return;
            }
            uint16_t tp1_value = input_p_ui8_payload[0] | (input_p_ui8_payload[1] << 8);  // Little Endian
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_rx_command_handler() - TP1 설정: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp1_value);
            #endif
            custom_set_occupancy_tp1(tp1_value);
            break;
        }
        

        case CMD_SET_TP2: {
            if(input_ui16_payload_len != 8){
                #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_rx_command_handler() - CMD_SET_TP2 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
                #endif
                return;
            }
            uint64_t tp2_value = 0;
            for(int i = 0; i < 8; i++){
                tp2_value |= ((uint64_t)input_p_ui8_payload[i] << (i * 8));
            }
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_rx_command_handler() - TP2 설정: %llu\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp2_value);
            #endif
            custom_set_adc_tp2(tp2_value);
            break;
        }
        
        case CMD_SET_TP1_RECHECK: {
            if(input_ui16_payload_len != 2){
                #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_rx_command_handler() - CMD_SET_TP1_RECHECK 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
                #endif
                return;
            }
            uint16_t tp1_recheck_value = input_p_ui8_payload[0] | (input_p_ui8_payload[1] << 8);  // Little Endian
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_rx_command_handler() - TP1_RECHECK 설정: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp1_recheck_value);
            #endif
            custom_set_recheck_tp1(tp1_recheck_value);
            break;
        }
        
        case CMD_GET_SETTINGS: {
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_rx_command_handler() - 설정값 요청 → SETTINGS 전송\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            custom_request_uart_tx(UART_TX_SETTINGS);
            break;
        }
        
        case CMD_SAVE_NVS: {
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s NVS 저장 요청\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            write_value_to_nvs();
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_GREEN"[★ RX]\t %s NVS 저장 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            break;
        }
        
        case CMD_RESET: {
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s 리셋 요청 - 1초 후 재시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            vTaskDelay(custom_ms_to_delay(1000));
            esp_restart();
            break;
        }
        
        default: {
            #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고]\t %s 알 수 없는 명령: 0x%02X\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_cte);
            #endif
            break;
        }
    }
}

/**
 * @brief 수신된 프레임 파싱 및 명령 처리
 */
static bool custom_rx_data_parse(uint8_t *input_p_ui8_data, size_t input_size_len){

    // #define CUSTOM_RX_DATA_PARSE_DEBUG  UART_DEBUG
    #define CUSTOM_RX_DATA_PARSE_DEBUG  true

    // 프레임 파싱: [STX:3] [CMD:1] [LEN:2] [PAYLOAD:N] [CHECKSUM:2] [ETX:3]
    uint8_t ui8_cmd = input_p_ui8_data[3];
    uint16_t ui16_payload_len = input_p_ui8_data[4] | (input_p_ui8_data[5] << 8);  // Little Endian
    // 길이 검증
    // size_t size_expected_len = 3 + 1 + 2 + ui16_payload_len + 2 + 3;

    // 최소 프레임 크기: STX(3) + CMD(1) + LEN(2) + CHECKSUM(2) + ETX(3) = 11
    if(input_size_len < UART_MIN_SIZE){
        #if CUSTOM_RX_DATA_PARSE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_rx_data_parse() - 최소 프레임 길이 부족 : got = %d / %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_size_len, UART_MIN_SIZE);
        #endif
        return false;
    }

    // ★ 체크섬 위치를 LEN 필드 기준으로 계산 (버퍼 끝에 추가 데이터가 있어도 정확히 파싱)
    // 체크섬 오프셋 = STX(3) + CMD(1) + LEN(2) + PAYLOAD(ui16_payload_len)
    size_t checksum_offset = 3 + 1 + 2 + ui16_payload_len;
    
    // 체크섬 검증
    uint16_t ui16_received_checksum = input_p_ui8_data[checksum_offset] | (input_p_ui8_data[checksum_offset + 1] << 8);
    uint16_t ui16_calculated_checksum = custom_calculate_checksum(input_p_ui8_data, checksum_offset);
    if(ui16_received_checksum != ui16_calculated_checksum){
        #if CUSTOM_RX_DATA_PARSE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_rx_data_parse() - 체크섬 불일치: got=0x%04X, calc=0x%04X (offset=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, ui16_received_checksum, ui16_calculated_checksum, checksum_offset);
        #endif
        return false;
    }
    
    // 명령 처리
    uint8_t *ui8_p_payload = (ui16_payload_len > 0) ? &input_p_ui8_data[6] : NULL;
    custom_rx_command_handler((cte)ui8_cmd, ui8_p_payload, ui16_payload_len);
    
    return true;
}

void custom_uart_rx_thread(void *arg){
    
    // #define CUSTOM_UART_RX_THREAD_DEBUG         UART_DEBUG
    #define CUSTOM_UART_RX_THREAD_DEBUG         true

    // static upsle upsle_switch_level = QUEUE_WAIT;
    // // static utrs utrs_uart_tx_request;
    // static utdte utdte_data_type;
    // static utfs utfs_uart_tx_frame;

    ursl ursl_uart_rx_switch_level = READ_RX_BYTE;

    bool b_in_frame = false;
    /** @brief RX 버퍼 */
    static uint8_t A_ui8_uart_rx_buffer[UART_BUFFER_SIZE];
    size_t size_rx_index = 0;
    uint8_t ui8_read = 0;

    while(true){
        switch(ursl_uart_rx_switch_level){
            case READ_RX_BYTE:
                if(ursl_uart_rx_switch_level == READ_RX_BYTE){
                    // UART에서 1바이트씩 읽기
                    int i_len = uart_read_bytes(DEBUG_UART_PORT, &ui8_read, 1, custom_ms_to_delay(UART_RX_TIMEOUT_MS));

                    // 수신 데이터 없으면
                    if(i_len <= 0){
                        // 기존 수신받던 데이터가 있으면
                        if(b_in_frame && size_rx_index > 0){
                            // 폐기
                            size_rx_index = 0;
                            b_in_frame = false;
                        }
                        break;
                    }

                    // 수신 데이터 있으면
                    // ★★★ 디버그: 수신된 모든 바이트 출력 ★★★
                    #if CUSTOM_UART_RX_THREAD_DEBUG
                    printf("[RX-BYTE] 0x%02X ('%c')\n", ui8_read, (ui8_read >= 32 && ui8_read < 127) ? ui8_read : '.');
                    #endif

                    ursl_uart_rx_switch_level = CHECK_STX;
                }
            case CHECK_STX:
                if(ursl_uart_rx_switch_level == CHECK_STX){
                    // STX 패턴 검출
                    if(!b_in_frame){
                        if(size_rx_index < UART_STX_PATTERN_SIZE){
                            A_ui8_uart_rx_buffer[size_rx_index++] = ui8_read;
                        }
                        if(size_rx_index != UART_STX_PATTERN_SIZE){
                            ursl_uart_rx_switch_level = READ_RX_BYTE;
                            break; 
                        }
                        else{                      
                            if(memcmp(A_ui8_uart_rx_buffer, ui8_A_uart_stx_pattern, UART_STX_PATTERN_SIZE) != 0){
                                // STX 아님 - 버퍼 시프트
                                A_ui8_uart_rx_buffer[0] = A_ui8_uart_rx_buffer[1];
                                A_ui8_uart_rx_buffer[1] = A_ui8_uart_rx_buffer[2];
                                size_rx_index = 2;
                                ursl_uart_rx_switch_level = READ_RX_BYTE;
                                break;
                            }
                            // 두 메모리 값이 동일한 경우
                            else{
                                b_in_frame = true;
                                ursl_uart_rx_switch_level = READ_RX_BYTE;
                                #if CUSTOM_UART_RX_THREAD_DEBUG
                                printf("[%s] "COLOR_CYAN"[RX]\t %s STX 감지됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                                #endif
                                break;  // ★ 다음 바이트를 읽기 위해 break (fall-through 방지)
                            }
                        }
                    }
                    else{
                        ursl_uart_rx_switch_level = STORAGE_DATA;
                    }
                }
            case STORAGE_DATA:
                if(ursl_uart_rx_switch_level == STORAGE_DATA){
                    // 프레임 내용 수신 중
                    if(size_rx_index >= UART_BUFFER_SIZE){
                        // 버퍼 오버플로우
                        #if CUSTOM_UART_RX_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s RX 버퍼 오버플로우\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                        #endif
                        size_rx_index = 0;
                        b_in_frame = false;
                        ursl_uart_rx_switch_level = READ_RX_BYTE;
                        break;
                    }
                    else{
                        #if CUSTOM_UART_RX_THREAD_DEBUG
                        printf("[STORAGE] idx=%d, byte=0x%02X\n", size_rx_index, ui8_read);
                        #endif
                        A_ui8_uart_rx_buffer[size_rx_index++] = ui8_read;
                        ursl_uart_rx_switch_level = CHECK_ETX;
                    }
                }
            case CHECK_ETX:
                if(ursl_uart_rx_switch_level == CHECK_ETX){

                    // 최소 프레임 크기(11) 이상이면 ETX 확인
                    // 프레임: STX(3) + CMD(1) + LEN(2) + PAYLOAD(N) + CHECKSUM(2) + ETX(3)
                    if(size_rx_index >= UART_MIN_SIZE){
                        if(memcmp(&A_ui8_uart_rx_buffer[size_rx_index - UART_ETX_PATTERN_SIZE], ui8_A_uart_etx_pattern, UART_ETX_PATTERN_SIZE) == 0){
                            #if CUSTOM_UART_RX_THREAD_DEBUG
                            printf("[%s] "COLOR_GREEN"[RX]\t %s 프레임 수신 완료 (%d bytes)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, size_rx_index);
                            #endif

                            // Queue를 넣어야 하나?

                            // ★ 디버그: 버퍼 덤프
                            #if CUSTOM_UART_RX_THREAD_DEBUG
                            printf("[RX-DUMP] ");
                            for(int i = 0; i < size_rx_index && i < 20; i++){
                                printf("%02X ", A_ui8_uart_rx_buffer[i]);
                            }
                            printf("\n");
                            #endif

                            if(!custom_rx_data_parse(A_ui8_uart_rx_buffer, size_rx_index)){
                                #if CUSTOM_UART_RX_THREAD_DEBUG
                                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s 프레임 분석 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                                #endif
                            }

                            size_rx_index = 0;
                            b_in_frame = false;
                        }
                    }
                    ursl_uart_rx_switch_level = READ_RX_BYTE;
                }
                break;

            default:
                break;
        }

        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, NOTIFY_SHUTDOWN_BIT, &ulNotificationValue, custom_ms_to_delay(10)) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_UART_RX_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_thread() - UART RX Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
                #endif
                break; // 루프 탈출
            }
        }

        // vTaskDelay(1);

    } // END while(true)

    TaskHandle_custom_uart_rx_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
}

//////////// RX //////////////////////

bool custom_uart_deinit(void){

    #define CUSTOM_UART_DEINIT_DEBUG         UART_DEBUG


    //////////// RX //////////////////////
    // ★★★ UART RX 스레드를 먼저 종료해야 함! (드라이버 삭제 전에)
    // UART RX 스레드가 uart_read_bytes()를 호출 중이면, 
    // 드라이버 삭제 시 Load access fault 발생!
    #if CUSTOM_UART_DEINIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - UART RX 스레드 종료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    // custom_uart_rx_deinit();
    if(custom_running_iSENSOR_uart_rx_thread()){
        // 1. 종료 신호 전송
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - xTaskNotify - uart_rx_thread 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        xTaskNotify(TaskHandle_custom_uart_rx_thread, NOTIFY_SHUTDOWN_BIT, eSetBits);
        
        // 2. 핸들이 NULL이 될 때까지 대기 (태스크가 스스로 NULL로 만들고 죽음)
        while(custom_running_iSENSOR_uart_rx_thread()){
            #if CUSTOM_UART_DEINIT_DEBUG
            printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_uart_deinit() - running_iSENSOR_uart_rx_thread() = %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, custom_running_iSENSOR_uart_rx_thread());
            #endif
            vTaskDelay(custom_ms_to_delay(10)); 
        }
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - uart_rx_thread 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
    }

    // custom_mpqs_queue_reset(&mpqs_uart_rx_request);
    // // ★ Mutex 보호 큐 해제
    // custom_mpqs_deinit(&mpqs_uart_rx_request, portMAX_DELAY);
    //////////// RX //////////////////////

    #if CUSTOM_UART_DEINIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - UART TX 스레드 종료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    if(custom_running_iSENSOR_uart_tx_thread()){
        // 1. 종료 신호 전송
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - xTaskNotify - uart_tx_thread 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        xTaskNotify(TaskHandle_custom_uart_tx_thread, NOTIFY_SHUTDOWN_BIT, eSetBits);
        
        // 2. 핸들이 NULL이 될 때까지 대기 (태스크가 스스로 NULL로 만들고 죽음)
        while(custom_running_iSENSOR_uart_tx_thread()){
            #if CUSTOM_UART_DEINIT_DEBUG
            printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_uart_deinit() - running_iSENSOR_uart_tx_thread() = %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, custom_running_iSENSOR_uart_tx_thread());
            #endif
            vTaskDelay(custom_ms_to_delay(10)); 
        }
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - uart_tx_thread 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
    }

    if(!custom_mpqs_queue_reset(&mpqs_uart_tx_request)){
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_deinit() - mpqs_uart_tx_request 큐 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return false;
    }
    
    // ★ Mutex 보호 큐 해제
    custom_mpqs_deinit(&mpqs_uart_tx_request, portMAX_DELAY);

    #if CUSTOM_UART_DEINIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_deinit() - uart_driver_delete() UART 드라이버 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
    #endif
    if(uart_driver_delete(DEBUG_UART_PORT) != ESP_OK){
        #if CUSTOM_UART_DEINIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_deinit() - uart_driver_delete() UART 드라이버 삭제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return false;
    }

    return true;
}








// // ============================================================================
// // UART RX (수신) 구현
// // ============================================================================

// // #include "../../shared_protocol.h"   // CommandType_t
// // #include "esp_system.h"              // esp_restart()

// /** @brief RX 스레드 핸들 */
// static TaskHandle_t TaskHandle_uart_rx_thread = NULL;

// // /** @brief RX 스레드 실행 플래그 */
// // static volatile bool running_uart_rx_thread = false;

// /** @brief RX 버퍼 */
// static uint8_t A_ui8_uart_rx_buffer[UART_RX_BUFFER_SIZE];

// // 내부 함수 선언
// static void custom_uart_rx_thread(void *arg);
// static bool custom_rx_parse_n_make_frame(uint8_t *data, size_t len);
// static void handle_rx_command(CommandType_t cmd, uint8_t *payload, uint16_t payload_len);

// // bool custom_uart_rx_init(void){

// //     // #define UART_RX_INIT_DEBUG  UART_DEBUG
// //     #define UART_RX_INIT_DEBUG  true

// //     if(running_uart_rx_thread){
// //         #if UART_RX_INIT_DEBUG
// //         printf("[%s] "COLOR_YELLOW"[경고-WARN]\t %s custom_uart_rx_init() - 이미 실행 중\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //         #endif
// //         return true;
// //     }

// //     #if UART_RX_INIT_DEBUG
// //     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_init() - UART RX 스레드 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //     #endif

// //     // ★ RX 스레드 시작 전 버퍼 한번 더 비우기 (혹시 모를 잔여 노이즈 제거)
// //     #if UART_RX_INIT_DEBUG
// //     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_init() - uart_flush_input() RX 버퍼 초기화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //     #endif
// //     uart_flush_input(DEBUG_UART_PORT);

// //     running_uart_rx_thread = true;
    
// //     BaseType_t result = xTaskCreate(
// //         custom_uart_rx_thread,
// //         "uart_rx_thread",
// //         UART_RX_THREAD_STACK_SIZE,
// //         NULL,
// //         5,  // 우선순위
// //         &TaskHandle_uart_rx_thread
// //     );
    
// //     if(result != pdPASS){
// //         #if UART_RX_INIT_DEBUG
// //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_rx_init() - 태스크 생성 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //         #endif
// //         running_uart_rx_thread = false;
// //         return false;
// //     }
    
// //     #if UART_RX_INIT_DEBUG
// //     printf("[%s] "COLOR_WHITE"[완료-OK]\t %s custom_uart_rx_init() - UART RX 스레드 시작 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //     #endif
// //     return true;
// // }

// // bool custom_uart_rx_deinit(void){

// //     // #define UART_RX_DEINIT_DEBUG  UART_DEBUG
// //     #define UART_RX_DEINIT_DEBUG  true

// //     if(!running_uart_rx_thread){
// //         #if UART_RX_DEINIT_DEBUG
// //         printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_deinit() - 이미 종료됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //         #endif
// //         return true;
// //     }
    
// //     #if UART_RX_DEINIT_DEBUG
// //     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_deinit() - UART RX 스레드 종료 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //     #endif
    
// //     // 1. 종료 플래그 설정 (RX 스레드가 이를 감지하고 종료됨)
// //     running_uart_rx_thread = false;
    
// //     // 2. RX 스레드가 종료될 때까지 대기 (최대 500ms)
// //     //    uart_read_bytes() 타임아웃(100ms) + 여유 시간
// //     if(TaskHandle_uart_rx_thread != NULL){
// //         int retry_count = 0;
// //         const int max_retries = 50;  // 50 * 10ms = 500ms 최대 대기
        
// //         while(TaskHandle_uart_rx_thread != NULL && retry_count < max_retries){
// //             #if UART_RX_DEINIT_DEBUG
// //             if(retry_count % 10 == 0){  // 100ms마다 로그
// //                 printf("[%s] "COLOR_YELLOW"[대기-WAIT]\t %s custom_uart_rx_deinit() - RX 스레드 종료 대기 중... (%d/%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, retry_count, max_retries);
// //             }
// //             #endif
// //             vTaskDelay(custom_ms_to_delay(10));
// //             retry_count++;
// //         }
        
// //         if(TaskHandle_uart_rx_thread != NULL){
// //             #if UART_RX_DEINIT_DEBUG
// //             printf("[%s] "COLOR_RED"[경고-WARN]\t %s custom_uart_rx_deinit() - RX 스레드 종료 타임아웃, 강제 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //             #endif
// //             // 타임아웃 시 강제 삭제 (위험할 수 있음)
// //             vTaskDelete(TaskHandle_uart_rx_thread);
// //             TaskHandle_uart_rx_thread = NULL;
// //         }
// //     }
    
// //     #if UART_RX_DEINIT_DEBUG
// //     printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_uart_rx_deinit() - UART RX 스레드 종료 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
// //     #endif
    
// //     return true;
// // }

// // bool custom_uart_rx_is_running(void){
// //     return running_uart_rx_thread;
// // }

// /**
//  * @brief UART RX 스레드 (PC → ESP32 명령 수신)
//  */
// static void custom_uart_rx_thread(void *arg){

//     // #define CUSTOM_UART_RX_THREAD_DEBUG  UART_DEBUG
//     #define CUSTOM_UART_RX_THREAD_DEBUG  true

//     #if CUSTOM_UART_RX_THREAD_DEBUG
//     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_thread() - RX Thread 시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//     #endif
    
//     size_t rx_index = 0;
//     bool in_frame = false;
    
//     while(running_uart_rx_thread){
//         // UART에서 1바이트씩 읽기
//         uint8_t byte;
//         int len = uart_read_bytes(DEBUG_UART_PORT, &byte, 1, custom_ms_to_delay(UART_RX_TIMEOUT_MS));
        
//         if(len <= 0){
//             // 타임아웃 - 부분 프레임이 있으면 폐기
//             if(in_frame && rx_index > 0){
//                 rx_index = 0;
//                 in_frame = false;
//             }
//             continue;
//         }
        
//         // ★★★ 디버그: 수신된 모든 바이트 출력 ★★★
//         #if CUSTOM_UART_RX_THREAD_DEBUG
//         printf("[RX-BYTE] 0x%02X ('%c')\n", byte, (byte >= 32 && byte < 127) ? byte : '.');
//         #endif

//         // STX 패턴 검출
//         if(!in_frame){
//             if(rx_index < UART_STX_PATTERN_SIZE){
//                 uart_rx_buffer[rx_index++] = byte;
//             }
            
//             if(rx_index == UART_STX_PATTERN_SIZE){
//                 if(memcmp(uart_rx_buffer, ui8_A_uart_stx_pattern, UART_STX_PATTERN_SIZE) == 0){
//                     in_frame = true;
//                     #if CUSTOM_UART_RX_THREAD_DEBUG
//                     printf("[%s] "COLOR_CYAN"[RX]\t %s STX 감지됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//                     #endif
//                 } else {
//                     // STX 아님 - 버퍼 시프트
//                     uart_rx_buffer[0] = uart_rx_buffer[1];
//                     uart_rx_buffer[1] = uart_rx_buffer[2];
//                     rx_index = 2;
//                 }
//             }
//             continue;
//         }
        
//         // 프레임 내용 수신 중
//         if(rx_index < UART_RX_BUFFER_SIZE){
//             uart_rx_buffer[rx_index++] = byte;
//         } else {
//             // 버퍼 오버플로우
//             #if CUSTOM_UART_RX_THREAD_DEBUG
//             printf("[%s] "COLOR_RED"[오류-ERROR]\t %s RX 버퍼 오버플로우\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//             #endif
//             rx_index = 0;
//             in_frame = false;
//             continue;
//         }
        
//         // 최소 프레임 크기(11) 이상이면 ETX 확인
//         // 프레임: STX(3) + CMD(1) + LEN(2) + PAYLOAD(N) + CHECKSUM(2) + ETX(3)
//         if(rx_index >= 11){
//             if(memcmp(&uart_rx_buffer[rx_index - UART_ETX_PATTERN_SIZE], ui8_A_uart_etx_pattern, UART_ETX_PATTERN_SIZE) == 0){
//                 #if CUSTOM_UART_RX_THREAD_DEBUG
//                 printf("[%s] "COLOR_GREEN"[RX]\t %s 프레임 수신 완료 (%d bytes)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, rx_index);
//                 #endif
//                 // 완전한 프레임 수신!
                
//                 custom_rx_parse_n_make_frame(uart_rx_buffer, rx_index);
                
//                 rx_index = 0;
//                 in_frame = false;
//             }
//         }
//     }
    
//     #if CUSTOM_UART_RX_THREAD_DEBUG
//     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_uart_rx_thread() - RX Thread 종료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//     #endif
    
//     // 핸들을 NULL로 설정하여 deinit에서 종료 감지 가능하게 함
//     TaskHandle_uart_rx_thread = NULL;
//     vTaskDelete(NULL);
// }

// /**
//  * @brief 수신된 프레임 파싱 및 명령 처리
//  */
// static bool custom_rx_parse_n_make_frame(uint8_t *data, size_t len){
//     // 최소 프레임 크기: STX(3) + CMD(1) + LEN(2) + CHECKSUM(2) + ETX(3) = 11
//     if(len < 11){
//         return false;
//     }
    
//     // 프레임 파싱: [STX:3] [CMD:1] [LEN:2] [PAYLOAD:N] [CHECKSUM:2] [ETX:3]
//     uint8_t cmd = data[3];
//     uint16_t payload_len = data[4] | (data[5] << 8);  // Little Endian
    
//     // 길이 검증
//     size_t expected_len = 3 + 1 + 2 + payload_len + 2 + 3;
//     if(len != expected_len){
//         #if UART_DEBUG
//         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s 프레임 길이 불일치: got=%d, expected=%d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, len, expected_len);
//         #endif
//         return false;
//     }
    
//     // 체크섬 검증
//     uint16_t received_checksum = data[len - 5] | (data[len - 4] << 8);
//     uint16_t calculated_checksum = calculate_checksum(data, len - 5);
    
//     if(received_checksum != calculated_checksum){
//         #if UART_DEBUG
//         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s 체크섬 불일치: got=0x%04X, calc=0x%04X\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, received_checksum, calculated_checksum);
//         #endif
//         return false;
//     }
    
//     // 명령 처리
//     uint8_t *payload = (payload_len > 0) ? &data[6] : NULL;
//     handle_rx_command((CommandType_t)cmd, payload, input_ui16_payload_len);
    
//     return true;
// }

// /**
//  * @brief 명령 핸들러
//  */
// static void handle_rx_command(CommandType_t cmd, uint8_t *payload, uint16_t payload_len){

//     // #define CUSTOM_RX_COMMAND_HANDLER_DEBUG  UART_DEBUG
//     #define CUSTOM_RX_COMMAND_HANDLER_DEBUG  true
    
//     switch(cmd){
//         case CMD_SET_TP1: {
//             if(input_ui16_payload_len != 2){
//                 #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//                 printf("[%s] "COLOR_RED"[오류-ERROR]\t %s CMD_SET_TP1 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
//                 #endif
//                 return;
//             }
//             uint16_t tp1_value = input_p_ui8_payload[0] | (input_p_ui8_payload[1] << 8);  // Little Endian
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s TP1 설정: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp1_value);
//             #endif
//             custom_set_occupancy_tp1(tp1_value);
//             break;
//         }
        
//         case CMD_SET_TP2: {
//             if(input_ui16_payload_len != 8){
//                 #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//                 printf("[%s] "COLOR_RED"[오류-ERROR]\t %s CMD_SET_TP2 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
//                 #endif
//                 return;
//             }
//             uint64_t tp2_value = 0;
//             for(int i = 0; i < 8; i++){
//                 tp2_value |= ((uint64_t)input_p_ui8_payload[i] << (i * 8));
//             }
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s TP2 설정: %llu\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp2_value);
//             #endif
//             custom_set_adc_tp2(tp2_value);
//             break;
//         }
        
//         case CMD_SET_TP1_RECHECK: {
//             if(input_ui16_payload_len != 2){
//                 #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//                 printf("[%s] "COLOR_RED"[오류-ERROR]\t %s CMD_SET_TP1_RECHECK 페이로드 길이 오류: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, input_ui16_payload_len);
//                 #endif
//                 return;
//             }
//             uint16_t tp1_recheck_value = input_p_ui8_payload[0] | (input_p_ui8_payload[1] << 8);  // Little Endian
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s TP1_RECHECK 설정: %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, tp1_recheck_value);
//             #endif
//             custom_set_recheck_tp1(tp1_recheck_value);
//             break;
//         }
        
//         case CMD_GET_SETTINGS: {
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s 설정값 요청 → SETTINGS 전송\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//             #endif
//             custom_request_uart_tx(UART_TX_SETTINGS);
//             break;
//         }
        
//         case CMD_SAVE_NVS: {
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s NVS 저장 요청\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//             #endif
//             write_value_to_nvs();
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_GREEN"[★ RX]\t %s NVS 저장 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//             #endif
//             break;
//         }
        
//         case CMD_RESET: {
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_WHITE"[진행-OK]\t %s 리셋 요청 - 1초 후 재시작\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
//             #endif
//             vTaskDelay(custom_ms_to_delay(1000));
//             esp_restart();
//             break;
//         }
        
//         default: {
//             #if CUSTOM_RX_COMMAND_HANDLER_DEBUG
//             printf("[%s] "COLOR_YELLOW"[경고]\t %s 알 수 없는 명령: 0x%02X\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, cmd);
//             #endif
//             break;
//         }
//     }
// }
