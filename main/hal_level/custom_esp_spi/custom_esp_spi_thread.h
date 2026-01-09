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
    #define SPI_IMAGE_SPI_BUSTER_SIZE       (64 * 5)    // bit

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
    #define SPI_CMD_SPI_BUSTER_SIZE         8           // bit

/** @} */ // end of SPI_CONFIG

/**
* @defgroup    SPI_COMMEND_CONFIG SPI 커멘드 설정 그룹
* @brief       SPI 커멘드 설정
* @details     
* @note        
* @{
*/
    /**
    * @brief       SPI 명령어 버스의 첫 번째 단어
    * @details     SPI 명령어 버스의 첫 번째 단어를 정의합니다
    * @note        0xA5 = 0b 1010_0101
    */
    #define SPI_FIRST_WORD                  0XA5
    // /**
    //  * @def         SPI_WRONG_WORD
    //  * @brief       SPI 명령어 버스의 잘못된 단어
    //  * @details     SPI 명령어 버스의 잘못된 단어를 정의합니다
    //  */
    // #define SPI_WRONG_WORD                  0X88
/** @} */ // end of SPI_COMMEND_CONFIG

/**
 * @brief       SPI Image 수신 Thread Stack Size
 * @details     project_top.h에서 정의되지 않은 경우, 기본값 8KB 설정
 * @warning     사이즈 변경하지 말 것
 */
#define SPI_IMAGE_RX_STACK_SIZE                     (1024 * 8)
// /**
//  * @brief       SPI Image 수신 Thread Stack Size
//  * @details     project_top.h에서 정의되지 않은 경우, 기본값 8KB 설정
//  * @warning     사이즈 변경하지 말 것
//  */
// #define SPI_IMAGE_RECIVE_DATA_PROCESS_STACK_SIZE    (1024 * 8)
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
 * @brief       spi_image_rx_thread() Function
 * @attention   static
 * @param[in]   void
 * @return      void
 * @details     SPI Image 수신 Thread
 */
static void spi_image_rx_thread(void *arg);

#endif