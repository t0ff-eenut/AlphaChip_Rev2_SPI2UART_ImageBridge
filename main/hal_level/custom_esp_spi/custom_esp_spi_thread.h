/**
 * @file        custom_esp_spi_thread.h
 * @brief       ESP32의 SPI Pin 할당 및 제어 헤더 파일
 * @author      T0T
 * @date        2026-01-14
 * @version     1.0.0
 *
 * @details     iSENSOR Occupancy Sensor에서 ADC 버퍼 및 설정값을 SPI를 통해 전송하는 모듈
 *              Queue를 통해 전송 요청을 받아서 해당하는 데이터를 직렬화하여 전송
 */
#ifndef CUSTOM_ESP_SPI_THREAD_H
#define CUSTOM_ESP_SPI_THREAD_H

// ADC는 Queue 모듈에 의존 (같은 계층 내 직접 참조)
#include "custom_esp_queue.h"
#include "driver/spi_slave.h"
/**
* @defgroup    SPI_IMAGE_CONFIG 이미지 출력용 SPI 설정 그룹
* @brief       이미지 출력용 SPI 설정
* @details     
* @note        
* @{
*/
    #ifndef SPI_IMAGE_RCV_HOST
        /**
         * @brief       SPI Image 수신 Host
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 SPI2_HOST 설정
         */
        #define SPI_IMAGE_RCV_HOST              SPI2_HOST
    #endif
    #ifndef SPI_IMAGE_GPIO_SCLK
        /**
         * @brief       SPI Image Clock GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_12 설정
         */
        #define SPI_IMAGE_GPIO_SCLK             GPIO_NUM_12
    #endif
    #ifndef SPI_IMAGE_GPIO_MOSI
        /**
         * @brief       SPI Image MOSI GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_11 설정
         */
        #define SPI_IMAGE_GPIO_MOSI             GPIO_NUM_11
    #endif
    #ifndef SPI_IMAGE_GPIO_MISO
        /**
         * @brief       SPI Image MISO GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_13 설정
         */
        #define SPI_IMAGE_GPIO_MISO             GPIO_NUM_13
    #endif
    #ifndef SPI_IMAGE_GPIO_CS
        /**
         * @brief       SPI Image Chip Select GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_10 설정
         */
        #define SPI_IMAGE_GPIO_CS               GPIO_NUM_10
    #endif

    /**
    * @brief       SPI Image 버스 크기
    * @details     SPI Image 수신 Host의 버스 크기를 정의합니다.
    * @note        Max 320 bit
    */
    #define SPI_IMAGE_SPI_BUSTER_BIT_COUNT       (64 * 5)    // bit

/** @} */ // end of SPI_IMAGE_CONFIG

/**
* @defgroup    SPI_CONFIG SPI 설정 그룹
* @brief       SPI 설정
* @details     
* @note        
* @{
*/
    #ifndef SPI_CMD_RCV_HOST
        /**
         * @brief       SPI 명령어 전송 Host
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 SPI3_HOST 설정
         */
        #define SPI_CMD_RCV_HOST                SPI3_HOST
    #endif
    #ifndef SPI_CMD_GPIO_SCLK
        /**
         * @brief       SPI 명령어 Clock GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_36 설정
         */
        #define SPI_CMD_GPIO_SCLK               GPIO_NUM_36
    #endif
    #ifndef SPI_CMD_GPIO_MOSI
        /**
         * @brief       SPI 명령어 MOSI GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_35 설정
         */
        #define SPI_CMD_GPIO_MOSI               GPIO_NUM_35
    #endif
    #ifndef SPI_CMD_GPIO_MISO
        /**
         * @brief       SPI 명령어 MISO GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_37 설정
         */
        #define SPI_CMD_GPIO_MISO               GPIO_NUM_37
    #endif
    #ifndef SPI_CMD_GPIO_CS
        /**
         * @brief       SPI 명령어 Chip Select GPIO Pin 번호
         * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_39 설정
         */
        #define SPI_CMD_GPIO_CS                 GPIO_NUM_39
    #endif
    /**
     * @brief       SPI 명령어 버스 크기
     * @details     SPI 명령어 전송 Host의 버스 크기를 정의합니다
     */
    #define SPI_CMD_SPI_BUSTER_BIT_COUNT         8           // bit

/** @} */ // end of SPI_CONFIG

