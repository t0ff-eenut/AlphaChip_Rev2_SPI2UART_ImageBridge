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

#include "iSENSOR_memory.h"
#include <string.h>

// UART
#include "driver/uart.h"

#include <stdbool.h>
#include <stdint.h>

/*===========================================================================*/
/* 메크로 정의 */
/*===========================================================================*/
/**
 * @defgroup    BAUD_RATE 환경 변수 그룹
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

    /**
     * @brief       Debug Port Baud Rate
     * @details     Debug Port에서 사용할 Baud Rate
     */
    #define DEBUG_PORT_BAUD_RATE_SEL        BAUD_RATE_1152000
    /**
     * @brief       Upload Log Port Baud Rate
     * @details     Upload Log Port에서 사용할 Baud Rate
     */
    #define UPLOAD_LOG_PORT_BAUD_RATE_SEL   BAUD_RATE_1500000
/** @} */ // end of BAUD_RATE

#ifndef DEBUG_UART_PORT
    /**
     * @brief       Debug Port Number
     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 UART_NUM_1 설정
     */
    #define DEBUG_UART_PORT             UART_NUM_1
#endif
#ifndef DEBUG_RXD_GPIO_NUM
    /**
     * @brief       Debug Port RXD GPIO Number
     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_6 설정
     */
    #define DEBUG_RXD_GPIO_NUM          GPIO_NUM_6
#endif
#ifndef DEBUG_TXD_GPIO_NUM
    /**
     * @brief       Debug Port TXD GPIO Number
     * @details     custom_esp_gpio.h에서 정의되지 않은 경우, 기본값 GPIO_NUM_7 설정
     */
    #define DEBUG_TXD_GPIO_NUM          GPIO_NUM_7
#endif

#ifndef UART_BUFFER_SIZE
    /**
     * @brief       UART 버퍼 크기
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 256 설정
     */
    #define UART_BUFFER_SIZE                 256   // UART 버퍼 크기
#endif

#ifndef UART_TX_THREAD_STACK_SIZE
    /**
     * @brief       UART TX Thread Stack Size
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 4KB 설정
     * @warning     사이즈 변경하지 말 것
     */
    #define UART_TX_THREAD_STACK_SIZE           (1024 * 4)
#endif

#ifndef UART_RX_THREAD_STACK_SIZE
    /**
     * @brief       UART RX Thread Stack Size
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 4KB 설정
     * @warning     사이즈 변경하지 말 것
     */
    #define UART_RX_THREAD_STACK_SIZE           (1024 * 4)
#endif

#ifndef UART_RX_TIMEOUT_MS
    /**
     * @brief       UART RX Timeout
     * @details     project_top.h에서 정의되지 않은 경우, 기본값 100ms 설정
     */
    #define UART_RX_TIMEOUT_MS                  100
#endif

/**
 * @defgroup    UART_PROTOCOL 환경 변수 그룹
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
    #define UART_STX_PATTERN_SIZE            3
    /**
     * @brief       UART STX Pattern
     * @details     UART 전송 시작 Signal
                    멀티바이트 동기 패턴 (데이터 충돌 방지)
     */
    #define UART_STX_PATTERN                 {0xAA, 0x55, 0xCC}
    /**
     * @brief       UART Command Size
     * @details     UART 명령어 크기
     */
    #define UART_CMD_SIZE                    1
    /**
     * @brief       UART Length Size
     * @details     UART 데이터 길이 크기
     */
    #define UART_LEN_SIZE                    2
    // #define UART_PAYLOAD_SIZE                1500
    /**
     * @brief       UART Checksum Size
     * @details     UART 체크섬 크기
     */
    #define UART_CHECKSUM_SIZE               2
    /**
     * @brief       UART ETX Pattern Size
     * @details     UART 전송 종료 Signal 길이
     */
    #define UART_ETX_PATTERN_SIZE            3
    /**
     * @brief       UART ETX Pattern
     * @details     UART 전송 종료 Signal
     */
    #define UART_ETX_PATTERN                 {0xDD, 0x55, 0xAA}
    /**
     * @brief       UART Minimum Size
     * @details     UART 최소 크기
     */
    #define UART_MIN_SIZE                    (UART_STX_PATTERN_SIZE + UART_CMD_SIZE + UART_LEN_SIZE + UART_CHECKSUM_SIZE + UART_ETX_PATTERN_SIZE)   
    // STX/ETX 패턴은 custom_esp_uart_thread.c 내부에서만 사용 (static)
    /**
     * @brief       UART Maximum Payload Size
     * @details     UART 최대 페이로드 크기
     */
    #define UART_MAX_PAYLOAD_SIZE            1500   // 최대 페이로드 크기
