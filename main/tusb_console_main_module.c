// Queue
#include "queue/queue.h"
// SPI
#include "spi/spi.h"
// UART
// USB
#include "uart/uart.h"


// int i_useable_heap_size = 0;
// int i_constant_useable_heap_size = 0;
// int i_useing_stack_size = 0;

// void view_heap_stack(char* c_function){
//     heap_caps_print_heap_info(MALLOC_CAP_DEFAULT); // 이건 직접 출력합니다, printf로 감싸지 마세요.
    
//     printf("%s - 사용가능 heap : %d\t 차이 : %d\n", c_function, heap_caps_get_free_size(MALLOC_CAP_DEFAULT), i_useable_heap_size - heap_caps_get_free_size(MALLOC_CAP_DEFAULT));
//     i_useable_heap_size = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);

//     printf("%s - 연속 heap : %d\t 차이 : %d\n", c_function, heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT), i_useable_heap_size - heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));                                                    
//     i_constant_useable_heap_size = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);

//     printf("%s - 사용 stack : %d\t 차이 : %d\n\n", c_function, uxTaskGetStackHighWaterMark(NULL), i_useable_heap_size - uxTaskGetStackHighWaterMark(NULL)); 
//     i_useing_stack_size = uxTaskGetStackHighWaterMark(NULL);
// }

void init(void)
{
    view_heap_stack("Main init Start");

    if (queue_init()){
        printf("Queue Error\n");
        // error
    }
    if (gpio_init()){
        printf("GPIO Error\n");
        // error
    }    
    if (spi_init()){
        printf("SPI Error\n");
        // error
    }
    if (uart_init()){
        printf("UART Error\n");
        // error
    }

    // // uint32_t getHeapSize(); //total heap size
    // // uint32_t getFreeHeap(); //available heap
    // // uint32_t getMinFreeHeap(); //lowest level of free heap since boot
    // // uint32_t getMaxAllocHeap(); //largest block of heap that can be allocated at once
    // printf("HeapSize : %d\n", ESP.getHeapSize());
    // printf("FreeHeapSize : %d\n", ESP.getFreeHeap());
    // printf("MinHeapSize : %d\n", ESP.getMinFreeHeap());
    // printf("MaxHeapSize : %d\n", ESP.getMaxAllocHeap());

}

void app_main(void)
{
    init();
    while (1){
        vTaskDelay(1);
    }
}
