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

    static rgis rgis_value;  // static으로 선언하여 스택 오버플로우 방지 (BSS 섹션에 할당)

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
                    rgis_value = custom_get_spi_image();
                    
                    if(rgis_value.cqrre_value == QUEUE_IS_READY){
                        #if APP_IMAGE_PROCESSING_START_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - 꺼내기 성공\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                        #endif
                        break;
                        // ipsle_image_processing_status_level = REMAKE_IMAGE;
                    }
                    else if(rgis_value.cqrre_value == QUEUE_IS_EMPTY){
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
                    ipsle_image_processing_status_level = SEND_IMAGE_TO_UART;
                }
            case SEND_IMAGE_TO_UART:
                if(ipsle_image_processing_status_level == SEND_IMAGE_TO_UART){
                    #if APP_IMAGE_PROCESSING_START_DEBUG
                    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_image_processing_start() - SEND_IMAGE_TO_UART\n" COLOR_RESET, custom_getRuntimeString(), app_image_processing_TAG);
                    #endif
                    ipsle_image_processing_status_level = READ_IMAGE_TO_SPI;
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