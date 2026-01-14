/**
 * @file        custom_esp_uart_thread.c
 * @brief       UART DATA 전송 모듈
 * @author      T0T
 * @date        2026-01-14
 * @version     1.0.0
 * 
 * @details     
 */

#include "custom_esp_uart_thread.h"

/**
 * @brief       custom_esp_uart_thread.c 파일 디버깅 여부
 * @details     custom_esp_uart_thread.c 파일에서 디버깅 Print 사용 여부를 정의합니다
 */
#define UART_DEBUG         DEBUG
// #define SPI_DEBUG         false

/**
 * @brief       custom_esp_uart_thread.c 디버깅 Tag
 * @details     Debug Print 시 custom_esp_spi_thread.c 파일을 구분하기 위한 Tag
 */
static const char *custom_esp_uart_TAG = "[@]custom_esp_uart_thread.c";

/**
 * @brief       UART 수신 큐
 * @details     UART 수신 큐 (uint64_t*)
 */
static mpqs mpqs_uart_image_rx;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
bool custom_uart_init(void){

    #define CUSTOM_UART_INIT_DEBUG         UART_DEBUG

    if(!custom_mpqs_init(&mpqs_uart_image_rx, UART_IMAGE_MAX_COUNT, sizeof(void**), "mpqs_uart_image_rx")){
        #if CUSTOM_UART_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_uart_init() - mpqs_uart_image_rx Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }

}


// iSENSOR_Mode에서 사용
void custom_set_uart_image(rsuis input_rsuis){
    #define CUSTOM_SET_UART_IMAGE_DEBUG         UART_DEBUG

    static uint8_t** A_ui8_frame_buf = NULL;
    A_ui8_frame_buf = (uint8_t**)pvPortMalloc(IMAGE_HEIGHT * sizeof(uint8_t*));
    if(A_ui8_frame_buf == NULL){
        // 메모리 할당 실패 처리
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return;
    }
    // 각 행에 대해 열(column) 할당
    for(int i = 0; i < IMAGE_HEIGHT; i++) {
        A_ui8_frame_buf[i] = (uint8_t*)pvPortMalloc(IMAGE_WIDTH * sizeof(uint8_t));
        if(A_ui8_frame_buf[i] == NULL) {
            // 이전에 할당한 메모리 해제 후 오류 처리
            for(int j = 0; j < i; j++) {
                vPortFree(A_ui8_frame_buf[j]);
            }
            vPortFree(A_ui8_frame_buf);
            #if CUSTOM_SET_UART_IMAGE_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            continue;
        }
        memset(A_ui8_frame_buf[i], 0, IMAGE_WIDTH * sizeof(uint8_t));
    }
    memcpy(A_ui8_frame_buf, input_rsuis.A_ui8_uart_frame_buf, sizeof(A_ui8_frame_buf));

    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    input_rsuis.cqrre_value = custom_queue_safe_send(&mpqs_uart_image_rx, &A_ui8_frame_buf, 1);
    if(input_rsuis.cqrre_value == QUEUE_IS_READY){
        #if 0
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
    }
    // return input_rsuis;
}

// iSENSOR_Mode에서 사용
rsuis custom_get_uart_image(void){
    #define CUSTOM_GET_UART_IMAGE_DEBUG         UART_DEBUG

    rsuis rsuis_value = {QUEUE_IS_ERROR, NULL};
    static uint8_t** A_ui8_receive_uart_frame_buf = NULL;

    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    rsuis_value.cqrre_value = custom_queue_safe_receive(&mpqs_uart_image_rx, &A_ui8_receive_uart_frame_buf, 1);
    if(rsuis_value.cqrre_value == QUEUE_IS_READY){
        #if 0
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
        memcpy(rsuis_value.A_ui8_uart_frame_buf, A_ui8_receive_uart_frame_buf, sizeof(rsuis_value.A_ui8_uart_frame_buf));
        // rgis_value.ui64_image_value = *ui64_receive_spi_image_value;
        for(int j = 0; j < IMAGE_HEIGHT; j++) {
            vPortFree(A_ui8_receive_uart_frame_buf[j]);
        }
        vPortFree(A_ui8_receive_uart_frame_buf);
    }
    return rsuis_value;
}