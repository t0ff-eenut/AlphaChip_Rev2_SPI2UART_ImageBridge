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
 * @brief       SPI 수신 큐
 * @details     SPI 수신 큐 (uint64_t*)
 */
static mpqs mpqs_spi_image_application;

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

    if(!custom_mpqs_init(&mpqs_spi_image_rx, SPI_IMAGE_BUFFER_LENGTH, SPI_IMAGE_BUSTER_SIZE, "mpqs_spi_image_rx")){
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
    
    // if(!custom_mpqs_init(&mpqs_spi_image_application, SPI_IMAGE_BUFFER_LENGTH, sizeof(void*), "mpqs_spi_image_application")){
    if(!custom_mpqs_init(&mpqs_spi_image_application, SPI_IMAGE_MAX_COUNT, (SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS, "mpqs_spi_image_application")){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - mpqs_spi_image_application Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    i_xTaskCreate_return_value = xTaskCreate(custom_spi_image_rx_thread, "SPI_IMAGE_receive_THREAD", SPI_IMAGE_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &TaskHandle_custom_spi_image_rx_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    i_xTaskCreate_return_value = xTaskCreate(custom_spi_image_rx_process_thread, "SPI_IMAGE_receive_DATA_PROCESS_THREAD", SPI_IMAGE_RECEIVE_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &TaskHandle_custom_spi_image_rx_process_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_SPI_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_spi_init() - SPI_IMAGE_receive_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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

    // 속도를 위해 정적배열 사용
    uint64_t A_ui64_spi_sendbuf[5] = {0};                                                           // SPI 전송 버퍼(64 * 5) = 320bit    
    uint64_t A_ui64_spi_recvbuf[5] = {0};                                                           // SPI 수신 버퍼(64 * 5) = 320bit

    // SPI Slave 트랜잭션 생성
    static spi_slave_transaction_t spi_image_slave_transaction;
    spi_image_slave_transaction.length = SPI_IMAGE_SPI_BUSTER_BIT_COUNT;    // (8 * 8 * 5) = 320bit
    spi_image_slave_transaction.tx_buffer = A_ui64_spi_sendbuf;
    spi_image_slave_transaction.rx_buffer = A_ui64_spi_recvbuf;
    
    while(true){
        // SPI Image Queue 존재 Check
        if(mpqs_spi_image_rx.QueueHandle_queue == NULL){
            #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_thread() - mpqs_spi_image_rx.QueueHandle_queue 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
            #endif
        }
        else{
            memset(A_ui64_spi_sendbuf, 0, SPI_IMAGE_BUSTER_SIZE);
            memset(A_ui64_spi_recvbuf, 0, SPI_IMAGE_BUSTER_SIZE);

            // SPI 통신 시작
            if(spi_slave_transmit(SPI_IMAGE_RCV_HOST, &spi_image_slave_transaction, portMAX_DELAY) == ESP_OK){
                cqrre cqrre_spi_image_rx_send_result = custom_queue_safe_send(&mpqs_spi_image_rx, A_ui64_spi_recvbuf, 1);
                if(cqrre_spi_image_rx_send_result != QUEUE_IS_READY){
                    #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image 전송 실패 (-> mpqs_spi_image_rx) (cqrre_spi_image_rx_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, cqrre_spi_image_rx_send_result);
                    #endif
                }
                // else{
                //     #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                //     printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image 전송 성공 (-> mpqs_spi_image_rx) 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_rx));
                //     #endif
                // }
            }
            else{
                // 실패한 경우
                #if CUSTOM_SPI_IMAGE_RX_THREAD_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_thread() - SPI Image 수신 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
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
        // vTaskDelay(1);
    } // END while(true)
    TaskHandle_custom_spi_image_rx_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
}

static void custom_spi_image_rx_process_thread(void *arg){

    #define CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG         SPI_DEBUG

    esp_err_t esp_err = ESP_OK;

    // uint64_t* A_ui64_image_data = (uint64_t*)malloc((SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
    // if(A_ui64_image_data == NULL){
    //     // 메모리 할당 실패 처리
    //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
    //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - A_ui64_image_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    //     #endif
    //     // return ERROR_MALLOC_FAIL;
    // }
    // memset(A_ui64_image_data, 0, (SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
    // // uint64_t* A_ui64_image_data = NULL;
    uint64_t A_ui64_image_data[(SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS] = {0,};


    uint8_t ui8_receive_image_addr = 0;
    // uint8_t ui8_receive_image_addr_last = SPI_IMAGE_END_ADDRESS - 1;
    uint8_t ui8_receive_image_addr_last = 0;
    bool A_b_addr[SPI_IMAGE_END_ADDRESS] = {false,};
    uint8_t ui8_lost_count = 0;

    while(true){
        // SPI Image Queue 존재 Check
        if(mpqs_spi_image_application.QueueHandle_queue == NULL){
            #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
            printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_process_thread() - mpqs_spi_image_application.QueueHandle_queue 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
            #endif
        }else{
            // 큐에서 40바이트 데이터를 받을 배열
            uint64_t A_ui64_spi_recvbuf[5] = {0};
            // cqrre cqrre_spi_image_rx_result = custom_queue_safe_receive(&mpqs_spi_image_rx, A_ui64_spi_recvbuf, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
            cqrre cqrre_spi_image_rx_result = custom_queue_safe_receive(&mpqs_spi_image_rx, A_ui64_spi_recvbuf, 1);
            if(cqrre_spi_image_rx_result == QUEUE_IS_READY){
            // 큐에서 데이터 수신, Mutex 내부에서 타임아웃 0으로 설정
            // BaseType_t BaseType_receive_result = xQueueReceive(mpqs_spi_image_rx.QueueHandle_queue, A_ui64_spi_recvbuf, 1);
            // if(BaseType_receive_result == pdPASS){

                // Endian Swap
                for (sirse sirse_index = 0; sirse_index < SPI_IMAGE_BUSTER_END_ADDRESS; sirse_index++){
                    A_ui64_spi_recvbuf[sirse_index] = custom_swap_endian_ui64(A_ui64_spi_recvbuf[sirse_index]);
                }

                // #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                // printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - A_ui64_spi_recvbuf 내용 : \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                // for (sirse sirse_index = 0; sirse_index < SPI_IMAGE_BUSTER_END_ADDRESS; sirse_index++) {
                //     printf(" [%d] : 0x%016llX\n", sirse_index, A_ui64_spi_recvbuf[sirse_index]);
                // }
                // #endif

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

                        // Frame 정리 후 Queue 전송
                        // cqrre cqrre_spi_image_application_send_result = custom_queue_safe_send(&mpqs_spi_image_application, A_ui64_image_data, 0);
                        // if(cqrre_spi_image_application_send_result != QUEUE_IS_READY){
                        //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                        //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 실패 (-> mpqs_spi_image_application) (cqrre_spi_image_application_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, cqrre_spi_image_application_send_result);
                        //     #endif
                        //     // free(A_ui64_image_data);
                        // }
                        // else{
                        //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                        //     printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 성공 (-> mpqs_spi_image_application) 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_application));
                        //     #endif
                        // }

                        
                        #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                        printf("개수 : %d\n", custom_queue_safe_messages_waiting(&mpqs_spi_image_rx));
                        #endif


                        // A_ui64_image_data = NULL;

                        // 초기화
                        // A_ui64_image_data = (uint64_t*)malloc((SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
                        // if(A_ui64_image_data == NULL){
                        //     // 메모리 할당 실패 처리
                        //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                        //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - A_ui64_image_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                        //     #endif
                        //     // return ERROR_MALLOC_FAIL;
                        // }
                        memset(A_ui64_image_data, 0, (SPI_IMAGE_BUSTER_DATA_SIZE) * SPI_IMAGE_END_ADDRESS);
                        memset(A_b_addr, false, SPI_IMAGE_END_ADDRESS);
                        ui8_lost_count = 0;
                    }

                    if(((ui8_receive_image_addr_last + 1) % SPI_IMAGE_END_ADDRESS) != ui8_receive_image_addr){
                        // 빈공간이 생김
                        ui8_lost_count += (ui8_receive_image_addr + SPI_IMAGE_END_ADDRESS - ui8_receive_image_addr_last - 1) % SPI_IMAGE_END_ADDRESS;
                    }


                    // #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                    // printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - 주소 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, ui8_receive_image_addr);
                    // #endif
                    A_b_addr[ui8_receive_image_addr] = true;
                    for(int i_index = 0; i_index < SPI_IMAGE_BUSTER_END_DATA_ARRAY; i_index++){
                        A_ui64_image_data[(SPI_IMAGE_BUSTER_END_DATA_ARRAY * ui8_receive_image_addr) + i_index] = A_ui64_spi_recvbuf[DATA_64BIT_0 + i_index];
                    }
                    ui8_receive_image_addr_last = ui8_receive_image_addr;
                }
            }
            else{
                #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Thread] custom_spi_image_rx_process_thread() - mpqs_spi_image_rx 큐 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
                #endif
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
        // vTaskDelay(1);
    } // END while(true)

    #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_spi_image_rx_process_thread() - SPI Image RX Process Thread 종료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG);
    #endif

    TaskHandle_custom_spi_image_rx_process_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
} // END adc_buf