/** @} */ // end of UART_PROTOCOL




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
 * @enum        utdte
 * @typedef     uart_tx_data_type_enum
 * @brief       UART TX Data Type Enum
 * @details     
 */
typedef enum uart_tx_data_type_enum {
    UART_TX_ADC_RAW_BUFFER,             /**< 0 : ADC Raw Buffer */
    UART_TX_ADC_RAW_VOLTAGE_BUFFER,     /**< 1 : ADC Raw Voltage Buffer */
    UART_TX_ADC_SW_HPF_BUFFER,          /**< 2 : ADC SW HPF Buffer */
    UART_TX_ADC_SW_BPF_BUFFER,          /**< 3 : ADC SW BPF Buffer */
    UART_TX_HPF_BUFFER,                 /**< 4 : HPF Buffer */
    UART_TX_HPF_VOLTAGE_BUFFER,         /**< 5 : HPF Voltage Buffer */
    UART_TX_BPF_BUFFER,                 /**< 6 : BPF Buffer */
    UART_TX_BPF_VOLTAGE_BUFFER,         /**< 7 : BPF Voltage Buffer */
    UART_TX_SETTINGS,                   /**< 8 : Settings */
    UART_TX_ALL_DATA                    /**< 9 : All Data */
} utdte;

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

 /**
 * @enum        ursl
 * @typedef     uart_rx_switch_level_enum
 * @brief       UART RX Switch Level Enum
 * @details     
 */
typedef enum uart_rx_switch_level_enum{
    READ_RX_BYTE,                       /**< 0 : Read RX Byte */
    CHECK_STX,                          /**< 1 : Check STX */
    STORAGE_DATA,                       /**< 2 : Storage Data */
    CHECK_ETX,                          /**< 3 : Check ETX */
} ursl;

/**
 * @enum        mte
 * @typedef     message_type_enum
 * @brief       ESP32 → PC 데이터 타입 (UART TX 데이터)
 */
typedef enum message_type_enum{
    MSG_TYPE_SENSOR_DATA = 0x01,     /**< 0x01 : Sensor Data */
    // 나중에 새로운 메시지 타입을 여기에 추가
    // MSG_TYPE_CONFIG_UPDATE = 0x02,
} mte;

/**
 * @enum        cte
 * @typedef     command_type_enum
 * @brief       PC → ESP32 명령 타입 (UART RX 명령)
 * @note        기존 데이터 타입(0x00~0x09)과 충돌하지 않도록 0x10부터 시작
 */
typedef enum command_type_enum{
    CMD_SET_TP1         = 0x10,     /**< 0x10 : TP1 임계값 설정 (payload: uint16_t) */
    CMD_SET_TP2         = 0x11,     /**< 0x11 : TP2 카운트 설정 (payload: uint64_t) */
    CMD_SET_TP1_RECHECK = 0x12,     /**< 0x12 : TP1 Recheck 임계값 설정 (payload: uint16_t) */
    CMD_GET_SETTINGS    = 0x20,     /**< 0x20 : 현재 설정값 요청 (payload: 없음) */
    CMD_SAVE_NVS        = 0x30,     /**< 0x30 : 현재 설정을 NVS에 저장 (payload: 없음) */
    CMD_RESET           = 0xF0,     /**< 0xF0 : ESP32 소프트 리셋 (payload: 없음) */
} cte;

/*===========================================================================*/
/* 구조체 정의 */
/*===========================================================================*/
 /**
 * @struct      euccs
 * @typedef     esp_uart_channel_config_struct
 * @brief       ESP UART Channel Config Struct
 * @details     
 */
// PWM 채널 설정 구조체
typedef struct esp_uart_channel_config_struct{
    int                     int_uart_port;          /**< UART Port */
    int                     int_baud_rate;          /**< Baud Rate */
    uart_word_length_t      uart_word_length;       /**< Word Length */
    uart_parity_t           uart_parity;            /**< Parity */
    uart_stop_bits_t        uart_stop_bits;         /**< Stop Bits */
    uart_hw_flowcontrol_t   uart_hw_flowcontrol;    /**< Hardware Flow Control */
    uart_sclk_t             uart_sclk;
} euccs;

