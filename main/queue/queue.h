#include "esp32_s3.h"
// Queue
#include "freertos/queue.h"

#include "esp_heap_caps.h"
#define PIXEL_COUNT                 64*64
#define SPI_IMAGE_ADDR              128
#define REGISTER                    64  // 개
#define _32BIT                      4   // 8*4
#define _64BIT                      8   // 8*8
#define IMAGE                       5   // 장

#define IMAGE_SIGNAL    0xCC
#define CMD_SIGNAL      0xDD
#define READ_CMD        (0 << 7)
#define WRITE_CMD       (1 << 7)

#define CMD_READ_REG    0
#define CMD_WRITE_REG   1

#define SPI_IMAGE_RECIVE_BUFFER     IMAGE
#define IMAGE_BUFFER                IMAGE
#define CMD_BUFFER                  IMAGE

// #define SPI_CMD_RECIVE_BUFFER          100
// #define UART_RECIVE_BUFFER          16
#define UART_RECIVE_BUFFER         (_32BIT+_32BIT)*CMD_BUFFER


#define ERROR_MALLOC_FAIL           -1

void view_heap_stack(char* c_function);
int queue_init(void);