/**
 * @file        custom_esp_uart_thread.h
 * @brief       ESP32의 UART Pin 할당 및 제어 헤더 파일
 * @author      T0T
 * @date        2026-01-07
 * @version     1.0.0
 * 
 * @details     iSENSOR AlphaChip Rev.2에서 SPI로 수신받은 1 Frame 이미지 데이터를 UART로 전송하는 모듈
 *              Queue를 통해 SPI 데이터를 받아서 Frame으로 재구성 후 Frame 데이터를 직렬화하여 전송
 *              uart_init 후 uart_tx_thread 실행
 */
#ifndef CUSTOM_ESP_UART_THREAD_H
#define CUSTOM_ESP_UART_THREAD_H

// UART는 Queue 모듈에 의존 (같은 계층 내 직접 참조)
#include "custom_esp_queue.h"

// // UART
// #include "driver/uart.h"

// #include <stdbool.h>
// #include <stdint.h>

// #define BAUD_RATE_115200    115200
// #define BAUD_RATE_230400    230400
// #define BAUD_RATE_460800    460800
// #define BAUD_RATE_500000    500000
// #define BAUD_RATE_576000    576000
// #define BAUD_RATE_921600    921600
// #define BAUD_RATE_1000000   1000000
// #define BAUD_RATE_1152000   1152000
// #define BAUD_RATE_1500000   1500000
// #define BAUD_RATE_2000000   2000000
// #define BAUD_RATE_2500000   2500000
// #define BAUD_RATE_3000000   3000000
// #define BAUD_RATE_3500000   3500000
// #define BAUD_RATE_4000000   4000000
// #define DEBUG_PORT_BAUD_RATE_SEL        BAUD_RATE_1152000
// #define UPLOAD_LOG_PORT_BAUD_RATE_SEL   BAUD_RATE_1500000

// #ifndef DEBUG_UART_PORT
//     #if CONFIG_IDF_TARGET_ESP32C3
//         #if ESP32C3 == ESP32C3_MINI
//             #define DEBUG_UART_PORT            UART_NUM_1
//         #elif ESP32C3 == ESP32C3_SUPER_MINI
//             #define DEBUG_UART_PORT            UART_NUM_1
//         #endif
//     #else
//         #define DEBUG_UART_PORT                UART_NUM_1
//     #endif
// #endif
// #ifndef DEBUG_RXD_GPIO_NUM
//     #if CONFIG_IDF_TARGET_ESP32C3
//         #if ESP32C3 == ESP32C3_MINI
//             #define DEBUG_RXD_GPIO_NUM          GPIO_NUM_6
//         #elif ESP32C3 == ESP32C3_SUPER_MINI
//             #define DEBUG_RXD_GPIO_NUM          GPIO_NUM_6
//         #endif
//     #else
//         #define DEBUG_RXD_GPIO_NUM              GPIO_NUM_18
//     #endif
// #endif
// #ifndef DEBUG_TXD_GPIO_NUM
//     #if CONFIG_IDF_TARGET_ESP32C3
//         #if ESP32C3 == ESP32C3_MINI
//             #define DEBUG_TXD_GPIO_NUM          GPIO_NUM_7
//         #elif ESP32C3 == ESP32C3_SUPER_MINI
//             #define DEBUG_TXD_GPIO_NUM          GPIO_NUM_7
//         #endif
//     #else
//         #define DEBUG_TXD_GPIO_NUM              GPIO_NUM_17
//     #endif
// #endif

// #ifndef UART_BUFFER_SIZE
//     #define UART_BUFFER_SIZE                 256   // UART 버퍼 크기
// #endif

// #define UART_TX_THREAD_STACK_SIZE           (1024 * 4)
// #define UART_RX_THREAD_STACK_SIZE           (1024 * 4)

// #define UART_RX_TIMEOUT_MS                  100

// // 프로토콜 관련 정의
// // 멀티바이트 동기 패턴 (데이터 충돌 방지)
// #define UART_STX_PATTERN_SIZE            3
// #define UART_STX_PATTERN                 {0xAA, 0x55, 0xCC}
// #define UART_CMD_SIZE                    1
// #define UART_LEN_SIZE                    2
// // #define UART_PAYLOAD_SIZE                1500
// #define UART_CHECKSUM_SIZE               2
// #define UART_ETX_PATTERN_SIZE            3
// #define UART_ETX_PATTERN                 {0xDD, 0x55, 0xAA}
// #define UART_MIN_SIZE                    (UART_STX_PATTERN_SIZE + UART_CMD_SIZE + UART_LEN_SIZE + UART_CHECKSUM_SIZE + UART_ETX_PATTERN_SIZE)   
// // STX/ETX 패턴은 custom_esp_uart_thread.c 내부에서만 사용 (static)
// #define UART_MAX_PAYLOAD_SIZE            1500   // 최대 페이로드 크기


