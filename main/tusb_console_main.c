#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/reent.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

// Queue
#include "freertos/queue.h"

// UART
#include "esp_system.h"
#include "driver/uart.h"
#include "string.h"

// SPI
#include "driver/spi_slave.h"
#include "esp_timer.h"

// USB
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "sdkconfig.h"

// UART
static const int RX_BUF_SIZE = 1024;
#define TXD_PIN             (GPIO_NUM_4)
#define RXD_PIN             (GPIO_NUM_5)
#define UART_BUFFER_SIZE    (8 * 5)

// SPI
#ifdef CONFIG_IDF_TARGET_ESP32
#define RCV_HOST            HSPI_HOST
#else
#define RCV_HOST            SPI2_HOST
#endif
#define GPIO_MOSI           GPIO_NUM_11
#define GPIO_MISO           GPIO_NUM_13
#define GPIO_SCLK           GPIO_NUM_12
#define GPIO_CS             GPIO_NUM_10


#define SPI_Buster_Size     (8 * 8 * 5)
uint64_t** p_u64_check_send_data_buf;
uint64_t** p_u64_check_recive_data_buf;

//uint64_t u64_text_send_time = 0;

// #define BAUD_RATE_115200	115200
// #define BAUD_RATE_230400	230400
// #define BAUD_RATE_460800	460800
// #define BAUD_RATE_115200    115200
// #define BAUD_RATE_230400    230400
// #define BAUD_RATE_460800    460800
// #define BAUD_RATE_500000    500000
// #define BAUD_RATE_576000    576000
// #define BAUD_RATE_921600    921600
// #define BAUD_RATE_1000000   1000000
// #define BAUD_RATE_1152000   1152000
// #define BAUD_RATE_1500000   1500000
// #define BAUD_RATE_2000000   2000000
// #define BAUD_RATE_2500000   2500000
// #define BAUD_RATE_3000000   3000000
// #define BAUD_RATE_3500000   3500000
// #define BAUD_RATE_4000000   4000000
// // #define BAUD_RATE_SEL       BAUD_RATE_3000000 영상이 깨짐 17ms
// // #define BAUD_RATE_SEL       BAUD_RATE_2500000
// // #define BAUD_RATE_SEL       BAUD_RATE_2000000       // 25ms 영상이 가끔 깨짐
// // #define BAUD_RATE_SEL       BAUD_RATE_1500000       // 33ms
// #define BAUD_RATE_SEL       BAUD_RATE_115200       // 33ms

