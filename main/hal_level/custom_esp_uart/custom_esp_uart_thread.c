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
 * @details     UART 수신 큐 (uint8_t**)
 */
static mpqs mpqs_uart_image_rx;

/*===========================================================================*/
/* 함수 정의 */
/*===========================================================================*/
bool custom_uart_init(void){

    #define CUSTOM_UART_INIT_DEBUG         UART_DEBUG

    if(!custom_mpqs_init(&mpqs_uart_image_rx, UART_IMAGE_MAX_COUNT, sizeof(void*), "mpqs_uart_image_rx")){
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
    return true;
}

// iSENSOR_Mode에서 사용
cqrre custom_set_uart_image(ruifas input_ruifas){
    #define CUSTOM_SET_UART_IMAGE_DEBUG         UART_DEBUG

    // rsuis_value.cqrre_value = QUEUE_IS_ERROR;
    ruifas* ruifas_value = (ruifas*)pvPortMalloc(sizeof(ruifas));
    if(ruifas_value == NULL){
        // 메모리 할당 실패 처리
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - ruifas_value 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        return QUEUE_IS_ERROR;
    }
    ruifas_value->ui8_height = input_ruifas.ui8_height;
    ruifas_value->ui8_width = input_ruifas.ui8_width;
    ruifas_value->A_ui8_uart_frame_buf = (uint8_t**)pvPortMalloc(ruifas_value->ui8_height * sizeof(uint8_t*));
    if(ruifas_value->A_ui8_uart_frame_buf == NULL){
        // 메모리 할당 실패 처리
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif
        vPortFree(ruifas_value);
        return QUEUE_IS_ERROR;
    }
    // 각 행에 대해 열(column) 할당
    for(int i = 0; i < ruifas_value->ui8_height; i++) {
        ruifas_value->A_ui8_uart_frame_buf[i] = (uint8_t*)pvPortMalloc(ruifas_value->ui8_width * sizeof(uint8_t));
        if(ruifas_value->A_ui8_uart_frame_buf[i] == NULL) {
            // 이전에 할당한 메모리 해제 후 오류 처리
            for(int j = 0; j < i; j++) {
                vPortFree(ruifas_value->A_ui8_uart_frame_buf[j]);
            }
            vPortFree(ruifas_value->A_ui8_uart_frame_buf);
            vPortFree(ruifas_value);
            #if CUSTOM_SET_UART_IMAGE_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
            #endif
            return QUEUE_IS_ERROR;
        }
        memset(ruifas_value->A_ui8_uart_frame_buf[i], 0, ruifas_value->ui8_width * sizeof(uint8_t));
    }

    // ★ 각 행의 실제 데이터를 복사 (포인터 배열이 아닌 실제 데이터!)
    for(int i = 0; i < ruifas_value->ui8_height; i++) {
        memcpy(ruifas_value->A_ui8_uart_frame_buf[i], 
               input_ruifas.A_ui8_uart_frame_buf[i], 
               ruifas_value->ui8_width * sizeof(uint8_t));
    }
    cqrre cqrre_value = custom_queue_safe_send(&mpqs_uart_image_rx, &ruifas_value, 1);


    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    // input_rsuis->cqrre_value = custom_queue_safe_send(&mpqs_uart_image_rx, &rsuis_value, 1);
    if(cqrre_value == QUEUE_IS_READY){
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_set_uart_image() - Queue 전송 성공\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG);
        #endif

        #if 0
        printf("CUSTOM_SET_UART_IMAGE : RECEIVED FRAME\n");
        for (int y = 0; y < ruifas_value->ui8_height; y++) {
            for (int x = 0; x < ruifas_value->ui8_width; x++) {
                printf("[%d]  ", ruifas_value->A_ui8_uart_frame_buf[y][x]);
            }   
            printf("\n");
        }
        #endif

    }
    else{
        #if CUSTOM_SET_UART_IMAGE_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_set_uart_image() - Queue 전송 실패 Error Num : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_uart_TAG, cqrre_value);
        #endif
    }

    return cqrre_value;
}

// iSENSOR_Mode에서 사용
rguis custom_get_uart_image(void){
    #define CUSTOM_GET_UART_IMAGE_DEBUG         UART_DEBUG

    rguis rguis_value = {QUEUE_IS_ERROR, 0, 0, {0,}};
    ruifas* ruifas_value = NULL;

    // ★ Mutex 보호 큐 수신 (Thread-Safe)
    rguis_value.cqrre_value = custom_queue_safe_receive(&mpqs_uart_image_rx, &ruifas_value, 1);
    if(rguis_value.cqrre_value == QUEUE_IS_READY){
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
        rguis_value.ui8_height = ruifas_value->ui8_height;
        rguis_value.ui8_width = ruifas_value->ui8_width;
        for(int i = 0; i < rguis_value.ui8_height; i++) {
            memcpy(rguis_value.A_ui8_uart_frame_buf[i], 
                ruifas_value->A_ui8_uart_frame_buf[i], 
                rguis_value.ui8_width * sizeof(uint8_t));
        }
        // rgis_value.ui64_image_value = *ui64_receive_spi_image_value;

        for(int j = 0; j < ruifas_value->ui8_height; j++) {
            vPortFree(ruifas_value->A_ui8_uart_frame_buf[j]);
        }
        vPortFree(ruifas_value->A_ui8_uart_frame_buf);
        vPortFree(ruifas_value);
    }

    return rguis_value;
}