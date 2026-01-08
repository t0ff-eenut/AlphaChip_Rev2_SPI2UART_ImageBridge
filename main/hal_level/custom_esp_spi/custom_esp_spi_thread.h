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
    /**
    * @def         SPI_IMAGE_RCV_HOST
    * @brief       SPI Image 수신 Host
    * @details     SPI Image 수신 Host를 정의합니다
    */
    #define SPI_IMAGE_RCV_HOST              SPI2_HOST
    /**
    * @def         SPI_IMAGE_GPIO_SCLK
    * @brief       SPI Image Clock GPIO Pin 번호
    * @details     SPI Image 수신 Host의 Clock GPIO Pin 번호를 정의합니다
    */
    #define SPI_IMAGE_GPIO_SCLK             GPIO_NUM_12
    /**
    * @def         SPI_IMAGE_GPIO_MOSI
    * @brief       SPI Image MOSI GPIO Pin 번호
    * @details     SPI Image 수신 Host의 MOSI GPIO Pin 번호를 정의합니다
    */
    #define SPI_IMAGE_GPIO_MOSI             GPIO_NUM_11
    /**
    * @def         SPI_IMAGE_GPIO_MISO
    * @brief       SPI Image MISO GPIO Pin 번호
    * @details     SPI Image 수신 Host의 MISO GPIO Pin 번호를 정의합니다
    */
    #define SPI_IMAGE_GPIO_MISO             GPIO_NUM_13
    /**
    * @def         SPI_IMAGE_GPIO_CS
    * @brief       SPI Image Chip Select GPIO Pin 번호
    * @details     SPI Image 수신 Host의 Chip Select GPIO Pin 번호를 정의합니다
    */
    #define SPI_IMAGE_GPIO_CS               GPIO_NUM_10
    /**
    * @def         SPI_IMAGE_SPI_BUSTER_SIZE
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
    /**
    * @def         SPI_CMD_RCV_HOST
    * @brief       SPI 명령어 전송 Host
    * @details     SPI 명령어 전송 Host를 정의합니다
    */
    #define SPI_CMD_RCV_HOST                SPI3_HOST
    /**
    * @def         SPI_CMD_GPIO_SCLK
    * @brief       SPI 명령어 Clock GPIO Pin 번호
    * @details     SPI 명령어 전송 Host의 Clock GPIO Pin 번호를 정의합니다
    */
    #define SPI_CMD_GPIO_SCLK               GPIO_NUM_36
    /**
    * @def         SPI_CMD_GPIO_MOSI
    * @brief       SPI 명령어 MOSI GPIO Pin 번호
    * @details     SPI 명령어 전송 Host의 MOSI GPIO Pin 번호를 정의합니다
    */
    #define SPI_CMD_GPIO_MOSI               GPIO_NUM_35
    /**
    * @def         SPI_CMD_GPIO_MISO
    * @brief       SPI 명령어 MISO GPIO Pin 번호
    * @details     SPI 명령어 전송 Host의 MISO GPIO Pin 번호를 정의합니다
    */
    #define SPI_CMD_GPIO_MISO               GPIO_NUM_37
    /**
    * @def         SPI_CMD_GPIO_CS
    * @brief       SPI 명령어 Chip Select GPIO Pin 번호
    * @details     SPI 명령어 전송 Host의 Chip Select GPIO Pin 번호를 정의합니다
    */
    #define SPI_CMD_GPIO_CS                 GPIO_NUM_39
    /**
    * @def         SPI_CMD_SPI_BUSTER_SIZE
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
    * @def         SPI_FIRST_WORD
    * @brief       SPI 명령어 버스의 첫 번째 단어
    * @details     SPI 명령어 버스의 첫 번째 단어를 정의합니다
    * @note        0xA5 = 0b 1010_0101
    #define SPI_FIRST_WORD                  0XA5
    // /**
    //  * @def         SPI_WRONG_WORD
    //  * @brief       SPI 명령어 버스의 잘못된 단어
    //  * @details     SPI 명령어 버스의 잘못된 단어를 정의합니다
    //  */
    // #define SPI_WRONG_WORD                  0X88
/** @} */ // end of SPI_COMMEND_CONFIG

#define SPI_IMAGE_RX_STACK_SIZE                     (1024 * 8)
#define SPI_IMAGE_RECIVE_DATA_PROCESS_STACK_SIZE    (1024 * 8)
#define SPI_CMD_TX_STACK_SIZE                       (1024 * 3)

// /**
//  * @enum        spi_send_state_enum(ssse)
//  * @brief       SPI Send State Enum
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param RECIVE_UART_DATA            0
//  * @param SEND_CMD_1_WRITE_REG        1
//  * @param SEND_CMD_2                  2
//  * @param SEND_CMD_3                  3
//  * 
//  * @see         
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// typedef enum spi_send_state_enum{
//     RECIVE_UART_DATA,
//     SEND_CMD_1_WRITE_REG,
//     SEND_CMD_2,
//     SEND_CMD_3
// } ssse;

