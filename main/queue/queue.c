#include "queue/queue.h"

// Error 체크
esp_err_t esp_err;
int i_useable_heap_size = 0;
int i_constant_useable_heap_size = 0;
int i_useing_stack_size = 0;
int i_useable_heap_size_bak = 0;
int i_constant_useable_heap_size_bak = 0;
int i_useing_stack_size_bak = 0;

void view_heap_stack(char* c_function){
    // 100 이하
//    heap_caps_print_heap_info(MALLOC_CAP_DEFAULT); // 이건 직접 출력합니다, printf로 감싸지 마세요.
//     Heap summary for capabilities 0x00001000:
//   At 0x3fc97070 len 337568 free 321684 allocated 13944 min_free 321684
//     largest_free_block 319488 alloc_blocks 42 free_blocks 1 total_blocks 43
//   At 0x3fce9710 len 22308 free 21572 allocated 0 min_free 21572
//     largest_free_block 21504 alloc_blocks 0 free_blocks 1 total_blocks 1
//   At 0x3fcf0000 len 32768 free 32032 allocated 0 min_free 32032
//     largest_free_block 31744 alloc_blocks 0 free_blocks 1 total_blocks 1
//   At 0x600fe100 len 7912 free 7532 allocated 0 min_free 7532
//     largest_free_block 7168 alloc_blocks 0 free_blocks 1 total_blocks 1
//   Totals:
//     free 382820 allocated 13944 min_free 382820 largest_free_block 319488
    i_useable_heap_size_bak = i_useable_heap_size;
    i_useable_heap_size = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    if(i_useable_heap_size < 100){
        printf("%s - 사용가능 heap : %d word %d Byte\t 차이 : %d\n", c_function, i_useable_heap_size, i_useable_heap_size * 4, i_useable_heap_size_bak - i_useable_heap_size);
    }

    i_constant_useable_heap_size_bak = i_constant_useable_heap_size;
    i_constant_useable_heap_size = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    if(i_constant_useable_heap_size < 100){
        printf("%s - 사용가능 연속 heap : %d word %d Byte\t 차이 : %d\n", c_function, i_constant_useable_heap_size, i_constant_useable_heap_size * 4, i_constant_useable_heap_size_bak - i_constant_useable_heap_size);
    }
    i_useing_stack_size_bak = i_useing_stack_size;
    i_useing_stack_size = uxTaskGetStackHighWaterMark(NULL);
    if(i_useing_stack_size < 100){
        printf("%s - 사용 가능 stack : %d word %d Byte\t 차이 : %d\n\n", c_function, i_useing_stack_size, i_useing_stack_size * 4, i_useing_stack_size_bak - i_useing_stack_size);
    }
}

// Queue
QueueHandle_t Q_ui64_spi_recive_image_320bit = NULL;
QueueHandle_t Q_ui8_frame_32768bit = NULL;
QueueHandle_t Q_ui8_uart_recive_8bit = NULL;
// QueueHandle_t Q_ui8_uart_image_recive_chk_8bit = NULL;
QueueHandle_t Q_ui32_cmd_128bit = NULL;
QueueHandle_t Q_ui32_spi_recive_reg_64bit = NULL;

int queue_init(void){
    
    view_heap_stack("queue_init start");
// queue_init start - 사용가능 heap : 382820        차이 : 0
// queue_init start - 연속 heap : 319488    차이 : 0
// queue_init start - 사용 stack : 1912     차이 : 0

    // Chip ->(SPI)-> ESP32
    Q_ui64_spi_recive_image_320bit      = xQueueCreate(SPI_IMAGE_ADDR * SPI_IMAGE_RECIVE_BUFFER, sizeof(uint64_t) * 5);     // SPI 수신 데이터 큐 생성        (Chip -> ESP32)
    Q_ui8_frame_32768bit                = xQueueCreate(IMAGE_BUFFER, sizeof(uint8_t) * PIXEL_COUNT);                        // 만들어진 Frame 큐 생성         (64x64)
    Q_ui32_spi_recive_reg_64bit         = xQueueCreate((_32BIT*REGISTER)*2, sizeof(uint32_t) * 2);                          // SPI 수신데이터 REG 큐 생성

    // ESP32 <-(UART)<- PC
    // Q_ui8_uart_image_recive_chk_8bit    = xQueueCreate(IMAGE_BUFFER, sizeof(uint8_t));                                      // IMAGE 정상 수신 검증 큐 생성
    Q_ui8_uart_recive_8bit              = xQueueCreate(UART_RECIVE_BUFFER, sizeof(uint8_t));                                // UART 수신 데이터 큐 생성       (ESP32 <- PC)
    Q_ui32_cmd_128bit                   = xQueueCreate(CMD_BUFFER, sizeof(uint32_t) * 4);                                   // 만들어진 CMD 데이터 큐 생성      (ESP32 <- PC)
    
    view_heap_stack("queue_init end");
// queue_init end - 사용가능 heap : 334036  차이 : 48784
// queue_init end - 연속 heap : 270336      차이 : 49152
// queue_init end - 사용 stack : 1912       차이 : 0

    return 0;
}