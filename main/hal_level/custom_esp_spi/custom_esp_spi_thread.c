/*
******************************************************************************
* File Name          : custom_all_adc_process_thread.c
* Description        : ADC DATA READ AND PROCESSING MODULE
******************************************************************************
* ADC 입력 ANALOG SIGNAL을 ESP32 ADC로 읽어들이기 위한 모듈
* ADC 출력 데이터를 QUEUE로 전송하여 Buffer를 구성
* adc_init 후 adc_buf Thread 실행
******************************************************************************

******************************************************************************
* first update : 2026/01/02
******************************************************************************
* final update : 2026/01/02
******************************************************************************
*/

#include "custom_esp_spi_thread.h"
#include "lwip/sockets.h"

#define ADC_DEBUG         DEBUG
// #define ADC_DEBUG         false

static const char *custom_esp_adc_TAG = "[@]custom_esp_adc.c";


static adc_oneshot_unit_handle_t adc_oneshot_unit_handle;
static adc_cali_handle_t adc_cali_handle;

static volatile TaskHandle_t TaskHandle_custom_all_adc_read_thread = NULL;
static volatile TaskHandle_t TaskHandle_custom_all_adc_thread = NULL;

static volatile bool b_all_adc_break = false;                                // 외부 핸들 함수 만들기
static volatile bool b_all_adc_breaked = false;                              // 외부 핸들 함수 만들기

#if ADC_RAW_ENABLE
    // ═══════════════════════════════════════════════════════════════════════════════
    // Mutex 보호 큐 (Thread-Safe Queue) - RAW 채널
    // ═══════════════════════════════════════════════════════════════════════════════
    static mpqs mpqs_iSENSOR_raw_read;  // Raw ADC 값 저장 큐 (uint16_t)
    static mpqs mpqs_iSENSOR_raw;       // 처리된 ADC 값 저장 큐 (avs*)
#endif

#if ADC_HPF_ENABLE
    // ═══════════════════════════════════════════════════════════════════════════════
    // Mutex 보호 큐 (Thread-Safe Queue) - HPF 채널 (조건부)
    // ═══════════════════════════════════════════════════════════════════════════════
    static mpqs mpqs_iSENSOR_hpf_read;  // HPF Raw ADC 값 저장 큐 (uint16_t)
    static mpqs mpqs_iSENSOR_hpf;       // HPF 처리된 ADC 값 저장 큐 (avs*)
    // HPF 채널은 외부 하드웨어에서 이미 필터링된 신호를 받으므로 소프트웨어 필터 불필요
#endif

#if ADC_BPF_ENABLE
    // ═══════════════════════════════════════════════════════════════════════════════
    // Mutex 보호 큐 (Thread-Safe Queue) - BPF 채널 (조건부)
    // ═══════════════════════════════════════════════════════════════════════════════
    static mpqs mpqs_iSENSOR_bpf_read;  // BPF Raw ADC 값 저장 큐 (uint16_t)
    static mpqs mpqs_iSENSOR_bpf;       // BPF 처리된 ADC 값 저장 큐 (avs*)
    // BPF 채널은 외부 하드웨어에서 이미 필터링된 신호를 받으므로 소프트웨어 필터 불필요
#endif

#if ADC_SW_HPF_ENABLE
    // ═══════════════════════════════════════════════════════════════════════════════
    // SW HPF 필터 (High-Pass Filter) - DC 성분 제거
    // ═══════════════════════════════════════════════════════════════════════════════
    static float hpf_coeffs[5];
    static float hpf_delay[2];
    #if ADC_SW_BPF_ENABLE
        // ═══════════════════════════════════════════════════════════════════════════════
        // SW BPF 필터 (Band-Pass Filter) - HPF + LPF 직렬 연결
        // BPF = HPF(0.3Hz) → LPF(5Hz) = 0.3Hz ~ 5Hz 대역 통과
        // ═══════════════════════════════════════════════════════════════════════════════
        static float lpf_coeffs[5];   // BPF의 LPF 단 (고주파 차단)
        static float lpf_delay[2];    // BPF LPF 지연 상태
    #endif
#endif


bool custom_adc_init(void){
    #define CUSTOM_ADC_INIT_DEBUG         ADC_DEBUG

    static int i_xTaskCreate_return_value = 0;
    
    static adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id    = ADC_RAW_UNIT,
        .ulp_mode   = ADC_ULP_MODE_DISABLE
    };
    #if CUSTOM_ADC_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - adc_oneshot_new_unit(&unit_cfg, &adc_oneshot_unit_handle) ADC Unit Handle 생성\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    if(adc_oneshot_new_unit(&unit_cfg, &adc_oneshot_unit_handle) != ESP_OK){
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - adc_oneshot_new_unit(&unit_cfg, &adc_oneshot_unit_handle) ADC Unit Handle 생성 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif

        custom_adc_deinit();
        return false;
    }

    static adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth   = ADC_BITWIDTH_12,
        .atten      = ADC_ATTEN_DB_12
    };

