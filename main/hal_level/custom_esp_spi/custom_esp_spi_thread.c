/**
 * @file        custom_spi_thread.c
 * @brief       SPI DATA 모듈
 * @author      T0T
 * @date        2026-01-09
 * @version     1.0.0
 * 
 * @details     SPI를 통해 이미지 데이터를 수신 및 제어 Signal을 송신하는 모듈
 */
#include "custom_esp_spi_thread.h"

/**
 * @brief       custom_esp_spi_thread.c 파일 디버깅 여부
 * @details     custom_esp_spi_thread.c 파일에서 디버깅 Print 사용 여부를 정의합니다
 */
#define SPI_DEBUG         DEBUG
// #define SPI_DEBUG         false

/**
 * @brief       custom_esp_spi_thread.c 디버깅 Tag
 * @details     Debug Print 시 custom_esp_spi_thread.c 파일을 구분하기 위한 Tag
 */
static const char *custom_esp_spi_TAG = "[@]custom_esp_spi_thread.c";

/**
 * @brief       custom_esp_spi_thread.c 파일에서 사용하는 Task Handle
 * @details     custom_spi_image_rx_thread의 Task Handle을 저장
 */
static volatile TaskHandle_t TaskHandle_custom_spi_image_rx_thread = NULL;
static volatile TaskHandle_t TaskHandle_custom_spi_image_rx_process_thread = NULL;

/**
 * @brief       SPI 수신 큐
 * @details     SPI 수신 큐 (uint64_t*)
 */
static mpqs mpqs_spi_image_rx;
/**
 * @brief       SPI Image To App 큐
 * @details     SPI Image To App 큐 (uint64_t*)
 */
static mpqs mpqs_spi_image_to_app;

/*===========================================================================*/
/* 구조체 정의 */
/*===========================================================================*/
/**
 * @brief       SPI 채널 설정 테이블
 * @details     SPI 채널 설정 테이블 (딕셔너리처럼 사용)
 */
