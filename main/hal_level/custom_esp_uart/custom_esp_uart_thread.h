/**
 * @file        custom_esp_uart_thread.h
 * @brief       ESP32의 UART Pin 할당 및 제어 헤더 파일
 * @author      T0T
 * @date        2025-12-04
 * @version     1.0.0
 *
 * @details     iSENSOR Occupancy Sensor에서 ADC 버퍼 및 설정값을 UART를 통해 전송하는 모듈
 *              Queue를 통해 전송 요청을 받아서 해당하는 데이터를 직렬화하여 전송
 */
#ifndef CUSTOM_ESP_UART_THREAD_H
#define CUSTOM_ESP_UART_THREAD_H

// UART는 Queue 모듈에 의존 (같은 계층 내 직접 참조)
#include "custom_esp_queue.h"

// UART
#include "driver/uart.h"

/*===========================================================================*/
/* 메크로 정의 */
/*===========================================================================*/
/**
 * @defgroup    BAUD_RATE 보레이트 설정 그룹
 * @brief       프로젝트에서 사용되는 UART Baud Rate 환경 변수들
 * @details     
 * @{
 */
    /**
     * @brief       115200 Baud Rate
     * @details     115200 Baud Rate
     */
    #define BAUD_RATE_115200    115200
    /**
     * @brief       230400 Baud Rate
     * @details     230400 Baud Rate
     */
    #define BAUD_RATE_230400    230400
    /**
     * @brief       460800 Baud Rate
     * @details     460800 Baud Rate
     */
    #define BAUD_RATE_460800    460800
    /**
     * @brief       500000 Baud Rate
     * @details     500000 Baud Rate
     */
    #define BAUD_RATE_500000    500000
    /**
     * @brief       576000 Baud Rate
     * @details     576000 Baud Rate
     */
    #define BAUD_RATE_576000    576000
    /**
     * @brief       921600 Baud Rate
     * @details     921600 Baud Rate
     */
    #define BAUD_RATE_921600    921600
    /**
     * @brief       1000000 Baud Rate
     * @details     1000000 Baud Rate
     */
    #define BAUD_RATE_1000000   1000000
    /**
     * @brief       1152000 Baud Rate
     * @details     1152000 Baud Rate
     */
    #define BAUD_RATE_1152000   1152000
    /**
     * @brief       1500000 Baud Rate
     * @details     1500000 Baud Rate
     */
    #define BAUD_RATE_1500000   1500000
    /**
     * @brief       2000000 Baud Rate
     * @details     2000000 Baud Rate
     */
    #define BAUD_RATE_2000000   2000000
    /**
     * @brief       2500000 Baud Rate
     * @details     2500000 Baud Rate
     */
    #define BAUD_RATE_2500000   2500000
    /**
     * @brief       3000000 Baud Rate
     * @details     3000000 Baud Rate
     */
    #define BAUD_RATE_3000000   3000000
    /**
     * @brief       3500000 Baud Rate
     * @details     3500000 Baud Rate
     */
    #define BAUD_RATE_3500000   3500000
    /**
     * @brief       4000000 Baud Rate
     * @details     4000000 Baud Rate
     */
    #define BAUD_RATE_4000000   4000000

    #ifndef DEBUG_UART_PORT
        /**
        * @brief       Debug Port Number
        * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 UART_NUM_1 설정
        */
        #define DEBUG_UART_PORT             UART_NUM_1
    #endif
    #ifndef DEBUG_TXD_GPIO_NUM
        /**
        * @brief       Debug Port TXD GPIO Number
        * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_1 설정
        */
        #define DEBUG_TXD_GPIO_NUM          GPIO_NUM_1
    #endif
    #ifndef DEBUG_RXD_GPIO_NUM
        /**
        * @brief       Debug Port RXD GPIO Number
        * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_2 설정
        */
        #define DEBUG_RXD_GPIO_NUM          GPIO_NUM_2
    #endif
    /**
     * @brief       Debug Port Baud Rate
     * @details     Debug Port에서 사용할 Baud Rate
     */
    #define DEBUG_PORT_BAUD_RATE_SEL        BAUD_RATE_2000000

    // #ifndef DEBUG_UART_PORT
    //     /**
    //     * @brief       Debug Port Number
    //     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 UART_NUM_1 설정
    //     */
    //     #define DEBUG_UART_PORT             UART_NUM_1
    // #endif
    // #ifndef DEBUG_RXD_GPIO_NUM
    //     /**
    //     * @brief       Debug Port RXD GPIO Number
    //     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_6 설정
    //     */
    //     #define DEBUG_RXD_GPIO_NUM          GPIO_NUM_6
    // #endif
    // #ifndef DEBUG_TXD_GPIO_NUM
    //     /**
    //     * @brief       Debug Port TXD GPIO Number
    //     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_7 설정
    //     */
    //     #define DEBUG_TXD_GPIO_NUM          GPIO_NUM_7
    // #endif

    /**
     * @brief       Upload Log Port Baud Rate
     * @details     Upload Log Port에서 사용할 Baud Rate
     */
    #define UPLOAD_LOG_PORT_BAUD_RATE_SEL   BAUD_RATE_2000000
