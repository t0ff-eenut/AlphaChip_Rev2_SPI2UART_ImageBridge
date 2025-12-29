/*
************************************************************************************************************
* File Name          : main.c
* Description        : Main program body
************************************************************************************************************
* (주)아이센(iSEN)에서 개발한 AlphaChip Rev.2의 RAW Image Data를 수신받아 재구성하여 UART를 통해 PC로 송신하는 ESP32 동작 Firmware Code
************************************************************************************************************

************************************************************************************************************
* first update : 2025/12/29
************************************************************************************************************
* final update : 2025/12/29
************************************************************************************************************
*/

#include "hw_level_handle.h"         // HAL Level 통합 헤더 사용


// // Queue
// #include "queue/queue.h"
// // SPI
// #include "spi/spi.h"
// // UART
// // USB
// #include "uart/uart.h"

// void init(void) {
//   view_heap_stack("Main init Start");

//   if (queue_init()) {
//     printf("Queue Error\n");
//     // error
//   }
//   if (gpio_init()) {
//     printf("GPIO Error\n");
//     // error
//   }
//   if (spi_init()) {
//     printf("SPI Error\n");
//     // error
//   }
//   if (uart_init()) {
//     printf("UART Error\n");
//     // error
//   }
// }


#define MAIN_DEBUG         DEBUG
// #define MAIN_DEBUG         false
static const char *main_TAG = "[@]main.c";

typedef enum initial_list_enum{
    INIT_LIST_GPIO,
    INIT_LIST_SPI,
    INIT_LIST_UART,
    INIT_LIST_END,
}ile;
static bool b_A_init_states[INIT_LIST_END] = {false,};

static bool initial(void);
static bool deinitial(void);

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

    static sble sble_boot_level     = SWITCH_INITIAL;           // 부팅 Lev을 SWITCH_INITIAL 설정
    static bool b_success           = false;

    // init();
    // while (1) {
    //     vTaskDelay(1);
    // }

    switch(sble_boot_level){
        case SWITCH_INITIAL:
            if(sble_boot_level == SWITCH_INITIAL){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [SWITCH_INITIAL] - 초기 설정 진행\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, SWITCH_INITIAL);
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
                    sble_boot_level = SWITCH_BOOTING_END;
                }
                else{
                    sble_boot_level = SWITCH_BOOTING_CHECK_CAUSE;
                }
            }

        case SWITCH_BOOTING_CHECK_CAUSE:
            if(sble_boot_level == SWITCH_BOOTING_CHECK_CAUSE){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [SWITCH_BOOTING_CHECK_CAUSE] - 부팅 판단\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, SWITCH_BOOTING_CHECK_CAUSE);
                #endif
                switch(g_esp_sleep_wakeup_cause){
                    case ESP_SLEEP_WAKEUP_UNDEFINED:
                        #if MAIN_DEBUG
                        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - 첫 부팅인 경우\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - FIRST_BOOT Mode 시작\n" COLOR_RESET, custom_getRuntimeString(), main_TAG);
                        #endif
                        // sble_boot_level = SWITCH_BOOTING_END;
                        // custom_set_iSENSOR_mode_switch_level(FIRST_BOOT);
                        sble_boot_level = SWITCH_BOOTING_APPLICATION_START;
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
                        sble_boot_level = SWITCH_BOOTING_APPLICATION_START;
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

        case SWITCH_BOOTING_APPLICATION_START:
            if(sble_boot_level == SWITCH_BOOTING_APPLICATION_START){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [SWITCH_BOOTING_APPLICATION_START] - Mode 실행\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, SWITCH_BOOTING_APPLICATION_START);
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
                sble_boot_level = SWITCH_BOOTING_END;
            }
            
        case SWITCH_BOOTING_END:
            if(sble_boot_level == SWITCH_BOOTING_END){
                #if MAIN_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s app_main() - Booting Lev.%d [SWITCH_BOOTING_END] - 동작 종료\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, SWITCH_BOOTING_END);
                #endif
                break;
            }

        default:
            #if MAIN_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s app_main() - Booting Lev.%d - Boot Level Error\n" COLOR_RESET, custom_getRuntimeString(), main_TAG, sble_boot_level);
            #endif
            break;
    }
}

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