///////////////////////////////////////////////////////////// 메크로 각 /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    #if ADC_RAW_ENABLE
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - RAW 채널(ADC_RAW_CHANNEL) 설정\n"
            "1. adc_oneshot_config_channel() | ADC_RAW_CHANNEL\n"
            "2. custom_mpqs_init() | mpqs_iSENSOR_raw_read\n"
            "3. custom_mpqs_init() | mpqs_iSENSOR_raw\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif

        // ★ GPIO2는 ESP32-C3의 스트래핑 핀으로 기본 내부 풀업이 활성화됨
        // 외부 센서 연결 시 HIGH로 고정되는 문제를 방지하기 위해 풀업/풀다운 비활성화
        // gpio_set_pull_mode(GPIO_NUM_2, GPIO_FLOATING);
        // 또는
        // gpio_pullup_dis(GPIO_NUM_2);
        // gpio_pulldown_dis(GPIO_NUM_2);

        if(adc_oneshot_config_channel(adc_oneshot_unit_handle, ADC_RAW_CHANNEL, &chan_cfg) != ESP_OK){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - RAW 채널(ADC_RAW_CHANNEL) 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            // custom_adc_deinit();
            return false;
        }

        // ★ Mutex 보호 큐 초기화
        if(!custom_mpqs_init(&mpqs_iSENSOR_raw_read, ADC_READ_BUFFER_SIZE, sizeof(uint16_t), "mpqs_iSENSOR_adc_read")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_adc_read Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
        // ★ Mutex 보호 큐 초기화
        if(!custom_mpqs_init(&mpqs_iSENSOR_raw, ADC_BUFFER_SIZE, sizeof(avs*), "mpqs_iSENSOR_adc")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_adc Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
    #endif

    #if ADC_HPF_ENABLE
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - HPF 채널(ADC_HPF_CHANNEL) 설정\n"
            "\t\t\t1. adc_oneshot_config_channel() | ADC_HPF_CHANNEL\n"
            "\t\t\t2. custom_mpqs_init() | mpqs_iSENSOR_hpf_read\n"
            "\t\t\t3. custom_mpqs_init() | mpqs_iSENSOR_hpf\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        if(adc_oneshot_config_channel(adc_oneshot_unit_handle, ADC_HPF_CHANNEL, &chan_cfg) != ESP_OK){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - HPF 채널(ADC_HPF_CHANNEL) 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            // custom_adc_deinit();
            return false;
        }
        if(!custom_mpqs_init(&mpqs_iSENSOR_hpf_read, ADC_READ_BUFFER_SIZE, sizeof(uint16_t), "mpqs_iSENSOR_hpf_read")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_hpf_read Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
        if(!custom_mpqs_init(&mpqs_iSENSOR_hpf, ADC_BUFFER_SIZE, sizeof(avs*), "mpqs_iSENSOR_hpf")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_hpf Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
    #endif
    
    #if ADC_BPF_ENABLE
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - BPF 채널(ADC_BPF_CHANNEL) 설정\n"
            "\t\t\t1. adc_oneshot_config_channel() | ADC_BPF_CHANNEL\n"
            "\t\t\t2. custom_mpqs_init() | mpqs_iSENSOR_bpf_read\n"
            "\t\t\t3. custom_mpqs_init() | mpqs_iSENSOR_bpf\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        if(adc_oneshot_config_channel(adc_oneshot_unit_handle, ADC_BPF_CHANNEL, &chan_cfg) != ESP_OK){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - BPF ADC 채널 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            // custom_adc_deinit();
            return false;
        }

        if(!custom_mpqs_init(&mpqs_iSENSOR_bpf_read, ADC_READ_BUFFER_SIZE, sizeof(uint16_t), "mpqs_iSENSOR_bpf_read")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_bpf_read Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
        if(!custom_mpqs_init(&mpqs_iSENSOR_bpf, ADC_BUFFER_SIZE, sizeof(avs*), "mpqs_iSENSOR_bpf")){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_bpf Mutex 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            return false;
        }
    #endif
///////////////////////////////////////////////////////////// 메크로 각 /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    #if ADC_SW_VOLTAGE_ENABLE
        static adc_cali_curve_fitting_config_t cali_cfg = {
            .unit_id = ADC_RAW_UNIT,
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_12,
        };
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) Cali Handle에 전압 보정 Handle 설정(1회만 실행)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        if(adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) != ESP_OK){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - adc_cali_create_scheme_curve_fitting(&cali_cfg, &adc_cali_handle) Cali Handle에 전압 보정 Handle 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            // custom_adc_deinit();
            return false;
        }
    #endif

    #if ADC_SW_HPF_ENABLE
        // 정규화된 주파수 = 차단 주파수 / 샘플링 주파수 (반드시 0 ~ 0.5 사이)
        // ═══════════════════════════════════════════════════════════════════════════════
        // 1. SW HPF 필터 계수 생성 (단독 HPF용)
        // ═══════════════════════════════════════════════════════════════════════════════
        if(dsps_biquad_gen_hpf_f32(hpf_coeffs, LOW_CUTOFF_FREQ / SAMPLING_FREQ, BUTTERWORTH_Q_FACTOR) != ESP_OK){
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - SW HPF 필터 설정 실패 (차단: %.1fHz)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, LOW_CUTOFF_FREQ);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
            // custom_adc_deinit();
            return false;
        }
        hpf_delay[0] = 0; hpf_delay[1] = 0;
        #if CUSTOM_ADC_INIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - SW HPF 필터 설정 완료 (차단: %.1fHz)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, LOW_CUTOFF_FREQ);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif


        #if ADC_SW_BPF_ENABLE
            // 2-2. BPF의 LPF 단 (고주파 차단)
            if(dsps_biquad_gen_lpf_f32(lpf_coeffs, HIGH_CUTOFF_FREQ / SAMPLING_FREQ, BUTTERWORTH_Q_FACTOR) != ESP_OK){
                #if CUSTOM_ADC_INIT_DEBUG
                printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - SW BPF(LPF단) 필터 설정 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #if PRINT_DELAY
                ////////////////////////////////////////////////////////
                vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
                ////////////////////////////////////////////////////////
                #endif
                #endif
                // custom_adc_deinit();
                return false;
            }
            lpf_delay[0] = 0; lpf_delay[1] = 0;
            
            #if CUSTOM_ADC_INIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - SW BPF 필터 설정 완료 (대역: %.1fHz ~ %.1fHz)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, LOW_CUTOFF_FREQ, HIGH_CUTOFF_FREQ);
            #if PRINT_DELAY
            ////////////////////////////////////////////////////////
            vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
            ////////////////////////////////////////////////////////
            #endif
            #endif
        #endif
        
    #endif