// /**
// * @defgroup    SPI_COMMEND_CONFIG SPI 커멘드 설정 그룹
// * @brief       SPI 커멘드 설정
// * @details     
// * @note        
// * @{
// */
//     /**
//     * @brief       SPI 명령어 버스의 첫 번째 단어
//     * @details     SPI 명령어 버스의 첫 번째 단어를 정의합니다
//     * @note        0xA5 = 0b 1010_0101
//     */
//     #define SPI_FIRST_WORD                  0XA5
//     // /**
//     //  * @def         SPI_WRONG_WORD
//     //  * @brief       SPI 명령어 버스의 잘못된 단어
//     //  * @details     SPI 명령어 버스의 잘못된 단어를 정의합니다
//     //  */
//     // #define SPI_WRONG_WORD                  0X88
// /** @} */ // end of SPI_COMMEND_CONFIG

#define IMAGE_WIDTH     64
#define IMAGE_HEIGHT    64

#define SPI_IMAGE_BUSTER_END_ADDRESS                        5
/**
* @brief       SPI Image 버스 크기
* @details     SPI Image 버스 크기를 정의합니다
* @todo     
*/
#define SPI_IMAGE_BUSTER_SIZE                               sizeof(uint64_t) * SPI_IMAGE_BUSTER_END_ADDRESS
/**
* @brief       
* @details     
* @todo     
*/
#define SPI_IMAGE_BUSTER_END_DATA_ARRAY                     4
/**
* @brief       
* @details     
* @todo     
*/
#define SPI_IMAGE_BUSTER_DATA_SIZE                          sizeof(uint64_t) * SPI_IMAGE_BUSTER_END_DATA_ARRAY
/**
* @brief       
* @details     
* @todo     
*/
#define SPI_IMAGE_END_ADDRESS                               128
/**
* @brief       
* @details     
* @todo     
*/
#define SPI_IMAGE_MAX_COUNT                                 10


#ifndef SPI_IMAGE_BUFFER_LENGTH
    /**
    * @brief       SPI 이미지 수신 버퍼 크기
    * @details     SPI 이미지 수신 버퍼 크기 (ESP-IDF 최소 요구: > 128)
    */
    #define SPI_IMAGE_BUFFER_LENGTH                            SPI_IMAGE_END_ADDRESS * SPI_IMAGE_MAX_COUNT             // SPI 이미지 수신 버퍼 크기 (ESP-IDF 최소 요구: > 128)
#endif

/**
 * @brief       SPI Image 수신 Thread Stack Size
 * @details     project_top.h에서 정의되지 않은 경우, 기본값 8KB 설정
 * @warning     사이즈 변경하지 말 것
 */
#define SPI_IMAGE_RX_STACK_SIZE                     (1024 * 8)
/**
 * @brief       SPI Image 수신 데이터 재구성 Thread Stack Size
 * @details     project_top.h에서 정의되지 않은 경우, 기본값 8KB 설정
 * @warning     사이즈 변경하지 말 것
 * @note        SPI_IMAGE_BUSTER_DATA_SIZE = sizeof(uint64_t) * 4 = 32
 *              SPI_IMAGE_END_ADDRESS = 128
 *              배열 요소 수 = 32 * 128 = 4096 개의 uint64_t
 *              스택 사용량 = 4096 × 8 bytes = 32,768 bytes = 32KB!!
 *              스택 크기 = 8KB ← 부족!
 */
// #define SPI_IMAGE_RECEIVE_DATA_PROCESS_STACK_SIZE    (1024 * 8)      8KB
#define SPI_IMAGE_RECEIVE_DATA_PROCESS_STACK_SIZE    (1024 * 48)
// /**
//  * @brief       SPI 명령어 전송 Thread Stack Size
//  * @details     project_top.h에서 정의되지 않은 경우, 기본값 3KB 설정
//  * @warning     사이즈 변경하지 말 것
//  */
// #define SPI_CMD_TX_STACK_SIZE                       (1024 * 3)

/*===========================================================================*/
/* 열거형 정의 */
/*===========================================================================*/
/**
 * @enum        escie
 * @typedef     esp_spi_channel_id_enum
 * @brief       ESP SPI Channel ID Enum
 * @details     
 */
typedef enum esp_spi_channel_id_enum{
    SPI_CH_IMAGE,     /**< 0 : Image Channel */
    SPI_CH_CONTROL,   /**< 1 : Control Channel */
    SPI_CH_MAX        /**< 2 : Max Channel */
} escie;

