

/*
******************************************************************************
* File Name          : custom_esp_gpio.c
* Description        : ESP32의 GPIO 설정 및 제어
******************************************************************************
* ESP32의 GPIO 설정 및 제어를 위한 코드
* PIR 센서 OUTPUT, 외부 택드 버튼, 서보 모터 제어 등을 포함
* INTERRUPT 만 손 보면 될 듯
******************************************************************************

******************************************************************************
* first update : 2025/12/30
******************************************************************************
* final update : 2025/12/30
******************************************************************************
*/
#include "custom_esp_gpio.h"

// #define GPIO_DEBUG  DEBUG
#define GPIO_DEBUG  false

static const char *custom_esp_gpio_TAG = "[@]custom_esp_gpio.c";

// GPIO Setting 여부
static bool b_A_gpio_states[40] = {false,};

// // PWM 채널 설정 테이블 (딕셔너리처럼 사용)
// static const epccs pwm_config_table[] = {
//     [PWM_CH_LED_PWM] = {
//         .gpio_num           = LED_PWM_GPIO_NUM,
//         .ledc_mode_num      = LED_PWM_MODE,
//         .ledc_timer_num     = LED_PWM_TIMER,
//         .ledc_channel_num   = LED_PWM_CHANNEL,
//         .ledc_timer_bit     = LED_PWM_DUTY_RES,
//         .ui32_frequency     = LED_PWM_FREQUENCY,
//         .ledc_clk_cfg_num   = LEDC_AUTO_CLK,
//     },
// #if !LED_STRIP_ENABLE
//     [PWM_CH_BLUE_LED] = {
//         .gpio_num           = BLUE_LED_PWM_GPIO_NUM,
//         .ledc_mode_num      = BLUE_LED_PWM_MODE,
//         .ledc_timer_num     = BLUE_LED_PWM_TIMER,
//         .ledc_channel_num   = BLUE_LED_PWM_CHANNEL,
//         .ledc_timer_bit     = BLUE_LED_PWM_DUTY_RES,
//         .ui32_frequency     = BLUE_LED_PWM_FREQUENCY,
//         .ledc_clk_cfg_num   = LEDC_AUTO_CLK,
//     },
// #endif
// };
#if LED_STRIP_ENABLE
static led_strip_handle_t led_strip_handle;
#endif

// ledc_channel_t get_ledc_channel_num(epcie input_epcie){
//     return pwm_config_table[input_epcie].ledc_channel_num;
// }


