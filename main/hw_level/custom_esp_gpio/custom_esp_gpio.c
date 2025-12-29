

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
* first update : 2025/10/13
******************************************************************************
* final update : 2025/11/27
******************************************************************************
*/
#include "custom_esp_gpio.h"

// #define GPIO_DEBUG  DEBUG
#define GPIO_DEBUG  false

static const char *custom_esp_gpio_TAG = "[@]custom_esp_gpio.c";

// GPIO Setting 여부
static bool b_A_gpio_states[40] = {false,};

// PWM 채널 설정 테이블 (딕셔너리처럼 사용)
static const epccs pwm_config_table[] = {
    [PWM_CH_LED_PWM] = {
        .gpio_num           = LED_PWM_GPIO_NUM,
        .ledc_mode_num      = LED_PWM_MODE,
        .ledc_timer_num     = LED_PWM_TIMER,
        .ledc_channel_num   = LED_PWM_CHANNEL,
        .ledc_timer_bit     = LED_PWM_DUTY_RES,
        .ui32_frequency     = LED_PWM_FREQUENCY,
        .ledc_clk_cfg_num   = LEDC_AUTO_CLK,
    },
#if !LED_STRIP_ENABLE
    [PWM_CH_BLUE_LED] = {
        .gpio_num           = BLUE_LED_PWM_GPIO_NUM,
        .ledc_mode_num      = BLUE_LED_PWM_MODE,
        .ledc_timer_num     = BLUE_LED_PWM_TIMER,
        .ledc_channel_num   = BLUE_LED_PWM_CHANNEL,
        .ledc_timer_bit     = BLUE_LED_PWM_DUTY_RES,
        .ui32_frequency     = BLUE_LED_PWM_FREQUENCY,
        .ledc_clk_cfg_num   = LEDC_AUTO_CLK,
    },
#endif
};
#if LED_STRIP_ENABLE
static led_strip_handle_t led_strip_handle;
#endif