static const esccs spi_config_table[] = {
    [SPI_CH_IMAGE] = {
        .mosi_io_num    = SPI_IMAGE_GPIO_MOSI,
        .miso_io_num    = SPI_IMAGE_GPIO_MISO,
        .sclk_io_num    = SPI_IMAGE_GPIO_SCLK,
        .quadwp_io_num  = -1,
        .quadhd_io_num  = -1,
        .mode           = 0,
        .spics_io_num   = SPI_IMAGE_GPIO_CS,
        .queue_size     = 3,
        .flags          = 0,
    },
    // [SPI_CH_CONTROL] = {
    //     .mosi_io_num    = SPI_IMAGE_GPIO_MOSI,
    //     .miso_io_num    = SPI_IMAGE_GPIO_MISO,
    //     .sclk_io_num    = SPI_IMAGE_GPIO_SCLK,
    //     .quadwp_io_num  = -1,
    //     .quadhd_io_num  = -1,
    //     .mode           = 0,
    //     .spics_io_num   = SPI_IMAGE_GPIO_CS,
    //     .queue_size     = 3,
    //     .flags          = 0,
    // },
};

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
bool custom_spi_init(void){

    #define CUSTOM_SPI_INIT_DEBUG         SPI_DEBUG

    // SPI IMAGE 초기세팅
    static spi_bus_config_t spi_image_bus_config;
    spi_image_bus_config.mosi_io_num = spi_config_table[SPI_CH_IMAGE].mosi_io_num;
    spi_image_bus_config.miso_io_num = spi_config_table[SPI_CH_IMAGE].miso_io_num;
    spi_image_bus_config.sclk_io_num = spi_config_table[SPI_CH_IMAGE].sclk_io_num;
    spi_image_bus_config.quadwp_io_num = spi_config_table[SPI_CH_IMAGE].quadwp_io_num;
    spi_image_bus_config.quadhd_io_num = spi_config_table[SPI_CH_IMAGE].quadhd_io_num;

    static spi_slave_interface_config_t spi_image_slave_interface_config;
    spi_image_slave_interface_config.mode = spi_config_table[SPI_CH_IMAGE].mode;
    spi_image_slave_interface_config.spics_io_num = spi_config_table[SPI_CH_IMAGE].spics_io_num;
    spi_image_slave_interface_config.queue_size = spi_config_table[SPI_CH_IMAGE].queue_size;
    spi_image_slave_interface_config.flags = spi_config_table[SPI_CH_IMAGE].flags;

    static int i_xTaskCreate_return_value = 0;

    #if CUSTOM_SPI_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_spi_init() - SPI 파라미터 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(spi_slave_initialize(SPI_IMAGE_RCV_HOST, &spi_image_bus_config, &spi_image_slave_interface_config, SPI_DMA_CH_AUTO) != ESP_OK){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI 파라미터 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_spi_deinit();
        return false;
    }

    // if(!custom_mpqs_init(&mpqs_spi_image_rx, SPI_IMAGE_BUFFER_LENGTH, SPI_IMAGE_BUSTER_SIZE, "mpqs_spi_image_rx")){
    if(!custom_mpqs_init(&mpqs_spi_image_rx, SPI_IMAGE_BUFFER_LENGTH, sizeof(void*), "mpqs_spi_image_rx")){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - mpqs_spi_image_rx Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    
    if(!custom_mpqs_init(&mpqs_spi_image_to_app, SPI_IMAGE_MAX_COUNT, sizeof(void*), "mpqs_spi_image_to_app")){
    // if(!custom_mpqs_init(&mpqs_spi_image_to_app, SPI_IMAGE_MAX_COUNT, (SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS, "mpqs_spi_image_to_app")){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - mpqs_spi_image_to_app Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

    // Thread 생성
    #if CUSTOM_SPI_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    // i_xTaskCreate_return_value = xTaskCreate(custom_spi_image_rx_thread, "SPI_IMAGE_RECEIVE_THREAD", SPI_IMAGE_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &TaskHandle_custom_spi_image_rx_thread);
    // 변경 → CPU 0에 고정
    i_xTaskCreate_return_value = xTaskCreatePinnedToCore(custom_spi_image_rx_thread, "SPI_IMAGE_RECEIVE_THREAD", 
                                    SPI_IMAGE_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, 
                                    &TaskHandle_custom_spi_image_rx_thread, 
                                    0);  // ← CPU 0
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }

    #if CUSTOM_SPI_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_DATA_PROCESS_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    // i_xTaskCreate_return_value = xTaskCreate(custom_spi_image_rx_process_thread, "SPI_IMAGE_RECEIVE_DATA_PROCESS_THREAD", SPI_IMAGE_RECEIVE_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &TaskHandle_custom_spi_image_rx_process_thread);
    // 변경 → CPU 1에 고정
    i_xTaskCreate_return_value = xTaskCreatePinnedToCore(custom_spi_image_rx_process_thread, "SPI_IMAGE_RECEIVE_DATA_PROCESS_THREAD", 
                                SPI_IMAGE_RECEIVE_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, 
                                &TaskHandle_custom_spi_image_rx_process_thread, 
                                1);  // ← CPU 1
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_DATA_PROCESS_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_RECEIVE_DATA_PROCESS_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
    // SPI Image 수신 데이터 재구성 스레드 시작
    
    return true;
}

uint64_t custom_swap_endian_ui64(uint64_t ui64_input_value){
    uint64_t ui64_result = 0;
    for (int i = 0; i < 8; i++) {
        ui64_result <<= 8;
        ui64_result |= (ui64_input_value & 0xFF);
        ui64_input_value >>= 8;
    }
    return ui64_result;
}