bool custom_gpio_init(void){
    #define CUSTOM_GPIO_INIT_DEBUG         GPIO_DEBUG

    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - GPIO 초기 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    //////////////////////////////
    /// LED Strip GPIO Initial ///
    //////////////////////////////
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - RGB LED Control을 위해 LED_STRIP_GPIO_NUM[%d] Strip Config 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_STRIP_GPIO_NUM[%d] led_strip_config_t.strip_gpio_num[LED_STRIP_GPIO_NUM(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM, LED_STRIP_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_STRIP_GPIO_NUM[%d] led_strip_config_t.max_leds[LED_STRIP_LEN(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM, LED_STRIP_LEN);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_STRIP_GPIO_NUM[%d] led_strip_rmt_config_t.resolution_hz[LED_STRIP_RESOLUTION_HZ(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM, LED_STRIP_RESOLUTION_HZ);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_STRIP_GPIO_NUM[%d] led_strip_rmt_config_t.flags.with_dma[LED_STRIP_WITH_DMA(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM, LED_STRIP_WITH_DMA);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    static const led_strip_config_t strip_config = {
        .strip_gpio_num = LED_STRIP_GPIO_NUM,
        .max_leds = LED_STRIP_LEN, // LED 개수
    };
    static const led_strip_rmt_config_t rmt_config = {
        .resolution_hz = LED_STRIP_RESOLUTION_HZ,
        .flags.with_dma = LED_STRIP_WITH_DMA,
    };
    if(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_handle) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - LED_STRIP_GPIO_NUM[%d] led_strip_new_rmt_device() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM);
        #endif
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        return false;
    }
    b_A_gpio_states[LED_STRIP_GPIO_NUM] = true;


    /////////////////////////////
    /// SPI Init ///
    /////////////////////////////





    ////////////////////////////////////////
    /// UART RX GPIO Pre-Init (노이즈 방지) ///
    ////////////////////////////////////////
    // UART 드라이버 초기화 전 RX 핀에 풀업 설정하여 플로팅으로 인한 쓰레기값 유입 방지
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - DEBUG_UART RX 핀 노이즈 방지를 위한 DEBUG_RXD_GPIO_NUM[%d] 풀업 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - DEBUG_RXD_GPIO_NUM[%d] GPIO_MODE_INPUT 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - DEBUG_RXD_GPIO_NUM[%d] GPIO_PULLUP_ENABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - DEBUG_RXD_GPIO_NUM[%d] GPIO_PULLDOWN_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - DEBUG_RXD_GPIO_NUM[%d] GPIO_INTR_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    static const gpio_config_t debug_uart_rx_io_conf = {
        .pin_bit_mask   = (1ULL << DEBUG_RXD_GPIO_NUM),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,       // 풀업 활성화로 노이즈 방지
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE
    };
    if(gpio_config(&debug_uart_rx_io_conf) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - DEBUG_RXD_GPIO_NUM[%d] gpio_config() 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, DEBUG_RXD_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_gpio_states[DEBUG_RXD_GPIO_NUM] = true;

    ////////////////////////////////////////
    /// UPLOAD_LOG_RXD GPIO Pre-Init (노이즈 방지) ///
    ////////////////////////////////////////
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - UPLOAD_LOG RX 핀 노이즈 방지를 위한 UPLOAD_LOG_RXD_GPIO_NUM[%d] 풀업 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - UPLOAD_LOG_RXD_GPIO_NUM[%d] GPIO_MODE_INPUT 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - UPLOAD_LOG_RXD_GPIO_NUM[%d] GPIO_PULLUP_ENABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - UPLOAD_LOG_RXD_GPIO_NUM[%d] GPIO_PULLDOWN_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - UPLOAD_LOG_RXD_GPIO_NUM[%d] GPIO_INTR_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    static const gpio_config_t uart0_rx_io_conf = {
        .pin_bit_mask   = (1ULL << UPLOAD_LOG_RXD_GPIO_NUM),
        .mode           = GPIO_MODE_INPUT,
        .pull_up_en     = GPIO_PULLUP_ENABLE,       // 풀업 활성화로 노이즈 방지
        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
        .intr_type      = GPIO_INTR_DISABLE
    };
    if(gpio_config(&uart0_rx_io_conf) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - UPLOAD_LOG_RXD_GPIO_NUM[%d] gpio_config() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, UPLOAD_LOG_RXD_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_gpio_states[UPLOAD_LOG_RXD_GPIO_NUM] = true;



    
    #if CUSTOM_GPIO_INIT_DEBUG
    // 활성화된 GPIO 핀 목록 출력
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - 활성화된 GPIO 핀 : ", custom_getRuntimeString(), custom_esp_gpio_TAG);
    for(int i = 0; i < 40; i++){
        if(b_A_gpio_states[i]){
            printf("GPIO%d ", i);
        }
    }
    printf("\n" COLOR_RESET);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - GPIO 초기 설정 END\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    return true;
}

bool custom_gpio_set_led_strip_color(const uint32_t input_ui32_red_value, const uint32_t input_ui32_green_value, const uint32_t input_ui32_blue_value){

    #define CUSTOM_GPIO_SET_LED_STRIP_COLOR_DEBUG   GPIO_DEBUG

    #if CUSTOM_GPIO_SET_LED_STRIP_COLOR_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_set_led_strip_color() - LED Strip Color 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_set_led_strip_color() - input_ui32_red_value : %ld | input_ui32_green_value : %ld | input_ui32_blue_value : %ld \n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, input_ui32_red_value, input_ui32_green_value, input_ui32_blue_value);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    if(!led_strip_set_pixel(led_strip_handle, 0, input_ui32_red_value, input_ui32_green_value, input_ui32_blue_value)){
        #if CUSTOM_GPIO_SET_LED_STRIP_COLOR_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_set_led_strip_color() - led_strip_set_pixel() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////    
        #endif
        #endif
        return false;
    }
    if(!led_strip_refresh(led_strip_handle)){
        #if CUSTOM_GPIO_SET_LED_STRIP_COLOR_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_set_led_strip_color() - led_strip_refresh() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
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

void custom_gpio_clear_led_strip(void){
    #if LED_STRIP_ENABLE
        led_strip_clear(led_strip_handle);
    #else
        #warning "led_strip_handle이 정의되어 있지 않습니다."
        #if CUSTOM_GPIO_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_clear_led_strip() - led_strip_handle이 정의되어 있지 않습니다.\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
        #endif
    #endif
}

bool custom_gpio_deinit(void){
    #define CUSTOM_GPIO_DEINIT_DEBUG         GPIO_DEBUG

    custom_gpio_clear_led_strip();
    if(b_A_gpio_states[LED_STRIP_GPIO_NUM]){
        // 2. LED 스트립 리소스 해제
        #if CUSTOM_GPIO_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_deinit() - led_strip_handle 설정 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        if(!led_strip_del(led_strip_handle)){
            #if CUSTOM_GPIO_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_deinit() - LED_STRIP_GPIO_NUM[%d] led_strip_del() led_strip_handle 설정 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_STRIP_GPIO_NUM);
            #endif
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            return false;
        }
        led_strip_handle = NULL; // 핸들을 NULL로 설정하여 중복 해제 방지
        b_A_gpio_states[LED_STRIP_GPIO_NUM] = false;
    }

    return true;
}