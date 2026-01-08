/**
 * @file        main.c
 * @brief       iSENSOR 프로젝트의 시작 코드 파일
 * @author      T0T
 * @date        2025-12-19
 * @version     1.0.0
 * 
 * @details     (주)아이센(iSEN)에서 개발한 AlphaChip Rev.2의 RAW Image Data를 수신받아 재구성하여 UART를 통해 PC로 송신하는 ESP32 동작 Firmware Code
 */

#include "hw_level_handle.h"         // HAL Level 통합 헤더 사용

/**
 * @brief       메인 함수 디버깅 여부
 * @details     메인 함수에서 디버깅을 위한 디버깅 여부를 정의합니다
 */
#define MAIN_DEBUG         DEBUG
// #define MAIN_DEBUG         false

/**
 * @brief       Debug Print Tag
 * @details     Debug Print 시 main.c 파일을 구분하기 위한 Tag
 */
static const char *main_TAG = "[@]main.c";

/*===========================================================================*/
/* 열거형 정의
/*===========================================================================*/
/**
 * @enum        ile
 * @typedef     initial_list_enum
 * @brief       초기화 리스트 열거형
 * @details     초기화 순서에 따라 모듈의 초기화 상태를 저장하는 배열
 */
typedef enum initial_list_enum{
    INIT_LIST_GPIO,     /**< 0 : GPIO 초기화 */
    INIT_LIST_SPI,      /**< 1 : SPI 초기화 */
    INIT_LIST_UART,     /**< 2 : UART 초기화 */
    INIT_LIST_END,      /**< 3 : 초기화 종료 */
}ile;

/*===========================================================================*/
/* 변수 정의
/*===========================================================================*/
/**
 * @brief       모듈 초기화 상태 저장 배열
 * @details     초기화 순서에 따라 모듈의 초기화 상태를 저장하는 배열
 */
static bool b_A_init_states[INIT_LIST_END] = {false,};

/*===========================================================================*/
/* 함수 정의
/*===========================================================================*/
/**
 * @brief       initial() Function
 * @attention   static[내부 전용]
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     필요한 모듈 초기화 함수
 *              - (동작 순서)
 */
static bool initial(void);

/**
 * @brief       deinitial() Function
 * @attention   static[내부 전용]
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     초기화한 모듈 해제 함수
 */
static bool deinitial(void);

/**
 * @brief       app_main() Function
 * @param[in]   void
 * @return      void
 * @details     전체 어플리케이션 동작 중에서 가장 먼저 동작하는 함수
 */