///////////////////////////////////////////////////////////// 메크로 각 /////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread 생성
    #if CUSTOM_ADC_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - ADC_READ_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    i_xTaskCreate_return_value = xTaskCreate(custom_all_adc_read_thread, "ADC_READ_THREAD", ADC_READ_THREAD_STACK_SIZE, NULL, configMAX_PRIORITIES - 1, &TaskHandle_custom_all_adc_read_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_BAND_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - ADC_READ_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_BAND_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - ADC_READ_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }

    #if CUSTOM_ADC_INIT_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_init() - ADC_THREAD 실행\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
    #if PRINT_DELAY
    ////////////////////////////////////////////////////////
    vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
    ////////////////////////////////////////////////////////
    #endif
    #endif
    i_xTaskCreate_return_value = xTaskCreate(custom_all_adc_process_thread, "ADC_THREAD", ADC_THREAD_STACK_SIZE, NULL, configMAX_PRIORITIES - 2, &TaskHandle_custom_all_adc_thread);
    if(i_xTaskCreate_return_value == pdFAIL){
        #if CUSTOM_BAND_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - ADC_PROCESS_THREAD 실행 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
    else if(i_xTaskCreate_return_value == errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY){
        #if CUSTOM_BAND_INIT_DEBUG
        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - ADC_PROCESS_THREAD 필요한 메모리 확보 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #if PRINT_DELAY
        ////////////////////////////////////////////////////////
        vTaskDelay(custom_ms_to_delay(DEBUG_DELAY_TIME_MS)); ///
        ////////////////////////////////////////////////////////
        #endif
        #endif
        // custom_adc_deinit();
        return false;
    }
///////////////////////////////////////////////////////////// 메크로 각 /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    return true;
}

bool custom_running_iSENSOR_all_adc_read_thread(void){
    return TaskHandle_custom_all_adc_read_thread != NULL;
}
bool custom_running_iSENSOR_all_adc_thread(void){
    return TaskHandle_custom_all_adc_thread != NULL;
}

///// 메크로 //////
void custom_set_all_adc_break(bool b_break){
    b_all_adc_break = b_break;
}
bool custom_get_all_adc_break(void){
    return b_all_adc_break;
}
void custom_set_all_adc_breaked(bool b_breaked){
    b_all_adc_breaked = b_breaked;
}
bool custom_get_all_adc_breaked(void){
    return b_all_adc_breaked;
}
///// 메크로 //////