static void custom_spi_image_rx_thread(void *arg){

    #define CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG         SPI_DEBUG

    static uint64_t* A_ui64_spi_sendbuf = NULL;
    static uint64_t* A_ui64_spi_recvbuf = NULL;

    // SPI Slave 트랜잭션 생성
    static spi_slave_transaction_t spi_image_slave_transaction;
    spi_image_slave_transaction.length = SPI_IMAGE_SPI_BUSTER_BIT_COUNT;    // (8 * 8 * 5) = 320bit
    
    while(true){
        // SPI Image Queue 존재 Check
        if(mpqs_spi_image_rx.QueueHandle_queue == NULL){
            #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_thread() - mpqs_spi_image_rx.QueueHandle_queue 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
            #endif
        }
        else{
            if(A_ui64_spi_sendbuf == NULL){
                // 초기화
                A_ui64_spi_sendbuf = (uint64_t*)pvPortMalloc(SPI_IMAGE_BUSTER_SIZE);
                if(A_ui64_spi_sendbuf == NULL){
                    // 메모리 할당 실패 처리
                    #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - A_ui64_spi_sendbuf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                    #endif
                    continue; // 다시 반복 시작
                }
                memset(A_ui64_spi_sendbuf, 0, sizeof(A_ui64_spi_sendbuf));
                spi_image_slave_transaction.tx_buffer = A_ui64_spi_sendbuf;
            }
            if(A_ui64_spi_recvbuf == NULL){
                // 초기화
                A_ui64_spi_recvbuf = (uint64_t*)pvPortMalloc(SPI_IMAGE_BUSTER_SIZE);
                if(A_ui64_spi_recvbuf == NULL){
                    // 메모리 할당 실패 처리
                    #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - A_ui64_spi_recvbuf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                    #endif
                    vPortFree(A_ui64_spi_sendbuf);
                    A_ui64_spi_sendbuf = NULL;
                    continue; // 다시 반복 시작
                }
                memset(A_ui64_spi_recvbuf, 0, sizeof(A_ui64_spi_recvbuf));
                spi_image_slave_transaction.rx_buffer = A_ui64_spi_recvbuf;
            }

            // SPI 통신 시작
            if(spi_slave_transmit(SPI_IMAGE_RCV_HOST, &spi_image_slave_transaction, portMAX_DELAY) == ESP_OK){
                // SPI 수신 성공한 경우
                cqrre cqrre_spi_image_rx_send_result = custom_queue_safe_send(&mpqs_spi_image_rx, &A_ui64_spi_recvbuf, 1);
                if(cqrre_spi_image_rx_send_result != QUEUE_IS_READY){
                    #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image 전송 실패 (-> mpqs_spi_image_rx) (cqrre_spi_image_rx_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, cqrre_spi_image_rx_send_result);
                    #endif
                }
                else{
                    #if 0
                    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image 전송 성공 (-> mpqs_spi_image_rx) 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_rx));
                    #endif
                    #if 0
                    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_thread() - A_ui64_spi_recvbuf 내용 : \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                    for (sirse sirse_index = CMD_N_ADDR; sirse_index < SPI_IMAGE_BUSTER_END_ADDRESS; sirse_index++) {
                        printf(" [%d] : 0x%016llX\n", sirse_index, A_ui64_spi_recvbuf[sirse_index]);
                    }
                    #endif
                    vPortFree(A_ui64_spi_sendbuf);
                    A_ui64_spi_sendbuf = NULL;
                    A_ui64_spi_recvbuf = NULL;
                }
            }
            else{
                // SPI 수신 실패한 경우
                #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - SPI 수신 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                #endif
            }
        }

        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, NOTIFY_SHUTDOWN_BIT, &ulNotificationValue, 0) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_UART_RX_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image RX Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                #endif
                break; // 루프 탈출
            }
        }
    } // END while(true)
    TaskHandle_custom_spi_image_rx_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
}