void app_main(void) {
    #if MAIN_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - 기기 동작 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    g_esp_sleep_wakeup_cause = esp_sleep_get_wakeup_cause();    // 기기가 부팅 된 이유를 확인하는 변수
    custom_wakeup_cause_print(g_esp_sleep_wakeup_cause);

    static dble dble_boot_level     = BOOTING_LEVEL_INITIAL;           // 부팅 Lev을 BOOTING_LEVEL_INITIAL 설정
    static bool b_success           = false;

    // init();
    // while (1) {
    //     vTaskDelay(1);
    // }

    switch(dble_boot_level){
        case BOOTING_LEVEL_INITIAL:
            if(dble_boot_level == BOOTING_LEVEL_INITIAL){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [BOOTING_LEVEL_INITIAL] - 초기 설정 진행\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, BOOTING_LEVEL_INITIAL);
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - initial() 진입 \n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                #if PRINT_DELAY
                ////////////////////////////////////////////////////////
                vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
                ////////////////////////////////////////////////////////
                #endif
                #endif

                b_success = initial();
                if(!b_success){
                    #if INITIAL_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_main() - initial() 초기 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                    #if PRINT_DELAY
                    ////////////////////////////////////////////////////////
                    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
                    ////////////////////////////////////////////////////////
                    #endif
                    #endif
                    dble_boot_level = BOOTING_LEVEL_END;
                }
                else{
                    dble_boot_level = BOOTING_LEVEL_CHECK_CAUSE;
                }
            }

        case BOOTING_LEVEL_CHECK_CAUSE:
            if(dble_boot_level == BOOTING_LEVEL_CHECK_CAUSE){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [BOOTING_LEVEL_CHECK_CAUSE] - 부팅 판단\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, BOOTING_LEVEL_CHECK_CAUSE);
                #endif
                switch(g_esp_sleep_wakeup_cause){
                    case ESP_SLEEP_WAKEUP_UNDEFINED:
                        #if MAIN_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - 첫 부팅인 경우\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - FIRST_BOOT Mode 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        #endif
                        // dble_boot_level = BOOTING_LEVEL_END;
                        // custom_set_iSENSOR_mode_switch_level(FIRST_BOOT);
                        dble_boot_level = BOOTING_LEVEL_APPLICATION_START;
                        break;

                    case ESP_SLEEP_WAKEUP_GPIO:
                    case ESP_SLEEP_WAKEUP_EXT0:
                        #if MAIN_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - EXT0,GPIO로 깨어난 경우\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        #endif

                        #if LED_STRIP_ENABLE
                        // LED GREEN /////////////////////////////////
                        custom_gpio_set_led_strip_color(0, 255, 0);
                        //////////////////////////////////////////////
                        #else
                        custom_pwm_set_duty(get_ledc_channel_num(PWM_CH_BLUE_LED), (33 * custom_pwm_get_max_duty(PWM_CH_BLUE_LED)) / 100);
                        #endif

                    case ESP_SLEEP_WAKEUP_TIMER:
                        #if MAIN_DEBUG
                        if(g_esp_sleep_wakeup_cause != ESP_SLEEP_WAKEUP_EXT0){
                            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Timer로 깨어난 경우\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        }
                        #endif

                        // #if MAIN_DEBUG
                        // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - READ_VALUE_FROM_NVS Mode 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        // #endif
                        // custom_set_iSENSOR_mode_switch_level(READ_VALUE_FROM_NVS);
                        dble_boot_level = BOOTING_LEVEL_APPLICATION_START;
                        break;

                    case ESP_SLEEP_WAKEUP_EXT1:
                        break;
                    case ESP_SLEEP_WAKEUP_TOUCHPAD:
                        break;
                    case ESP_SLEEP_WAKEUP_ULP:
                        break;
                    case ESP_SLEEP_WAKEUP_UART:
                        break;
                    case ESP_SLEEP_WAKEUP_WIFI:
                        break;
                    case ESP_SLEEP_WAKEUP_COCPU:
                        break;
                    case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
                        break;
                    case ESP_SLEEP_WAKEUP_BT:
                        break;
                    default:
                        break;
                }
            }

        case BOOTING_LEVEL_APPLICATION_START:
            if(dble_boot_level == BOOTING_LEVEL_APPLICATION_START){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [BOOTING_LEVEL_APPLICATION_START] - Mode 실행\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, BOOTING_LEVEL_APPLICATION_START);
                #endif

                ////// ESP-C3-SuperMini가 아닌경우

                #if LED_STRIP_ENABLE
                // LED WHITE /////////////////////////////////
                custom_gpio_set_led_strip_color(255, 255, 255);
                //////////////////////////////////////////////
                #else
                custom_pwm_set_duty(PWM_CH_BLUE_LED, (100 * custom_pwm_get_max_duty(PWM_CH_BLUE_LED)) / 100);
                #endif

                // imss_return_value = custom_image_processing_start();

                // if(!imss_return_value.b_thread_start){
                //     if(!imss_return_value.b_occupancy){
                //         #if MAIN_DEBUG
                //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_main() - custom_iSENSOR_mode_start() - Mode Thread 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                //         #endif
                //     }
                //     else{
                //         #if MAIN_DEBUG
                //         printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - custom_iSENSOR_mode_start() - FIRST_ON\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                //         #endif
                //     }
                // }
                // else{
                //     #if MAIN_DEBUG
                //     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - custom_iSENSOR_mode_start() - 정상 종료\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                //     #endif
                //     if(!imss_return_value.b_occupancy){
                //         #if MAIN_DEBUG
                //         printf("[%s] "COLOR_YELLOW"[경고-WARNING]\t %s app_main() - custom_iSENSOR_mode_start() - 재실 인식 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                //         #endif
                //     }
                //     else{
                //         #if MAIN_DEBUG
                //         printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - custom_iSENSOR_mode_start() - 재실 인식 성공\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                //         #endif
                //     }
                // }
                dble_boot_level = BOOTING_LEVEL_END;
            }
            
        case BOOTING_LEVEL_END:
            if(dble_boot_level == BOOTING_LEVEL_END){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [BOOTING_LEVEL_END] - 동작 종료\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, BOOTING_LEVEL_END);
                #endif
                break;
            }

        default:
            #if MAIN_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_main() - Booting Lev.%d - Boot Level Error\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, dble_boot_level);
            #endif
            break;
    }
}