void custom_all_adc_read_thread(void *arg){

    #define CUSTOM_ALL_ADC_READ_THREAD_DEBUG         ADC_DEBUG

    while(true){
        if(!custom_get_all_adc_break()){
            if(custom_get_all_adc_breaked()){
                #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                printf("[%s] "COLOR_BLUE"[특수-Case]\t %s [Multi Thread] custom_all_adc_read_thread() - b_adc_breaked 해제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #endif
                custom_set_all_adc_breaked(false);
            }


            // RAW ADC Queue 존재 Check
            if(mpqs_iSENSOR_raw_read.QueueHandle_queue == NULL){
                #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                printf("[%s] "COLOR_YELLOW"[경고-WARRING]\t %s [Multi Thread] custom_all_adc_read_thread() - mpqs_iSENSOR_raw_read.QueueHandle_queue 준비 안됨\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #endif
            }
            else{
                // #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                // printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_all_adc_read_thread() - ADC값을 QUEUE로 전송\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                // #endif

                // ═══════════════════════════════════════════════════════════════
                // RAW 채널 읽기 (조건부)
                // ═══════════════════════════════════════════════════════════════
                #if ADC_RAW_ENABLE
                    static uint16_t ui16_adc_read_value = 0;
                    adc_oneshot_read(adc_oneshot_unit_handle, ADC_RAW_CHANNEL, &ui16_adc_read_value);
                    // ★ Mutex 보호 큐 송신 (Thread-Safe)
                    cqrre cqrre_raw_send_result = custom_queue_safe_send(&mpqs_iSENSOR_raw_read, &ui16_adc_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    if(cqrre_raw_send_result != QUEUE_IS_READY){
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_read_thread() - mpqs_iSENSOR_raw_read 전송 실패 (cqrre_raw_send_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_raw_send_result);
                        #endif
                    }
                    else{
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Multi Thread] custom_all_adc_read_thread() - RAW 전송 (-> mpqs_iSENSOR_raw_read) : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, ui16_adc_read_value);
                        #endif
                    }
                    (void)cqrre_raw_send_result; // 미사용 경고 방지
                #endif

                // ═══════════════════════════════════════════════════════════════
                // HPF 채널 읽기 (조건부)
                // ═══════════════════════════════════════════════════════════════
                #if ADC_HPF_ENABLE
                    static uint16_t ui16_hpf_read_value = 0;
                    adc_oneshot_read(adc_oneshot_unit_handle, ADC_HPF_CHANNEL, &ui16_hpf_read_value);
                    // ★ Mutex 보호 큐 송신 (Thread-Safe)
                    cqrre cqrre_hpf_result = custom_queue_safe_send(&mpqs_iSENSOR_hpf_read, &ui16_hpf_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                    if(cqrre_hpf_result != QUEUE_IS_READY){
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Multi Thread] custom_all_adc_read_thread() - HPF 전송 실패 (cqrre_hpf_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_hpf_result);
                        #endif
                    }
                    else{
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Multi Thread] custom_all_adc_read_thread() - HPF 전송 (-> mpqs_iSENSOR_hpf_read) : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, ui16_hpf_read_value);
                        #endif
                    }
                    #endif
                    (void)cqrre_hpf_result; // 미사용 경고 방지
                #endif

                // ═══════════════════════════════════════════════════════════════
                // BPF 채널 읽기 (조건부)
                // ═══════════════════════════════════════════════════════════════
                #if ADC_BPF_ENABLE
                {
                    static uint16_t ui16_bpf_read_value = 0;
                    adc_oneshot_read(adc_oneshot_unit_handle, ADC_BPF_CHANNEL, &ui16_bpf_read_value);
                    cqrre cqrre_bpf_result = custom_queue_safe_send(&mpqs_iSENSOR_bpf_read, &ui16_bpf_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                    if(cqrre_bpf_result != QUEUE_IS_READY){
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Multi Thread] custom_all_adc_read_thread() - BPF 전송 실패 (cqrre_bpf_result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_bpf_result);
                        #endif
                    }
                    else{
                        #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                        printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s [Multi Thread] custom_all_adc_read_thread() - BPF 전송 (-> mpqs_iSENSOR_bpf_read) : %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, ui16_bpf_read_value);
                        #endif
                    }
                    #endif
                    (void)cqrre_bpf_result; // 미사용 경고 방지
                }
                #endif
            } // END if(mpqs_iSENSOR_adc_read.queue == NULL) else
        }
        else{
            if(!custom_get_all_adc_breaked()){
                #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                printf("[%s] "COLOR_BLUE"[특수-Case]\t %s [Multi Thread] custom_all_adc_read_thread() - b_adc_breaked 설정\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #endif
                custom_set_all_adc_breaked(true);
            }
        }

        // 종료 Signal 확인
        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, 0, &ulNotificationValue, custom_ms_to_delay(10)) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_all_adc_read_thread() - ADC Read Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #endif
                break; // 루프 탈출
            }
        }

        vTaskDelay(custom_ms_to_delay(ADC_SPEED_MS));
    }  // END while(true)
    TaskHandle_custom_all_adc_read_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
}