static void custom_spi_image_rx_process_thread(void *arg){

    #define CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG         SPI_DEBUG

    static esp_err_t esp_err = ESP_OK;

    esp_task_wdt_delete(NULL);  // 현재 태스크를 워치독에서 제외

    static uint64_t* A_ui64_spi_recvbuf = NULL;
    static uint64_t* A_ui64_image_data = NULL;

    static uint8_t ui8_receive_image_addr = 0;
    static uint8_t ui8_receive_image_addr_last = 0;
    static bool A_b_addr[SPI_IMAGE_END_ADDRESS] = {false,};
    static uint8_t ui8_lost_count = 0;

    while(true){
        // SPI Image Queue 존재 Check
        if(mpqs_spi_image_to_app.QueueHandle_queue == NULL){
            #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_process_thread() - mpqs_spi_image_to_app.QueueHandle_queue 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
            #endif
        }
        else{
            cqrre cqrre_spi_image_rx_result = custom_queue_safe_receive(&mpqs_spi_image_rx, &A_ui64_spi_recvbuf, 1);
            if(cqrre_spi_image_rx_result == QUEUE_IS_READY){
                // Endian Swap
                for (sirse sirse_index = CMD_N_ADDR; sirse_index < SPI_IMAGE_BUSTER_END_ADDRESS; sirse_index++){
                    A_ui64_spi_recvbuf[sirse_index] = custom_swap_endian_ui64(A_ui64_spi_recvbuf[sirse_index]);
                }
                // #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                #if 0
                printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - A_ui64_spi_recvbuf 내용 : \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                for (sirse sirse_index = CMD_N_ADDR; sirse_index < SPI_IMAGE_BUSTER_END_ADDRESS; sirse_index++) {
                    printf(" [%d] : 0x%016llX\n", sirse_index, A_ui64_spi_recvbuf[sirse_index]);
                }
                #endif

                // CMD가 1이라면 -> WRITE_SIGNAL
                // (1XXXXXXX_XXXXXXXX) 0(NNNNNNNN_NNNNNNNN) 1(NNNNNNNN_NNNNNNN) 2(NNNNNNNN_NNNNNNNN) 3(NNNNNNNN_NNNNNNNN)
                if(A_ui64_spi_recvbuf[CMD_N_ADDR] & 1ULL << 63){
                    // ADDR 확인
                    // (X1111111_XXXXXXXX) 0(NNNNNNNN_NNNNNNNN) 1(NNNNNNNN_NNNNNNN) 2(NNNNNNNN_NNNNNNNN) 3(NNNNNNNN_NNNNNNNN)
                    ui8_receive_image_addr = (A_ui64_spi_recvbuf[CMD_N_ADDR] & (127ULL << 56)) >> 56;

                    // 새로운 이미지로 생성
                    if(ui8_receive_image_addr < ui8_receive_image_addr_last){
                        // Debug
                        if(ui8_lost_count > 0){
                            // A_b_addr 프린트해서 빈 주소 파악
                            #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - 빈 주소 : \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                            for(int i_index = 0; i_index < SPI_IMAGE_END_ADDRESS; i_index++){
                                if(A_b_addr[i_index] == false){
                                    // 빈 주소 파악
                                    printf("[%d] ", i_index);
                                }
                            }
                            printf("\n");
                            #endif
                        }

                        if(A_ui64_image_data != NULL){
                            // Frame 정리 후 Queue 전송 (포인터의 주소를 전달!)
                            cqrre cqrre_spi_image_application_send_result = custom_queue_safe_send(&mpqs_spi_image_to_app, &A_ui64_image_data, 0);
                            if(cqrre_spi_image_application_send_result != QUEUE_IS_READY){
                                #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 실패 (-> mpqs_spi_image_to_app) (cqrre_spi_image_application_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, cqrre_spi_image_application_send_result);
                                #endif
                                // 전송 실패 시에만 메모리 해제
                                vPortFree(A_ui64_image_data);
                                A_ui64_image_data = NULL;
                            }
                            else{
                                #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                                printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 성공 (-> mpqs_spi_image_to_app) 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_to_app));
                                #endif
                                // 전송 성공 시 소유권 이전 (수신측에서 해제)
                                A_ui64_image_data = NULL;
                            }
                        }
                        #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - mpqs_spi_image_rx 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_rx));
                        #endif
                    }

                    if(A_ui64_image_data == NULL){
                        // 초기화
                        A_ui64_image_data = (uint64_t*)pvPortMalloc((SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
                        if(A_ui64_image_data == NULL){
                            // 메모리 할당 실패 처리
                            #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - A_ui64_image_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                            #endif
                            // return ERROR_MALLOC_FAIL;
                            continue; // 다시 반복 시작
                        }
                        memset(A_ui64_image_data, 0, (SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
                        memset(A_b_addr, false, SPI_IMAGE_END_ADDRESS);
                        ui8_lost_count = 0;
                    }

                    if(((ui8_receive_image_addr_last + 1) % SPI_IMAGE_END_ADDRESS) != ui8_receive_image_addr){
                        // 빈공간이 생김
                        ui8_lost_count += (ui8_receive_image_addr + SPI_IMAGE_END_ADDRESS - ui8_receive_image_addr_last - 1) % SPI_IMAGE_END_ADDRESS;
                    }

                    #if 0
                    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - 주소 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, ui8_receive_image_addr);
                    #endif
                    A_b_addr[ui8_receive_image_addr] = true;
                    for(int i_index = 0; i_index < SPI_IMAGE_BUSTER_END_DATA_ARRAY; i_index++){
                        A_ui64_image_data[(SPI_IMAGE_BUSTER_END_DATA_ARRAY * ui8_receive_image_addr) + i_index] = A_ui64_spi_recvbuf[DATA_64BIT_0 + i_index];
                    }
                    ui8_receive_image_addr_last = ui8_receive_image_addr;
                }

                vPortFree(A_ui64_spi_recvbuf);
                A_ui64_spi_recvbuf = NULL;
                taskYIELD();
            }
            else{
                #if 0
                printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_process_thread() - mpqs_spi_image_rx 큐 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                #endif
                vTaskDelay(1);
            }
        }

        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, 0, &ulNotificationValue, 0) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_spi_image_rx_process_thread() - SPI Image RX Process Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                #endif
                break; // 루프 탈출
            }
        }
    } // END while(true)

    #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_spi_image_rx_process_thread() - SPI Image RX Process Thread 종료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #endif

    TaskHandle_custom_spi_image_rx_process_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
} // END adc_buf