// // #pragma pack(push, 1)    MSVC 스타일
// typedef struct uart_tx_request_struct{
//     utdte       utdte_data_type;
//     void*       p_void_data_ptr;
//     uint16_t    ui16_data_size;
// } __attribute__((packed)) utrs; // GCC 스타일
// #pragma pack(pop)
 /**
 * @struct      utfs
 * @typedef     uart_tx_frame_struct
 * @brief       UART TX Frame Struct
 * @details     
 */
typedef struct uart_tx_frame_struct{
    uint8_t     ui8_stx[UART_STX_PATTERN_SIZE];     /**< Start of Text (3 bytes: AA 55 CC) */
    uint8_t     ui8_data_type;                      /**< Data Type (1 byte) */
    uint16_t    ui16_data_length;                   /**< Data Length (2 bytes, Little Endian) */
    uint8_t     ui8_payload[UART_MAX_PAYLOAD_SIZE]; /**< Payload (payload_index bytes) */
    uint16_t    ui16_checksum;                      /**< Checksum (2 bytes, checksum calculation target) */
    uint8_t     ui8_etx[UART_ETX_PATTERN_SIZE];     /**< End of Text (3 bytes: DD 55 AA) */
} __attribute__((packed)) utfs;

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
 * @brief       custom_running_iSENSOR_uart_tx_thread() Function
 * @param[in]   void
 * @return      bool    true : 실행 중, false : 실행 중지
 * @details     UART TX Thread 실행 함수
 */
bool custom_running_iSENSOR_uart_tx_thread(void);

/**
 * @brief       custom_request_uart_tx() Function
 * @param[in]   utdte input_utdte  전송할 데이터 타입
 * @return      bool    true : 요청 성공, false : 요청 실패
 * @details     UART TX Thread 실행 함수
 */
bool custom_request_uart_tx(utdte input_utdte);

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
 * @param[in]   utdte input_utdte_data_type  전송할 데이터 타입
 * @param[in]   utfs* input_utfs_p_frame  UART TX Frame Struct 포인터
 * @return      bool    true : 성공, false : 실패
 * @details     UART TX Request Struct를 UART TX Frame Struct로 직렬화하는 함수
 */
// static bool custom_prepare_and_serialize_data(utrs* input_utrs_p_request, utfs* input_utfs_p_frame);
static bool custom_prepare_n_serialize_data(utdte input_utdte_data_type, utfs* input_utfs_p_frame);

/**
 * @brief       custom_uart_tx_thread Function
 * @param[in]   void *arg
 * @return      void
 * @details     UART Thread 실행 함수
 */
void custom_uart_tx_thread(void *arg);

/**
 * @brief       custom_uart_wait_all_tx_done Function
 * @param[in]   uint32_t input_ui32_timeout_ms  최대 대기 시간 (ms)
 * @return      bool    true : 완료, false : 타임아웃
 * @warning     DeepSleep 진입 전에 반드시 호출!
 * @details     모든 UART 전송이 완전히 끝났는지 확인하고 대기
 *              1. Queue 비었는지 확인
 *              2. Thread 전송 중이 아닌지 확인
 *              3. UART TX FIFO 비었는지 확인
 *              4. 안전 마진 대기
 */
bool custom_uart_wait_all_tx_done(void);

/**
 * @brief       custom_rx_command_handler Function
 * @attention   static[내부 전용]
 * @param[in]   cte input_cte  명령어 타입
 * @param[in]   uint8_t *input_p_ui8_payload  명령어 데이터
 * @param[in]   uint16_t input_ui16_payload_len  명령어 데이터 길이
 * @return      void
 * @details     명령어를 처리하는 함수
 */
static void custom_rx_command_handler(cte input_cte, uint8_t *input_p_ui8_payload, uint16_t input_ui16_payload_len);

/**
 * @brief       custom_rx_data_parse Function
 * @attention   static[내부 전용]
 * @param[in]   uint8_t *input_p_ui8_data  명령어 데이터
 * @param[in]   size_t input_size_len  명령어 데이터 길이
 * @return      bool    true : 성공, false : 실패
 * @details     명령어 데이터를 파싱하는 함수
 */
static bool custom_rx_data_parse(uint8_t *input_p_ui8_data, size_t input_size_len);

/**
 * @brief       custom_uart_rx_thread Function
 * @param[in]   void *arg
 * @return      void
 * @details     UART Thread 실행 함수
 */
void custom_uart_rx_thread(void *arg);

/**
 * @brief       custom_uart_deinit Function
 * @param[in]   void
 * @return      bool    true : 종료 성공, false : 종료 실패
 * @warning     DeepSleep 진입 전에 반드시 호출!
 * @details     UART Thread 종료 함수
 */
bool custom_uart_deinit(void);

#endif