void custom_all_adc_process_thread(void *arg){
    #define CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG         ADC_DEBUG
    // #define CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG         false

    esp_err_t esp_err = ESP_OK;

    while(true){

        static uint16_t ui16_read_value = 0;
        static uint16_t ui16_voltage_value = 0;
        static float    f_read_value = 0;
        static float    f_hpf_value = 0;
        static float    f_bpf_value = 0;

        #if ADC_RAW_ENABLE
            // ═══════════════════════════════════════════════════════════════
            // RAW 채널 처리 (조건부)
            // ═══════════════════════════════════════════════════════════════
            static avs* avs_raw_data = NULL;
            // RAW Read Queue에서 데이터 수신
            cqrre cqrre_raw_result = custom_queue_safe_receive(&mpqs_iSENSOR_raw_read, &ui16_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
            if(cqrre_raw_result == QUEUE_IS_READY){
                // 데이터 할당 및 처리
                avs_raw_data = (avs*)malloc(sizeof(avs));
                if(avs_raw_data == NULL){
                    #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - avs_raw_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                    #endif
                    continue;
                }
                else{
                    memset(avs_raw_data, 0, sizeof(avs));
                    avs_raw_data->ui16_raw_value = ui16_read_value;
                    #if ADC_SW_VOLTAGE_ENABLE
                        // Voltage 변환
                        esp_err = adc_cali_raw_to_voltage(adc_cali_handle, ui16_read_value, &ui16_voltage_value);
                        if(esp_err != ESP_OK){
                            #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC SW Voltage Error: %s\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, esp_err_to_name(esp_err));
                            #endif
                            free(avs_raw_data);
                            avs_raw_data = NULL;
                            continue;
                        }
                        avs_raw_data->ui16_voltage_value = ui16_voltage_value;
                    #endif
                    
                    #if ADC_SW_HPF_ENABLE || ADC_SW_BPF_ENABLE
                        f_read_value = (float)ui16_read_value;
                    #endif
                    #if ADC_SW_HPF_ENABLE
                        esp_err = dsps_biquad_f32(&f_read_value, &f_hpf_value, 1, hpf_coeffs, hpf_delay);
                        if(esp_err != ESP_OK){
                            #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC SW HPF Error: %s\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, esp_err_to_name(esp_err));
                            #endif
                            free(avs_raw_data);
                            avs_raw_data = NULL;
                            continue;
                        }
                        avs_raw_data->f_hpf_value = f_hpf_value;
                    #endif
                    #if ADC_SW_BPF_ENABLE
                        esp_err = dsps_biquad_f32(&f_hpf_value, &f_bpf_value, 1, lpf_coeffs, lpf_delay);
                        if(esp_err != ESP_OK){
                            #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC SW BPF Error: %s\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, esp_err_to_name(esp_err));
                            #endif
                            free(avs_raw_data);
                            avs_raw_data = NULL;
                            continue;
                        }
                        avs_raw_data->f_bpf_value = f_bpf_value;
                    #endif
                    
                    // Queue로 전송
                    cqrre cqrre_send = custom_queue_safe_send(&mpqs_iSENSOR_raw, &avs_raw_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    if(cqrre_send != QUEUE_IS_READY){
                        free(avs_raw_data);
                    }
                    avs_raw_data = NULL;  // 소유권 이전 또는 해제 완료
                }
            }
        #endif // ADC_RAW_ENABLE

        #if ADC_HPF_ENABLE
            // ═══════════════════════════════════════════════════════════════
            // HPF 채널 처리 (조건부) - 외부 하드웨어 필터링된 신호 그대로 사용
            // ═══════════════════════════════════════════════════════════════
            static avs* avs_hpf_data = NULL;
            // HPF Read Queue에서 데이터 수신
            cqrre cqrre_hpf_result = custom_queue_safe_receive(&mpqs_iSENSOR_hpf_read, &ui16_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
            if(cqrre_hpf_result == QUEUE_IS_READY){
                // 데이터 할당 및 처리
                avs_hpf_data = (avs*)malloc(sizeof(avs));
                if(avs_hpf_data == NULL){
                    #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - avs_hpf_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                    #endif
                    continue;
                }
                else{
                    memset(avs_hpf_data, 0, sizeof(avs));
                    avs_hpf_data->ui16_raw_value = ui16_read_value;

                    #if ADC_SW_VOLTAGE_ENABLE
                        // Voltage 변환
                        esp_err = adc_cali_raw_to_voltage(adc_cali_handle, ui16_read_value, &ui16_voltage_value);
                        if(esp_err != ESP_OK){
                            #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC SW Voltage Error: %s\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, esp_err_to_name(esp_err));
                            #endif
                            free(avs_hpf_data);
                            avs_hpf_data = NULL;
                            continue;
                        }
                        avs_hpf_data->ui16_voltage_value = ui16_voltage_value;
                    #endif
                    
                    // Queue로 전송
                    cqrre cqrre_send = custom_queue_safe_send(&mpqs_iSENSOR_hpf, &avs_hpf_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    if(cqrre_send != QUEUE_IS_READY){
                        free(avs_hpf_data);
                    }
                    avs_hpf_data = NULL;  // 소유권 이전 또는 해제 완료
                }
            }
        #endif // ADC_HPF_ENABLE

        #if ADC_BPF_ENABLE
            // ═══════════════════════════════════════════════════════════════
            // BPF 채널 처리 (조건부) - 외부 하드웨어 필터링된 신호 그대로 사용
            // ═══════════════════════════════════════════════════════════════
            static avs* avs_bpf_data = NULL;
            // BPF Read Queue에서 데이터 수신
            cqrre cqrre_bpf_result = custom_queue_safe_receive(&mpqs_iSENSOR_bpf_read, &ui16_read_value, custom_ms_to_delay(MUTEX_TIMEOUT_MS));
            if(cqrre_bpf_result == QUEUE_IS_READY){
                // 데이터 할당 및 처리
                avs_bpf_data = (avs*)malloc(sizeof(avs));
                if(avs_bpf_data == NULL){
                    #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                    printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - avs_bpf_data 할당 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                    #endif
                    continue;
                }
                else{
                    memset(avs_bpf_data, 0, sizeof(avs));
                    avs_bpf_data->ui16_raw_value = ui16_read_value;

                    #if ADC_SW_VOLTAGE_ENABLE
                        // Voltage 변환
                        esp_err = adc_cali_raw_to_voltage(adc_cali_handle, ui16_read_value, &ui16_voltage_value);
                        if(esp_err != ESP_OK){
                            #if CUSTOM_ALL_ADC_PROCESS_THREAD_DEBUG
                            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC SW Voltage Error: %s\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, esp_err_to_name(esp_err));
                            #endif
                            free(avs_bpf_data);
                            avs_bpf_data = NULL;
                            continue;
                        }
                        avs_bpf_data->ui16_voltage_value = ui16_voltage_value;
                    #endif
                    
                    // Queue로 전송
                    cqrre cqrre_send = custom_queue_safe_send(&mpqs_iSENSOR_bpf, &avs_bpf_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));
                    if(cqrre_send != QUEUE_IS_READY){
                        free(avs_bpf_data);
                    }
                    avs_bpf_data = NULL;  // 소유권 이전 또는 해제 완료
                }
            }
        #endif // ADC_BPF_ENABLE

        uint32_t ulNotificationValue;
        if(xTaskNotifyWait(0, 0, &ulNotificationValue, custom_ms_to_delay(10)) == pdPASS){
            if((ulNotificationValue & NOTIFY_SHUTDOWN_BIT) != 0){
                #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_all_adc_process_thread() - ADC Read Thread 종료 Signal\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
                #endif
                break; // 루프 탈출
            }
        }
        
        vTaskDelay(1);
    } // END while(true)

    #if CUSTOM_ALL_ADC_READ_THREAD_DEBUG
    printf("[%s] "COLOR_WHITE"[진행-OK]\t %s [Multi Thread] custom_all_adc_process_thread() - b_custom_adc_thread_end_signal = true\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
    #endif

    TaskHandle_custom_all_adc_thread = NULL;
    vTaskDelete(NULL);   // 현재 Task 정상 종료
} // END adc_buf

void custom_reset_all_adc_queues(void){
    #define CUSTOM_ADC_RESET_DEBUG         ADC_DEBUG

    #if ADC_RAW_ENABLE
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_raw_read)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_raw_read Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_raw)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_raw Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif
    
    #if ADC_HPF_ENABLE
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_hpf_read)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_hpf_read Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_hpf)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_hpf Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif
    
    #if ADC_BPF_ENABLE
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_bpf_read)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_bpf_read Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        if(!custom_mpqs_queue_reset(&mpqs_iSENSOR_bpf)){
            #if CUSTOM_ADC_RESET_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_init() - mpqs_iSENSOR_bpf Queue 초기화 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif
}