// iSENSOR_Mode에서 사용
rgis custom_get_spi_image(void){
    #define CUSTOM_GET_SPI_IMAGE_DEBUG         SPI_IMAGE_DEBUG

    rgis rgis_value = {QUEUE_IS_ERROR, {0,}};
    static uint64_t* ui64_receive_spi_image_value = NULL;

    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    rgis_value.cqrre_value = custom_queue_safe_receive(&mpqs_spi_image_to_app, &ui64_receive_spi_image_value, 1);
    if(rgis_value.cqrre_value == QUEUE_IS_READY){
        #if 1
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_get_spi_image() - ui64_receive_spi_image 내용 : \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
        // for (int i_addr_index = 0; i_addr_index < SPI_IMAGE_END_ADDRESS; i_addr_index++) {
        //     for (sirse sirse_index = CMD_N_ADDR; sirse_index < SPI_IMAGE_BUSTER_END_DATA_ARRAY; sirse_index++) {
        //         printf(" [%d][%d](%d): 0x%016llX\n", i_addr_index, sirse_index, (SPI_IMAGE_BUSTER_END_DATA_ARRAY * i_addr_index) + sirse_index, ui64_receive_spi_image_value[(SPI_IMAGE_BUSTER_END_DATA_ARRAY * i_addr_index) + sirse_index]);
        //     }
        //     printf("\n");
        // }
        for (int i_addr_index = 0; i_addr_index < (SPI_IMAGE_END_ADDRESS * SPI_IMAGE_BUSTER_END_DATA_ARRAY); i_addr_index++) {
            printf("[%d]: 0x%016llX\n", i_addr_index, ui64_receive_spi_image_value[i_addr_index]);
        }
        #endif

        memcpy(rgis_value.ui64_image_value, ui64_receive_spi_image_value, sizeof(rgis_value.ui64_image_value));
        // rgis_value.ui64_image_value = *ui64_receive_spi_image_value;
        vPortFree(ui64_receive_spi_image_value); 
        ui64_receive_spi_image_value = NULL;
    }
    return rgis_value;
}


    // TODO : deinit 필요