/** @} */ // end of BAUD_RATE

/**
* @brief       
* @details     
* @todo     
*/
#define UART_IMAGE_MAX_COUNT    10

/**
 * @defgroup    UART_PROTOCOL 프로토콜 설정 그룹
 * @brief       프로젝트에서 사용되는 UART 프로토콜 관련 환경 변수들
 * @details     
 * @{
 */
    // 프로토콜 관련 정의
    // 멀티바이트 동기 패턴 (데이터 충돌 방지)
    /**
     * @brief       UART STX Pattern Size
     * @details     UART 전송 시작 Signal 길이
     */
    #define UART_STX_PATTERN_SIZE            2
    /**
     * @brief       UART STX Pattern
     * @details     UART 전송 시작 Signal
                    멀티바이트 동기 패턴 (데이터 충돌 방지)
     */
    #define UART_STX_PATTERN                 {0xAA, 0x55}
    /**
     * @brief       
     * @details     
     */
    #define UART_FRAME_HEIGHT_PACKIT_SIZE          1
    /**
     * @brief       
     * @details     
     */
    #define UART_FRAME_WIDTH_PACKIT_SIZE           1

    /**
     * @brief       UART Checksum Size
     * @details     UART 체크섬 크기
     */
    #define UART_CHECKSUM_PACKIT_SIZE               2
    /**
     * @brief       UART ETX Pattern Size
     * @details     UART 전송 종료 Signal 길이
     */
    #define UART_ETX_PATTERN_SIZE            2
    /**
     * @brief       UART ETX Pattern
     * @details     UART 전송 종료 Signal
     */
    #define UART_ETX_PATTERN                 {0x55, 0xAA}
    // /**
    //  * @brief       UART Minimum Size
    //  * @details     UART 최소 크기
    //  */
    #define UART_DEFAULT_TOTAL_SIZE          UART_STX_PATTERN_SIZE + UART_FRAME_HEIGHT_PACKIT_SIZE + UART_FRAME_WIDTH_PACKIT_SIZE + UART_CHECKSUM_PACKIT_SIZE + UART_ETX_PATTERN_SIZE
    // STX/ETX 패턴은 custom_esp_uart_thread.c 내부에서만 사용 (static)
    /**
     * @brief       UART Maximum Payload Size
     * @details     UART 최대 페이로드 크기
     */
    #define UART_MAX_PAYLOAD_SIZE            64*64   // 최대 페이로드 크기
/** @} */ // end of UART_PROTOCOL

#ifndef UART_TX_THREAD_STACK_SIZE
    /**
     * @brief       UART TX Thread Stack Size
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 4KB 설정
     * @warning     사이즈 변경하지 말 것
     */
    #define UART_TX_THREAD_STACK_SIZE           (1024 * 4)
#endif

/*===========================================================================*/
/* 열거형 정의 */
/*===========================================================================*/
/**
 * @enum        eucie
 * @typedef     esp_uart_channel_id_enum
 * @brief       ESP UART Channel ID Enum
 * @details     
 */
typedef enum esp_uart_channel_id_enum{
    UART_CH_DEBUG,     /**< 0 : Debug Channel */
    UART_CH_UPLOAD,    /**< 1 : Upload Channel */
    UART_CH_MAX        /**< 2 : Max Channel */
} eucie;

 /**
 * @enum        upsle
 * @typedef     uart_process_switch_level_enum
 * @brief       UART Process Switch Level Enum
 * @details     
 */
typedef enum uart_process_switch_level_enum{
    QUEUE_WAIT,                         /**< 0 : Queue Wait */
    DATA_PREPARE,                       /**< 1 : Data Prepare */
    UART_TRANSMIT                       /**< 2 : UART Transmit */
} upsle;

/*===========================================================================*/
/* 구조체 정의 */
/*===========================================================================*/
 /**
 * @struct      euccs
 * @typedef     esp_uart_channel_config_struct
 * @brief       ESP UART Channel Config Struct
 * @details     
 */
