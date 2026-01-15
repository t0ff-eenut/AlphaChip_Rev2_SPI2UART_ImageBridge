    //      COLS (= ui8_width)
    //     ◄─────────────────────►
    // ┌───┬───┬───┬───┬───┬───┬───┐  ▲
    // │   │   │   │   │   │   │   │  │
    // ├───┼───┼───┼───┼───┼───┼───┤  │
    // │   │   │   │   │   │   │   │  │ ROWS
    // ├───┼───┼───┼───┼───┼───┼───┤  │ (= ui8_height)
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
    static uint8_t ui8_height = 0;
    static uint8_t ui8_width = 0;

    uint8_t **A_ui8_frame_buf = NULL;

    static ruifas ruifas_value;
    static rguis rguis_value;

    bool b_while_end = false;


    while(!b_while_end){
        switch(ipsle_image_processing_status_level){
            case MODE_START:
                if(ipsle_image_processing_status_level == MODE_START){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - MODE_START\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
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
                        ipsle_image_processing_status_level = MAKE_FRAME_MALLOC;
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
                        
            case MAKE_FRAME_MALLOC:
                if(ipsle_image_processing_status_level == MAKE_FRAME_MALLOC){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - MAKE_FRAME_MALLOC\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    if(rgsis_value.ui8_end_address < 32){
                        ui8_height = 16;
                        ui8_width = 16;
                    }
                    else{
                        ui8_height = 64;
                        ui8_width = 64;
                    }

                    A_ui8_frame_buf = (uint8_t**)pvPortMalloc(ui8_height * sizeof(uint8_t*));
                    if(A_ui8_frame_buf == NULL){
                        // 메모리 할당 실패 처리
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_image_processing_start() - A_ui8_frame_buf 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        continue; // 다시 반복 시작
                    }
                    // 각 행에 대해 열(column) 할당
                    for(int i = 0; i < ui8_height; i++) {
                        A_ui8_frame_buf[i] = (uint8_t*)pvPortMalloc(ui8_width * sizeof(uint8_t));
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
                        memset(A_ui8_frame_buf[i], 0, ui8_width * sizeof(uint8_t));
                    }

                    ipsle_image_processing_status_level = REMAKE_IMAGE;
                }

            case REMAKE_IMAGE:
                if(ipsle_image_processing_status_level == REMAKE_IMAGE){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - REMAKE_IMAGE\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    // 2D 배열 포인터로 캐스팅 (복사 없이 빠름)
                    uint8_t (*p_image)[ui8_width] = (uint8_t (*)[ui8_width])rgsis_value.ui64_image_value;

                    ipsle_image_processing_status_level = SEND_IMAGE_TO_UART;
                    break;
                }

            case SEND_IMAGE_TO_UART:
                if(ipsle_image_processing_status_level == SEND_IMAGE_TO_UART){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - SEND_IMAGE_TO_UART\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    // rguis_value.cqrre_value = QUEUE_IS_ERROR;
                    ruifas_value.ui8_height = ui8_height;
                    ruifas_value.ui8_width = ui8_width;
                    ruifas_value.A_ui8_uart_frame_buf = A_ui8_frame_buf;
                    cqrre cqrre_value = custom_set_uart_image(ruifas_value);
                    if(cqrre_value == QUEUE_IS_READY){
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - UART로 이미지 보내기 성공\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                    }
                    else if(cqrre_value == QUEUE_IS_FULL){
                        // 큐가 비어있으면
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 가득 차 있음\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                    }
                    else{
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s custom_queue_safe_receive() - 큐 Error\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                    }
                    // ipsle_image_processing_status_level = VIEW_UART_DATA;
                    ipsle_image_processing_status_level = FREE_FRAME_MALLOC;
                }
            
            case VIEW_UART_DATA:
                if(ipsle_image_processing_status_level == VIEW_UART_DATA){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - VIEW_UART_DATA\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    rguis_value = custom_get_uart_image();

                    ipsle_image_processing_status_level = FREE_FRAME_MALLOC;
                    break;
                }

            case FREE_FRAME_MALLOC:
                if(ipsle_image_processing_status_level == FREE_FRAME_MALLOC){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - FREE_FRAME_MALLOC\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif

                    for(int j = 0; j < ui8_height; j++) {
                        vPortFree(A_ui8_frame_buf[j]);
                    }
                    vPortFree(A_ui8_frame_buf);
                    A_ui8_frame_buf = NULL;

                    ipsle_image_processing_status_level = READ_IMAGE_TO_SPI;
                    
                    break;
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