void init(void)
{
    esp_err_t ret;
    // UART
    const uart_config_t uart_config = {
        .baud_rate = BAUD_RATE_SEL,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    // SPI
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_MOSI,
        .miso_io_num = GPIO_MISO,
        .sclk_io_num = GPIO_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    spi_slave_interface_config_t slvcfg = {
        .mode = 0,
        .spics_io_num = GPIO_CS,
        .queue_size = 3,
        .flags = 0,
    };
    gpio_set_pull_mode(GPIO_MOSI, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_SCLK, GPIO_PULLUP_ONLY);
    gpio_set_pull_mode(GPIO_CS, GPIO_PULLUP_ONLY);
    ret = spi_slave_initialize(RCV_HOST, &buscfg, &slvcfg, SPI_DMA_CH_AUTO);
    assert(ret == ESP_OK);

    // SPI_RX, TX Buffer

    p_u64_check_recive_data_buf         = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    p_u64_check_send_data_buf           = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    for(int i = 0; i < 128; i++){       
        p_u64_check_recive_data_buf[i]  = (uint64_t*)malloc(sizeof(uint64_t) * 5);
        p_u64_check_send_data_buf[i]    = (uint64_t*)malloc(sizeof(uint64_t) * 5);
    }

    // USB
    const tinyusb_config_t tusb_cfg = {
        .device_descriptor  = NULL,
        .string_descriptor  = NULL,
        .external_phy       = false, // In the most cases you need to use a `false` value
#if (TUD_OPT_HIGH_SPEED)
        .fs_configuration_descriptor = NULL,
        .hs_configuration_descriptor = NULL,
        .qualifier_descriptor = NULL,
#else
        .configuration_descriptor = NULL,
#endif // TUD_OPT_HIGH_SPEED
    };
    ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

    tinyusb_config_cdcacm_t acm_cfg = { 0 }; // the configuration uses default values
    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
}

// Queue
QueueHandle_t image_queue = NULL;

int i_addr = 0;
// Uart Recive 실시간 동작
static void spi_rx_task(void *arg)
{    
    int i_spi_frame_count = 0;

    esp_err_t esp_err;
    spi_slave_transaction_t spi_tran;
    // esp32
    uint64_t sendbuf[5] = {0};
    uint64_t recvbuf[5] = {0};
    spi_tran.length = SPI_Buster_Size;
    spi_tran.tx_buffer = sendbuf;
    spi_tran.rx_buffer = recvbuf;

    // 반복작업
    while(1){
        // Recive SPI
        while(i_addr < 127){

            memset(recvbuf, 0, sizeof(uint64_t) * 5);

            esp_err = spi_slave_transmit(RCV_HOST, &spi_tran, portMAX_DELAY);         // 64bit * 5 Recive
            assert(esp_err == ESP_OK);

            // for(int i_count = 0; i_count < 5; i_count++){
            //     printf("recvbuf[%d] : %16lli\n", i_count, recvbuf[i_count]);
            // }

            if(recvbuf[0] & 1ULL << 63){
                i_addr = (recvbuf[0] & 127ULL << 56) >> 56;
                for(int i_count_64bit = 0; i_count_64bit < 5; i_count_64bit++){
                    p_u64_check_recive_data_buf[i_addr][i_count_64bit] = recvbuf[i_count_64bit];
                }
            }   
        }
        i_addr = 0;

        // Queue
        xQueueSend(image_queue, p_u64_check_recive_data_buf, 0);
        i_spi_frame_count++;
        printf("1. i_spi_frame_count : %d\n", i_spi_frame_count);

        // printf("Queue number : %lu \n",uxQueueMessagesWaiting(image_queue));
        // b_frame_done = true;
        // printf("RECIVE_SPI_END\n");
        //u64_text_send_time = esp_timer_get_time();
    }   // END While(1)
}


// static void print_next_line(void *arg){
//     while(1){
//         if (esp_timer_get_time() - u64_text_send_time > 3000000)    // 마지막 text 3초 이상
//         {
//             printf("\n");
//             u64_text_send_time = esp_timer_get_time();
//         }else{
//             vTaskDelay(1);
//         }
//     }
// }

void app_main(void)
{
    // queue
    int i_queue_frame_count = 0;
    //image_queue = xQueueCreate(30, sizeof((uint64_t**)malloc(sizeof(uint64_t*) * 128))); 
    image_queue = xQueueCreate(10, sizeof((uint64_t**)malloc(sizeof((uint64_t*)malloc(sizeof(uint64_t)*5))*128))); // 이벤트를 저장하는 큐 생성.

    uint64_t** p_u64_temp_buf = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    for(int i = 0; i < 128; i++){       
        p_u64_temp_buf[i]  = (uint64_t*)malloc(sizeof(uint64_t) * 5);
    }

    // 소요시간 측정용
    // uint64_t u64_UART_Sender_Timer_Start, u64_UART_Sender_Timer_End;
    // UART, SPI
    init();
    //esp_tusb_init_console(TINYUSB_CDC_ACM_0); // log to uart
    
    //xTaskCreate(print_next_line, "PRINT_NEXTLINE", 1024 * 2, NULL, configMAX_PRIORITIES - 3, NULL);
    xTaskCreate(spi_rx_task, "SPI_TASK_THREAD", 1024 * 4, NULL, configMAX_PRIORITIES - 1, NULL);

    
    // UART SENDER 작업
    while(1){
        //if (b_frame_done){
        if(uxQueueMessagesWaiting(image_queue) > 0){
            
            printf("2. Queue Use : %d\n", uxQueueMessagesWaiting(image_queue));
            //u64_UART_Sender_Timer_Start = esp_timer_get_time();
            // printf("wait time : %16lli ms (%16lli us)\n", (u64_UART_Sender_Timer_Start - u64_UART_Sender_Timer_End) / 1000, u64_UART_Sender_Timer_Start - u64_UART_Sender_Timer_End);

            // uint64_t** p_u64_temp_buf = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
            // for(int i = 0; i < 128; i++){       
            //     p_u64_temp_buf[i]  = (uint64_t*)malloc(sizeof(uint64_t) * 5);
            // }
            
            xQueueReceive(image_queue, p_u64_temp_buf, portMAX_DELAY);
            i_queue_frame_count++;
            printf("3. i_queue_frame_count : %d\n", i_queue_frame_count);

            // for(int i_sel_320bit = 0; i_sel_320bit < 128; i_sel_320bit++){
            //     for(int i_count_64bit = 0; i_count_64bit < 5; i_count_64bit++){
            //         if(i_count_64bit == 0){
            //             printf("cmd = %2lli\n",                          (p_u64_temp_buf[i_sel_320bit][i_count_64bit] & (1ULL << 63)) >> 63);
            //             printf("addr = %2lli\n",                         (p_u64_temp_buf[i_sel_320bit][i_count_64bit] & (127ULL << 56)) >> 56);
            //             // printf("cmd = %d\n", (p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (1ULL << 63)) >> 63);
            //             // printf("addr = %d\n", p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (127ULL << 56) >> 56);
            //         }else{
            //             printf("data%d = %08llX\n", (i_count_64bit*2)-2   ,(p_u64_temp_buf[i_sel_320bit][i_count_64bit] & (0xFFFFFFFFULL << 32)) >> 32);
            //             printf("data%d = %08llX\n", (i_count_64bit*2)-1   ,p_u64_temp_buf[i_sel_320bit][i_count_64bit] & 0xFFFFFFFFULL);
            //         }
            //     }
            //     printf("\n");
            // }

            // print
            // for(int i_sel_320bit = 0; i_sel_320bit < 128; i_sel_320bit++){
            //     for(int i_count_64bit = 0; i_count_64bit < 5; i_count_64bit++){
            //         if(i_count_64bit == 0){
            //             printf("cmd = %2lli\n",                          (p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (1ULL << 63)) >> 63);
            //             printf("addr = %2lli\n",                         (p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (127ULL << 56)) >> 56);
            //             // printf("cmd = %d\n", (p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (1ULL << 63)) >> 63);
            //             // printf("addr = %d\n", p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (127ULL << 56) >> 56);
            //         }else{
            //             printf("data%d = %08llX\n", (i_count_64bit*2)-2   ,(p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & (0xFFFFFFFFULL << 32)) >> 32);
            //             printf("data%d = %08llX\n", (i_count_64bit*2)-1   ,p_u64_check_recive_data_buf[i_sel_320bit][i_count_64bit] & 0xFFFFFFFFULL);
            //         }
            //     }
            //     printf("\n");
            // }
            

            //printf("wait time : %16lli ms (%16lli us)\n", (u64_UART_Sender_Timer_Start - u64_UART_Sender_Timer_End) / 1000, u64_UART_Sender_Timer_Start - u64_UART_Sender_Timer_End);
            for(int i_sel_320bit = 0; i_sel_320bit < 128; i_sel_320bit++){
                //printf("SPI_RECIVE_DATA_MEM_Print[%d] : %016llX %016llX %016llX %016llX %016llX\n", i_sel_320bit ,  p_u64_check_recive_data_buf[i_sel_320bit][0],  p_u64_check_recive_data_buf[i_sel_320bit][1],  p_u64_check_recive_data_buf[i_sel_320bit][2],  p_u64_check_recive_data_buf[i_sel_320bit][3],  p_u64_check_recive_data_buf[i_sel_320bit][4]);
                // USB 전송용
                //printf("%016llX%016llX%016llX%016llX%016llX", p_u64_check_recive_data_buf[i_sel_320bit][0],  p_u64_check_recive_data_buf[i_sel_320bit][1],  p_u64_check_recive_data_buf[i_sel_320bit][2],  p_u64_check_recive_data_buf[i_sel_320bit][3],  p_u64_check_recive_data_buf[i_sel_320bit][4]);
                uart_write_bytes(UART_NUM_1, p_u64_temp_buf[i_sel_320bit], UART_BUFFER_SIZE);
            }
            //u64_UART_Sender_Timer_End = esp_timer_get_time();
            printf("SEND_UART_END\n");
            //printf("UART time : %16lli ms (%16lli us)\n", (u64_UART_Sender_Timer_End - u64_UART_Sender_Timer_Start) / 1000, u64_UART_Sender_Timer_End - u64_UART_Sender_Timer_Start);
            //b_frame_done = false;

        }
        else{
            vTaskDelay(1);
            // Delay(0.1);
        }
    }   // END While(1)



    for (int i = 0; i < 128; i++) {
        free(p_u64_check_send_data_buf[i]);
        free(p_u64_check_recive_data_buf[i]);
        free(p_u64_temp_buf[i]);
    }
    free(p_u64_check_send_data_buf); //동적할당 해제
    free(p_u64_check_recive_data_buf); //동적할당 해제
    free(p_u64_temp_buf); //동적할당 해제

    //esp_tusb_deinit_console(TINYUSB_CDC_ACM_0); // log to uart
}
