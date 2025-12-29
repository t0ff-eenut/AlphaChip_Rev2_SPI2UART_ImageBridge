#include "gpio/gpio.h"

// UART
#include "driver/uart.h"

// USB
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "sdkconfig.h"

// static const int RX_BUF_SIZE = 1024;

#define TXD_PIN             (GPIO_NUM_1)
#define RXD_PIN             (GPIO_NUM_2)
// #define UART_BUFFER_SIZE    (64)
// #define UART_BUFFER_SIZE    (1024)
#define UART_BUFFER_SIZE    PIXEL_COUNT

#define BAUD_RATE_115200    115200
#define BAUD_RATE_230400    230400
#define BAUD_RATE_460800    460800
#define BAUD_RATE_500000    500000
#define BAUD_RATE_576000    576000
#define BAUD_RATE_921600    921600
#define BAUD_RATE_1000000   1000000
#define BAUD_RATE_1152000   1152000
#define BAUD_RATE_1500000   1500000
#define BAUD_RATE_2000000   2000000
#define BAUD_RATE_2500000   2500000
#define BAUD_RATE_3000000   3000000
#define BAUD_RATE_3500000   3500000
#define BAUD_RATE_4000000   4000000

// #define BAUD_RATE_SEL       BAUD_RATE_115200
#define BAUD_RATE_SEL       BAUD_RATE_576000
// #define BAUD_RATE_SEL       BAUD_RATE_1152000

// 1word = 4byte  4096word = 16384byte(16KB)
// uart_rx_thread : lost stack : 8304 ( = 33216)
// 10240 - 8304 = 1,936 use < 2048
#define UART_RX_STACK_SIZE                          (1024 * 3)
// uart_rx_data_process : lost stack : 2116 ( = 8464)
// 4096 - 2116 = 1,980 use < 2048
#define UART_RX_DATA_PROCESS_STACK_SIZE             (1024 * 3)
// uart_tx_thread : lost stack : 2144 ( = 8576)
// 4096 - 2144 = 1,952 use < 2048
#define UART_TX_STACK_SIZE                          (1024 * 3)

#pragma pack(push, 1)
typedef struct {
    uint8_t*    ui8_signal;
    uint8_t*    ui8_cmd;
    uint8_t*    ui8_addr_length;
    uint8_t*    ui8_addr;
    uint8_t*    ui8_data_length;
    uint8_t*    ui8_data;
    uint8_t*    ui8_chksum;
    uint8_t*    ui8_dummy;  
}uart_structer;
#pragma pack(pop)


typedef enum {
    UART_STATE_SIGNAL,
    UART_STATE_CMD,
    UART_STATE_ADDR,
    UART_STATE_DATA,
    UART_STATE_CHKSUM
} uart_recive_state;



// void view_heap_stack(char* c_function);
bool init_uart_structer(uart_structer* buf);
void free_uart_structer(uart_structer* buf);
void free_transaction(uart_structer* buf);

uint8_t checksum(uart_structer* uart_recive_data);
void uart_rx_thread(void);
uint8_t uart_rx_data_process(void);
uint8_t uart_tx_transaction(uart_structer* uart_send_data);
uint8_t uart_tx_thread(void *arg);
// static void uart_tx_task(void *arg);
int uart_init(void);