// UART 채널 설정 구조체
typedef struct esp_uart_channel_config_struct{
    int                     int_uart_port;          // UART 포트 번호
    int                     int_baud_rate;          // 보드레이트
    uart_word_length_t      uart_word_length;       // 데이터 비트
    uart_parity_t           uart_parity;            // 패리티
    uart_stop_bits_t        uart_stop_bits;         // 정지 비트
    uart_hw_flowcontrol_t   uart_hw_flowcontrol;    // 하드웨어 플로우 제어
    uart_sclk_t             uart_sclk;              // 클럭 소스
} euccs;

/**
 * @struct      send_spi_image_to_app_struct
 * @typedef     send_spi_image_to_app_struct
 * @brief       
 * @details     
 */
typedef struct receive_uart_image_from_app_struct{
    uint8_t ui8_height;
    uint8_t ui8_width;
    uint8_t** A_ui8_uart_frame_buf;              /**< 이미지 버퍼 */
} ruifas;

// typedef struct return_set_uart_image_struct{
//     uint8_t ui8_height;
//     uint8_t ui8_width;
//     uint8_t A_ui8_uart_frame_buf[64][64];              /**< 이미지 버퍼 */
// } rsuis;

/**
 * @struct      return_get_uart_image_struct
 * @typedef     return_get_uart_image_struct
 * @brief       
 * @details     
 */
typedef struct return_get_uart_image_struct{
    cqrre cqrre_value;                      /**< SQueue 상태 확인 값 */
    uint8_t ui8_height;
    uint8_t ui8_width;
    uint8_t A_ui8_uart_frame_buf[64][64];              /**< 이미지 버퍼 */
} rguis;

#pragma pack(push, 1)
/**
 * @struct      uart_frame_header_t
 * @typedef     uart_frame_header_t
 * @brief       
 * @details     
 */
typedef struct uart_tx_frame_struct{
    uint8_t ui8_stx[UART_STX_PATTERN_SIZE];    // 0xAA, 0x55
    uint8_t ui8_width;              // 64
    uint8_t ui8_height;             // 64
    uint8_t A_ui8_uart_frame_buf[UART_MAX_PAYLOAD_SIZE];              /**< 이미지 버퍼 */
    uint16_t ui16_checksum;           // XOR 체크섬 (선택)
    uint8_t ui8_etx[UART_ETX_PATTERN_SIZE];      // 0x55, 0xAA
}utfs;
#pragma pack(pop)

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
/**
 * @brief       custom_uart_init() Function
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     UART 초기화 함수
 */
bool custom_uart_init(void);

/**
 * @brief       custom_set_uart_image() Function
 * @param[in]   ruifas input_ruifas
 * @return      cqrre
 * @details     
 */
cqrre custom_set_uart_image(ruifas input_ruifas);

/**
 * @brief       custom_get_uart_image() Function
 * @param[in]   void
 * @return      rguis
 * @details     
 */
rguis custom_get_uart_image(void);

/**
 * @brief       custom_get_uart_working Function
 * @param[in]   void
 * @return      bool    true : 전송 중, false : 유휴 상태
 * @details     UART Thread가 현재 전송 중인지 확인
 */
bool custom_get_uart_working(void);
/**
 * @brief       custom_set_uart_working Function
 * @param[in]   bool input_b_working  true : 전송 중, false : 유휴 상태
 * @return      void
 * @details     UART Thread가 현재 전송 중인지 확인
 */
void custom_set_uart_working(bool input_b_working);

/**
 * @brief       custom_calculate_checksum() Function
 * @attention   static[내부 전용]
 * @param[in]   uint8_t* input_ui8_p_data  데이터 포인터
 * @param[in]   uint16_t input_ui16_length  데이터 길이
 * @return      uint16_t    체크섬
 * @details     데이터의 체크섬을 계산하는 함수
 */
static uint16_t custom_calculate_checksum(uint8_t* input_ui8_p_data, uint16_t input_ui16_length);


/**
 * @brief       prepare_and_serialize_data Function
 * @attention   static[내부 전용]
 * @param[in]   ruifas* input_ruifas_value  UART TX Request Struct 포인터
 * @param[in]   utfs* input_utfs_value  UART TX Frame Struct 포인터
 * @return      bool    true : 성공, false : 실패
 * @details     UART TX Request Struct를 UART TX Frame Struct로 직렬화하는 함수
 */
 static bool custom_prepare_n_serialize_data(ruifas* input_ruifas_value, utfs* input_utfs_value);

/**
 * @brief       custom_uart_tx_thread Function
 * @param[in]   void *arg
 * @return      void
 * @details     UART Thread 실행 함수
 */
void custom_uart_tx_thread(void *arg);

#endif
