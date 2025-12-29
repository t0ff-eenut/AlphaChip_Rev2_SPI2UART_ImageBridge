#include "gpio/gpio.h"
// SPI
#include "driver/spi_slave.h"
#include "esp_timer.h"

// SPI
#ifdef CONFIG_IDF_TARGET_ESP32
#define RCV_HOST            HSPI_HOST
#else
#define SPI_IMAGE_RCV_HOST              SPI2_HOST
#define SPI_CMD_RCV_HOST                SPI3_HOST
#endif
#define SPI_IMAGE_GPIO_SCLK             GPIO_NUM_12
#define SPI_IMAGE_GPIO_MOSI             GPIO_NUM_11
#define SPI_IMAGE_GPIO_MISO             GPIO_NUM_13
#define SPI_IMAGE_GPIO_CS               GPIO_NUM_10
#define SPI_IMAGE_SPI_BUSTER_SIZE       (64 * 5)    // bit

#define SPI_CMD_GPIO_SCLK               GPIO_NUM_36
#define SPI_CMD_GPIO_MOSI               GPIO_NUM_35
#define SPI_CMD_GPIO_MISO               GPIO_NUM_37
#define SPI_CMD_GPIO_CS                 GPIO_NUM_39
#define SPI_CMD_SPI_BUSTER_SIZE         8           // bit

#define SPI_FIRST_WORD                  0XA5
#define SPI_WRONG_WORD                  0X88

// 1word = 4byte  4096word = 16384byte(16KB)
// spi_image_rx_thread : lost stack : 1988 ( = 7952)
// 4096 - 1988 = 2,108 use < 3072
#define SPI_IMAGE_RX_STACK_SIZE                     (1024 * 8)
// spi_image_rx_data_process : lost stack : 2152 ( = 8608)
// 4096 - 2152 = 1,944 use < 2048
#define SPI_IMAGE_RECIVE_DATA_PROCESS_STACK_SIZE    (1024 * 8)
// spi_cmd_tx_thread : lost stack : 2200 ( = 8800)
// 4096 - 2200 = 1,896 use < 2048
#define SPI_CMD_TX_STACK_SIZE                       (1024 * 3)

#pragma pack(push, 1)
typedef struct {
    uint8_t* ui8_signal;
    uint8_t* ui8_cmd;
    uint8_t* ui8_addr_length;
    uint8_t* ui8_addr;
    uint8_t* ui8_data_length;
    uint8_t* ui8_data;
    uint8_t* ui8_crc8;
    uint8_t* ui8_dummy;
}spi_structer;
#pragma pack(pop)

typedef enum {
    RECIVE_UART_DATA,
    // SEND_CMD_0_READ_REG,
    SEND_CMD_1_WRITE_REG,
    SEND_CMD_2,
    SEND_CMD_3
} spi_send_state;

static bool init_spi_send(spi_structer* buf);                                                                               // 1. SPI 구조체 초기화
static void free_spi_send(spi_structer* buf);                                                                               // 2. SPI 구조체 해제
static uint8_t crc8_proc(uint8_t crc8_value);                                                                               // 3. CRC8 Transaction
static uint8_t crc8(spi_structer* spistructer_spi_recive_data);                                                             // 4. CRC8 Process

static void spi_image_rx_thread(void *arg);                                                                                 // 5. SPI Frame 수신 [ISP] Chip -> ESP
static void spi_image_rx_data_process(void);                                                                                // 6. Frame 재구성

static void spi_receive_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value);                      // 7. SPI 1Byte 수신 [CPU] Chip -> ESP
static uint8_t spi_cmd_rx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_recv_buf);    // 8. SPI 1Byte 송신 [CPU] Chip <- ESP
static uint8_t spi_rx_CMD_data_setting(spi_structer* spistructer_spi_recv_buf);                                             // 9. SPI CMD 수신 Transaction [CPU] Chip -> ESP

static void spi_send_single_byte(spi_slave_transaction_t* spi_transaction, uint8_t* ui8_buf_value);                         // 10. SPI CMD 수신 Data Setting (수신 공간 할당)
static uint8_t spi_tx_transaction(spi_slave_transaction_t* spi_transaction, spi_structer* spistructer_spi_send_buf);        // 11. SPI CMD 송신 Transaction [CPU] Chip <- ESP
static uint8_t spi_tx_CMD_data_setting(uint32_t* A_ui32_cmd_q_recv_128bit);                                                 // 12. SPI CMD 송신 Data Setting (송신 공간 할당 및 데이터 세팅)
static uint8_t uart_recive_cmd_spi_tx_thread(void *arg);                                                                    // 13. Uart Recive CMD Data Process

int spi_init(void);                                                                                                         // 14. SPI 초기화