#if ADC_RAW_ENABLE
    // iSENSOR_Mode에서 사용
    rgas custom_get_raw_avs(void){
        #define CUSTOM_GET_RAW_AVS_DEBUG         ADC_DEBUG

        rgas rgas_value = {QUEUE_IS_ERROR, 
            {
                0,
                #if ADC_SW_VOLTAGE_ENABLE
                0,
                #endif
                #if ADC_SW_HPF_ENABLE
                0.0f,
                #endif
                #if ADC_SW_BPF_ENABLE
                0.0f,
                #endif
            }
        };
        avs* avs_receive_data = NULL;

        // ★ Mutex 보호 큐 수신 (Thread-Safe)
        rgas_value.cqrre_value = custom_queue_safe_receive(&mpqs_iSENSOR_raw, &avs_receive_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));

        if(rgas_value.cqrre_value == QUEUE_IS_READY){
            #if CUSTOM_GET_RAW_AVS_DEBUG
                printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_get_avs() - mpqs_iSENSOR_raw -> avs_receive_data 데이터 꺼내기 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG); 
                printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_get_avs() - "
                    #if ADC_RAW_ENABLE
                    "Raw: %d"
                    #endif
                    #if ADC_SW_VOLTAGE_ENABLE
                    ", Voltage: %d mV"
                    #endif
                    #if ADC_SW_HPF_ENABLE
                    ", HPF: %f"
                    #endif
                    #if ADC_SW_BPF_ENABLE
                    ", BPF: %f"
                    #endif
                    "\n" COLOR_RESET, 
                    custom_getRuntimeString(), custom_esp_adc_TAG
                    #if ADC_RAW_ENABLE
                    , (uint16_t)avs_receive_data->ui16_raw_value
                    #endif
                    #if ADC_SW_VOLTAGE_ENABLE
                    , (uint16_t)avs_receive_data->ui16_voltage_value
                    #endif
                    #if ADC_SW_HPF_ENABLE
                    , avs_receive_data->f_hpf_value
                    #endif
                    #if ADC_SW_BPF_ENABLE
                    , avs_receive_data->f_bpf_value
                    #endif
                );
            #endif

            rgas_value.avs_value = *avs_receive_data;
            free(avs_receive_data); 
            avs_receive_data = NULL;
        }
        // else{
        //     if(rgas_value.cqrre_value == MPQS_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_NOT_READY){

        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_BUSY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_EMPTY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_FULL){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_ERROR){

        //     }
        // }

        return rgas_value;
    }
#endif // ADC_RAW_ENABLE

#if ADC_HPF_ENABLE
    // iSENSOR_Mode에서 사용
    // ═══════════════════════════════════════════════════════════════════════════════
    // HPF 채널 API 구현 (조건부)
    // ═══════════════════════════════════════════════════════════════════════════════
    rgas custom_get_hpf_avs(void){
        #define CUSTOM_GET_HPF_AVS_DEBUG         ADC_DEBUG

        rgas rgas_value = {QUEUE_IS_ERROR, 
            {
                0,
                #if ADC_SW_VOLTAGE_ENABLE
                0,
                #endif
            }
        };
        avs* avs_receive_data = NULL;

        // ★ Mutex 보호 큐 수신 (Thread-Safe)
        rgas_value.cqrre_value = custom_queue_safe_receive(&mpqs_iSENSOR_hpf, &avs_receive_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));

        if(rgas_value.cqrre_value == QUEUE_IS_READY){
            #if CUSTOM_GET_HPF_AVS_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_get_avs() - mpqs_iSENSOR_hpf -> avs_receive_data 데이터 꺼내기 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG); 
            printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_get_avs() - "
                #if ADC_RAW_ENABLE
                "Raw: %d"
                #endif
                #if ADC_SW_VOLTAGE_ENABLE
                ", Voltage: %d mV"
                #endif
                "\n" COLOR_RESET, 
                custom_getRuntimeString(), custom_esp_adc_TAG
                #if ADC_RAW_ENABLE
                , (uint16_t)avs_receive_data->ui16_raw_value
                #endif
                #if ADC_SW_VOLTAGE_ENABLE
                , (uint16_t)avs_receive_data->ui16_voltage_value
                #endif
            );
            #endif

            rgas_value.avs_value = *avs_receive_data;
            free(avs_receive_data); 
            avs_receive_data = NULL;
        }
        // else{
        //     if(rgas_value.cqrre_value == MPQS_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_NOT_READY){

        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_BUSY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_EMPTY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_FULL){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_ERROR){

        //     }
        // }

        return rgas_value;
    }