/**
 * @brief       initial() Function
 * @attention   static[내부 전용]
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     필요한 모듈 초기화 함수
 */
static bool initial(void){
    #define INITIAL_DEBUG         MAIN_DEBUG

    ////////////////////////////
    /// GPIO Booting Setting ///
    ////////////////////////////
    #if INITIAL_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s initial() - GPIO Booting Setting\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(!custom_gpio_init()){
        #if INITIAL_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s initial() - custom_gpio_init() GPIO 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #endif
        return false;
    }
    b_A_init_states[INIT_LIST_GPIO] = true;

    #if LED_STRIP_ENABLE
    #if INITIAL_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s initial() - White LED ON(100 / 255) - HW Level Booting 완료\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #endif
    // LED GREEN /////////////////////////////////
    custom_gpio_set_led_strip_color(100, 100, 100);
    //////////////////////////////////////////////
    #else
    #if INITIAL_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s initial() - Blue LED ON(33%%) - HW Level Booting 완료\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    custom_pwm_set_duty(PWM_CH_BLUE_LED, (33 * custom_pwm_get_max_duty(PWM_CH_BLUE_LED)) / 100);
    #endif

    ///////////////////////////
    /// SPI Booting Setting ///
    ///////////////////////////
    #if INITIAL_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s initial() - Custom SPI Booting Setting\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(!custom_spi_init()){
        #if INITIAL_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s initial() - custom_spi_init() Custom SPI 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_init_states[INIT_LIST_SPI] = true;

    ////////////////////////////
    /// UART Booting Setting ///
    ////////////////////////////
    #if INITIAL_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s initial() - Custom UART Booting Setting\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(!custom_uart_init()){
        #if INITIAL_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s initial() - custom_uart_init() Custom UART 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_init_states[INIT_LIST_UART] = true;

    return true;
}

/**
 * @brief       deinitial() Function
 * @attention   static[내부 전용]
 * @param[in]   void
 * @return      bool    true : 초기화 성공, false : 초기화 실패
 * @details     초기화한 모듈 해제 함수
 */
static bool deinitial(void){
    #define DEINITIAL_DEBUG         MAIN_DEBUG

    // static envs envs_input_nvs_value;
    static bool b_success = true;

    if(b_A_init_states[INIT_LIST_UART]){
        ///////////////////////////
        /// UART Ending Setting ///
        ///////////////////////////
        #if DEINITIAL_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s deinitial() - custom_uart_deinit() UART 해제 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        b_success &= custom_uart_deinit();
        if(!b_success){
            #if DEINITIAL_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s deinitial() - custom_uart_deinit() UART 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return b_success;
        }
        b_A_init_states[INIT_LIST_UART] = false;

    }
    if(b_A_init_states[INIT_LIST_SPI]){
        //////////////////////////
        /// SPI Ending Setting ///
        //////////////////////////
        #if DEINITIAL_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s deinitial() - custom_spi_deinit() SPI 해제 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        b_success &= custom_spi_deinit();
        if(!b_success){
            #if DEINITIAL_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s deinitial() - custom_spi_deinit() Custom SPI 종료 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return b_success;
        }
        b_A_init_states[INIT_LIST_SPI] = false;
    }
    if(b_A_init_states[INIT_LIST_GPIO]){
        ///////////////////////////
        /// GPIO Ending Setting ///
        ///////////////////////////
        #if DEINITIAL_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s deinitial() - Custom GPIO 종료 Setting\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        b_success &= custom_gpio_deinit();
        if(!b_success){
            #if DEINITIAL_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s deinitial() - custom_gpio_init() Custom GPIO 종료 실패\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return b_success;
        }
        b_A_init_states[INIT_LIST_GPIO] = false;
    }

    return b_success;
}