// // PWM 채널 설정 구조체
// typedef struct esp_uart_channel_config_struct{
//     int                     int_uart_port;          // GPIO 핀 번호
//     int                     int_baud_rate;          // 
//     uart_word_length_t      uart_word_length;       // 속도 모드
//     uart_parity_t           uart_parity;            // LEDC 타이머
//     uart_stop_bits_t        uart_stop_bits;         // LEDC 채널
//     uart_hw_flowcontrol_t   uart_hw_flowcontrol;    // 듀티 해상도
//     uart_sclk_t             uart_sclk;
// } euccs;

// /**
//  * @brief UART 채널 식별자 enum
//  * @details LED_PWM (GPIO5)과 LED_BLUE (GPIO8) 두 채널을 하나의 API로 제어하기 위함
//  */
// typedef enum esp_uart_channel_id_enum{
//     UART_CH_DEBUG,     ///< GPIO5 - 외부 LED PWM 제어용
//     UART_CH_UPLOAD,
//     UART_CH_MAX              ///< PWM 채널 개수
// } eucie;

// /**
//  * @enum        uart_tx_data_type_enum(utdte)
//  * @brief       UART TX Data Type Enum
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param UART_TX_ADC_RAW_BUFFER            0
//  * @param UART_TX_ADC_RAW_VOLTAGE_BUFFER    1
//  * @param UART_TX_ADC_SW_HPF_BUFFER         2
//  * @param UART_TX_ADC_SW_BPF_BUFFER         3
//  * @param UART_TX_HPF_BUFFER                4
//  * @param UART_TX_HPF_VOLTAGE_BUFFER        5
//  * @param UART_TX_BPF_BUFFER                6
//  * @param UART_TX_BPF_VOLTAGE_BUFFER        7
//  * @param UART_TX_SETTINGS                  8
//  * @param UART_TX_ALL_DATA                  9
//  * 
//  * @see         
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef enum uart_tx_data_type_enum {
//     UART_TX_ADC_RAW_BUFFER,
//     UART_TX_ADC_RAW_VOLTAGE_BUFFER,
//     UART_TX_ADC_SW_HPF_BUFFER,         // SW HPF 버퍼
//     UART_TX_ADC_SW_BPF_BUFFER,         // SW BPF 버퍼
//     UART_TX_HPF_BUFFER,
//     UART_TX_HPF_VOLTAGE_BUFFER,
//     UART_TX_BPF_BUFFER,
//     UART_TX_BPF_VOLTAGE_BUFFER,

//     UART_TX_SETTINGS,

//     UART_TX_ALL_DATA
// } utdte;

// /**
//  * @enum        uart_process_switch_level_enum(upsle)
//  * @brief       UART Process Switch Level enum
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param QUEUE_WAIT             0  // Queue 대기
//  * @param DATA_PREPARE           1  // 데이터 준비
//  * @param UART_TRANSMIT          2  // UART 전송
//  * 
//  * @see         
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef enum uart_process_switch_level_enum{
//     QUEUE_WAIT,
//     DATA_PREPARE,
//     UART_TRANSMIT
// } upsle;

// /**
//  * @enum        uart_rx_switch_level_enum(ursl)
//  * @brief       UART RX Switch Level enum
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param QUEUE_WAIT             0  // Queue 대기
//  * @param DATA_PREPARE           1  // 데이터 준비
//  * @param UART_TRANSMIT          2  // UART 전송
//  * 
//  * @see         
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef enum uart_rx_switch_level_enum{
//     READ_RX_BYTE,
//     CHECK_STX,
//     STORAGE_DATA,
//     CHECK_ETX,
// } ursl;

// // /**
// //  * @struct      uart_tx_request_struct(utrs)
// //  * @brief       UART TX Request Struct
// //  * @attention   *주의사항
// //  * @warning     *경고
// //  * @note        *참고사항
// //  *
// //  * @param utdte         data_type       // 전송할 데이터 타입
// //  * @param void*         data_ptr        // 추가 데이터 포인터 (선택, NULL 가능)
// //  * @param uint16_t      data_size       // 데이터 크기
// //  * 
// //  * @details     디테일 설명
// //  * @todo        todo
// //  * @bug         bug
// //  */
// // // #pragma pack(push, 1)    MSVC 스타일
// // typedef struct uart_tx_request_struct{
// //     utdte       utdte_data_type;
// //     void*       p_void_data_ptr;
// //     uint16_t    ui16_data_size;
// // } __attribute__((packed)) utrs; // GCC 스타일
// // #pragma pack(pop)
// /**
//  * @struct      uart_tx_frame_struct(utfs)
//  * @brief       UART TX Frame Struct
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param uint8_t       stx             // Start of Text (0x02)
//  * @param uint8_t       data_type       // 데이터 타입 (utdte)
//  * @param uint16_t      data_length     // 페이로드 길이
//  * @param uint8_t[]     payload         // 실제 데이터 (가변 길이)
//  * @param uint16_t      checksum        // 체크섬 (CRC16 또는 Sum)
//  * @param uint8_t       etx             // End of Text (0x03)
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef struct uart_tx_frame_struct{
//     uint8_t     ui8_stx[UART_STX_PATTERN_SIZE];     // 3 bytes: AA 55 CC
//     uint8_t     ui8_data_type;                      // 1 byte
//     uint16_t    ui16_data_length;                   // 2 bytes (Little Endian)
//     uint8_t     ui8_payload[UART_MAX_PAYLOAD_SIZE]; // payload_index bytes
//     uint16_t    ui16_checksum;                      // 2 bytes (체크섬 계산 대상이 아님!)
//     uint8_t     ui8_etx[UART_ETX_PATTERN_SIZE];     // 3 bytes: DD 55 AA
// } __attribute__((packed)) utfs;



// //////// RX ////////////////////////////////////////////////////////////////////////
// // /**
// //  * @struct      uart_tx_frame_struct(utfs)
// //  * @brief       UART TX Frame Struct
// //  * @attention   *주의사항
// //  * @warning     *경고
// //  * @note        *참고사항
// //  *
// //  * @param uint8_t       stx             // Start of Text (0x02)
// //  * @param uint8_t       data_type       // 데이터 타입 (utdte)
// //  * @param uint16_t      data_length     // 페이로드 길이
// //  * @param uint8_t[]     payload         // 실제 데이터 (가변 길이)
// //  * @param uint16_t      checksum        // 체크섬 (CRC16 또는 Sum)
// //  * @param uint8_t       etx             // End of Text (0x03)
// //  * 
// //  * @details     디테일 설명
// //  * @todo        todo
// //  * @bug         bug
// //  */
 
// // typedef struct uart_rx_frame_struct{
// //     uint16_t    ui16_adc_data;
// //     uint16_t    ui16_voltage_data;
// //     uint16_t    ui16_tp1;
// //     uint8_t     ui8_tp2;
// //     uint8_t     ui8_switch_status;
// //     bool        b_occu_triger;
// //     uint16_t    A_ui16_adc_buf[WINDOW_SIZE];
// //     uint16_t    A_ui16_adc_delta_buf[WINDOW_SIZE];
// //     bool        A_b_occu_buf[WINDOW_SIZE];
// // } __attribute__((packed)) urfs;

// // 모든 메시지 타입을 관리하는 Enum (ESP32 → PC 데이터 전송용)
// typedef enum message_type_enum{
//     MSG_TYPE_SENSOR_DATA = 0x01,
//     // 나중에 새로운 메시지 타입을 여기에 추가
//     // MSG_TYPE_CONFIG_UPDATE = 0x02,
// } mte;

// /**
//  * @enum CommandType_t
//  * @brief PC → ESP32 명령 타입 (UART RX 명령)
//  * @note  기존 데이터 타입(0x00~0x09)과 충돌하지 않도록 0x10부터 시작
//  */
// typedef enum command_type_enum{
//     CMD_SET_TP1         = 0x10,     /**< TP1 임계값 설정 (payload: uint16_t) */
//     CMD_SET_TP2         = 0x11,     /**< TP2 카운트 설정 (payload: uint64_t) */
//     CMD_SET_TP1_RECHECK = 0x12,     /**< TP1 Recheck 임계값 설정 (payload: uint16_t) */
//     CMD_GET_SETTINGS    = 0x20,     /**< 현재 설정값 요청 (payload: 없음) */
//     CMD_SAVE_NVS        = 0x30,     /**< 현재 설정을 NVS에 저장 (payload: 없음) */
//     CMD_RESET           = 0xF0,     /**< ESP32 소프트 리셋 (payload: 없음) */
// } cte;
// //////// RX ////////////////////////////////////////////////////////////////////////

// /**
//  * @brief       custom_uart_init Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      bool    true : 초기화 성공, false : 초기화 실패
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_uart_init(void);

// /**
//  * @brief       running_uart_tx_thread Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      bool    true : 실행 중, false : 실행 중지
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_running_uart_tx_thread(void);

// /**
//  * @brief       custom_request_uart_tx Function
//  * @attention   *주의사항
//  * @param[in]   utdte input_utdte  전송할 데이터 타입
//  * @return      bool    true : 요청 성공, false : 요청 실패
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     UART TX Queue에 전송 요청을 추가하는 함수
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_request_uart_tx(utdte input_utdte);

// /**
//  * @brief       custom_get_uart_working Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      bool    true : 전송 중, false : 유휴 상태
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     UART Thread가 현재 전송 중인지 확인
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_get_uart_working(void);

// /**
//  * @brief       custom_set_uart_working Function
//  * @attention   *주의사항
//  * @param[in]   bool input_b_working  true : 전송 중, false : 유휴 상태
//  * @return      void
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     UART Thread가 현재 전송 중인지 확인
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_set_uart_working(bool input_b_working);

// /**
//  * @brief       calculate_checksum Function
//  * @attention   *주의사항
//  * @param[in]   uint8_t* input_ui8_p_data  데이터 포인터
//  * @param[in]   uint16_t input_ui16_length  데이터 길이
//  * @return      uint16_t    체크섬
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     데이터의 체크섬을 계산하는 함수
//  * @todo        todo
//  * @bug         bug
//  */
// static uint16_t custom_calculate_checksum(uint8_t* input_ui8_p_data, uint16_t input_ui16_length);

// /**
//  * @brief       prepare_and_serialize_data Function
//  * @attention   *주의사항
//  * @param[in]   utrs* input_utrs_p_request  UART TX Request Struct 포인터
//  * @param[in]   utfs* input_utfs_p_frame  UART TX Frame Struct 포인터
//  * @return      bool    true : 성공, false : 실패
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     UART TX Request Struct를 UART TX Frame Struct로 직렬화하는 함수
//  * @todo        todo
//  * @bug         bug
//  */
// // static bool custom_prepare_and_serialize_data(utrs* input_utrs_p_request, utfs* input_utfs_p_frame);
// static bool custom_prepare_n_serialize_data(utdte utdte_data_type, utfs* input_utfs_p_frame);

// /**
//  * @brief       custom_uart_tx_thread Function
//  * @attention   *주의사항
//  * @param[in]   void *arg
//  * @return      void
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_uart_tx_thread(void *arg);

// /**
//  * @brief       custom_uart_wait_all_tx_done Function
//  * @attention   *주의사항
//  * @param[in]   uint32_t input_ui32_timeout_ms  최대 대기 시간 (ms)
//  * @return      bool    true : 완료, false : 타임아웃
//  * @warning     *경고
//  * @note        DeepSleep 진입 전에 반드시 호출!
//  * 
//  * @details     모든 UART 전송이 완전히 끝났는지 확인하고 대기
//  *              1. Queue 비었는지 확인
//  *              2. Thread 전송 중이 아닌지 확인
//  *              3. UART TX FIFO 비었는지 확인
//  *              4. 안전 마진 대기
//  * @todo        todo
//  * @bug         bug
//  */
// // bool custom_uart_wait_all_tx_done(uint32_t input_ui32_timeout_ms);
// bool custom_uart_wait_all_tx_done(void);

// // ============================================================================
// // UART RX (수신) 관련 정의
// // ============================================================================
// static void custom_rx_command_handler(cte input_cte, uint8_t *input_p_ui8_payload, uint16_t input_ui16_payload_len);
// static bool custom_rx_data_parse(uint8_t *input_p_ui8_data, size_t input_size_len);
// void custom_uart_rx_thread(void *arg);


// /**
//  * @brief       custom_uart_deinit Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      bool    true : 종료 성공, false : 종료 실패
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_uart_deinit(void);



#endif