#endif // ADC_HPF_ENABLE

#if ADC_BPF_ENABLE
    // iSENSOR_Mode에서 사용
    // ═══════════════════════════════════════════════════════════════════════════════
    // BPF 채널 API 구현 (조건부)
    // ═══════════════════════════════════════════════════════════════════════════════
    rgas custom_get_bpf_avs(void){
        #define CUSTOM_GET_BPF_AVS_DEBUG         ADC_DEBUG

        rgas rgas_value = {QUEUE_IS_ERROR, 
            {
                0,
                #if ADC_SW_VOLTAGE_ENABLE
                0,
                #endif
            }
        };
        avs* avs_receive_data = NULL;

        // ★ Mutex 보호 큐 수신 (Thread-Safe)
        rgas_value.cqrre_value = custom_queue_safe_receive(&mpqs_iSENSOR_bpf, &avs_receive_data,  custom_ms_to_delay(MUTEX_TIMEOUT_MS));

        if(rgas_value.cqrre_value == QUEUE_IS_READY){
            #if CUSTOM_GET_BPF_AVS_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_get_avs() - mpqs_iSENSOR_bpf -> avs_receive_data 데이터 꺼내기 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG); 
            printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_get_avs() - "
                #if ADC_RAW_ENABLE
                "Raw: %d"
                #endif
                #if ADC_SW_VOLTAGE_ENABLE
                ", Voltage: %d mV"
                #endif
                "\n" COLOR_RESET, 
                custom_getRuntimeString(), custom_esp_adc_TAG
                #if ADC_RAW_ENABLE
                , (uint16_t)avs_receive_data->ui16_raw_value
                #endif
                #if ADC_SW_VOLTAGE_ENABLE
                , (uint16_t)avs_receive_data->ui16_voltage_value
                #endif
            );
            #endif

            rgas_value.avs_value = *avs_receive_data;
            free(avs_receive_data); 
            avs_receive_data = NULL;
        }
        // else{
        //     if(rgas_value.cqrre_value == MPQS_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_NOT_READY){
        //         #if CUSTOM_GET_RAW_AVS_DEBUG
        //         printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_get_avs() - mpqs_iSENSOR_adc 수신 실패 (result=%d)\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, cqrre_receive_result);
        //         #endif
        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_NOT_READY){

        //     }
        //     else if(rgas_value.cqrre_value == MUTEX_IS_BUSY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_EMPTY){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_FULL){

        //     }
        //     else if(rgas_value.cqrre_value == QUEUE_IS_ERROR){

        //     }
        // }

        return rgas_value;
    }
#endif // ADC_HPF_ENABLE