ledc_channel_t get_ledc_channel_num(epcie input_epcie){
    return pwm_config_table[input_epcie].ledc_channel_num;
}


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


    ///////////////////////////////////
    /// INTER_DEBUG_MODE GPIO Initial ///
    ///////////////////////////////////
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] 초기화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] GPIO_MODE_INPUT 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] GPIO_PULLUP_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] GPIO_PULLDOWN_ENABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] GPIO_INTR_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    static const gpio_config_t debug_mode_io_conf  = {
        .pin_bit_mask   = (1ULL << INTER_DEBUG_MODE_GPIO_NUM),    //	INTER_DEBUG_MODE_GPIO_NUM 핀 선택 (비트마스크 방식)
        .mode           = GPIO_MODE_INPUT,                  //	입력 모드 설정
        .pull_up_en     = GPIO_PULLUP_DISABLE,              //	풀업 저항 비활성화
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,             //	풀다운 저항 활성화
        .intr_type      = GPIO_INTR_DISABLE                 //	인터럽트 비활성화
    };
    if(gpio_config(&debug_mode_io_conf ) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - INTER_DEBUG_MODE_GPIO_NUM[%d] gpio_config() 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, INTER_DEBUG_MODE_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_gpio_states[INTER_DEBUG_MODE_GPIO_NUM] = true;
    

    /*
    /////////////////////////////
    /// NVS 초기화 버튼 Initial ///
    /////////////////////////////
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - NVS 초기화를 위한 NVS_RESET_BUTTON_GPIO_NUM[%d] 설정(BOOT 버튼)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - NVS_RESET_BUTTON_GPIO_NUM[%d] GPIO_MODE_INPUT 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
    #endif
    
    b_success &= !gpio_set_direction(NVS_RESET_BUTTON_GPIO_NUM, GPIO_MODE_INPUT);
    if(!b_success){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - NVS_RESET_BUTTON_GPIO_NUM[%d] GPIO_MODE_INPUT 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
        #endif
        return b_success;
    }
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - NVS_RESET_BUTTON_GPIO_NUM[%d] GPIO_PULLUP_ONLY 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
    #endif
    
    b_success &= !gpio_set_pull_mode(NVS_RESET_BUTTON_GPIO_NUM, GPIO_PULLUP_ONLY);
    if(!b_success){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - NVS_RESET_BUTTON_GPIO_NUM[%d] GPIO_PULLUP_ONLY 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
        #endif
        return b_success;
    }
    b_A_gpio_states[NVS_RESET_BUTTON_GPIO_NUM] = true;
    */

    ///////////////////////////////////
    /// PIR Signal 수신 GPIO Initial ///
    ///////////////////////////////////
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - 외부 PIR Signal 수신을 위한 PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] 초기화\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] GPIO_MODE_INPUT 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] GPIO_PULLUP_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] GPIO_PULLDOWN_ENABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] GPIO_INTR_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    static const gpio_config_t pir_output_io_conf = {
        .pin_bit_mask   = (1ULL << PIR_SIGNAL_OUTPUT_GPIO_NUM),    //	PIR_SIGNAL_OUTPUT_GPIO_NUM 핀 선택 (비트마스크 방식)
        .mode           = GPIO_MODE_INPUT,                  //	입력 모드 설정
        .pull_up_en     = GPIO_PULLUP_DISABLE,              //	풀업 저항 비활성화
        .pull_down_en   = GPIO_PULLDOWN_ENABLE,             //	풀다운 저항 활성화
        .intr_type      = GPIO_INTR_DISABLE                 //	인터럽트 비활성화
    };
    if(gpio_config(&pir_output_io_conf) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] gpio_config() 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_gpio_states[PIR_SIGNAL_OUTPUT_GPIO_NUM] = true;

    ////////////////////////////
    /// LED PWM GPIO Initial ///
    ////////////////////////////
    // #if CUSTOM_GPIO_INIT_DEBUG
    // printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] GPIO_INTR_DISABLE 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
    // #endif
    // [추가됨] DeepSleep에서 깨어났을 때 Hold 된 GPIO를 해제해야 제어 가능
    // gpio_hold_dis(LED_PWM_GPIO_NUM); 

    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - LED PWM Signal 출력을 위한 LED_PWM_GPIO_NUM[%d] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config_t.speed_mode[pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config_t.timer_num[LED_PWM_TIMER(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_TIMER);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config_t.duty_resolution[LED_PWM_DUTY_RES(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_DUTY_RES);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config_t.freq_hz[LED_PWM_FREQUENCY(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_FREQUENCY);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config_t.clk_cfg[LEDC_AUTO_CLK(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LEDC_AUTO_CLK);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif

    // 1. LEDC 타이머 설정
    static ledc_timer_config_t s_ledc_timer_cfg = {
        .speed_mode       = pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num,
        .timer_num        = pwm_config_table[PWM_CH_LED_PWM].ledc_timer_num,
        .duty_resolution  = pwm_config_table[PWM_CH_LED_PWM].ledc_timer_bit,
        .freq_hz          = pwm_config_table[PWM_CH_LED_PWM].ui32_frequency,
        .clk_cfg          = pwm_config_table[PWM_CH_LED_PWM].ledc_clk_cfg_num,
    };
    if(ledc_timer_config(&s_ledc_timer_cfg) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_timer_config() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    #if CUSTOM_GPIO_INIT_DEBUG
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.speed_mode[pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.channel[LED_PWM_CHANNEL(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_CHANNEL);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.timer_sel[LED_PWM_TIMER(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_TIMER);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.intr_type[LEDC_INTR_DISABLE(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LEDC_INTR_DISABLE);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.gpio_num[LED_PWM_GPIO_NUM(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM, LED_PWM_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.duty[0] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
    printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.hpoint[0] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    // 2. LEDC 채널 설정 (LED_PWM)
    static ledc_channel_config_t s_ledc_channel_cfg_pwm = {
        .speed_mode     = pwm_config_table[PWM_CH_LED_PWM].ledc_mode_num,
        .channel        = pwm_config_table[PWM_CH_LED_PWM].ledc_channel_num,
        .timer_sel      = pwm_config_table[PWM_CH_LED_PWM].ledc_timer_num,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LED_PWM_GPIO_NUM,
        .duty           = 0, // 초기 듀티 사이클 (0%)
        .hpoint         = 0
    };
    if(ledc_channel_config(&s_ledc_channel_cfg_pwm) != ESP_OK){
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - LED_PWM_GPIO_NUM[%d] ledc_channel_config() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        return false;
    }
    b_A_gpio_states[LED_PWM_GPIO_NUM] = true;


    #if LED_STRIP_ENABLE
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

    #else
        ////////////////////////////
        /// LED PWM GPIO Initial ///
        ////////////////////////////
        // LEDC 채널 설정 (LED_BLUE)
        #if CUSTOM_GPIO_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_init() - BLUE LED PWM Signal 출력을 위한 BLUE_LED_PWM_GPIO_NUM[%d] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.speed_mode[BLUE_LED_PWM_MODE(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM, BLUE_LED_PWM_MODE);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.channel[BLUE_LED_PWM_CHANNEL(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM, BLUE_LED_PWM_CHANNEL);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.timer_sel[BLUE_LED_PWM_TIMER(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM, BLUE_LED_PWM_TIMER);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.intr_type[LEDC_INTR_DISABLE(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM, LEDC_INTR_DISABLE);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.gpio_num[BLUE_LED_PWM_GPIO_NUM(%d)] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM, BLUE_LED_PWM_GPIO_NUM);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.duty[0] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config_t.hpoint[0] 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        static ledc_channel_config_t s_ledc_channel_cfg_blue = {
            .speed_mode     = BLUE_LED_PWM_MODE,
            .channel        = BLUE_LED_PWM_CHANNEL,
            .timer_sel      = BLUE_LED_PWM_TIMER,
            .intr_type      = LEDC_INTR_DISABLE,
            .gpio_num       = BLUE_LED_PWM_GPIO_NUM,
            .duty           = 0, // 초기 듀티 사이클 (0%)
            .hpoint         = 0
        };
        if(ledc_channel_config(&s_ledc_channel_cfg_blue) != ESP_OK){
            #if CUSTOM_GPIO_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_init() - BLUE_LED_PWM_GPIO_NUM[%d] ledc_channel_config() 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
        b_A_gpio_states[BLUE_LED_PWM_GPIO_NUM] = true;
    #endif

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

/// PWM //////////////////////////////////////////////////////////////////////////
uint32_t custom_pwm_get_max_duty(epcie input_epcie){
    return (1 << pwm_config_table[input_epcie].ledc_timer_bit) - 1;
}
uint32_t custom_pwm_get_frequency(epcie input_epcie){
    return ledc_get_freq(pwm_config_table[input_epcie].ledc_mode_num, pwm_config_table[input_epcie].ledc_channel_num);
}
void custom_pwm_set_duty(epcie input_epcie, uint32_t intput_ui32_duty){
    // 해상도(Bit)에 따른 최대값(Max Duty) 자동 계산
    uint32_t ui32_max_duty = custom_pwm_get_max_duty(input_epcie);
    // 설정한 해상도 범위를 넘지 않도록 제한
    if (intput_ui32_duty > ui32_max_duty)
        intput_ui32_duty = ui32_max_duty;
    ledc_set_duty(pwm_config_table[input_epcie].ledc_mode_num, pwm_config_table[input_epcie].ledc_channel_num, intput_ui32_duty);
    ledc_update_duty(pwm_config_table[input_epcie].ledc_mode_num, pwm_config_table[input_epcie].ledc_channel_num);
}

uint32_t custom_pwm_get_duty(epcie input_epcie){
    return ledc_get_duty(pwm_config_table[input_epcie].ledc_mode_num, pwm_config_table[input_epcie].ledc_channel_num);
}
/// PWM //////////////////////////////////////////////////////////////////////////

#if LED_STRIP_ENABLE
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
#endif

bool custom_gpio_pir_output_on_check(void){
    return gpio_get_level(PIR_SIGNAL_OUTPUT_GPIO_NUM) != 0;
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
    #endif

    if(b_A_gpio_states[NVS_RESET_BUTTON_GPIO_NUM]){
        // 1. NVS 리셋 버튼 GPIO 해제
        #if CUSTOM_GPIO_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_deinit() - NVS 초기화를 위한 NVS_RESET_BUTTON_GPIO_NUM[%d] 설정 해제(BOOT 버튼)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
        #endif
        if(!gpio_reset_pin(NVS_RESET_BUTTON_GPIO_NUM)){
            #if CUSTOM_GPIO_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_deinit() - NVS_RESET_BUTTON_GPIO_NUM[%d] gpio_reset_pin() 설정 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, NVS_RESET_BUTTON_GPIO_NUM);
            #endif
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            return false;
        }
        b_A_gpio_states[NVS_RESET_BUTTON_GPIO_NUM] = false;
    }

    // if(b_A_gpio_states[PIR_SIGNAL_OUTPUT_GPIO_NUM]){
    //     // 3. SENSOR 모드에서 사용된 GPIO 해제
    //     #if CUSTOM_GPIO_DEINIT_DEBUG
    //     printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_deinit() - 외부 PIR Signal Interrupt 수신을 위한 PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] 설정 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    //     #endif
    //     b_success &= !gpio_reset_pin(PIR_SIGNAL_OUTPUT_GPIO_NUM);
    //     if(!b_success){
    //         #if CUSTOM_GPIO_DEINIT_DEBUG
    //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_deinit() - PIR_SIGNAL_OUTPUT_GPIO_NUM[%d] gpio_reset_pin() 설정 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, PIR_SIGNAL_OUTPUT_GPIO_NUM);
    //         #endif
    //         return b_success;
    //     }
    //     b_A_gpio_states[PIR_SIGNAL_OUTPUT_GPIO_NUM] = false;
    // }
    
    if(b_A_gpio_states[LED_PWM_GPIO_NUM]){
        // 4. LED PWM GPIO 해제
        #if CUSTOM_GPIO_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_deinit() - LED PWM Signal 송신을 위한 LED_PWM_GPIO_NUM[%d] 설정 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        
        // custom_pwm_set_duty(0);
        // // 1. PWM 출력 정지 (Idle Level을 0으로 설정하여 끔)
        // ledc_stop(LED_PWM_MODE, LED_PWM_CHANNEL, 0);

        // 2. GPIO 핀 리셋 (입력 모드, 풀업/풀다운 없음 상태로 초기화)
        // b_success &= !gpio_reset_pin(LED_PWM_GPIO_NUM);
        // gpio_config_t cfg = {
        //     .pin_bit_mask = BIT64(LED_PWM_GPIO_NUM),
        //     .mode = GPIO_MODE_OUTPUT,
        //     //for powersave reasons, the GPIO should not be floating, select pullup
        //     .pull_up_en = false,
        //     .pull_down_en = true,
        //     .intr_type = GPIO_INTR_DISABLE,
        // };
        // b_success &= !gpio_config(&cfg);
        // if(!b_success){
        //     #if CUSTOM_GPIO_DEINIT_DEBUG
        //     printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_deinit() - LED_PWM_GPIO_NUM[%d] gpio_config() 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, LED_PWM_GPIO_NUM);
        //     #endif
        //     return b_success;
        // }


        // gpio_set_level(GPIO_NUM_3, 1); // High 출력


        // gpio_hold_en(LED_PWM_GPIO_NUM);
        b_A_gpio_states[LED_PWM_GPIO_NUM] = false;
    }

    #if LED_STRIP_ENABLE
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
    #else
        if(b_A_gpio_states[BLUE_LED_PWM_GPIO_NUM]){
            // LED PWM GPIO 해제
            // ※ 주의: LED_BLUE와 LED_PWM은 같은 타이머(LEDC_TIMER_0)를 공유함
            //   - ledc_stop()은 채널 단위로 동작하므로 다른 채널에 영향 없음
            //   - 타이머 함수(ledc_timer_rst, ledc_timer_pause 등)는 절대 호출하지 말 것!
            //     → 호출 시 LED_PWM도 함께 영향받음
            #if CUSTOM_GPIO_DEINIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_gpio_deinit() - LED PWM Signal 송신을 위한 BLUE_LED_PWM_GPIO_NUM[%d] 설정 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            
            // 1. PWM duty를 0으로 설정
            custom_pwm_set_duty(pwm_config_table[PWM_CH_BLUE_LED].ledc_channel_num, 0);
            
            // 2. PWM 출력 정지 (Idle Level을 0으로 설정하여 끔) - 채널만 정지, 타이머는 유지
            ledc_stop(pwm_config_table[PWM_CH_BLUE_LED].ledc_mode_num, pwm_config_table[PWM_CH_BLUE_LED].ledc_channel_num, 0);

            // 3. GPIO 핀 리셋 (기본 상태로 초기화: input, pull-up 활성)
            if(gpio_reset_pin(BLUE_LED_PWM_GPIO_NUM) != ESP_OK){
                #if CUSTOM_GPIO_DEINIT_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_gpio_deinit() - BLUE_LED_PWM_GPIO_NUM[%d] gpio_reset_pin() 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_gpio_TAG, BLUE_LED_PWM_GPIO_NUM);
                #endif
                #if PRINT_DELAY
                ////////////////////////////////////////////////////////
                vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
                ////////////////////////////////////////////////////////
                #endif
                return false;
            }

            b_A_gpio_states[BLUE_LED_PWM_GPIO_NUM] = false;
        }
    #endif

    return true;
}