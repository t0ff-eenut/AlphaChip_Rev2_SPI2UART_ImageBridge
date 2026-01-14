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


/**
* @brief       
* @details     
* @todo     
*/
#define UART_IMAGE_MAX_COUNT                                 20

/*===========================================================================*/
/* 구조체 정의 */
/*===========================================================================*/
typedef struct return_set_uart_image_struct{
    cqrre cqrre_value;                      /**< SQueue 상태 확인 값 */
    uint8_t** A_ui8_uart_frame_buf;              /**< 이미지 버퍼 */
} rsuis;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
/**
 * @brief       custom_set_uart_image Function
 * @param[in]   rsuis input_rsuis
 * @return      void
 * @details     
 */
void custom_set_uart_image(rsuis input_rsuis);

/**
 * @brief       custom_set_uart_image Function
 * @param[in]   rsuis input_rsuis
 * @return      void
 * @details     
 */
rsuis custom_get_uart_image(void){

#endif
