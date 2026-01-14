    //      COLS (= IMAGE_WIDTH)
    //     ◄─────────────────────►
    // ┌───┬───┬───┬───┬───┬───┬───┐  ▲
    // │   │   │   │   │   │   │   │  │
    // ├───┼───┼───┼───┼───┼───┼───┤  │
    // │   │   │   │   │   │   │   │  │ ROWS
    // ├───┼───┼───┼───┼───┼───┼───┤  │ (= IMAGE_HEIGHT)
    // │   │   │   │   │   │   │   │  │
    // └───┴───┴───┴───┴───┴───┴───┘  ▼
/**
 * @file        app_image_processing.c
 * @brief       Image Processing 모듈
 * @author      T0T
 * @date        2026-01-14
 * @version     1.0.0
 * 
 * @details     SPI를 통해 이미지 데이터를 수신 및 제어 Signal을 송신하는 모듈
 */
#include "app_image_processing.h"

/**
 * @brief       app_image_processing_start() 함수 디버깅 여부
 * @details     app_image_processing_start() 함수에서 디버깅 Print 사용 여부를 정의합니다
 */
#define APP_IMAGE_PROCESSING_DEBUG         DEBUG

/**
 * @brief       app_image_processing_start() 함수 디버깅 Tag
 * @details     Debug Print 시 app_image_processing_start() 함수를 구분하기 위한 Tag
 */
static const char *app_image_processing_TAG = "[@]app_image_processing.c";


volatile static ipsle ipsle_image_processing_status_level = MODE_START;
// GENERATE_GETTER_SETTER(ipsle,    image_processing_status_level,    ipsle_image_processing_status_level)