// /**
//  * @struct      spi_value_struct(svs)
//  * @brief       SPI Value Struct
//  * @attention   *주의사항
//  * @warning     *경고
//  * @note        *참고사항
//  *
//  * @param uint8_t*      ui8_signal
//  * @param uint8_t*      ui8_cmd
//  * @param uint8_t*      ui8_addr_length
//  * @param uint8_t*      ui8_addr
//  * @param uint8_t*      ui8_data_length
//  * @param uint8_t*      ui8_data
//  * @param uint8_t*      ui8_crc8
//  * @param uint8_t*      ui8_dummy
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
//  #pragma pack(push, 1)
// typedef struct spi_value_struc{
//     uint8_t* ui8_signal;
//     uint8_t* ui8_cmd;
//     uint8_t* ui8_addr_length;
//     uint8_t* ui8_addr;
//     uint8_t* ui8_data_length;
//     uint8_t* ui8_data;
//     uint8_t* ui8_crc8;
//     uint8_t* ui8_dummy;
// }svs;
// #pragma pack(pop)

// /**
//  * @brief       custom_spi_init Function
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
// bool custom_spi_init(void);

// /**
//  * @brief       running_spi_rx_thread Function
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
// bool custom_running_spi_rx_thread(void);
// /**
//  * @brief       custom_set_spi_rx_break Function
//  * @attention   SPI 수신 중단 시그널 설정
//  * @param[in]   b_break     true : SPI 수신 중단, false : SPI 수신 재개
//  * @return      void
//  * @warning     *경고
//  * @note        SPI 수신을 중단할 때 사용
//  * 
//  * @details     b_spi_rx_break 플래그를 설정하여 SPI 수신 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_set_spi_rx_break(bool b_break);
// /**
//  * @brief       custom_get_spi_rx_break Function
//  * @attention   SPI 수신 중단 시그널 읽기
//  * @param[in]   void
//  * @return      bool    true : SPI 수신 중단, false : SPI 수신 재개
//  * @warning     *경고
//  * @note        SPI 수신을 중단할 때 사용
//  * 
//  * @details     b_spi_rx_break 플래그를 읽어 SPI 수신 스레드의 동작을 확인합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_get_spi_rx_break(void);
// /**
//  * @brief       custom_set_spi_rx_breaked Function
//  * @attention   SPI 수신 중단 완료 시그널 설정
//  * @param[in]   b_break     true : SPI 수신 중단, false : SPI 수신 재개
//  * @return      void
//  * @warning     *경고
//  * @note        SPI 수신을 중단할 때 사용
//  * 
//  * @details     b_spi_rx_breaked 플래그를 설정하여 SPI 수신 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_set_spi_rx_breaked(bool b_breaked);
// /**
//  * @brief       custom_get_spi_rx_breaked Function
//  * @attention   SPI 수신 중단 완료 시그널 읽기
//  * @param[in]   void
//  * @return      bool    true : SPI 수신 중단 완료, false : SPI 수신 중단 완료
//  * @warning     *경고
//  * @note        SPI 수신을 중단할 때 사용
//  * 
//  * @details     b_spi_rx_breaked 플래그를 읽어 SPI 수신 스레드의 동작을 확인합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_get_spi_rx_breaked(void);

// /**
//  * @brief       running_spi_rx_thread Function
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
// bool custom_running_spi_rx_process_thread(void);

// /**
//  * @brief       running_spi_tx_thread Function
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
// bool custom_running_spi_tx_thread(void);

































// static bool init_spi_send(spi_structer* buf);                                                                               // 1. SPI 구조체 초기화
// static void free_spi_send(spi_structer* buf);                                                                               // 2. SPI 구조체 해제
// static uint8_t crc8_proc(uint8_t crc8_value);                                                                               // 3. CRC8 Transaction
// static uint8_t crc8(spi_structer* spistructer_spi_recive_data);                                                             // 4. CRC8 Process

// static void spi_image_rx_thread(void *arg);                                                                                 // 5. SPI Frame 수신 [ISP] Chip -> ESP
// static void spi_image_rx_data_process(void);                                                                                // 6. Frame 재구성

// static void spi_receive_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value);                      // 7. SPI 1Byte 수신 [CPU] Chip -> ESP
// static uint8_t spi_cmd_rx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_recv_buf);    // 8. SPI 1Byte 송신 [CPU] Chip <- ESP
// static uint8_t spi_rx_CMD_data_setting(spi_structer* spistructer_spi_recv_buf);                                             // 9. SPI CMD 수신 Transaction [CPU] Chip -> ESP

// static void spi_send_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value);                         // 10. SPI CMD 수신 Data Setting (수신 공간 할당)
// static uint8_t spi_tx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_send_buf);        // 11. SPI CMD 송신 Transaction [CPU] Chip <- ESP
// static uint8_t spi_tx_CMD_data_setting(uint32_t* A_ui32_cmd_q_recv_128bit);                                                 // 12. SPI CMD 송신 Data Setting (송신 공간 할당 및 데이터 세팅)
// static uint8_t uart_recive_cmd_spi_tx_thread(void *arg);                                                                    // 13. Uart Recive CMD Data Process

// int spi_init(void);  

// /**
//  * @brief       custom_adc_init Function
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
// bool custom_adc_init(void);

