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
// #define SPI_DEBUG         DEBUG
#define SPI_DEBUG         false

/**
 * @brief       custom_esp_spi_thread.c 디버깅 Tag
 * @details     Debug Print 시 custom_esp_spi_thread.c 파일을 구분하기 위한 Tag
 */
static const char *custom_esp_spi_TAG = "[@]custom_esp_spi_thread.c";

/**
 * @brief       custom_esp_spi_thread.c 파일에서 사용하는 Task Handle
 * @details     custom_spi_thread의 Task Handle을 저장
 */
static volatile TaskHandle_t TaskHandle_custom_spi_thread = NULL;

/**
 * @brief       SPI 수신 큐
 * @details     SPI 수신 큐 (utrs)
 */
static mpqs mpqs_spi_rx_request;


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

    // SPI Image 수신 스레드 시작 
    xTaskCreate(spi_image_rx_thread, "SPI_IMAGE_RECIVE_THREAD", SPI_IMAGE_RX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    // // SPI Image 수신 데이터 재구성 스레드 시작
    // xTaskCreate(spi_image_rx_data_process, "SPI_IMAGE_RECIVE_DATA_PROCESS_THREAD", SPI_IMAGE_RECIVE_DATA_PROCESS_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);
    // // SPI CMD 송신 스레드 시작
    // xTaskCreate(uart_recive_cmd_spi_tx_thread, "SPI_SEND_THREAD", SPI_CMD_TX_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, NULL);

    return true;

}

static void spi_image_rx_thread(void *arg){

    #define SPI_IMAGE_RX_THREAD_DEBUG         SPI_DEBUG

    uint64_t A_ui64_spi_sendbuf[5] = {0};                                                           // SPI 전송 버퍼(64 * 5) = 320bit    
    uint64_t A_ui64_spi_recvbuf[5] = {0};                                                           // SPI 수신 버퍼(64 * 5) = 320bit

    // SPI Slave 트랜잭션 생성
    spi_slave_transaction_t spi_image_transaction;
    spi_image_transaction.length    = SPI_IMAGE_SPI_BUSTER_SIZE;                                    // (8 * 8 * 5) = 320bit
    spi_image_transaction.tx_buffer = A_ui64_spi_sendbuf;                                           // 전송 버퍼 연결
    spi_image_transaction.rx_buffer = A_ui64_spi_recvbuf;                                           // 수신 버퍼 연결

    // SPI 수신 데이터(320bit) 큐에 입력
    while(1){   // Thread 반복
        // SPI 이미지 데이터 수신 공간 초기화
        memset(A_ui64_spi_recvbuf, 0, (SPI_IMAGE_SPI_BUSTER_SIZE / 8));                                        // Read Buffer Clear
        // SPI 통신 시작
        if(spi_slave_transmit(SPI_IMAGE_RCV_HOST, &spi_image_transaction, portMAX_DELAY) == ESP_OK){
            // Queue에 입력 (Mutex Lock)
            xQueueSend(Q_ui64_spi_recive_image_320bit, A_ui64_spi_recvbuf, 0);
        }
        else{
            // 실패한 경우
        }
    }
}