void app_image_processing_start(void){
    #define APP_IMAGE_PROCESSING_START_DEBUG         APP_IMAGE_PROCESSING_DEBUG

    static rgsis rgsis_value;  // static으로 선언하여 스택 오버플로우 방지 (BSS 섹션에 할당)
    static rsuis rsuis_value;
    bool b_while_end = false;

    uint8_t **A_ui8_frame_buf = NULL;

    while(!b_while_end){
        switch(ipsle_image_processing_status_level){
            case MODE_START:
                if(ipsle_image_processing_status_level == MODE_START){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - MODE_START\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    ipsle_image_processing_status_level = MAKE_FRAME_MALLOC;
                }
            
            case MAKE_FRAME_MALLOC:
                if(ipsle_image_processing_status_level == MAKE_FRAME_MALLOC){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - MAKE_FRAME_MALLOC\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    A_ui8_frame_buf = (uint8_t**)pvPortMalloc(IMAGE_HEIGHT * sizeof(uint8_t*));
                    if(A_ui8_frame_buf == NULL){
                        // 메모리 할당 실패 처리
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_image_processing_start() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        continue; // 다시 반복 시작
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
                            #if APP_IMAGE_PROCESSING_START_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_image_processing_start() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                            #endif
                            continue;
                        }
                        memset(A_ui8_frame_buf[i], 0, IMAGE_WIDTH * sizeof(uint8_t));
                    }

                    ipsle_image_processing_status_level = READ_IMAGE_TO_SPI;
                }

            case READ_IMAGE_TO_SPI:
                if(ipsle_image_processing_status_level == READ_IMAGE_TO_SPI){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - READ_IMAGE_TO_SPI\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    rgsis_value = custom_get_spi_image();
                    
                    if(rgsis_value.cqrre_value == QUEUE_IS_READY){
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - 꺼내기 성공\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        #if 0
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s app_image_processing_start() - rgsis_value.ui64_image_value 내용 : \n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        for (int i_addr_index = 0; i_addr_index < (SPI_IMAGE_END_ADDRESS * SPI_IMAGE_BUSTER_END_DATA_ARRAY); i_addr_index++) {
                            printf("[%d]: 0x%016llX\n", i_addr_index, rgsis_value.ui64_image_value[i_addr_index]);
                        }
                        #endif
                        // break;
                        ipsle_image_processing_status_level = REMAKE_IMAGE;
                    }
                    else if(rgsis_value.cqrre_value == QUEUE_IS_EMPTY){
                        // 큐가 비어있으면
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        vTaskDelay(custom_ms_to_delay(50));
                        break;
                    }
                    else{
                        // 큐가 비어있으면
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 Error\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        vTaskDelay(custom_ms_to_delay(50));
                        break;
                    }
                }   
                
            case REMAKE_IMAGE:
                if(ipsle_image_processing_status_level == REMAKE_IMAGE){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - REMAKE_IMAGE\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    // 2D 배열 포인터로 캐스팅 (복사 없이 빠름)
                    uint8_t (*p_image)[IMAGE_WIDTH] = (uint8_t (*)[IMAGE_WIDTH])rgsis_value.ui64_image_value;
                    // 픽셀 접근
                    printf("GEN FRAME\n");
                    for (int y = 0; y < IMAGE_HEIGHT; y++) {
                        for (int x = 0; x < IMAGE_WIDTH; x++) {
                            A_ui8_frame_buf[y][x] = p_image[y][x];
                            printf("[%d]  ", A_ui8_frame_buf[y][x]);
                        }
                        printf("\n");
                    }

                    ipsle_image_processing_status_level = READ_IMAGE_TO_SPI;
                    break;
                    // ipsle_image_processing_status_level = SEND_IMAGE_TO_UART;
                }

            case SEND_IMAGE_TO_UART:
                if(ipsle_image_processing_status_level == SEND_IMAGE_TO_UART){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - SEND_IMAGE_TO_UART\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    

                    // // Frame 정리 후 Queue 전송 (포인터의 주소를 전달!)
                    // cqrre cqrre_spi_image_application_send_result = custom_queue_safe_send(&mpqs_uart_image_from_app, &A_ui8_frame_buf, 0);
                    // if(cqrre_spi_image_application_send_result != QUEUE_IS_READY){
                    //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                    //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 실패 (-> mpqs_spi_image_to_app) (cqrre_spi_image_application_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, cqrre_spi_image_application_send_result);
                    //     #endif
                    //     // 전송 실패 시에만 메모리 해제
                    //     vPortFree(A_ui64_image_data);
                    //     A_ui64_image_data = NULL;
                    // }
                    // else{
                    //     #if CUSTOM_SPI_IMAGE_RX_PROCESS_THREAD_DEBUG
                    //     printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Thread] custom_spi_image_rx_process_thread() - SPI Image 전송 성공 (-> mpqs_spi_image_to_app) 개수 : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_spi_TAG, custom_queue_safe_messages_waiting(&mpqs_spi_image_to_app));
                    //     #endif
                    //     // 전송 성공 시 소유권 이전 (수신측에서 해제)
                    //     A_ui64_image_data = NULL;
                    // }
                    rsuis_value.cqrre_value = QUEUE_IS_ERROR;
                    rsuis_value.A_ui8_uart_frame_buf = A_ui8_frame_buf;
                    custom_set_uart_image(rsuis_value);
                    if(rsuis_value.cqrre_value == QUEUE_IS_READY){
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - 보내기 성공\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        #if 0
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s app_image_processing_start() - rgsis_value.ui64_image_value 내용 : \n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        for (int i_addr_index = 0; i_addr_index < (SPI_IMAGE_END_ADDRESS * SPI_IMAGE_BUSTER_END_DATA_ARRAY); i_addr_index++) {
                            printf("[%d]: 0x%016llX\n", i_addr_index, rgsis_value.ui64_image_value[i_addr_index]);
                        }
                        #endif
                        // break;
                        for(int j = 0; j < IMAGE_HEIGHT; j++) {
                            vPortFree(A_ui8_frame_buf[j]);
                        }
                        vPortFree(A_ui8_frame_buf);
                        A_ui8_frame_buf = NULL;
                        ipsle_image_processing_status_level = REMAKE_IMAGE;
                    }
                    else if(rgsis_value.cqrre_value == QUEUE_IS_FULL){
                        // 큐가 비어있으면
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 가득 차 있음\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        vTaskDelay(custom_ms_to_delay(50));
                        break;
                    }
                    else{
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 Error\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        vTaskDelay(custom_ms_to_delay(50));
                        break;
                    }

                    // custom_request_uart_tx(UART_TX_ADC_RAW_BUFFER);
                    ipsle_image_processing_status_level = MAKE_FRAME_MALLOC;
                }
                
            case MODE_END:
                if(ipsle_image_processing_status_level == MODE_END){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - MODE_END\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    b_while_end = true;
                }
        }
        vTaskDelay(1);
    }
}