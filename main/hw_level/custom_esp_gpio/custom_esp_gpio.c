/**
 * @file        custom_esp_gpio.c
 * @brief       ESP32의 GPIO Pin 할당 및 제어 코드 파일
 * @author      T0T
 * @date        2025-12-30
 * @version     1.0.0
 * 
 * @details     이 파일은 ESP32의 GPIO 핀 할당과 LED Strip 제어 기능 구현에 필요한 변수 및 함수를 제공함.
 *              - SPI 핀 설정
 *              - UART 핀 설정
 *              - LED Strip 핀 설정 및 제어 기능
 * 
 * @todo        SPI Init 기능 구현해야함
 */

#include "custom_esp_gpio.h"

/**
 * @brief       custom_esp_gpio.c 파일 디버깅 여부
 * @details     custom_esp_gpio.c 파일에서 디버깅 Print 사용 여부를 정의합니다
 */
#define GPIO_DEBUG  DEBUG
// #define GPIO_DEBUG  false

/**
 * @brief       custom_esp_gpio.c 디버깅 Tag
 * @details     Debug Print 시 custom_esp_gpio.c 파일을 구분하기 위한 Tag
 */
static const char *custom_esp_gpio_TAG = "[@]custom_esp_gpio.c";

/*===========================================================================*/
/* 변수 정의
/*===========================================================================*/
/**
 * @brief       GPIO Pin 초기화 상태 저장 배열
 * @details     GPIO Pin 초기화 여부를 저장하는 배열
 */
static bool b_A_gpio_states[40] = {false,};

#if LED_STRIP_ENABLE
    /**
    * @brief       LED Strip 핸들
    * @details     LED Strip를 제어하기 위한 핸들 변수
    */
    static led_strip_handle_t led_strip_handle;
#endif

/*===========================================================================*/
/* 함수 정의
/*===========================================================================*/
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


    //////////////////////////////////////
    /// SPI GPIO Pre-Init (노이즈 방지) ///
    //////////////////////////////////////





    ////////////////////////////////////////////////
    /// DEBUG UART RX GPIO Pre-Init (노이즈 방지) ///
    ////////////////////////////////////////////////
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

    ////////////////////////////////////////////
    /// UPLOAD RX GPIO Pre-Init (노이즈 방지) ///
    ////////////////////////////////////////////
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



    #if LED_STRIP_ENABLE
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
    #endif

    return true;
}