/**
 * @enum        sirse
 * @typedef     spi_image_rx_structure_enum
 * @brief       SPI Image 수신 버퍼 구조 Enum
 * @details     
 */
typedef enum spi_image_rx_structure_enum{
    CMD_N_ADDR,         /**< 0 : Read RX Buster */
    DATA_64BIT_0,       /**< 1 : 64bit Data */
    DATA_64BIT_1,       /**< 2 : 64bit Data */
    DATA_64BIT_2,       /**< 3 : 64bit Data */
    DATA_64BIT_3,       /**< 4 : 64bit Data */
    // BUSTER_END_ADDRESS  /**< 5 : Buster End Address */
} sirse;

/*===========================================================================*/
/* 구조체 정의 */
/*===========================================================================*/
 /**
 * @struct      esccs
 * @typedef     esp_spi_channel_config_struct
 * @brief       ESP SPI Channel Config Struct
 * @details     
 */
typedef struct esp_spi_channel_config_struct{
    int mosi_io_num;          /**< MOSI GPIO 핀 번호 */
    int miso_io_num;          /**< MISO GPIO 핀 번호 */
    int sclk_io_num;          /**< SCLK GPIO 핀 번호 */
    int quadwp_io_num;        /**< QUADWP GPIO 핀 번호 */
    int quadhd_io_num;        /**< QUADHD GPIO 핀 번호 */
    int mode;                 /**< SPI 모드 */
    int spics_io_num;         /**< SPI Chip Select GPIO 핀 번호 */
    int queue_size;           /**< SPI 큐 사이즈 */
    int flags;                /**< SPI 플래그 */
} esccs;

// /**
//  * @struct      srics
//  * @typedef     spi_rx_image_chunk_struct
//  * @brief       SPI 이미지 수신 Chunk 구조체
//  * @details     SPI 이미지 수신 Chunk 구조체
//  */
// typedef struct spi_rx_image_chunk_struct{
//     uint8_t ui8_address;
//     uint64_t* ui64_buster_rx_data;
// } srics;

/**
 * @struct      rgsis
 * @typedef     return_get_spi_image_struct
 * @brief       
 * @details     
 */
typedef struct return_get_spi_image_struct{
    cqrre cqrre_value;                      /**< SQueue 상태 확인 값 */
    uint64_t ui64_image_value[(SPI_IMAGE_BUSTER_END_DATA_ARRAY) * SPI_IMAGE_END_ADDRESS];             /**< SPI 이미지 값 */
} rgsis;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
/**
 * @brief       custom_spi_init() Function
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     SPI 초기화 함수
 */
bool custom_spi_init(void);

/**
 * @brief       custom_swap_endian_ui64() Function
 * @param[in]   uint64_t    input_ui64
 * @return      uint64_t    ui64_result
 * @details     uint64_t를 뒤집는 함수
 * @note        
            // ui64_spi_recv_q_recv_320bit[0] : 0000000000000080 ::
            // ui64_spi_recv_q_recv_320bit[0] : 0000000000000081 :: 
            // 타겟
            // ui64_spi_recv_q_recv_320bit[0] : 8000000000000000 ::
            // ui64_spi_recv_q_recv_320bit[0] : 8100000000000000 :: 
 */
uint64_t custom_swap_endian_ui64(uint64_t input_ui64);

/**
 * @brief       spi_image_rx_thread() Function
 * @attention   static
 * @param[in]   void
 * @return      void
 * @details     SPI Image 수신 Thread
 */
static void custom_spi_image_rx_thread(void *arg);

/**
 * @brief       custom_spi_image_rx_process_thread() Function
 * @attention   static
 * @param[in]   void
 * @return      void
 * @details     SPI Image 수신 데이터 재구성 Thread
 * @note        
                // SPI 수신 데이터 버퍼(128 * 5) = 1Frame
                //////////          //
                // 64bit            //
                // 64bit            //
                // 64bit    320bit  //
                // 64bit            //
                // 64bit            //
                //////////          //
                //  .               //
                //  .               //  128개   =   1Frame
                //  .               //
                //////////          //
                // 64bit            //
                // 64bit            //
                // 64bit    320bit  //
                // 64bit            //
                // 64bit            //
                //////////          //
 */
static void custom_spi_image_rx_process_thread(void *arg);

/**
 * @brief       custom_get_spi_image() Function
 * @param[in]   void
 * @return      rgis
 * @details     SPI Image 전송
 */
rgis custom_get_spi_image(void);

#endif