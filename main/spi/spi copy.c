// SPI
#include "spi/spi.h"
// from queue.c
extern QueueHandle_t image_queue;
uint64_t** p_u64_check_send_data_buf;
uint64_t** p_u64_check_recive_data_buf;

int i_addr = 0;
static void spi_rx_task(void *arg)
{    
    p_u64_check_recive_data_buf         = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    // p_u64_check_send_data_buf           = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    for(int i = 0; i < 256; i++){       
        p_u64_check_recive_data_buf[i]  = (uint64_t*)malloc(sizeof(uint64_t) * 5);
        // p_u64_check_send_data_buf[i]    = (uint64_t*)malloc(sizeof(uint64_t) * 5);
        memset(p_u64_check_recive_data_buf[i], 0, sizeof(uint64_t) * 5);
        // memset(p_u64_check_send_data_buf[i], 0, sizeof(uint64_t) * 5);
    }


    // // uint64_t** AA_u64_recive_frame = (uint64_t**)malloc(sizeof(uint64_t*) * 128);
    // // for(int i = 0; i < 128; i++){       
    // //     AA_u64_recive_frame[i]  = (uint64_t*)malloc(sizeof(uint64_t) * 5);
    // // }

    uint64_t** f_u8_recive_frame = (uint8_t**)malloc(sizeof(uint8_t*) * 64);
    for(int i = 0; i < 128; i++){       
        f_u8_recive_frame[i]  = (uint8_t*)malloc(sizeof(uint8_t) * 64);
    }

    int i_spi_frame_count = 0;

    esp_err_t esp_err;
    spi_slave_transaction_t spi_tran;
    // esp32
    // WORD_ALIGNED_ATTR uint64_t sendbuf[5] = {0};
    uint64_t sendbuf[5] = {0};
    // WORD_ALIGNED_ATTR uint64_t recvbuf[5] = {0};
    uint64_t recvbuf[5] = {0};
    spi_tran.tx_buffer = sendbuf;
    spi_tran.rx_buffer = recvbuf;
    spi_tran.length = SPI_Buster_Size;

    int i_count = 0;
    bool b_frame_start = false;
    bool b_frame_done = false;
    // 반복작업
    while(1){
        // Recive SPI
        while(!b_frame_done){

            memset(recvbuf, 0, sizeof(uint64_t) * 5);
            esp_err = spi_slave_transmit(RCV_HOST, &spi_tran, portMAX_DELAY);         // 64bit * 5 Recive
            assert(esp_err == ESP_OK);

            // if(recvbuf[0] & 1ULL << 63){
            //     i_addr = (recvbuf[0] & 127ULL << 56) >> 56;
            //     // // for(int i_count_64bit = 0; i_count_64bit < 5; i_count_64bit++){
            //     // //     AA_u64_recive_frame[i_addr][i_count_64bit] = recvbuf[i_count_64bit];
            //     // // }


            //     for (int i_sel_64bit = 0; i_sel_64bit < 4; i_sel_64bit++) {
            //         for (int i_sel_8bit = 0; i_sel_8bit < 8; i_sel_8bit++) {
            //             printf("%016X\t", recvbuf[i_sel_64bit + 1]);

            //             printf("%d\t", (8 * (7 - i_sel_8bit)));
            //             printf("%d\t", recvbuf[i_sel_64bit + 1] & (255ULL << (8 * (7 - i_sel_8bit))) >> (8 * (7 - i_sel_8bit)));
            //             // f_u8_recive_frame[i_addr / 2][((i_addr % 2) * 32) + i_sel_64bit * 8 + (7 - i_sel_8bit)] = recvbuf[i_sel_64bit+1] & (255ULL << (7 - i_sel_8bit));
            //         }
            //         printf("\n");
            //     }
            // }  
            if(recvbuf[0] & 1ULL << 63){
                i_addr = (recvbuf[0] & 127ULL << 56) >> 56;
                if (b_frame_start & !b_frame_done & (i_addr == 0)){
                    b_frame_done = true;
                    break;
                }
                for (int i_sel_64bit = 0; i_sel_64bit < 5; i_sel_64bit++) {
                    b_frame_start = true;
                    p_u64_check_recive_data_buf[i_count][i_sel_64bit] = recvbuf[i_sel_64bit];
                    i_count++;
                    if (i_count == 128){
                        b_frame_done = true;
                        i_count = 0;
                        break;
                    }
                }
                if (b_frame_done){
                    b_frame_start = false;
                    break;
                }
            }
            // printf("\n\n");
        }
        
        if (b_frame_done){
            for (int i_sel_320it = 0; i_sel_320it < 128; i_sel_320it++) {
                for (int i_sel_64bit = 0; i_sel_64bit < 5; i_sel_64bit++) {
                    printf("%08X\t", p_u64_check_recive_data_buf[i_sel_320it][i_sel_64bit]);
                }
            printf("\n");
            }
            b_frame_done = false;
        }

        // Queue
        // // xQueueSend(image_queue, AA_u64_recive_frame, 0);
        i_spi_frame_count++;
        printf("1. i_spi_frame_count : %d\n", i_spi_frame_count);
        //vTaskDelay(1);
    }   // END While(1)

    // // for (int i = 0; i < 128; i++) {
    // //     free(AA_u64_recive_frame[i]);
    // // }
    // // free(AA_u64_recive_frame); //동적할당 해제
}


int spi_init(void){
    // SPI
    esp_err_t ret;
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

    xTaskCreate(spi_rx_task, "SPI_TASK_THREAD", 1024 * 4, NULL, configMAX_PRIORITIES - 1, NULL);
    return 0;
}