// /**
//  * @brief       running_iSENSOR_all_adc_read_thread Function
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
// bool custom_running_iSENSOR_all_adc_read_thread(void);

// /**
//  * @brief       running_iSENSOR_all_adc_thread Function
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
// bool custom_running_iSENSOR_all_adc_thread(void);

// /**
//  * @brief       custom_set_all_adc_break Function
//  * @attention   ADC 읽기 중단 설정
//  * @param[in]   b_break     true : ADC 읽기 중단, false : ADC 읽기 재개
//  * @return      void
//  * @warning     *경고
//  * @note        DeepSleep 진입 전 ADC 읽기를 중단할 때 사용
//  * 
//  * @details     b_adc_break 플래그를 설정하여 ADC 읽기 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_set_all_adc_break(bool b_break);
// /**
//  * @brief       custom_get_all_adc_break Function
//  * @attention   ADC 읽기 중단 설정
//  * @param[in]   b_break     true : ADC 읽기 중단, false : ADC 읽기 재개
//  * @return      void
//  * @warning     *경고
//  * @note        DeepSleep 진입 전 ADC 읽기를 중단할 때 사용
//  * 
//  * @details     b_adc_break 플래그를 설정하여 ADC 읽기 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_get_all_adc_break(void);
// /**
//  * @brief       custom_set_all_adc_breaked Function
//  * @attention   ADC 읽기 중단 설정
//  * @param[in]   b_break     true : ADC 읽기 중단, false : ADC 읽기 재개
//  * @return      void
//  * @warning     *경고
//  * @note        DeepSleep 진입 전 ADC 읽기를 중단할 때 사용
//  * 
//  * @details     b_adc_break 플래그를 설정하여 ADC 읽기 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_set_all_adc_breaked(bool b_breaked);
// /**
//  * @brief       custom_get_all_adc_breaked Function
//  * @attention   ADC 읽기 중단 설정
//  * @param[in]   b_break     true : ADC 읽기 중단, false : ADC 읽기 재개
//  * @return      void
//  * @warning     *경고
//  * @note        DeepSleep 진입 전 ADC 읽기를 중단할 때 사용
//  * 
//  * @details     b_adc_break 플래그를 설정하여 ADC 읽기 스레드의 동작을 제어합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// bool custom_get_all_adc_breaked(void);

// /**
//  * @brief       custom_all_adc_read_thread Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      void
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_all_adc_read_thread(void *arg);

// /**
//  * @brief       custom_all_adc_process_thread Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      void
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     디테일 설명
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_all_adc_process_thread(void *arg);

// /**
//  * @brief       custom_reset_all_adc_queues Function
//  * @attention   *주의사항
//  * @param[in]   void
//  * @return      void
//  * @warning     *경고
//  * @note        *참고사항
//  * 
//  * @details     ADC 관련 큐를 초기화(비우기)합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// void custom_reset_all_adc_queues(void);

// #if ADC_RAW_ENABLE
//     /**
//     * @brief       custom_get_raw_avs Function
//     * @attention   *주의사항
//     * @param[in]   void
//     * @return      rgas    return_get_avs_struct
//     * @warning     *경고
//     * @note        *참고사항
//     * 
//     * @details     디테일 설명
//     * @todo        todo
//     * @bug         bug
//     */
//     rgas custom_get_raw_avs(void);
// #endif
// #if ADC_HPF_ENABLE
//     /**
//     * @brief       custom_get_hpf_avs Function
//     * @attention   *주의사항
//     * @param[in]   void
//     * @return      rgas    return_get_avs_struct
//     * @warning     *경고
//     * @note        *참고사항
//     * 
//     * @details     디테일 설명
//     * @todo        todo
//     * @bug         bug
//     */
//     rgas custom_get_hpf_avs(void);
// #endif
// #if ADC_BPF_ENABLE
//     /**
//     * @brief       custom_get_bpf_avs Function
//     * @attention   *주의사항
//     * @param[in]   void
//     * @return      rgas    return_get_avs_struct
//     * @warning     *경고
//     * @note        *참고사항
//     * 
//     * @details     디테일 설명
//     * @todo        todo
//     * @bug         bug
//     */
//     rgas custom_get_bpf_avs(void);
// #endif

// /**
//  * @brief       custom_adc_deinit Function
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
// bool custom_adc_deinit(void);

// /**
//  * @brief       custom_adc_wait_all_queues_done Function
//  * @attention   모든 ADC 큐가 처리될 때까지 대기
//  * @param[in]   timeout_ms  대기 시간 제한 (밀리초)
//  * @return      bool    true : 모든 큐 처리 완료, false : 타임아웃
//  * @warning     *경고
//  * @note        DeepSleep 진입 전 모든 ADC 데이터가 처리될 때까지 대기
//  * 
//  * @details     Raw ADC 큐 -> Processed ADC 큐 순서로 처리가 완료될 때까지 대기합니다.
//  * @todo        todo
//  * @bug         bug
//  */
// // bool custom_adc_wait_all_queues_done(uint32_t timeout_ms);
// bool custom_adc_wait_all_queues_done(void);

#endif