bool custom_adc_deinit(void){

    #define CUSTOM_ADC_DEINIT_DEBUG         ADC_DEBUG

    if(custom_running_iSENSOR_all_adc_read_thread()){

        custom_set_all_adc_break(true);
        while(custom_get_all_adc_breaked() == false){
            vTaskDelay(custom_ms_to_delay(10));
        }

        // 1. 종료 신호 전송
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - xTaskNotify - iSENSOR_adc_read_thread 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        xTaskNotify(TaskHandle_custom_all_adc_read_thread, NOTIFY_SHUTDOWN_BIT, eSetBits);
        // 2. 핸들이 NULL이 될 때까지 대기 (태스크가 스스로 NULL로 만들고 죽음)
        while(custom_running_iSENSOR_all_adc_read_thread()){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - running_iSENSOR_adc_read_thread() = %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, custom_running_iSENSOR_all_adc_read_thread());
            #endif
            vTaskDelay(custom_ms_to_delay(10)); 
        }
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - iSENSOR_adc_read_thread 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
    }

    if(custom_running_iSENSOR_all_adc_thread()){
        // 1. 종료 신호 전송
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - xTaskNotify - iSENSOR_adc_thread 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        xTaskNotify(TaskHandle_custom_all_adc_thread, NOTIFY_SHUTDOWN_BIT, eSetBits);
        // 2. 핸들이 NULL이 될 때까지 대기 (태스크가 스스로 NULL로 만들고 죽음)
        while(custom_running_iSENSOR_all_adc_thread()){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - running_iSENSOR_adc_thread() = %d\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, custom_running_iSENSOR_all_adc_thread());
            #endif
            vTaskDelay(custom_ms_to_delay(10)); 
        }
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - iSENSOR_adc_thread 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
    }

    if(adc_oneshot_unit_handle){
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - adc_oneshot_del_unit(adc_oneshot_unit_handle) ADC Unit Handle 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(adc_oneshot_del_unit(adc_oneshot_unit_handle) != ESP_OK){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - adc_oneshot_del_unit(adc_oneshot_unit_handle) ADC Unit Handle 삭제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
        else{
            adc_oneshot_unit_handle = NULL;
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - adc_oneshot_unit_handle 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    }

    if(adc_cali_handle){
        #if CUSTOM_ADC_DEINIT_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - adc_cali_delete_scheme_curve_fitting(adc_cali_handle) Cali Handle에 전압 보정 Handle 삭제\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(adc_cali_delete_scheme_curve_fitting(adc_cali_handle) != ESP_OK){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - adc_cali_delete_scheme_curve_fitting(adc_cali_handle) Cali Handle에 전압 보정 Handle 삭제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
        else{
            adc_cali_handle = NULL;
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_deinit() - adc_cali_handle 삭제 완료\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    }

    custom_reset_all_adc_queues();
    
    #if ADC_RAW_ENABLE
        // ★ Mutex 보호 큐 해제
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_raw_read, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_raw_read, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_raw, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_raw, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
    #endif

    #if ADC_HPF_ENABLE
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_hpf_read, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_hpf_read, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_hpf, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_hpf, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
    #endif

    #if ADC_BPF_ENABLE
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_bpf_read, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_bpf_read, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
        if(!custom_mpqs_deinit(&mpqs_iSENSOR_bpf, portMAX_DELAY)){
            #if CUSTOM_ADC_DEINIT_DEBUG
            printf("[%s] "COLOR_RED"[오류-ERROR]\t %s custom_adc_deinit() - custom_mpqs_deinit(&mpqs_iSENSOR_bpf, portMAX_DELAY) 큐 해제 실패\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
            return false;
        }
    #endif
    
    return true;
}



// bool custom_adc_queues_empty(void){
//     #define CUSTOM_ADC_QUEUES_EMPTY_DEBUG         ADC_DEBUG

//     UBaseType_t UBaseType_Messages_in_ui16_iSENSOR_adc_read = 0;
//     UBaseType_t UBaseType_Messages_in_ui16_iSENSOR_adc = 0;

//     if(QueueHandle_ui16_iSENSOR_adc_read != NULL){
//         UBaseType_Messages_in_ui16_iSENSOR_adc_read = uxQueueMessagesWaiting(QueueHandle_ui16_iSENSOR_adc_read);
        
//     }
//     if(QueueHandle_avs_iSENSOR_adc != NULL){
//         UBaseType_Messages_in_ui16_iSENSOR_adc = uxQueueMessagesWaiting(QueueHandle_avs_iSENSOR_adc);
//     }

//     #if CUSTOM_ADC_QUEUES_EMPTY_DEBUG
//     printf("[%s] "COLOR_BLACK"[정보-INFO]\t %s custom_adc_queues_empty() - Raw Queue: %u개, Processed Queue: %u개\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG, UBaseType_Messages_in_ui16_iSENSOR_adc_read, UBaseType_Messages_in_ui16_iSENSOR_adc);
//     #endif

//     return (UBaseType_Messages_in_ui16_iSENSOR_adc_read == 0) && (UBaseType_Messages_in_ui16_iSENSOR_adc == 0);
// }

bool custom_adc_wait_all_queues_done(void){
    #define CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG         ADC_DEBUG
    // #define CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG         true

    #if ADC_RAW_ENABLE
        // ═══════════════════════════════════════════
        // Raw ADC READ 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - Raw ADC READ 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_raw_read.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_raw_read) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - Raw ADC READ 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        // ═══════════════════════════════════════════
        // Raw ADC 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - Raw ADC 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_raw.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_raw) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - Raw ADC 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif

    #if ADC_HPF_ENABLE
        // ═══════════════════════════════════════════
        // HPF ADC READ 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - HPF ADC READ 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_hpf_read.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_hpf_read) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - HPF ADC READ 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        // ═══════════════════════════════════════════
        // HPF ADC 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - HPF ADC 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_hpf.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_hpf) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - HPF ADC 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif
    
    #if ADC_BPF_ENABLE
        // ═══════════════════════════════════════════
        // BPF ADC READ 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - BPF ADC READ 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_bpf_read.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_bpf_read) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - BPF ADC READ 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
        // ═══════════════════════════════════════════
        // BPF ADC 큐가 비었는지 확인
        // ═══════════════════════════════════════════
        #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
        printf("[%s] "COLOR_WHITE"[진행-OK]\t %s custom_adc_wait_all_queues_done() - BPF ADC 큐 처리 대기\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
        #endif
        if(mpqs_iSENSOR_bpf.QueueHandle_queue != NULL){
            while(custom_queue_safe_messages_waiting(&mpqs_iSENSOR_bpf) > 0){
                vTaskDelay(1);  // ADC Thread가 처리할 시간 주기
            }
            #if CUSTOM_ADC_WAIT_ALL_QUEUES_DONE_DEBUG
            printf("[%s] "COLOR_GREEN"[완료-OK]\t %s custom_adc_wait_all_queues_done() - BPF ADC 큐 비어있음\n" COLOR_RESET, custom_getRuntimeString(), custom_esp_adc_TAG);
            #endif
        }
    #endif

    return true;
}